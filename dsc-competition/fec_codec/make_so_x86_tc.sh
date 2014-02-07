#!/bin/sh
echo "Creating wrapper..."
swig -python cat_cccodec3.i
echo "Compiling..."
gcc -fPIC -c cat_cccodec3.c cat_cccodec3_wrap.c \
    -I /usr/include/python2.7
echo "Linking..."
gcc -shared cat_cccodec3.o cat_cccodec3_wrap.o\
	sumsq.o sumsq_port.o cpu_mode_port.o fec.o sim.o \
	viterbi27.o viterbi27_port.o viterbi29.o viterbi29_port.o viterbi39.o viterbi39_port.o \
	viterbi615.o viterbi615_port.o encode_rs_char.o encode_rs_int.o encode_rs_8.o \
	decode_rs_char.o decode_rs_int.o decode_rs_8.o \
	init_rs_char.o init_rs_int.o ccsds_tab.o \
	encode_rs_ccsds.o decode_rs_ccsds.o ccsds_tal.o \
	dotprod.o dotprod_port.o \
	peakval.o peakval_port.o \
	-o cat_libfec.so
	#-o _cat_cccodec3.so
	# cpu_mode_x86.o
	# viterbi27_mmx.o mmxbfly27.o viterbi27_sse.o ssebfly27.o viterbi27_sse2.o sse2bfly27.o	\
	# viterbi29_mmx.o mmxbfly29.o viterbi29_sse.o ssebfly29.o viterbi29_sse2.o sse2bfly29.o \
	# viterbi39_sse2.o viterbi39_sse.o viterbi39_mmx.o 	viterbi615_mmx.o viterbi615_sse.o viterbi615_sse2.o \
	# dotprod_mmx.o dotprod_mmx_assist.o 	dotprod_sse2.o dotprod_sse2_assist.o \
	# peakval_mmx.o peakval_mmx_assist.o 	peakval_sse.o peakval_sse_assist.o 	peakval_sse2.o peakval_sse2_assist.o \
	# sumsq_sse2.o sumsq_sse2_assist.o 	sumsq_mmx.o sumsq_mmx_assist.o \
	# cpu_features.o 
echo "Copying..."
cp _cat_cccodec3.so ../
echo "Done"
