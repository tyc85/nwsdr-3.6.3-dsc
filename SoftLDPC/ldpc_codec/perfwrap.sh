#!/bin/sh

swig -c++ -python -o PerfTest_wrap.cpp PerfTest.i

g++ -O2 -fPIC -c -I/usr/include/python2.7 PerfTest.cpp PerfTest_wrap.cpp

g++ -shared  -o _ldpc.so ArrayLDPC_Encoder.o ArrayLDPC_Decoder.o rngs.o rvgs.o PerfTest.o PerfTest_wrap.o 
