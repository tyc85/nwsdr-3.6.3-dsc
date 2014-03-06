#!/bin/sh


cd ldpc_codec
swig -c++ -python -o ldpc_wrap.cpp ldpc.i 

#g++ -O2 -fPIC -c -I/usr/include/python2.7 PerfTest.cpp PerfTest_wrap.cpp
#g++ -O2 -fPIC -c -I/usr/include/python2.7 PerfTest.cpp PerfTest_wrap.cpp
g++ -O2 -fPIC -c -I/usr/include/python2.7 PerfTest.cpp ArrayLDPC_Encoder.cpp ArrayLDPC_Decoder.cpp ldpc_wrap.cpp
g++ -shared  -o _ldpc.so ArrayLDPC_Encoder.o ArrayLDPC_Decoder.o rngs.o\
								 rvgs.o PerfTest.o ldpc_wrap.o 
cp _ldpc.so ldpc.py ../
sudo cp _ldpc.so /lib/

cd ../
echo "finished"
#PerfTest_wrap.o
