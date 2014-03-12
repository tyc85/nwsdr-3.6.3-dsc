#!/bin/sh

swig -c++ -python -o list_wrap.cc list.i
g++ -O2 -fPIC -c -I/usr/include/python2.7 list.cc list_wrap.cc
g++ list_wrap.o list.o -shared -o _list.so
