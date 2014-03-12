#!/usr/bin/env python

#import ctypes
#from ctypes import *
import ctypes
#from ctypes import CDLL
print "ctype improted"
#lib1 = ctypes.cdll("ldpc.so", modes=ctypes.RTLD_GLOBAL)
lib2 = ctypes.CDLL("./ldpc.so", mode=ctypes.RTLD_GLOBAL)

#lib1.ArrayLDPC_Debug();
