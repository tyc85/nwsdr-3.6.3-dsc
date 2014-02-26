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

import time, struct, sys, socket

from gnuradio import analog

#import os 
#print os.getpid()
#raw_input('Attach and press enter')

global bandchoose
bandchoose = 2

class my_top_block(gr.top_block):
    def __init__(self, modulator, options):
        gr.top_block.__init__(self)

        #samp_rate= 3000000
	samp_rate = options.bitrate*options.samples_per_symbol
	fa = samp_rate/4 #1000000

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
        self.txpath1 = transmit_path(modulator, options)
	self.txpath2 = transmit_path(modulator, options)

	#self.connect(self.txpath, self.sink)

	# Define the math operator blocks
	

	# Generate exp(jw1t) and exp(-jw1t)
	self.gr_multiply_xx_0 = gr.multiply_vff(1)
	self.gr_float_to_complex_0_0 = gr.float_to_complex(1)
	self.gr_float_to_complex_0 = gr.float_to_complex(1)
	self.const_source_x_0 = gr.sig_source_f(0, gr.GR_CONST_WAVE, 0, 0, -1)
	self.analog_sig_source_x_0_0 = gr.sig_source_f(samp_rate, gr.GR_SIN_WAVE, fa, 1, 0)
	self.analog_sig_source_x_0 = gr.sig_source_f(samp_rate, gr.GR_COS_WAVE, fa, 1, 0)


	# Combine signal from two subbands
	self.gr_multiply_xx_1 = gr.multiply_vcc(1)
	self.gr_multiply_xx_2 = gr.multiply_vcc(1)
	self.gr_c2f_1 = gr.complex_to_float(1)
	self.gr_c2f_2 = gr.complex_to_float(1)
	self.gr_add_xx_re = gr.add_vff(1)
	self.gr_add_xx_im = gr.add_vff(1)
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
	self.connect((self.gr_float_to_complex_0_0, 0), (self.gr_multiply_xx_1, 1))
	# txpath2 * exp(jw1t)
	#self.connect(self.txpath2, (self.gr_multiply_xx_2, 0))
	
	self.connect((self.gr_float_to_complex_0, 0), (self.gr_multiply_xx_2, 1))
	
	if bandchoose == 0:
		self.connect(self.txpath1, (self.gr_multiply_xx_1, 0))
		self.connect(self.gr_null_source_0 , (self.gr_multiply_xx_2, 0))
	elif bandchoose == 1:
		self.connect(self.gr_null_source_0 , (self.gr_multiply_xx_1, 0))
		self.connect(self.txpath2, (self.gr_multiply_xx_2, 0))
	else:
		self.connect(self.txpath1, (self.gr_multiply_xx_1, 0))
		self.connect(self.txpath2, (self.gr_multiply_xx_2, 0))

	self.connect((self.gr_multiply_xx_1, 0), self.gr_c2f_1)
	self.connect((self.gr_multiply_xx_2, 0), self.gr_c2f_2)

	self.connect((self.gr_c2f_1,0), (self.gr_add_xx_re,0))
	self.connect((self.gr_c2f_2,0), (self.gr_add_xx_re,1))

	self.connect((self.gr_c2f_1,1), (self.gr_add_xx_im,0))
	self.connect((self.gr_c2f_2,1), (self.gr_add_xx_im,1))

	self.connect(self.gr_add_xx_re, (self.gr_f2c,0))
	self.connect(self.gr_add_xx_im, (self.gr_f2c,1))
        self.connect(self.gr_f2c, self.sink)
# /////////////////////////////////////////////////////////////////////////////
#                                   main
# /////////////////////////////////////////////////////////////////////////////

def main():

    def send_pkt1(payload='', eof=False):
        return tb.txpath1.send_pkt(payload, eof)

    def send_pkt2(payload='', eof=False):
        return tb.txpath2.send_pkt(payload, eof)

    mods = digital.modulation_utils.type_1_mods()

    parser = OptionParser(option_class=eng_option, conflict_handler="resolve")
    expert_grp = parser.add_option_group("Expert")

    parser.add_option("-m", "--modulation", type="choice", choices=mods.keys(),
                      default='psk',
                      help="Select modulation from: %s [default=%%default]"
                            % (', '.join(mods.keys()),))

    parser.add_option("-s", "--size", type="eng_float", default=1442,
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
        
    # log parameter to OML
    #cmd1 = "/root/OML/omlcli --out h3_benchmark --line \""
    #cmd1 = cmd1 + " tx-freq=" + str(options.tx_freq)
    #cmd1 = cmd1 + " modulation=" + str(options.modulation)
    #cmd1 = cmd1 + " tx-gain=" + str(options.tx_gain)
    #cmd1 = cmd1 + " bitrate=" + str(options.bitrate)
    #cmd1 = cmd1 + " sps=" + str(options.samples_per_symbol)
    #cmd1 = cmd1 + " hostname=" + socket.gethostname()
    #cmd1 = cmd1 + "\""

    #from subprocess import os
    #os.system(cmd1)


    # Fetch packets from server

    #s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    #s.settimeout(10)
    #TCP_IP='idb2'
    #TCP_PORT=5100    
    #try:
    #   s.connect((TCP_IP, TCP_PORT))
    #except socket.timeout: 
    #   print"Connection timed out, try again later"
    #   return
    #except socket.error:
    #   print"Connection error"
    #   return

   
    #n = 0
    #pktno = 0
    #pkt_size = int(1442)
    #MESSAGE = struct.pack('!l',pkt_size-2)

    #while 1: #n < nbytes:
    #    if options.from_file is None:
    #        try:
    #           s.send(MESSAGE)
    #	    except socket.error:
    #           print"Connection error"
    #           return

   
    #n = 0
    #pktno = 0
    pkt_size = int(1442)
    #MESSAGE = struct.pack('!l',pkt_size-2)

    #while 1: #n < nbytes:
    #    if options.from_file is None:
    #        try:
    #           s.send(MESSAGE)
    #           data=s.recv(pkt_size-2)
    #        except socket.timeout: 
    #           print"Connection timed out, try again later"
    #           returnanalog_sig_source_x_0_0
    #        except socket.error:
    #           print"Connection closed"
    #           return
    #        if data.__len__() < 8:
    #           print "Connection timed out, try again later"
    #           break
    #        if options.verbose:
                # First 4 bytes are checksum followed by the 4 byte sequence number
    #               crc,sn = struct.unpack('!LL',data[:8])
    #               print "Seq #:", sn, " with CRC [", hex(crc), "]"
                
    #    else:
    #        data = source_file.read(pkt_size - 2)
    #        if data == '':
    #            break;
    nbytes = int(1e6 * options.megabytes)
    n = 0
    pktno = 0
    pkt_size = int(options.size)

    while n < nbytes:
        if options.from_file is None:
            data = (pkt_size - 2) * chr(pktno & 0xff) 
        else:
            data = source_file.read(pkt_size - 2)
            if data == '':
                break;

        payload = struct.pack('!H', pktno & 0xffff) + data
        if bandchoose == 0:
		payload1 = struct.pack('!H', pktno & 0xffff) + data
        	send_pkt1(payload1)
		pktno += 1
	elif bandchoose == 1:
		payload1 = struct.pack('!H', pktno & 0xffff) + data
		send_pkt2(payload1)
		pktno += 1
	else:
		payload1 = struct.pack('!H', pktno & 0xffff) + data
		send_pkt1(payload1)
		pktno += 1
		payload1 = struct.pack('!H', pktno & 0xffff) + data
		send_pkt2(payload1)
		pktno += 1

        n += len(payload)
        sys.stderr.write('.')
        if options.discontinuous and pktno % 5 == 4:
            time.sleep(1)
        pktno += 1
        
    #if options.from_file is None:
    #    s.close()
    #time.sleep(5)
    
    if bandchoose == 0:
        send_pkt1(eof=True)
    elif bandchoose == 1:
	send_pkt2(eof=True)
    else:
	send_pkt1(eof=True)
	send_pkt2(eof=True)

    tb.wait()                       # wait for it to finish

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass

