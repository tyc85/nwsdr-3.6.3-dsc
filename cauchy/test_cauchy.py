from ctypes import *
from random import randint
import binascii

lib = "./libfec.so"
dll = cdll.LoadLibrary(lib)
nn = 255
kk = 223
nbytes = 1446
len = dll.cat_codelength(nbytes)
print len
codeword = (c_byte * len)()
uncoded = (c_byte * len)()
#cw = 255*"a";
for i in range(1,nbytes):
	codeword[i] = 49 #randint(0,255)
print 'unc', binascii.hexlify(codeword)

diff = codeword
uncoded = codeword

dll.cat_encode(codeword,nbytes)
print 'enc', binascii.hexlify(codeword)
codeword[0] = 5
print 'err', binascii.hexlify(codeword)

dll.cat_decode(codeword,nbytes)
print 'dec', binascii.hexlify(codeword)

#for i in range(0,5):
#	print uncoded[i], codeword[i]
#	diff[i] = uncoded[i] - codeword[i]
#print binascii.hexlify(diff)

#print dll.cat_codelength(kk)
#print dll.cat_codelength(kk+1)
#print dll.cat_codelength(7*kk)
#print dll.cat_codelength(6*kk+3)
#print dll.cat_codelength(1440)
#print "Testing pointer output"
#dll.testout.argtypes = [POINTER(c_ubyte), POINTER(c_int)]
#sizeout = c_int(0)
#mem = (c_ubyte * 20)() 
#dll.testout(mem, byref(sizeout))
#print "Sizeout = " + str(sizeout.value)
#for i in range(0,sizeout.value):
#    print "Item " + str(i) + " = " + str(mem[i])
