
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <iostream>
#include "ArrayLDPCMacro.h"
#include "ArrayLDPC.h"
//#include <windows.h>
#include "PerfTest.h"
//-----------------------
//testing so loading in runtime 
#include <dlfcn.h>
//#include "fec.h"
//#include "cat_codec.h"
//-----------------------

using std::endl;
using std::cout;
using std::cin;


/*sample function code in cat_codec
int cat_codelength(int nbytes)
*/

int main(int argc, char *argv[])
{
	//char Filename[30];
	double db_start, db_end, db_step;
	int i = 0;
	char InfoStream[248] = "OMG how long should this string be to make it 248, just imagine that. \
								   I guess it's still not long enough. Let's see. This is a testing string \
									for a lot of characters so that we have some random bit stream that's\
									correct";
	
	void* handle;
	char *error;
	int (*clen)(int);
	handle = dlopen("libfec.so", RTLD_LAZY);
	if(!handle)
	{
		cout << dlerror() << endl;
		exit(1);
	}
	dlerror();

	*(void **) (&clen) = dlsym(handle, "cat_codelength");

	if((error = dlerror())!=NULL)
	{
		cout << error << endl;
		exit(1);
	}
	cout << "output of clen(10) is: " << clen(10) << endl << ", testing done" << endl;
	exit(1);
	// just use this one as testing platform
	if(1)
	{
		long MaxPckNum;
		double EbN0_dB;
		//--- debug mode, perform encoding and decoding and output BER/FER
		cout << "debug mode at EbN0 = ?dB. input:"; 
		cin >> EbN0_dB;
		ArrayLDPC_Debug(EbN0_dB);

		//--- Decoder time trial and Encoder time trial
		cout << "decoder/encoder time trial\n num of packets: "; 
		cin >> MaxPckNum;
		DecodeTrial(EbN0_dB, MaxPckNum);
		EncodeTrial(InfoStream, MaxPckNum);
	}
	else
	{
		long MaxPckNum;
		printf("argc = %d \n time trial with: manual input\n", argc);
		cout << "EbN0 in dB: ";
		cin >> db_start;
		cout << "simulate how many packets? ( " << 2209 
			  << " is the codeword length ): ";
		cin >> MaxPckNum;

		//ArrayLDPC_TimeTrial(db_start, MaxPckNum, "timing.txt");
	
	}
	return 0;
}
