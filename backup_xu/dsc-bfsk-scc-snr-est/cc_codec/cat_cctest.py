import struct
import numpy
from gnuradio import gru

#import crc

from ctypes import *
import gnuradio.digital.crc as crc
lib = "./_cat_cccodec.so"
Codec = cdll.LoadLibrary(lib)

block = POINTER(u_char)
nbytes = 255
rs_length = 255

Codec.cc_encode(block, bytes, rs_length)