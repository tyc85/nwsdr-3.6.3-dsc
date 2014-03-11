#!/usr/bin/env python
import timeit
from ctypes import *
import math
lib = "/lib/cauchy.so"
cauchy = cdll.LoadLibrary(lib)
def encode():
    totalpkt = 500
    msg_length = 1440
    pkt_length = 1444
    #msg = []
    #for i in xrange(0, totalpkt):
    #msg = msg + chr(i%255)*msg_length

    msg = totalpkt*chr(221)*msg_length
    coded_pkt = chr(0)*totalpkt*2*pkt_length

    cauchy.cat_encode_cauchy(msg, coded_pkt, 1440)
    print "finised encoding"
    return 0

def decode(coded_pkt):
    totalpkt = 500
    msg_length = 1440
    pkt_length = 1444
    
    #coded_pkt = chr(0)*totalpkt*2*pkt_length
    offset = 29*pkt_length
    extra = 2
    rec_pkt = coded_pkt[offset:offset+(totalpkt+extra)*pkt_length]

    decoded_pkt = chr(0)*msg_length*totalpkt # just blank array to be filled

    cauchy.cat_decode_cauchy(coded_pkt, decoded_pkt, 500)
    print "finished decoding"
    return 0
def func():
    return math.sqrt(1000)
def main():
    #print "encode time: "
    #timeit.timeit(func)
    #timeit.timeit(encode)
    #print "decode time: "
    #timeit.timeit(decode)
    #cauchy.cat_msg_compare(msg, decoded_pkt, 1440)

    totalpkt = 500
    msg_length = 1440
    pkt_length = 1444
    msg = totalpkt*chr(221)*msg_length
    coded_pkt = chr(0)*totalpkt*2*pkt_length

    cauchy.cat_encode_cauchy(msg, coded_pkt, 1440)
    ############ Lossing pkts ############
    offset = 29*pkt_length
    extra = 2
    rec_pkt = coded_pkt[offset:offset+(totalpkt+extra)*pkt_length]

    ############ Decoding ############
    decoded_pkt = chr(0)*msg_length*totalpkt # just blank array to be filled
    cauchy.cat_decode_cauchy(coded_pkt, decoded_pkt, 500)

    ############ Comaring ############
    cauchy.cat_msg_compare(msg, decoded_pkt, 1440)

if __name__ == "__main__":
    main()
