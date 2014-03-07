#!/bin/sh
# ideal version. but require change of cat_cccodec.pay too...many places to change
echo "Linking all objects in the folder for LDPC..."
gcc -shared -Xlinker *.o -o ldpc.so
echo "Done"
