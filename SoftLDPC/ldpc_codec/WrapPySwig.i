%module WrapPySwig

%{
#include "ArrayLDPCMacro.h"
encode_ldpc(FP_Encoder* p_encoder, unsigned char* uchar_in, unsigned char* uchar_out, int in_len_byte);
int decode_ldpc(FP_Decoder* p_decoder, unsigned char* uchar_in, unsigned char* uchar_out, int in_len_bit);
FP_Decoder* create_dec_obj();
FP_Encoder* create_enc_obj();
void destroy_enc_obj( FP_Encoder* object );
void destroy_dec_obj( FP_Decoder* object );
%}

encode_ldpc(FP_Encoder* p_encoder, unsigned char* uchar_in, unsigned char* uchar_out, int in_len_byte);
int decode_ldpc(FP_Decoder* p_decoder, unsigned char* uchar_in, unsigned char* uchar_out, int in_len_bit);
FP_Decoder* create_dec_obj();
FP_Encoder* create_enc_obj();
void destroy_enc_obj( FP_Encoder* object );
void destroy_dec_obj( FP_Decoder* object );
