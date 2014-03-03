#!/usr/bin/env python
#
# Copyright 2010,2011 Free Software Foundation, Inc.
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

from gnuradio import gr
from gnuradio import eng_notation
from gnuradio.eng_option import eng_option
from optparse import OptionParser

# From gr-digital
from gnuradio import digital

# from current dir
from transmit_path import transmit_path
from uhd_interface import uhd_transmitter

#import for sense
import threading
from uhd_interface import uhd_receiver
from sensing_path import sensing_path
#from allocation import *
#from sensereceive_path import sensereceive_path
from receive_path import receive_path 

import time, struct, sys

#import os 
#print os.getpid()
#raw_input('Attach and press enter')

global bandchoose
bandchoose = 2

class my_top_block(gr.top_block):
    def __init__(self, modulator, demodulator, rx_callback, options):
        gr.top_block.__init__(self)


#####################################################################
        #samp_rate= 3000000
        samp_rate = options.bitrate*options.samples_per_symbol
        fa = 1.25* samp_rate/4 #1000000

        global bandchoose

        if(options.tx_freq is not None):
            # Work-around to get the modulation's bits_per_symbol
            args = modulator.extract_kwargs_from_options(options)
            #symbol_rate = 1500000 / modulator(**args).bits_per_symbol()    #BZ
            symbol_rate = options.bitrate / modulator(**args).bits_per_symbol()

            self.sink = uhd_transmitter(options.args, symbol_rate,
                                        options.samples_per_symbol,
                                        options.tx_freq, options.tx_gain,
                                        options.spec, options.antenna,1)
                                        #options.verbose)
            options.samples_per_symbol = self.sink._sps
            
        elif(options.to_file is not None):
            sys.stderr.write(("Saving samples to '%s'.\n\n" % (options.to_file)))
            self.sink = gr.file_sink(gr.sizeof_gr_complex, options.to_file)
        else:
            sys.stderr.write("No sink defined, dumping samples to null sink.\n\n")
            self.sink = gr.null_sink(gr.sizeof_gr_complex)

        # do this after for any adjustments to the options that may
        # occur in the sinks (specifically the UHD sink)
        #self.txpath = transmit_path(modulator, options)

        #self.connect(self.txpath, self.sink)

	# do this after for any adjustments to the options that may
        # occur in the sinks (specifically the UHD sink)
        self.txpath0 = transmit_path(modulator, options)
        self.txpath1 = transmit_path(modulator, options)
        self.txpath2 = transmit_path(modulator, options)

	#self.connect(self.txpath, self.sink)

	# Define the math operator blocks
	

	# Generate exp(jw1t) and exp(-jw1t)
        self.gr_multiply_xx_0 = gr.multiply_vff(1)
        self.gr_float_to_complex_0_0 = gr.float_to_complex(1)
        self.gr_float_to_complex_0 = gr.float_to_complex(1)
        self.const_source_x_0 = gr.sig_source_f(0, gr.GR_CONST_WAVE, 0, -1, 0)
        self.analog_sig_source_x_0_0 = gr.sig_source_f(samp_rate, gr.GR_SIN_WAVE, fa, 1, 0)
        self.analog_sig_source_x_0 = gr.sig_source_f(samp_rate, gr.GR_COS_WAVE, fa, 1, 0)

        self.const_source_x_00 = gr.sig_source_c(0, gr.GR_CONST_WAVE, 0, 1, 0)
        self.const_source_x_11 = gr.sig_source_c(0, gr.GR_CONST_WAVE, 0, 1, 0)
        self.const_source_x_22 = gr.sig_source_c(0, gr.GR_CONST_WAVE, 0, 1, 0)
        self.gr_multiply_xx_00 = gr.multiply_vcc(1)
        self.gr_multiply_xx_11 = gr.multiply_vcc(1)
        self.gr_multiply_xx_22 = gr.multiply_vcc(1)
        
        

	# Combine signal from two subbands
        self.gr_multiply_xx_1 = gr.multiply_vcc(1)
        self.gr_multiply_xx_2 = gr.multiply_vcc(1)
        self.gr_c2f_1 = gr.complex_to_float(1)
        self.gr_c2f_2 = gr.complex_to_float(1)
        self.gr_c2f_0 = gr.complex_to_float(1)
        self.gr_add_xx_re = gr.add_vff(1)
        self.gr_add_xx_im = gr.add_vff(1)
        self.gr_add_xx_re_1 = gr.add_vff(1)
        self.gr_add_xx_im_1 = gr.add_vff(1)

        self.gr_f2c = gr.float_to_complex(1)
	
        self.gr_null_source_0 = gr.null_source(gr.sizeof_gr_complex*1)

	# output from gr_float_to_complex_0_0 is exp(-jw1t)
	# output from gr_float_to_complex_0 is exp(jw1t)
        self.connect((self.gr_multiply_xx_0, 0), (self.gr_float_to_complex_0_0, 1))
        self.connect((self.analog_sig_source_x_0, 0), (self.gr_float_to_complex_0_0, 0))
        self.connect((self.analog_sig_source_x_0_0, 0), (self.gr_float_to_complex_0, 1))
        self.connect((self.analog_sig_source_x_0, 0), (self.gr_float_to_complex_0, 0))
        self.connect((self.analog_sig_source_x_0_0, 0), (self.gr_multiply_xx_0, 0))
        self.connect((self.const_source_x_0, 0), (self.gr_multiply_xx_0, 1)) 

	# txpath1 * exp(-jw1t)
	#self.connect(self.txpath1, (self.gr_multiply_xx_1, 0))
        
	# txpath2 * exp(jw1t)
	#self.connect(self.txpath2, (self.gr_multiply_xx_2, 0))
	
        



       
        self.connect(self.txpath1, (self.gr_multiply_xx_11, 0))
        self.connect((self.const_source_x_11, 0), (self.gr_multiply_xx_11, 1))
        self.connect((self.gr_multiply_xx_11, 0), (self.gr_multiply_xx_1, 0))
        self.connect((self.gr_float_to_complex_0_0, 0), (self.gr_multiply_xx_1, 1))
        self.connect((self.gr_multiply_xx_1, 0), self.gr_c2f_1)
       
        self.connect(self.txpath2, (self.gr_multiply_xx_22, 0))
        self.connect((self.const_source_x_22, 0), (self.gr_multiply_xx_22, 1))
        self.connect((self.gr_multiply_xx_22, 0), (self.gr_multiply_xx_2, 0))     
        self.connect((self.gr_float_to_complex_0, 0), (self.gr_multiply_xx_2, 1))
        self.connect((self.gr_multiply_xx_2, 0), self.gr_c2f_2)
       
        self.connect(self.txpath0, (self.gr_multiply_xx_00, 0))
        self.connect((self.const_source_x_00, 0), (self.gr_multiply_xx_00, 1))
        self.connect((self.gr_multiply_xx_00, 0), self.gr_c2f_0)
      
        

        self.connect((self.gr_c2f_1,0), (self.gr_add_xx_re,0))
        self.connect((self.gr_c2f_2,0), (self.gr_add_xx_re,1))

        self.connect(self.gr_add_xx_re, (self.gr_add_xx_re_1,0))
        self.connect((self.gr_c2f_0,0), (self.gr_add_xx_re_1,1))

        self.connect((self.gr_c2f_1,1), (self.gr_add_xx_im,0))
        self.connect((self.gr_c2f_2,1), (self.gr_add_xx_im,1))

        self.connect(self.gr_add_xx_im, (self.gr_add_xx_im_1,0))
        self.connect((self.gr_c2f_0,1), (self.gr_add_xx_im_1,1))

        self.connect(self.gr_add_xx_re_1, (self.gr_f2c,0))
        self.connect(self.gr_add_xx_im_1, (self.gr_f2c,1))
        self.txgate = gr.copy(gr.sizeof_gr_complex)
        self.connect(self.gr_f2c, self.txgate, self.sink)



########################################################################















        self.tx_enabled = True
        sense_symbol_rate=2500000
        sense_samples_per_symbol=2
        sense_rx_freq=2500000000
        sense_rx_gain=20
        options.chbw_factor=1



        self.sensesource=uhd_receiver(options.args, sense_symbol_rate,
                                       sense_samples_per_symbol,
                                       sense_rx_freq, sense_rx_gain,
                                       options.spec, options.antenna,
                                       options.verbose)

        self.sensegate = gr.copy(gr.sizeof_gr_complex)

	# do sense
        self.sensepath = sensing_path(options)


	self.sense_flag=False
        self.senserxpath = receive_path(demodulator, rx_callback, options) 
        self.connect(self.sensesource, self.sensepath)

	

# /////////////////////////////////////////////////////////////////////////////
#                                   main
# /////////////////////////////////////////////////////////////////////////////
shutdown_event = threading.Event()

def main():
    #global tx_enabled, sense_flag
    #tx_enabled = False
    #sense_flag = True
  

#    def send_pkt(payload='', eof=False):
#        return tb.txpath.send_pkt(payload, eof)

    def send_pkt0(payload='', eof=False):
        return tb.txpath0.send_pkt(payload, eof)

    def send_pkt1(payload='', eof=False):
        return tb.txpath1.send_pkt(payload, eof)

    def send_pkt2(payload='', eof=False):
        return tb.txpath2.send_pkt(payload, eof)

    global n_rcvd, n_right 

    n_rcvd = 0
    n_right = 0
    
    def rx_callback(ok, payload):
        global n_rcvd, n_right
        (pktno,) = struct.unpack('!H', payload[0:2])
        n_rcvd += 1
        if ok:
            n_right += 1

        #print "ok = %5s  pktno = %4d  n_rcvd = %4d  n_right = %4d" % (
        #    ok, pktno, n_rcvd, n_right)
	
    #def dowork():
    #    global tx_enabled
    #    while not shutdown_event.is_set():
            #tx_enabled=False
            #tb.txgate.set_enabled(False)
            #time.sleep(0.2)
            #tb.sensegate.set_enabled(True)
            #time.sleep(0.2)
    #        if (time.time() % 10 <= 8):
    #            tx_enabled=True
    #            tb.txgate.set_enabled(True)
                #time.sleep(.01)
    #            tb.sensegate.set_enabled(False)
    #            sense_flag=True                 
    #        else:
    #            if sense_flag:
    #                tx_enabled=False
    #                tb.txgate.set_enabled(False)
                #time.sleep(.01)
    #                tb.sensegate.set_enabled(True)
    #                sense_flag=False
    #            else:
    #                tx_enabled=True
    #                tb.txgate.set_enabled(True)
                    #time.sleep(.01)
    #                tb.sensegate.set_enabled(False)                       	
                
    #        time.sleep(0.2)				

    mods = digital.modulation_utils.type_1_mods()
    demods = digital.modulation_utils.type_1_demods()

    parser = OptionParser(option_class=eng_option, conflict_handler="resolve")
    expert_grp = parser.add_option_group("Expert")

    parser.add_option("-m", "--modulation", type="choice", choices=mods.keys(),
                      default='psk',
                      help="Select modulation from: %s [default=%%default]"
                            % (', '.join(mods.keys()),))
    #parser.add_option("-d", "--demodulation", type="choice", choices=demods.keys(),
    #                  default='gmsk',
    #                  help="Select demodulation for sensing from: %s [default=%%default]"
    #                        % (', '.join(demods.keys()),))
    parser.add_option("-s", "--size", type="eng_float", default=1500,
                      help="set packet size [default=%default]")
    parser.add_option("-M", "--megabytes", type="eng_float", default=1.0,
                      help="set megabytes to transmit [default=%default]")
    parser.add_option("","--discontinuous", action="store_true", default=False,
                      help="enable discontinous transmission (bursts of 5 packets)")
    parser.add_option("","--from-file", default=None,
                      help="use intput file for packet contents")
    parser.add_option("","--to-file", default=None,
                      help="Output file for modulated samples")

    transmit_path.add_options(parser, expert_grp)
    uhd_transmitter.add_options(parser)


    for mod in mods.values():
        mod.add_options(expert_grp)

    (options, args) = parser.parse_args ()

    if len(args) != 0:
        parser.print_help()
        sys.exit(1)
           
    if options.from_file is not None:
        source_file = open(options.from_file, 'r')

    # build the graph
    tb = my_top_block(mods[options.modulation], demods[options.modulation], rx_callback, options)

    r = gr.enable_realtime_scheduling()
    if r != gr.RT_OK:
        print "Warning: failed to enable realtime scheduling"

    tb.start()                       # start flow graph
    #t = threading.Thread(target=dowork, args=(), name='worker')
    #t.start()

        
    # generate and send packets
    nbytes = int(1e6 * options.megabytes)
    n = 0
    sense_n=1
    pktno = 0
    pkt_size = int(options.size)
    count_pkt=0	

    while n < nbytes:
        if count_pkt%500 == 0 and sense_n==1: #sence once
            tb.txgate.set_enabled(False)
            time.sleep(.5)
            tb.sensegate.set_enabled(True) #t
            sense_result = tb.sensepath.GetAvailableSpectrum()
            
            #avail_subc_str = subc_bin2str(avail_subc_bin)
 #           print avail_subc_bin
            #time.sleep(0.002)
            sense_n=0
            #tb.txgate.set_enabled(True)
            #tb.sensegate.set_enabled(False)
        else:
            # linklab, loop to empty the lower layer buffers to avoid detecting old signals
            #send_pkt(eof=False)
            count_pkt += 1
            sense_n=1
            tb.txgate.set_enabled(True) #t
            #time.sleep(.01)
            tb.sensegate.set_enabled(False)
            #time.sleep(0.001)
            if options.from_file is None:
                data = (pkt_size - 2) * chr(pktno & 0xff) 
                
            else:
                data = source_file.read(pkt_size - 2)
                if data == '':
                    break;

            payload = struct.pack('!H', pktno & 0xffff) + data
            
            
            data_waste=(pkt_size - 2) * chr(0xff)

            #sense_result=[0,0,0]
            
            
            #time.sleep(1)
            
            if sense_result[0]==1:  # left
                tb.const_source_x_11.set_amplitude(1)
                payload1 = struct.pack('!H', pktno & 0xffff) + data
                send_pkt1(payload1)
                pktno += 1    
                n += len(payload1)
                #print 0
                sys.stderr.write('.')
            else:
                tb.const_source_x_11.set_amplitude(0)
                payload1 = struct.pack('!H', pktno & 0xffff) + data_waste
                send_pkt1(payload1)	            
	            
            if sense_result[1]==1:  #medium
                tb.const_source_x_00.set_amplitude(1)
                payload0 = struct.pack('!H', pktno & 0xffff) + data
                send_pkt0(payload0)
                pktno += 1    
                n += len(payload0)
                sys.stderr.write('.')
                #print 1
            else:
                tb.const_source_x_00.set_amplitude(0)
                payload0 = struct.pack('!H', pktno & 0xffff) + data_waste
                send_pkt0(payload0)	     	
                	            	        
            if sense_result[2]==1:  #right
                tb.const_source_x_22.set_amplitude(1)
                payload2 = struct.pack('!H', pktno & 0xffff) + data
                send_pkt2(payload2)
                pktno += 1    
                n += len(payload2)
                sys.stderr.write('.')
                #print 2	
            else:
                tb.const_source_x_22.set_amplitude(0)
                payload2 = struct.pack('!H', pktno & 0xffff) + data_waste
                send_pkt2(payload2)	                
                
            if options.discontinuous and pktno % 5 == 4:
                time.sleep(1)
            #pktno += 1


#            pktno += 1
#    send_pkt(eof=True)


 
    send_pkt0(eof=True)
    send_pkt1(eof=True)
    send_pkt2(eof=True)

    tb.wait()                       # wait for it to finish

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
