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

    totalpkt = 500
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
    #msg = totalpkt*chr(221)*msg_length
    group_size = 5
    msg_array = group_size*[0*chr(1)]
    rec_pkt_array = group_size*[0*chr(0)]
    coded_pkt_array = group_size*[chr(0)*pkt_length*totalpkt*2]

    decoded_pkt_array = group_size*[chr(0)*msg_length*totalpkt] # just blank array to be filled
    for j in xrange(0,group_size):
        for i in xrange(0, totalpkt):
            msg_array[j] = msg_array[j] + chr(j+48)*msg_length
        #print "msg_array[j] is ", msg_array[j]
        
        cauchy.cat_encode_cauchy(msg_array[j], coded_pkt_array[j], 1440)
        print "finished encoding array ", j
        offset = (j+3)*pkt_length
        extra = 2
        rec_pkt_array[j] = coded_pkt_array[j][offset:offset+(totalpkt+extra)*pkt_length]

        ############ Decoding ############
        cauchy.cat_decode_cauchy(coded_pkt_array[j], rec_pkt_array[j], 500)

        ############ Comaring ############
        cauchy.cat_msg_compare(msg_array[j], rec_pkt_array[j], 1440)
    
        coded_pkt = chr(0)*totalpkt*2*pkt_length


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
