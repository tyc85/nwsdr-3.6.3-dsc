#!/usr/bin/env python
#
# Copyright 2005,2006,2011 Free Software Foundation, Inc.
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
import time, struct, sys, socket, math, numpy
import threading
from gnuradio import digital

# from current dir
from transmit_path import transmit_path
from uhd_interface import uhd_transmitter
from receive_path import receive_path
from uhd_interface import uhd_receiver
from gnuradio import uhd

BANDWIDTH_H = 5000000
BANDWIDTH_L = 2000000
ROUNDSIZE = 1000

class my_top_block(gr.top_block):
    def __init__(self, options):
        gr.top_block.__init__(self)

        self._opts = options

        if(options.tx_freq is not None):
            self.sink = uhd_transmitter(options.args,
                                        options.bandwidth,
                                        options.tx_freq, options.tx_gain,
                                        options.spec, options.antenna,
                                        options.verbose)
        elif(options.to_file is not None):
            self.sink = gr.file_sink(gr.sizeof_gr_complex, options.to_file)
        else:
            self.sink = gr.null_sink(gr.sizeof_gr_complex)

        # do this after for any adjustments to the options that may
        # occur in the sinks (specifically the UHD sink)
        self.txpath = transmit_path(options)

        # receive path
        if(options.rx_freq is not None):
            self.source = uhd_receiver(options.args,
                                       options.bandwidth,
                                       options.rx_freq, options.rx_gain,
                                       options.spec, options.antenna,
                                       options.verbose)
        elif(options.from_file is not None):
            self.source = gr.file_source(gr.sizeof_gr_complex, options.from_file)
        else:
            self.source = gr.null_source(gr.sizeof_gr_complex)

        # Resample signal to exactly self.usrp_rate
        # FIXME: make one of the follow-on filters an arb resampler
        #rrate = usrp_rate / dev_rate
        #self.resamp = blks2.pfb_arb_resampler_ccf(rrate)

        
        fl = 512
        d = 5

        self.decim=block_decim(10,fl)
        self.decim=block_decim(d,fl)
        self.sum=block_add(fl)
        self.fft = block_fft2(fl)
        self.sink2 = gr.null_sink(4)
 

        self.txgate = gr.copy(gr.sizeof_gr_complex)
        self.rxgate = gr.copy(gr.sizeof_gr_complex)
    
        # now wire it all together
        self.connect (self.source, self.rxgate, self.decim, self.fft, self.sum, self.sink2)
        self.connect(self.txpath, self.txgate, self.sink)
        
        self.tx_enabled = False

class dsc_pkt_src(object):
    def __init__(self, server, port=5123 ):
        self.pkt_size = 1440 # 1440 bytes of data  
        self.pkt_server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.pkt_server_socket.connect((server,port))
        self.MESSAGE = struct.pack('!l', self.pkt_size)
        self.pkt_server_socket.send(self.MESSAGE)

    def read(self):
        try:
            data = self.pkt_server_socket.recv(self.pkt_size)
        except socket.error:
            print "Connection to packet server closed"
            return ''
       	self.pkt_server_socket.send(self.MESSAGE)
        return data
        
## block_decim ##
class block_decim(gr.decim_block):
    nblock=1
    block_size=1
    def __init__(self, in_nblock, in_block_size):
	self.nblock=in_nblock
	self.block_size=in_block_size
        gr.decim_block.__init__(
            self,
            name = "block_decim",
            in_sig = [numpy.complex64(self.block_size)],
            out_sig = [numpy.complex64(self.block_size)],
            decim = self.nblock
        )

    def work(self, input_items, output_items):
        if len(input_items[0])>= self.nblock*self.block_size:
            output_items[0][0:self.block_size]=input_items[0][0:self.block_size]
            return self.block_size
        else:
            return 0

## block_add ##
class block_add(gr.decim_block):
    S=[]
    block_size=1
    def __init__(self, in_block_size):
        self.block_size=in_block_size
        self.S=numpy.zeros(self.block_size)
        gr.decim_block.__init__(
            self,
            name = "block_add",
            in_sig = [numpy.float32(self.block_size)],
            out_sig = [numpy.float32(self.block_size)],
            decim = 1
        )

    def work(self, input_items, output_items):
        if len(input_items[0])>= self.block_size:
            self.S+= input_items[0][0:self.block_size]
            return self.block_size
        else:
            return 0

## block_fft2 ##
class block_fft2(gr.decim_block):
    fft_size=1
    def __init__(self, in_fft_size):
	self.fft_size=in_fft_size
        gr.decim_block.__init__(
            self,
            name = "block_fft2",
            in_sig = [numpy.complex64(self.fft_size)],
            #out_sig = [numpy.complex64(self.fft_size)],
            out_sig = [numpy.float32(self.fft_size)],
            decim=1
        )
    def work(self, input_items, output_items):
        #print "fft in=", input_items[0], self.fft_size
        if len(input_items[0]) >= self.fft_size:
            a=numpy.fft.fft(input_items[0][0:self.fft_size],self.fft_size)
            output_items[0][0:self.fft_size]=numpy.square(numpy.absolute(a))
            return self.fft_size
            #return len(output_items[0])
        else:
            return 0

# /////////////////////////////////////////////////////////////////////////////
#                                   main
# /////////////////////////////////////////////////////////////////////////////
shutdown_event = threading.Event()

def main():

    global n_rcvd, n_right, missing_packets, tx_enabled, tx_round, tb, totaldata
        
    n_rcvd = 0
    n_right = 0
    tx_round = 0
    tx_enabled = False
    totaldata = []
    first_run = True
    pktno = 0
    sendFullFile = True

    def dowork():
        global n_rcvd, n_right, missing_packets, tx_enabled, tx_round, tb, totaldata
        
        fbstate = 7
        while not shutdown_event.is_set():
            if time.time() % 10 <= 9 and tx_enabled == False:
                tx_enabled = True            
                tb.txgate.set_enabled(True)
                tb.rxgate.set_enabled(False)
                #sys.stderr.write("TX")

            if time.time() % 10 > 9 and tx_enabled == True:
                tx_enabled = False
                tb.txgate.set_enabled(False)
                time.sleep(0.2)
                tb.rxgate.set_enabled(True)
                tb.sum.S=numpy.zeros(512)
                #sys.stderr.write("RX")
                time.sleep(0.75)
                fbstate = read_tones()
                
                ########################################
                # Feedback message
                # fbstate = 9: High rate 
                # fbstate = 7: Low rate
                ##########################################
                if fbstate == 9 and tb._opts.bandwidth!=BANDWIDTH_H: # High-rate
                    tb._opts.bandwidth = BANDWIDTH_H
                    tb.sink.set_sample_rate(tb._opts.bandwidth)
                    tb.source.set_sample_rate(tb._opts.bandwidth)
                elif fbstate ==7 and tb._opts.bandwidth!=BANDWIDTH_L: # Low-rate
                    tb._opts.bandwidth = BANDWIDTH_L
                    tb.sink.set_sample_rate(tb._opts.bandwidth)
                    tb.source.set_sample_rate(tb._opts.bandwidth)
                elif fbstate !=9 and fbstate!=7 and tb._opts.bandwidth!=BANDWIDTH_L:
                    tb._opts.bandwidth = BANDWIDTH_L
                    tb.sink.set_sample_rate(tb._opts.bandwidth)
                    tb.source.set_sample_rate(tb._opts.bandwidth)
                #elif fbstate requires retransmission for tx_round
                #    tx_round = (tx_round +1) 
		
                    
                
            time.sleep(.01)

    def read_tones():
        global tb
        #t = time.time()
        #while time.time() < 1 +t:
         #   pass
        s=tb.sum.S
        thresh=sum(s[6:505])/500
        val=numpy.zeros(16)
        for i in range(20,260,15):
            val[(i-20)/15] = min(s[i],s[i+250])
        temp = 0
        for i in range(0,16):
            if temp<val[i]:
                temp = val[i]
                idx = i

	# Modified by Xu Chen: 2013-8-28
	'''
        if(temp>thresh):
            print "\nfound  max=", idx, "avg", thresh, "val", temp
        else:
            print "\nMISSED max=", idx, "avg", thresh, "val", temp
            idx = -1
	'''
	if (temp<=thresh):
		idx = -1

        return idx


    def send_pkt(payload='', eof=False):
        return tb.txpath.send_pkt(payload, eof)

    parser = OptionParser(option_class=eng_option, conflict_handler="resolve")
    expert_grp = parser.add_option_group("Expert")
    parser.add_option("", "--size", type="eng_float", default=400,
                      help="set packet size [default=%default]")
    parser.add_option("-M", "--megabytes", type="eng_float", default=1.0,
                      help="set megabytes to transmit [default=%default]")
    parser.add_option("","--discontinuous", action="store_true", default=False,
                      help="enable discontinuous mode")
    parser.add_option("","--from-file", default=None,
                      help="use intput file for packet contents")
    parser.add_option("","--to-file", default=None,
                      help="Output file for modulated samples")
    parser.add_option("","--discontinuous", action="store_true", default=False,
                      help="enable discontinuous")
    parser.add_option("","--from-file", default=None,
                      help="input file of samples to demod")
    parser.add_option("","--mode", default="COMP",
                      help="set match style [default=%default]")
    parser.add_option("-s", "--server", default="idb2",
                      help="server host name [default=%default]")


    transmit_path.add_options(parser, expert_grp)
    digital.ofdm_mod.add_options(parser, expert_grp)
    uhd_transmitter.add_options(parser)
    receive_path.add_options(parser, expert_grp)
    uhd_receiver.add_options(parser)
    digital.ofdm_demod.add_options(parser, expert_grp)

    (options, args) = parser.parse_args ()

    # Xu Chen: Hard Code Parameters 
    options.bandwidth = BANDWIDTH_L
    options.fft_length = 512
    options.occupied_tones = 300
    options.cp_length = 30
    options.tx_amplitude = 1
    options.modulation = "bpsk"
    options.tx_gain = 5
    options.rx_gain = 38
    
    if options.from_file is None:
        if options.rx_freq is None:
            sys.stderr.write("You must specify -f FREQ or --freq FREQ\n")
            parser.print_help(sys.stderr)
            sys.exit(1)

    # packet_source
    #serve = dsc_pkt_src(options.server)

    # build the graph
    tb = my_top_block(options)
    
    r = gr.enable_realtime_scheduling()
    if r != gr.RT_OK:
        print "Warning: failed to enable realtime scheduling"

    tb.start()                       # start flow graph
    
    t = threading.Thread(target=dowork, args=(), name='worker')
    t.start()
    
    while True:
        if tx_enabled:
            if first_run:
                #data = serve.read()
		# repeat the character "10" 1440 times and store in data
                data = 1440* chr(10)
                if len(data) != 1440: # End of file or Cannot fetch data
                    # print "No More"

                    if len(totaldata) == 0: # cannot fetch data from server
                        pass
                    else: # End of file
                        first_run = False
                else:
                    payload = struct.pack('!H', pktno & 0xffff) + data
                    send_pkt(payload)
                    sys.stderr.write('.')
                    totaldata.append(data)
                    pktno += 1

            else:
                if len(totaldata) > pktno+tx_round*ROUNDSIZE:
                    payload = struct.pack('!H', pktno & 0xffff) + totaldata[pktno + tx_round*ROUNDSIZE]
                else:
                    pktno = 0
                    payload = struct.pack('!H', pktno & 0xffff) + totaldata[pktno + tx_round*ROUNDSIZE]

                if len(payload) == 1442: # Make sure payload is 1442 bytes for decoder to work
                    send_pkt(payload)
                    sys.stderr.write('.')

                if sendFullFile:
                    pktno = (pktno + 1) % len(totaldata)
                else:                   
                    pktno = (pktno + 1) % ROUNDSIZE

    # generate and send packets
    tb.wait()                       # wait for it to finish    

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
