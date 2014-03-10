#include "compare_msg.h"
%module cauchy
%{
  extern int cauchy_encode(unsigned int* packets, unsigned int* message);
%}

extern int cauchy_encode(unsigned int* packets, unsigned int* message);

