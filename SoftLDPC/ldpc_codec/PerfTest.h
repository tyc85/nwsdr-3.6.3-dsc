#ifndef PERFTEST_H
#define PERFTEST_H
#include <iostream>

//-----------------------
#include "wifi_encoder.h"
#include "wifi_decoder.h"


FP_Encoder* create_enc_obj();
void destroy_enc_obj( FP_Encoder* object );

extern "C" FP_Decoder* create_dec_obj();
extern "C" void destroy_dec_obj( FP_Decoder* object );

int encode_ldpc(FP_Encoder* p_encoder, char* uchar_in, 
						char* uchar_out, int in_len_byte);
						
extern "C" int decode_ldpc(FP_Decoder* p_decoder, unsigned char* uchar_in, 
						unsigned char* uchar_out, int in_len_bit);

// general code:
FP_Encoder* create_enc_obj_general(const char *in, int vflag);

extern "C" FP_Decoder* create_dec_obj_general(const char *in_h, const char *in_idx, int vflag);

extern "C" FP_Decoder* decode_ldpc_general(FP_Decoder* p_decoder, unsigned char* uchar_in, 
						unsigned char* uchar_out, int in_len_bit);

int encode_ldpc_general(FP_Encoder* p_encoder, char* uchar_in, 
						char* uchar_out, int in_len_byte);
						


#endif
