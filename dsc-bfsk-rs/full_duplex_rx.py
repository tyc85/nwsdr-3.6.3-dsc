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
from gnuradio import gr, gru
from gnuradio import eng_notation
from gnuradio.eng_option import eng_option
from optparse import OptionParser

# From gr-digital
from gnuradio import digital

# from current dir
from receive_path import receive_path
from uhd_interface import uhd_receiver
from transmit_path import transmit_path
from uhd_interface import uhd_transmitter

import time, struct, sys, socket

from gnuradio import analog
from gnuradio.gr import firdes

class my_top_block(gr.top_block):
    def __init__(self, modulator, demodulator, rx_callback, options):
        gr.top_block.__init__(self)

        #------------Tx-------------#
        if(options.tx_freq is not None):
             # Work-around to get the modulation's bits_per_symbol
             
            args = modulator.extract_kwargs_from_options(options)
            symbol_rate = options.sub_bitrate / modulator(**args).bits_per_symbol()

            self.sink = uhd_transmitter(options.args, symbol_rate,
                                        options.samples_per_symbol,
                                        options.tx_freq, options.tx_gain,
                                        options.spec, options.antenna,
                                        options.verbose)
            options.samples_per_symbol = self.sink._sps
                
        elif(options.to_file is not None):
            sys.stderr.write(("Saving samples to '%s'.\n\n" % (options.to_file)))
            self.sink = gr.file_sink(gr.sizeof_gr_complex, options.to_file)
        else:
            sys.stderr.write("No sink defined, dumping samples to null sink.\n\n")
            self.sink = gr.null_sink(gr.sizeof_gr_complex)

        # do this after for any adjustments to the options that may
        # occur in the sinks (specifically the UHD sink)
        self.txpath = transmit_path(modulator, options)
        
        self.connect(self.txpath, self.sink)

        #------------Rx-------------#
        if(options.rx_freq is not None):
            # Work-around to get the modulation's bits_per_symbol
            args = demodulator.extract_kwargs_from_options(options)
            symbol_rate = options.main_bitrate / demodulator(**args).bits_per_symbol()

            self.source = uhd_receiver(options.args, symbol_rate,
                                       options.samples_per_symbol,
                                       options.rx_freq, options.rx_gain,
                                       options.spec, options.antenna,
                                       options.verbose)
            options.samples_per_symbol = self.source._sps

        elif(options.from_file is not None):
            sys.stderr.write(("Reading samples from '%s'.\n\n" % (options.from_file)))
            self.source = gr.file_source(gr.sizeof_gr_complex, options.from_file)
        else:
            sys.stderr.write("No source defined, pulling samples from null source.\n\n")
            self.source = gr.null_source(gr.sizeof_gr_complex)

        # Set up receive path
        # do this after for any adjustments to the options that may
        # occur in the sinks (specifically the UHD sink)
        self.rxpath = receive_path(demodulator, rx_callback, options) 

        self.connect(self.source, self.rxpath)


#---------Pkt_Sink for Rx-----------#
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
global n_rcvd, n_right

def main():

    global  n_rcvd, n_right, sink_serve

    n_rcvd = 0
    n_right = 0

    def send_pkt(payload='', eof=False):
        return tb.txpath.send_pkt(payload, eof)

    def rx_callback(ok, payload):
        #global n_rcvd, n_right, start_time, stop_rcv
        global n_rcvd, n_right
        (pktno,) = struct.unpack('!H', payload[0:2])
        n_rcvd += 1
        if ok:
            n_right += 1
            sink_serve.send(payload[2:])
        print "ok = %5s  pktno = %4d  n_rcvd = %4d  n_right = %4d" %(
            ok, pktno, n_rcvd, n_right)

    mods = digital.modulation_utils.type_1_mods()     # Modulator
    demods = digital.modulation_utils.type_1_demods() # Demodulator

    parser = OptionParser(option_class=eng_option, conflict_handler="resolve")
    expert_grp = parser.add_option_group("Expert")

    parser.add_option("-m", "--modulation", type="choice", choices=mods.keys(),
                      default='gmsk_cats',
                      help="Select modulation from: %s [default=%%default]"
                            % (', '.join(mods.keys()),))
    parser.add_option("-M", "--megabytes", type="eng_float", default=1.0,
                      help="set megabytes to transmit [default=%default]")
    parser.add_option("","--discontinuous", action="store_true", default=False,
                      help="enable discontinous transmission (bursts of 5 packets)")
    parser.add_option("","--from-file", default=None,
                      help="use intput file for packet contents")
    parser.add_option("","--to-file", default=None,
                      help="Output file for modulated samples")
    parser.add_option("-s", "--server", default="idb2",
                      help="server host name [default=%default]")
    parser.add_option("","--mode", default="COMP",
                      help="set match style [default=%default]")
    parser.add_option("","--sub-bitrate", type="eng_float", default=100e3, help="feedback bitrate [default=%default]")
    parser.add_option("","--main-bitrate", type="eng_float", default=2.5e6, help="main bitrate [default=%default]")
    parser.add_option("","--carrier-sep", type="eng_float", default=2.5e6, help="carrier frequency separation [default=%default]")

    transmit_path.add_options(parser, expert_grp)
    uhd_transmitter.add_options(parser)
    receive_path.add_options(parser, expert_grp)
    uhd_receiver.add_options(parser)
    
    for mod in demods.values():
        mod.add_options(expert_grp)
    
    for mod in mods.values():
        mod.add_options(expert_grp)

    (options, args) = parser.parse_args ()   
    #options.tx_freq = options.freq + options.carrier_sep/2
    #options.rx_freq = options.freq - options.carrier_sep/2

    #########################################
    # Xu Chen: Hard Code Parameters 
    
    #options.bitrate = 2500000
    #options.samples_per_symbol = 2
    #options.modulation = "gmsk_cats"
    #options.tx_amplitude = 0.8
    #options.tx_gain = 31.5
    #options.excess_bw = 0.35
   
    #######################################

    if len(args) != 0:
        parser.print_help()
        sys.exit(1)
           
    if options.from_file is not None:
        source_file = open(options.from_file, 'r')

    if options.from_file is None:
        if options.rx_freq is None:
            sys.stderr.write("You must specify -f FREQ or --freq FREQ\n")
            parser.print_help(sys.stderr)
            sys.exit(1)

    # packet_source
    sink_serve= dsc_pkt_sink(options.server)

    # build the graph
    tb = my_top_block(mods[options.modulation], demods[options.modulation], rx_callback, options)

    r = gr.enable_realtime_scheduling()
    if r != gr.RT_OK:
        print "Warning: failed to enable realtime scheduling"

    tb.start()                       # start flow graph

    #-------------------Tx Job-------------------#
    pktno = 0
    pkt_size = 1440

    while True:
        data = '%01440d' % n_right
        payload = struct.pack('!H', pktno & 0xffff) + data
        send_pkt(payload)
        sys.stderr.write('.')
        pktno += 1
        totaldata.append(data)
                
                

        
    send_pkt(eof=True)

    tb.wait()                       # wait for it to finish

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass


