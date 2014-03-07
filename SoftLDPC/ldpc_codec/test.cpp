
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <iostream>
#include "ArrayLDPCMacro.h"
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
	//double db_start, db_end, db_step;
	//int i = 0;
	FP_Decoder *decoder;
	FP_Decoder *(*get_obj)(void);
	void (*del_obj)(FP_Decoder*);
	void *handle;
	char *error;
	
	
	//--------testing ArrayLDCP_Debug()
	//-- now load .so file for LDPC code and see whether it works
	
	cout << "loading ldpc.so..." << endl;
	void* handle2;
	char *error2;
	int (*ArrayLDPC_Debug)(double);
	handle2 = dlopen("ldpc.so", RTLD_LAZY);
	if(!handle2)
	{
		cout << dlerror() << endl;
		exit(1);
	}
	// clear the error
	dlerror();
	*(void **) (&ArrayLDPC_Debug) = dlsym(handle2, "ArrayLDPC_Debug");

	if((error2 = dlerror())!=NULL)
	{
		cout << error2 << endl;
		exit(1);
	}
	cout << "running ArrayLDPC_Debug(5) " << endl;
	ArrayLDPC_Debug(5.0);
	
	exit(1); //end the testing here
	
	
	handle = dlopen("ldpc.so", RTLD_LAZY);
	if(!handle)
	{
		cout << dlerror() << endl;
		exit(1);
	}
	//cout << "testing instantiate a class pointer" << decoder << endl;
	get_obj = (FP_Decoder* (*)()) dlsym(handle, "create_dec_obj");
	del_obj = (void (*)(FP_Decoder*)) dlsym(handle, "destroy_dec_obj");
	cout << "trying to allocate memory\n";
	decoder  = get_obj();
	cout << "got the pointer\n";
	cout << "sign of -1 is " << decoder->sgn(-1) << endl;
	
	cout << "destroying the object\n";
	del_obj(decoder);
	cout << "destroyed\n";
	dlclose(handle);
	

	
	exit(1); //end the testing here
	
	
	
	// clear the error
	dlerror();
	
	int (*clen)(int);

	*(void **) (&clen) = dlsym(handle, "cat_codelength");

	if((error = dlerror())!=NULL)
	{
		cout << error << endl;
		exit(1);
	}
	*(void **) (&clen) = dlsym(handle, "cat_codelength");
	cout << "output of clen(10) is: " << clen(10) << endl << ", testing done" << endl;
	
	
	
	//cout << "running ArrayLDPC_Debug(5) " << endl;
	//ArrayLDPC_Debug(5.0);
	
	
	return 0;
}

