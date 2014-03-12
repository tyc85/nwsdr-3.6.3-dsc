#!/usr/bin/env python
#
# Copyright 2005,2006 Free Software Foundation, Inc.
# 
# This file is part of GNU Radio
# 
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

from gnuradio import gr, gru, blks2, window
from gnuradio import uhd#usrp
from gnuradio import eng_notation
import copy
import math
# from current dir
import numpy, struct



# linklab, define constants
FFT_SIZE      = 256     # fft size for sensing  
LOW_THRES     = -45        # low power threshold in dB to identify free freq blocks
HIGH_THRES    = -25        # high power threshold in dB to identify busy freq blocks
SMOOTH_LENGTH = 10       # smooth length
EDGE_THRES    = 5          # edge detection threshold in dB

SENSE_ALLTHR_MAX=-110
SENSE_ALLTHR_MIN=-150
SENSE_ALLTHR_MAX_COUNT=0
SENSE_ALLTHR_MIN_COUNT=0



# /////////////////////////////////////////////////////////////////////////////
#                              sensing path
# /////////////////////////////////////////////////////////////////////////////

class sensing_path(gr.hier_block2):
    def __init__(self, options):

	gr.hier_block2.__init__(self, "sensing_path",
				gr.io_signature(1, 1, gr.sizeof_gr_complex), # Input signature
				gr.io_signature(0, 0, 0)) # Output signature


        options = copy.copy(options)    # make a copy so we can destructively modify

        self._verbose        = options.verbose
       
        # linklab, fft size for sensing, different from fft length for tx/rx
        self.fft_size = FFT_SIZE

	self._thr_sense=options.thr_sense
	print self._thr_sense

        # interpolation rate: sensing fft size / ofdm fft size
        self.interp_rate = self.fft_size/FFT_SIZE #options.fft_length

        self._fft_length      = FFT_SIZE #options.fft_length
        self._occupied_tones  = FFT_SIZE #options.occupied_tones
        self.msgq             = gr.msg_queue()

        # linklab , setup the sensing path
        # FIXME: some components are not necessary
        self.s2p = gr.stream_to_vector(gr.sizeof_gr_complex, self.fft_size)
        mywindow = window.blackmanharris(self.fft_size)
        self.fft = gr.fft_vcc(self.fft_size, True, mywindow)
        power = 0
        for tap in mywindow:
            power += tap*tap
        self.c2mag = gr.complex_to_mag(self.fft_size)
        self.avg = gr.single_pole_iir_filter_ff(1.0, self.fft_size)

        # linklab, ref scale value from default ref_scale in usrp_fft.py
        ref_scale = 13490.0

        # FIXME  We need to add 3dB to all bins but the DC bin
        self.log = gr.nlog10_ff(20, self.fft_size,
                                -10*math.log10(self.fft_size)              # Adjust for number of bins
                                -10*math.log10(power/self.fft_size)        # Adjust for windowing loss
                                -20*math.log10(ref_scale/2))               # Adjust for reference scale

        self.sink = gr.message_sink(gr.sizeof_float * self.fft_size, self.msgq, True)
        self.connect(self, self.s2p, self.fft, self.c2mag, self.avg, self.log, self.sink)
        #self.connect(self, self.sink)

    def FFTresult(self):
        s = ""
        msg = self.msgq.delete_head()  # blocking read of message queue
        s = msg.to_string()

        # linklab, smooth psd by averaging over multiple observations
        #itemsize = int(msg.arg1())
        psd_temp = numpy.zeros(self.fft_size)
        #for i in range(0, 1):
        #    start = itemsize * (SMOOTH_LENGTH - i - 1)
        itemsize = int(msg.arg1())
        tmp = s[0:itemsize]
        psd_temp = numpy.fromstring(tmp, numpy.float32)
	
	#print psd_temp

        # linklab, rearrange psd bins
        psd = psd_temp.copy()
        psd[self.fft_size/2:self.fft_size] = psd_temp[0:self.fft_size/2]
        psd[0:self.fft_size/2] = psd_temp[self.fft_size/2:self.fft_size]

        
        print psd
        return psd

    def GetPSD(self):
        LOOP = 50
        self.msgq.flush()
        
        # linklab, loop to empty the lower layer buffers to avoid detecting old signals
        while self.msgq.count() < 60:
            pass
        self.msgq.flush()

        # linklab, loop until received enough number of observations for smoothing
        nitems = 0
        s = ""
        while nitems < SMOOTH_LENGTH:
            msg = self.msgq.delete_head()  # blocking read of message queue
            nitems += int(msg.arg2())
            s += msg.to_string()

        # linklab, smooth psd by averaging over multiple observations
        itemsize = int(msg.arg1())
        psd_temp = numpy.zeros(self.fft_size)
        for i in range(0, SMOOTH_LENGTH):
            start = itemsize * (SMOOTH_LENGTH - i - 1)
            tmp = s[start:start+itemsize]
            psd_temp += numpy.fromstring(tmp, numpy.float32)/SMOOTH_LENGTH
	
	#print psd_temp

        # linklab, rearrange psd bins
        psd = psd_temp.copy()
        psd[self.fft_size/2:self.fft_size] = psd_temp[0:self.fft_size/2]
        psd[0:self.fft_size/2] = psd_temp[self.fft_size/2:self.fft_size]

        return psd 

    # linklab, get available subcarriers via edge detection
    def GetAvailableSpectrum(self):
        global SENSE_ALLTHR_MAX_COUNT, SENSE_ALLTHR_MAX, SENSE_ALLTHR_MIN_COUNT, SENSE_ALLTHR_MIN
        
        # get PSD map
        psd = self.GetPSD()
       
        N = self.fft_size

        # calculate the number of bins on the left that are not allocatable
        zeros_on_left = int(math.ceil((self._fft_length - self._occupied_tones)/2.0)) * self.interp_rate
        
        # init avail_subc_bin to indicate all subcarriers are available
        avail_subc_bin = numpy.ones(self._occupied_tones)
       
        # freq domain smoothing
        for i in range(1, N-1):
            psd[i] = (psd[i-1] + psd[i] + psd[i+1]) / 3


        # identify and mark the rising and dropping edges
        # edge_markers: 1 rising, -1 dropping, 0 no edge
        edge_markers = numpy.zeros(N)
        for i in range(2,N-3):
            diff_forward = psd[i+2] - psd[i];
            diff_backward = psd[i] - psd[i-2];
            diff = psd[i+1] - psd[i-1]
            
            if diff_forward > EDGE_THRES:    # check rising edges
                edge_markers[i] = 1
            elif diff_backward < -EDGE_THRES:# check dropping edges
                edge_markers[i] = -1
    
        # use edge information to mark unavailable subcarriers
        # we treat the left half and right half separately to avoid the central freq artifact
        avail = numpy.zeros(N)

        # the right half subcarriers
        avail[0] = 1
        for i in range(1,N/2):
            if ((avail[i-1] == 1) and (edge_markers[i] == 1)):
                avail[i] = 0
            elif ((avail[i-1] == 0) and (edge_markers[i] == -1)) and edge_markers[i+1] != -1:
                avail[i] = 1
            else:
                avail[i] = avail[i-1]

        # the left half subcarriers
        avail[N-1] = 1
        for j in range(1,N/2):
            i = N - j - 1
            if ((avail[i+1] == 1) and (edge_markers[i] == -1)): 
                avail[i] = 0
            elif ((avail[i+1] == 0) and (edge_markers[i] == 1)) and edge_markers[i-1] != 1:
                    avail[i] = 1
            else:
                avail[i] = avail[i+1]

        # combine edge detection sensing results with energy sensing with HIGH_THRES and LOW_THRES
        for i in range(zeros_on_left, self.fft_size - zeros_on_left - 1):
            
            # map the PSD index i to subcarrier index
            carrier_index = (i - zeros_on_left) / self.interp_rate
            
            # if power very high or detected busy from edge detection, unavailable
            if psd[i] > HIGH_THRES  or avail[i] == 0:
                avail_subc_bin[carrier_index] = 0
            # if power very low, available 
            if psd[i] < LOW_THRES: 
                avail_subc_bin[carrier_index] = 1
    	#print psd
    	
    	
    	
    	temp_result_1=sum(psd[round(1*FFT_SIZE/12,0):round(3*FFT_SIZE/12,0)])/(round(3*FFT_SIZE/12,0)-round(1*FFT_SIZE/12,0))
    	temp_result_2=sum(psd[round(5*FFT_SIZE/12,0):round(7*FFT_SIZE/12,0)])/(round(7*FFT_SIZE/12,0)-round(5*FFT_SIZE/12,0))
    	temp_result_3=sum(psd[round(9*FFT_SIZE/12,0):round(11*FFT_SIZE/12,0)])/(round(11*FFT_SIZE/12,0)-round(9*FFT_SIZE/12,0))
    	#temp_result_2=sum(psd[round(FFT_SIZE/3,0):round(2*FFT_SIZE/3,0)])/(round(2*FFT_SIZE/3,0) - (round(FFT_SIZE/3,0)))
    	#temp_result_3=sum(psd[round(2*FFT_SIZE/3,0):round(FFT_SIZE,0)])/(round(FFT_SIZE,0) -  (round(2*FFT_SIZE/3,0)))
    	print temp_result_1
    	print temp_result_2
    	print temp_result_3
    	
    	
    	temp_result=[temp_result_1, temp_result_2, temp_result_3];
    	temp_result_max=max(temp_result);
    	temp_result_min=min(temp_result);
    	temp_result_dif=temp_result_max-temp_result_min;
    	
    	if temp_result_dif >= self._thr_sense: #some channel is available, some is non-available, the default value of _thr_sense=15dB
    	    sense_thread=(temp_result_max+temp_result_min)/2;
        else:  #all channels are availalbe or non-available  	    
    	    sense_thread=(SENSE_ALLTHR_MAX+SENSE_ALLTHR_MIN)/2;
    	    temp_result_ave=sum(temp_result)/3;
    	    #if all channels are available, the gain should be less than -130dB
    	    #if all channels are non-available, the gain should be larger than -130dB
    	    if temp_result_ave>=sense_thread: # all channels are non-available, update the sense_allthr_max
    	        SENSE_ALLTHR_MAX_COUNT += 1;
    	        SENSE_ALLTHR_MAX = (SENSE_ALLTHR_MAX*SENSE_ALLTHR_MAX_COUNT+ temp_result_ave)/(SENSE_ALLTHR_MAX_COUNT+1);
    	    else: #all channels are available, update the sense_allthr_min
    	        SENSE_ALLTHR_MIN_COUNT += 1;
    	        SENSE_ALLTHR_MIN = (SENSE_ALLTHR_MIN*SENSE_ALLTHR_MIN_COUNT+ temp_result_ave)/(SENSE_ALLTHR_MIN_COUNT+1);    	        
    	                	
    	
    	result=[1, 1, 1]
    	result_worst = 1
    	result_best =1
    	
    	if temp_result_1 >= sense_thread:
    	    result[0]=0  
    	if temp_result_2 >= sense_thread:
    	    result[1]=0   	    
    	if temp_result_3 >= sense_thread:
    	    result[2]=0
    	    
        if temp_result_1 == temp_result_max: #the left channel is the worst one
            result_worst = 0
        if temp_result_1 == temp_result_min: #the left channel is the best one
            result_best = 0    
        if temp_result_2 == temp_result_max: #the mid channel is the worst one
            result_worst = 1
        if temp_result_2 == temp_result_min: #the mid channel is the best one
            result_best = 1   
        if temp_result_3 == temp_result_max: #the right channel is the worst one
            result_worst = 2
        if temp_result_3 == temp_result_min: #the right channel is the best one
            result_best = 2                                   
                    
    	    
    	    	    
    	print psd   
        #print SENSE_ALLTHR_MAX_COUNT
        #print SENSE_ALLTHR_MAX
        #print SENSE_ALLTHR_MIN_COUNT
        #print SENSE_ALLTHR_MIN        
        #print sense_thread
        #print result    
        
        mask11=[106,107]
        #ave1=[7,8,13,14] 
        ave11=[102,103,110,111]
        mask12=[149,150]
        #ave1=[7,8,13,14] 
        ave12=[145,146,153,154]
        
        
        mask21=[85,86]
        #ave2=[18,19,24,25]
        ave21=[81,82,89,90]
        mask22=[170,171]
        #ave2=[18,19,24,25]
        ave22=[166,167,174,175]
        
        mask31=[63,64]
        #ave3=[39,40,45,46]
        ave31=[59,60,67,68]
        mask32=[191,192]
        #ave3=[39,40,45,46]
        ave32=[187,188,195,196]
        
        mask41=[42,43]
        #ave4=[50,51,56,58]
        ave41=[38,39,46,47]
        mask42=[213,214]
        #ave4=[50,51,56,58]
        ave42=[209,210,217,218]
        
        mask51=[21,22]        
        #ave4=[50,51,56,58]
        ave51=[17,18,25,26]
        mask52=[234,235]
        #ave4=[50,51,56,58]
        ave52=[230,231,238,239]
        
        narrow11=sum(psd[mask11])/2;
        narrow11_neighbor=sum(psd[ave11])/4;
        narrow12=sum(psd[mask12])/2;
        narrow12_neighbor=sum(psd[ave12])/4;        
        
        narrow21=sum(psd[mask21])/2;
        narrow21_neighbor=sum(psd[ave21])/4;
        narrow22=sum(psd[mask22])/2;
        narrow22_neighbor=sum(psd[ave22])/4;                
                 
        narrow31=sum(psd[mask31])/2;
        narrow31_neighbor=sum(psd[ave31])/4;
        narrow32=sum(psd[mask32])/2;
        narrow32_neighbor=sum(psd[ave32])/4;
        
        narrow41=sum(psd[mask41])/2;
        narrow41_neighbor=sum(psd[ave41])/4;
        narrow42=sum(psd[mask42])/2;
        narrow42_neighbor=sum(psd[ave42])/4;             
        
        narrow51=sum(psd[mask51])/2;
        narrow51_neighbor=sum(psd[ave51])/4;
        narrow52=sum(psd[mask52])/2;
        narrow52_neighbor=sum(psd[ave52])/4;    
        
                          

        narrow_flag=[0,0,0,0,0]
        
        narrow_diff=20
        
        if (narrow11 >= (narrow11_neighbor+narrow_diff)) & (narrow12 >= (narrow12_neighbor+narrow_diff)):
            narrow_flag[0] =1;
 
        if (narrow21 >= (narrow21_neighbor+narrow_diff)) & (narrow22 >= (narrow22_neighbor+narrow_diff)):
            narrow_flag[1] =1;           
            
        if (narrow31 >= (narrow31_neighbor+narrow_diff)) & (narrow32 >= (narrow32_neighbor+narrow_diff)):
            narrow_flag[2] =1;
 
        if (narrow41 >= (narrow41_neighbor+narrow_diff)) & (narrow42 >= (narrow42_neighbor+narrow_diff)):
            narrow_flag[3] =1; 
        if (narrow51 >= (narrow51_neighbor+narrow_diff)) & (narrow52 >= (narrow52_neighbor+narrow_diff)):
            narrow_flag[4] =1;             
        
        #print narrow1
        #print narrow1_neighbor
        #print narrow_flag

        index_msg=narrow_flag[0] + 2* narrow_flag[1]+ 4* narrow_flag[2] + 8* narrow_flag[3] + 16* narrow_flag[4]
        
        print narrow_flag
        print index_msg
        
        #if narrow_sum_flag >= 4:
        #    DONE1 = 1
        #else:
        #    DONE1 = 0        

#######################################################        
        
        #mask=[10,11,21,22,42,43,53,54]  #starting from 0
        #mask_ext=[9,10,11,12,20,21,22,23,41,42,43,44, 52,53,54,55]  #starting from 0
        
        #temp_psd_max=max(psd)
        #temp_psd_min=min(psd)
        #temp_psd_diff=temp_psd_max- temp_psd_min
        ##print psd
        #psd_max=max(psd)
        ##print psd_max
        #psd_desired_index=psd[mask]
        ##print psd_desired_index
        #psd[mask_ext]=-200
        
        #max_rest=max(psd)
        #min_desired=min(psd_desired_index)
        
        
        #max_rest=-200
        #FFT_SIZE_Half= FFT_SIZE/2
        #for i in range(0, FFT_SIZE_Half-2):
        #     temp= min(psd[i], psd[FFT_SIZE-1-i])
        #     if temp>max_rest:
        #         max_rest=temp
        #print "test"        
        #print min_desired
        #print max_rest
        #print "test"         
                 
 

        #if min_desired> (max_rest + 2.5):
        #    DONE2 = 1
        #else:
        #    DONE2 = 0
        
      
        #DONE = DONE2
        
        #print 0
#       # print "test"
#       # print psd[1]
#        print "test"
 
#       # if PATTERN       
        
        
        
       
        
        
        
        
        
        return result, result_worst, result_best, index_msg#DONE  #, PATTERN
        #return avail_subc_bin

