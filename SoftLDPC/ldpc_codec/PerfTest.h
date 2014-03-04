#ifndef PERFTEST_H
#define PERFTEST_H
#include <iostream>
//int ArrayLDPC_Debug(double EbN0_dB);

class t_enc
{
public:
	t_enc(){std::cout<<"contruct\n";};
	~t_enc(){std::cout<<"destruct\n";};
	void test() { std::cout<<"test\n"; };
};

FP_Encoder* create_enc_obj();
void destroy_enc_obj( FP_Encoder* object );

extern "C" FP_Decoder* create_dec_obj();
extern "C" void destroy_dec_obj( FP_Decoder* object );

int encode_ldpc(FP_Encoder* p_encoder, char* uchar_in, 
						char* uchar_out, int in_len_byte);
						
extern "C" int decode_ldpc(FP_Decoder* p_decoder, unsigned char* uchar_in, 
						unsigned char* uchar_out, int in_len_bit);

#endif
