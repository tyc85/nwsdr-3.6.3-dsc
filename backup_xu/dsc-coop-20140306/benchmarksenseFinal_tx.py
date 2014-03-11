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



class my_top_block(gr.top_block):
    def __init__(self, modulator, demodulator, rx_callback, options):
        gr.top_block.__init__(self)
		#parameters to sense channe
        #options.symbol_rate=2500000
        #options.samples_per_symbol=2
        #options.rx_freq=2500000000
        #options.rx_gain=20
        #options.chbw_factor=1
        sense_symbol_rate=2500000
        sense_samples_per_symbol=2
        sense_rx_freq=2500000000
        sense_rx_gain=20
        options.chbw_factor=1
	#options.samples_per_symbol,

 
        #args = demodulator.extract_kwargs_from_options(options)
        self.sensesource=uhd_receiver(options.args, sense_symbol_rate,
                                       sense_samples_per_symbol,
                                       sense_rx_freq, sense_rx_gain,
                                       options.spec, options.antenna,
                                       options.verbose)

        if(options.tx_freq is not None):
            # Work-around to get the modulation's bits_per_symbol
            args = modulator.extract_kwargs_from_options(options)
            symbol_rate = options.bitrate / modulator(**args).bits_per_symbol()

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


        self.txgate = gr.copy(gr.sizeof_gr_complex)
        self.sensegate = gr.copy(gr.sizeof_gr_complex)
        #self.msgq             = gr.msg_queue()

        # do this after for any adjustments to the options that may
        # occur in the sinks (specifically the UHD sink)
        self.txpath = transmit_path(modulator, options)

        self.connect(self.txpath, self.txgate, self.sink)

	# do sense
        self.sensepath = sensing_path(options)

        self.tx_enabled = True
	self.sense_flag=False
	self.connect(self.sensesource, self.sensepath)

	

# /////////////////////////////////////////////////////////////////////////////
#                                   main
# /////////////////////////////////////////////////////////////////////////////

#shutdown_event = threading.Event()

def main():

    def send_pkt(payload='', eof=False):
        return tb.txpath.send_pkt(payload, eof)

    global n_rcvd, n_right 

    n_rcvd = 0
    n_right = 0
    
 
			

    mods = digital.modulation_utils.type_1_mods()
    #demods = digital.modulation_utils.type_1_demods()

    parser = OptionParser(option_class=eng_option, conflict_handler="resolve")
    expert_grp = parser.add_option_group("Expert")

    parser.add_option("-m", "--modulation", type="choice", choices=mods.keys(),
                      default='psk',
                      help="Select modulation from: %s [default=%%default]"
                            % (', '.join(mods.keys()),))
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
	

    while n < nbytes:
        if pktno%1000 == 0 and sense_n==1: #sence once
            tb.txgate.set_enabled(False)
            #time.sleep(.01)
            tb.sensegate.set_enabled(True) #t
            avail_subc_bin = tb.sensepath.GetAvailableSpectrum()
            #avail_subc_str = subc_bin2str(avail_subc_bin)
 #           print avail_subc_bin
            #time.sleep(0.002)
            sense_n=0
            #tb.txgate.set_enabled(True)
            #tb.sensegate.set_enabled(False)
        else:
            # linklab, loop to empty the lower layer buffers to avoid detecting old signals
            #send_pkt(eof=False)
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
            send_pkt(payload)
            n += len(payload)
            sys.stderr.write('.')
            if options.discontinuous and pktno % 5 == 4:
                time.sleep(1)
            pktno += 1
    send_pkt(eof=True)

    tb.wait()                       # wait for it to finish

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
