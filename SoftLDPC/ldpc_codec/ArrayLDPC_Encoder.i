%module ldpc

%{
#include "ArrayLDPCMacro.h"
#include "PerfTest.h"
%}



class ControlFSM
{
public:
	ControlFSM(){ CurState = IDLE; };
	// Should change it to comb. circuit afterward
	void setState(int in){ CurState = in; };	
	int getState(){ return CurState;};
private:
	int CurState;
};



class FP_Encoder
{
public:
	FP_Encoder();
	FP_Encoder(const char*m, int);
	~FP_Encoder();
	int encode(unsigned char *, unsigned char *, int);
};

FP_Encoder* create_enc_obj();
void destroy_enc_obj( FP_Encoder* object);
FP_Decoder* create_dec_obj();
void destroy_dec_obj( FP_Decoder* object );



