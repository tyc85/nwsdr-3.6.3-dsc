#!/bin/sh


gcc -fPIC -O2 -c init_field.c get_msg.c encode.c lose_packets.c decode.c compare_msg.c cat_wrap.c
echo "done compiling"
gcc -shared -o cauchy.so init_field.o get_msg.o encode.o lose_packets.o decode.o compare_msg.o cat_wrap.o
echo "done creating cauchy.so file"
sudo cp cauchy.so /lib/
echo "done copying cauchy.so file to /lib/"
