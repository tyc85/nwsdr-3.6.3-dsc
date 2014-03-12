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

import time, struct, sys
import socket
#import os 
#print os.getpid()
#raw_input('Attach and press enter')

##################
# Server class
############3#####
class dsc_pkt_src(object):
    def __init__(self, server, port=5123 ):
        #self.pkt_size = 966 # 1440 bytes of data  
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


class my_top_block(gr.top_block):
    def __init__(self, modulator, options):
        gr.top_block.__init__(self)

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

        # do this after for any adjustments to the options that may
        # occur in the sinks (specifically the UHD sink)
        self.txpath = transmit_path(modulator, options)

        self.connect(self.txpath, self.sink)

# /////////////////////////////////////////////////////////////////////////////
#                                   main
# /////////////////////////////////////////////////////////////////////////////

def main():

    def send_pkt(payload='', eof=False):
        return tb.txpath.send_pkt(payload, eof)

    mods = digital.modulation_utils.type_1_mods()

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
    tb = my_top_block(mods[options.modulation], options)

    r = gr.enable_realtime_scheduling()
    if r != gr.RT_OK:
        print "Warning: failed to enable realtime scheduling"

    tb.start()                       # start flow graph
        
    # generate and send packets
    nbytes = int(1e6 * options.megabytes)
    n = 0
    pktno = 0
    pkt_size = int(options.size)
    serve = dsc_pkt_src("127.0.0.1")
    totaldata = []
    first_run = True

    while True:
        if first_run:
            if options.from_file is None:
                #data = (pkt_size - 2) * chr(0)
                #data = (pkt_size - 2) * chr(pktno & 0xff)
                #data = (pkt_size - 2) * chr( 0xff) 
                if pktno ==1000:
                    first_run = False

                data = serve.read()
                if len(data) != 1440:
                    print "data len is not 1440"
                    if len(totaldata) == 0: # cannot fetch data from server
                        pass
                    else: # End of file
                        first_run = False
                else:
                    payload = struct.pack('!H',pktno & 0xffff) + data
                    #print "payload length and pkt no in ldpc_tx are ", len(payload), pktno
                    send_pkt(payload)
                    sys.stderr.write('.')
                    #storing data into an array of old data
                    totaldata.append(data)
            else:
                data = source_file.read(pkt_size - 2)
                if data == '':
                    break;
                payload = struct.pack('!H',pktno & 0xffff) + data
                send_pkt(payload)
                sys.stderr.write('file.')
            pktno += 1
        else: # else if of first_run flag
            if len(totaldata) > pktno:
                payload = struct.pack('!H', pktno & 0xffff) + totaldata[pktno]
                send_pkt(payload)
                sys.stderr.write('nf.')
                pktno = (pktno + 1) % len(totaldata)
            else:
                pktno = 0
                payload = struct.pack('!H', pktno & 0xffff) + totaldata[pktno]
                send_pkt(payload)
                sys.stderr.write('nf.')
                pktno = (pktno + 1) % len(totaldata)

        
        n += len(payload)
        #sys.stderr.write('.')
        if options.discontinuous and pktno % 5 == 4:
            time.sleep(1)
        
    send_pkt(eof=True)
    print "finished sending"
    tb.wait()                       # wait for it to finish

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
