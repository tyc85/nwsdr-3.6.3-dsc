#!/usr/bin/env python
from ctypes import *
#import ctypes
import ldpc
print "improted"
#lib1 = ctypes.cdll("ldpc.so", modes=ctypes.RTLD_GLOBAL)
#lib2 = ctypes.CDLL("./ldpc.so", mode=ctypes.RTLD_GLOBAL)
#lib1.ArrayLDPC_Debug();
print "loading new enc object"
#obj = ldpc.t_enc()
#obj.test()

'''
fsm = ldpc.ControlFSM()
print "set state to 1"
fsm.setState(1)

print "get state: " 
print fsm.getState()
''' 
#str_bytes = '01234567890123456789'
#raw_bytes = (ctypes.c_ubyte * 20).from_buffer_copy(str_bytes)
#enc = ldpc.FP_Encoder()
#create an encoder point object
length_in = 1978
length_out = 2209


char_in = length_in*chr(27)
#print "input is all"
#print char_in
#uchar_in = (c_ubyte * length_in).from_buffer_copy(char_in)
#print uchar_in
#uchar_in = c_ubyte*10
# initialize the output char and unsigned output char
char_out = length_out*chr(48)
#uchar_out = (c_ubyt
# initialize the outpue * length_out).from_buffer_copy(char_out)


enc_ptr = ldpc.create_enc_obj()
#enc_ptr.loadfile("test.txt", 1)
# set the input and output char length


#print "before encoding\n"
#print char_out
ldpc.encode_ldpc(enc_ptr, char_in, char_out, 1978)
#print "after encoding\n"
#print char_out
ldpc.destroy_enc_obj(enc_ptr)


#dec_ptr = ldpc.create_dec_obj()
#print dec_ptr
#ldpc.destroy_dec_obj(dec_ptr)
