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

import threading

from gnuradio import gr
from gnuradio import eng_notation
from gnuradio.eng_option import eng_option
from optparse import OptionParser
import time, struct, sys, socket

import binascii

from gnuradio import digital

# from current dir
from transmit_path import transmit_path
from uhd_interface import uhd_transmitter
from receive_path import receive_path
from uhd_interface import uhd_receiver

#BANDWIDTH_H = 5000000
BANDWIDTH_H = 2000000
BANDWIDTH_L = 2000000
ROUNDSIZE = 1000

class my_top_block(gr.top_block):
    def __init__(self, callback, options):
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

        # Set up receive path
        # do this after for any adjustments to the options that may
        # occur in the sinks (specifically the UHD sink)
	self.txpath = transmit_path(options)
        self.rxpath = receive_path(callback, options)
        self.txgate = gr.copy(gr.sizeof_gr_complex)
        self.rxgate = gr.copy(gr.sizeof_gr_complex)
    
        self.connect(self.source, self.rxgate, self.rxpath)
	self.connect(self.txpath, self.txgate, self.sink)

        #self.connect(self.adder, self.txgate, self.sink) # used for sinusoid feedback

        self.tx_enabled = True

class dsc_pkt_sink(object):    
      def __init__(self, server, port=5125):       
          self.pkt_sink_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
          self.pkt_sink_socket.connect((server,port))

      def send(self, payload):
          try:
             self.pkt_sink_socket.recv(4)
             self.pkt_sink_socket.send(payload)
          except socket.error:
             print "Connection to packet sink closed"
        
# /////////////////////////////////////////////////////////////////////////////
#                                   main
# /////////////////////////////////////////////////////////////////////////////

shutdown_event = threading.Event()



def main():

    global n_rcvd, n_right, missing_packets, tx_enabled, fbstate,tb, pre_rcvd, pre_right # serve
        
    n_rcvd = 0
    n_right = 0
    pre_rcvd = 0
    pre_right = 0
    missing_packets = range(ROUNDSIZE)
    tx_enabled = False
    
    fbstate = 7

    def dowork():
        global tb, tx_enabled, fbstate, pre_rcvd, pre_right, fdpkt_no
        
        flag=0
    # !! is this indent done correct? 
        fdpkt_no = 0;
        while not shutdown_event.is_set():
            if time.time() % 10 <= 10 and tx_enabled==True:
                
                if flag==0:
                    pre_rcvd = n_rcvd
                    pre_right = n_right
                    flag = 1
                # Reset options.bandwidth at receiver according to fbsate
                if fbstate == 9 and tb._opts.bandwidth != BANDWIDTH_H:
                    tb._opts.bandwidth = BANDWIDTH_H
                    tb.sink.set_sample_rate(tb._opts.bandwidth)
                    tb.source.set_sample_rate(tb._opts.bandwidth)
                    
                    #print "L->H"
                elif fbstate == 7 and tb._opts.bandwidth != BANDWIDTH_L:
                    tb._opts.bandwidth = BANDWIDTH_L
                    tb.sink.set_sample_rate(tb._opts.bandwidth)
                    tb.source.set_sample_rate(tb._opts.bandwidth)
                    
                    #print "H->L"
                # IMPORTANT: fbstate can only be 9 or 7

                print "set tx_enabled false"
                tx_enabled = False
                tb.txgate.set_enabled(False)
                tb.rxgate.set_enabled(True)
        
                
            if time.time() % 10 > 10 and tx_enabled==False:
                #print "tx bandwidth1", tb.sig1.get_sampling_rate(), "tx bandwidth2", tb.sig2.get_sampeling_rate()
                # Define feedback state here
                if (n_rcvd > pre_rcvd ) and float(n_right - pre_right)/float(n_rcvd - pre_rcvd) > 0.8:
                    # High rate
                    fbstate = 9
                else:
                    # Low Rate
                    fbstate = 7
                   
                flag = 0
                
		  
                tb.rxgate.set_enabled(False)            
                tx_enabled = True

                tb.txgate.set_enabled(True)


		
		
		
                
            time.sleep(.01)

    def rx_callback(ok, payload):
        global n_rcvd, n_right, missing_packets, tx_enabled, fbstate #serve
#        if tx_enabled == True:
#            return
        n_rcvd += 1
        (pktno,) = struct.unpack('!H', payload[0:2])
        print "ok: %r \t pktno: %d \t n_rcvd: %d \t n_right: %d" % (ok, pktno, n_rcvd, n_right)
        if ok:
            n_right += 1
            #serve.send(payload[2:])

	    # IMPORTANT BUG! missing_packets ranges from 0 to roundsize-1, Can Never remove pktno if pktno >1
            try:
                missing_packets.remove(pktno)
            except ValueError:
                #print "Packet already received or incorrect"
            	pass

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
    options.modulation = "bpsk"
    options.rx_gain = 5
    options.tx_gain = 31.5
    options.tx_amplitude = 1   

    '''
    if options.from_file is None:
        if options.rx_freq is None:
            sys.stderr.write("You must specify -f FREQ or --freq FREQ\n")
            parser.print_help(sys.stderr)
            sys.exit(1)
    '''
    # packet_sink
    #serve = dsc_pkt_sink(options.server)

    # build the graph
    tb = my_top_block(rx_callback, options)
    
    r = gr.enable_realtime_scheduling()
    if r != gr.RT_OK:
        print "Warning: failed to enable realtime scheduling"


    tb.txgate.set_enabled(False)
    tb.start()                       # start flow graph
    t = threading.Thread(target=dowork, args=(), name='worker')
    t.start()
#    t.join(timeout=1.0)

    while True:
	if tx_enabled:
		data = chr(fbstate) + (1440-1)* chr(100 & 0xff)

		#print "feeding back"
		#print "data in char is ", binascii.hexlify(data[0])
		#print "data in numer is ", ord(data[0])
		fdbkpayload = struct.pack('!H', fdpkt_no & 0xffff) + data
		send_pkt(fdbkpayload)


    tb.wait()                       # wait for it to finish    
#    try:
#        while t.isAlive():
#            t.join(timeout=1.0)
#    except (KeyboardInterrupt, SystemExit):
#            shutdown_event.set()
#    pass


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        shutdown_event.set()
    pass
