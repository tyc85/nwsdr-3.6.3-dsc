%module perftest

%{
//#include "ArrayLDPCMacro.h"
#include "PerfTest.h"
//extern void test();
%}

extern int ArrayLDPC_Debug(double EbN0_dB);



class t_enc
{
public:
	t_enc();
	~t_enc();
	void test();
};



