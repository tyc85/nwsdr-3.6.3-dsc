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
	//FP_Encoder(const char *m, int);
	
	~FP_Encoder();
	int encode(unsigned char *, unsigned char *, int);
	void loadfile(const char *m, int);
};

FP_Encoder* create_enc_obj();
void destroy_enc_obj( FP_Encoder* object);
FP_Decoder* create_dec_obj();
void destroy_dec_obj( FP_Decoder* object );

int encode_ldpc(FP_Encoder* p_encoder, char* uchar_in, 
						char* uchar_out, int in_len_byte);

int decode_ldpc(FP_Decoder* p_decoder, unsigned char* uchar_in, 
						unsigned char* uchar_out, int in_len_bit);


