#!/bin/sh
LIBS = ArrayLDPC_Encoder.o ArrayLDPC_Decoder.o rngs.o rvgs.o PerfTest.o
echo "Creating wrapper..."
swig -python PerfTest.i
echo "Compiling..."
g++ -fPIC -c PerfTest.cpp PerfTest_wrap.cpp \
    -I/usr/include/python2.7
echo "Linking..."
g++ -shared .o cat_cccodec3_wrap.o \
	 -o ldpc_swig.so
