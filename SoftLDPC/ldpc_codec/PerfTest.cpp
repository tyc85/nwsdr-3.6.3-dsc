#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include "ArrayLDPCMacro.h"
#include "ArrayLDPC.h"
#include "rngs.h"
#include "rvgs.h"
#include "PerfTest.h"

//-----------------------
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>


using std::endl;
using std::cout;
using std::cin;
using std::ofstream;
//--------- new wrapper code for rate 1/2 802.11n ldpc code

// the input of the decode_ldpc is 8bit fixed point soft information
extern "C" FP_Decoder* decode_ldpc_general(FP_Decoder* p_decoder, unsigned char* uchar_in, 
						unsigned char* uchar_out, int in_len_bit)
{
	//static const int WIFI_CLENTH = 1944;
	//static const int WIFI_ILENTH = 972;
	static int int_in[CWD_LENGTH];// hardcode to the array code blocklength, s_int
	int i, j; 
	static float temp_in;
	static int counter;
	//static int address;
	static int cwd_cnt; 
	static const int char_size = sizeof(char)*8;
	static int sign;
	static int in_len_byte;
	static unsigned char byte;
	static float iter_count;
	static int iter;
	counter = 0;	
	
	int VarAddr;
	
	if(0)// test no decode case
	{
		cout << "no code in general decode, first info idx is " 
		<< p_decoder->getInfoIndex(0) << endl;
		for(i = 0; i < INFO_LENGTH; i ++)
		{
			//uchar_out[i] = 0;
			byte = 0;
			VarAddr = p_decoder->getInfoIndex(i);
			for(j = 0; j < 8; j++)
			{
				temp_in = (float(uchar_in[VarAddr*8 + j]) - 128);
				//d_packet_byte = (d_packet_byte << 1) | ((pkt_symbol[i*8 + j] - 128) > 0? 1:0);
				byte = (byte << 1) | ((temp_in) > 0? 1:0);

					//cout << 0;//uchar_out[i] = uchar_out[i] | (1 << j);
			}
			uchar_out[i] = byte;
		}
		//cin >> temp_in;
		return 0;
		
	}

	//cout << "check sum: ";
	if(CWD_LENGTH != in_len_bit)
	{
		cerr << "codeword length not match, constant is " << CWD_LENGTH 
		<< ", but in_len_bit is "<<in_len_bit<<endl;
		exit(1);
	}
	iter_count = 0;
	for(cwd_cnt = 0; cwd_cnt < 8; cwd_cnt++)
	{
		//every cwd_cnt + i*8 bit is the first batch
		counter = 0;
		//cout << " cwd_cnt " << cwd_cnt << endl;
		for(i = 0; i < CWD_LENGTH; i++)
		{	
			VarAddr = cwd_cnt + i*8;
			// NOTE: flip the sign here since the modulation in LDPC decoder 
			//is mapping 0 -> +1 and 1 -> -1. 
			//------ change the fixed point transformation in framer_sink
			// how to estimate channel value???
			// chl_val = 10
			
			temp_in = (float(uchar_in[VarAddr]) - 128)/32;
			temp_in = -temp_in*4; // chage sign and mul channel value est
			int_in[counter] = int(temp_in *(1 << FRAC_WIDTH)); //make to fixed point
			
			//int_in[counter] = -(float(uchar_in[VarAddr]) - 128);
			//int_in[counter] = 
			counter ++;
		}

		p_decoder -> setState(PCV);
		iter = p_decoder -> decode_general_fp(int_in);
		//cout << iter << ", ";
		iter_count += iter;
		p_decoder -> loadInfoBit_intr(uchar_out, cwd_cnt);
	}
	if(iter_count < MAX_ITER && iter_count > 1)
		cout << "avg iter: " << iter_count <<endl;

	//cout << "finished decode ldpc general p_dec = " << p_decoder << endl;
	return p_decoder;
}



int encode_ldpc_general(FP_Encoder* p_encoder, char* uchar_in, 
						char* uchar_out, int in_len_byte)
{
	//int int_in[2209];// hardcode to the array code blocklength, s_int
	int i, j; 
	static const int char_size = sizeof(char)*8;
	static int cwd_cnt; 
	static const int info_len = INFO_LENGTH;
	static unsigned int info_buffer[INFO_LENGTH]; 
	//1978 / 8 round up = 248, 1978 mod 8 = 2 
	// for RS outer code in_len_byte is 1670 bytes
	/* !!!! first consider no outer code case !!!!
	* suppose in_len_byte is 1978, then pass LSB of each byte 
	* to the info_buffer and encode it to a codeword (implicit interleaver)
	* at the decoder we do the reverse operation, decode and put into the
	* LSB of each byte, and the next LSB of each byte, etc
	*/
	//cout << "before recast:" << uchar_out << endl;
	unsigned char *t_uchar_in = reinterpret_cast<unsigned char*>(uchar_in);
	unsigned char *t_uchar_out = reinterpret_cast<unsigned char*>(uchar_out);

	//--- should do this somewhere else to optimize the speed
	for(i = 0; i < CWD_LENGTH; i++)
	{
		t_uchar_out[i] = 0;
	}
	// using matching number of byte input with the encoder's info length
	if(INFO_LENGTH != in_len_byte)
	{
		cerr << "INFO_LENGTH_BYTE != in_len_byte \n";
		exit(1); 
	}
	//-- encode 8 codewords
	for(cwd_cnt = 0; cwd_cnt < char_size; cwd_cnt ++)
	{
		//cout << "info buffer\n";
		for(i = 0; i < INFO_LENGTH; i ++)
		{
			info_buffer[i] = (t_uchar_in[i] >> cwd_cnt) & 1	;
			//cout << info_buffer[i];
		}
		//cout << endl;
		//padding zero= > not used for now
		/*
		for(i = in_len_byte; i < INFO_LENGTH_BYTE; i ++)
		{
			info_buffer[i] = 0;
		}
		*/
		p_encoder -> encode(info_buffer, INFO_LENGTH);
		p_encoder -> loadCodeword_intr(t_uchar_out, cwd_cnt);

	}
	return 0;
}
//---------- end of new code for general ldpc code







//---------code for wrapping the code into gnuradio
int encode_ldpc(FP_Encoder* p_encoder, char* uchar_in, 
						char* uchar_out, int in_len_byte)
{
	//int int_in[2209];// hardcode to the array code blocklength, s_int
	int i, j; 
	static const int char_size = sizeof(char)*8;
	static int cwd_cnt; 
	static const int info_len = INFO_LENGTH_BYTE;
	static unsigned int info_buffer[INFO_LENGTH_BYTE]; 
	//1978 / 8 round up = 248, 1978 mod 8 = 2 
	// for RS outer code in_len_byte is 1670 bytes
	/* !!!! first consider no outer code case !!!!
	* suppose in_len_byte is 1978, then pass LSB of each byte 
	* to the info_buffer and encode it to a codeword (implicit interleaver)
	* at the decoder we do the reverse operation, decode and put into the
	* LSB of each byte, and the next LSB of each byte, etc
	*/
	//cout << "before recast:" << uchar_out << endl;
	unsigned char *t_uchar_in = reinterpret_cast<unsigned char*>(uchar_in);
	unsigned char *t_uchar_out = reinterpret_cast<unsigned char*>(uchar_out);
	//cout << "after recast:" << t_uchar_out;
	//cout << "recast test: input pause" << t_uchar_out;
	//cin >> j;
	//--- should do this somewhere else to optimize the speed
	for(i = 0; i < CWD_LENGTH; i++)
	{
		t_uchar_out[i] = 0;
	}
	//cout << "uchar_out:" << (void*)uchar_out << endl;
	//cout << "t_uchar_out:" << (void*)t_uchar_out << endl;
	for(cwd_cnt = 0; cwd_cnt < char_size; cwd_cnt ++)
	{
		//cout << "info buffer\n";
		for(i = 0; i < in_len_byte; i ++)
		{
			info_buffer[i] = (t_uchar_in[i] >> cwd_cnt) & 1	;
			//cout << info_buffer[i];
		}
		//cout << endl;
		//padding zero= > not used for now
		/*
		for(i = in_len_byte; i < INFO_LENGTH_BYTE; i ++)
		{
			info_buffer[i] = 0;
		}
		*/
		p_encoder -> encode(info_buffer, 1978);
		//p_encoder -> loadCodeword_intr(t_uchar_out, cwd_cnt);
		p_encoder -> loadCodeword_intr(t_uchar_out, cwd_cnt);
		//cout << cwd_cnt << "th cwd_cnt: "<< t_uchar_out;
		//uchar_out is an char array with CWD_LENGTH_BYTE = 277 elements
		//p_encoder -> loadCodeword(uchar_out, CWD_LENGTH_BYTE);
	}
	return 0;
}

// the input of the decode_ldpc is 8bit fixed point soft information
extern "C" int decode_ldpc(FP_Decoder* p_decoder, unsigned char* uchar_in, 
						unsigned char* uchar_out, int in_len_bit)
{
	static int int_in[2209];// hardcode to the array code blocklength, s_int
	int i, j; 
	float temp_in;
	static int counter;
	//static int address;
	static int cwd_cnt; 
	static const int char_size = sizeof(char)*8;
	static int sign;
	static int in_len_byte;
	//unsigned char *t_uchar_in = reinterpret_cast<unsigned char*>(uchar_in);
	//unsigned char *t_uchar_out = reinterpret_cast<unsigned char*>(uchar_out);
	//unsigned char *t_uchar_in = uchar_in;
	//unsigned char *t_uchar_out = uchar_out;
	counter = 0;
	//cwd_cnt = 0;// repeat the decoding for 8 times
	// the info bits are padded with 1978 - 1670 = 308 zeros
	
	// there is some virtual interleaving going on
	// every i*8 th element is the first cwd
	
	//cout << "in ldpc_decode, in_len_bit is " <<in_len_bit << endl;
	/*cout << "check in_symbol (integer + 128): \n";
	for(i = 0; i < in_len_bit*8; i ++)
	{
		printf("%d ", uchar_in[i]);
	}*/
	
	
	unsigned char byte;
	int VarAddr;
	if(0)// test uncoded case
	{
		for(i = 0; i < 1978; i ++)
		{
			//uchar_out[i] = 0;
			byte = 0;
			
			for(j = 0; j < 8; j++)
			{
				temp_in = (float(uchar_in[i*8 + j]) - 128);
				//d_packet_byte = (d_packet_byte << 1) | ((pkt_symbol[i*8 + j] - 128) > 0? 1:0);
				byte = (byte << 1) | ((temp_in) > 0? 1:0);

					//cout << 0;//uchar_out[i] = uchar_out[i] | (1 << j);
			}
			uchar_out[i] = byte;
		}
		//cin >> temp_in;
		return 0;
		
	}
	if(0)// test no decode case
	{
		cout << "no code\n";
		for(i = 0; i < 1978; i ++)
		{
			//uchar_out[i] = 0;
			byte = 0;
			VarAddr = p_decoder->getInfoIndex(i);
			for(j = 0; j < 8; j++)
			{
				temp_in = (float(uchar_in[VarAddr*8 + j]) - 128);
				//d_packet_byte = (d_packet_byte << 1) | ((pkt_symbol[i*8 + j] - 128) > 0? 1:0);
				byte = (byte << 1) | ((temp_in) > 0? 1:0);

					//cout << 0;//uchar_out[i] = uchar_out[i] | (1 << j);
			}
			uchar_out[i] = byte;
		}
		//cin >> temp_in;
		return 0;
		
	}
	
	// TC: it is important to initialize output char
   //for(int i = 0; i < 1978; i ++)
	//	uchar_out[i] = 0;

	//cout << endl;
	//ofstream fout("./hard_out");
	//cout << "hard decision for first batch of 2209*8 bits" << endl;

	//cout << "check sum: ";
	for(cwd_cnt = 0; cwd_cnt < 8; cwd_cnt++)
	{
		//in_len_bit is 2209 in array code
		//every cwd_cnt + i*8 bit is the first batch
		counter = 0;
		//cout << " cwd_cnt " << cwd_cnt << endl;
		for(i = 0; i < in_len_bit; i++)
		{	
			VarAddr = cwd_cnt + i*8;
			// NOTE: flip the sign here since the modulation in LDPC decoder 
			//is mapping 0 -> +1 and 1 -> -1. 
			//if(sign < 0)
			//	int_in[counter] = (t_uchar_in[address] & 127);
			//else
			//	int_in[counter] = -(t_uchar_in[address] & 127);
			//printf("uchar_in at address is %d", uchar_in[address]);
			
			//int_in[counter] = int_in[counter];
			//cout << "int_in is " << int_in[counter] << endl;
			//------ change the fixed point transformation in framer_sink
			// how to estimate channel value???
			// chl_val = 10
			
			temp_in = (float(uchar_in[VarAddr]) - 128)/32;
			temp_in = -temp_in*4; // chage sign and mul channel value est
			int_in[counter] = int(temp_in *(1 << FRAC_WIDTH)); //make to fixed point
			
			//int_in[counter] = -(float(uchar_in[VarAddr]) - 128);
			//int_in[counter] = 
			counter ++;
		}
		//cout << counter << endl;
		if(0) // no code case
		{
			for(i = 0; i < 1978; i ++)
			{
				//uchar_out[i] = 0;
				VarAddr = p_decoder->getInfoIndex(i);
				byte = (int_in[VarAddr] > 0? 0:1);
				uchar_out[i] = uchar_out[i] + (byte << (7- cwd_cnt));
			}
		}
		
		//cout <<  p_decoder -> hardDecision(int_in);
		//cin >> temp_in;
		
		//------------ no decode test
		p_decoder -> setState(PCV);
		p_decoder -> decode_fixpoint(int_in);
		p_decoder -> loadInfoBit_intr(uchar_out, cwd_cnt);

		
		// need to perfectly concatenate the information bits 
		// need a bitwise counter to concat

	}
	//cout << endl;
	
	// unload the remaining bits from the last element 
	if(0)
	{
		cout << "check uchar_out (packed byte): \n";
		for(i = 0; i < 1978; i ++)
		{
			printf("%d ", uchar_out[i]);
		}
		cout << endl << "ldpc_decode done\n";
	}
	return 0;
}

extern "C" FP_Decoder* create_dec_obj()
{
	FP_Decoder *ptr;
	//cout << "new FP_Decoder\n";
	ptr = new FP_Decoder("array_infoindx.txt", 1);
	//ptr = new FP_Decoder;
	//ptr->setInfoIndex("array_infoindx.txt", 1);

	return ptr;
}

extern "C" void destroy_dec_obj( FP_Decoder* object )
{
  delete object;
}

FP_Encoder* create_enc_obj()
{

	//FP_Encoder *enc_ptr = new FP_Encoder;
	//cout << "calling loadfile\n";
	//enc_ptr->loadfile("G_array_forward.txt", 0);
	return new FP_Encoder("G_array_forward.txt", 1);
}

void destroy_enc_obj( FP_Encoder* object )
{
  delete object;
}

extern "C" FP_Decoder* create_dec_obj_general(const char *in_h, const char *in_idx, int vflag)
{
	FP_Decoder *ptr;
	//char *in = "array_infoindx.txt";
	
	ptr = new FP_Decoder(in_idx, vflag);
	ptr -> ReadH(in_h, vflag);
	//ptr = new FP_Decoder;
	//ptr->setInfoIndex("array_infoindx.txt", 1);
	//ptr -> setInfoIndex(in, vflag);
	return ptr;
}

FP_Encoder* create_enc_obj_general(const char *in, int vflag)
{

	//FP_Encoder *enc_ptr = new FP_Encoder;
	//cout << "calling loadfile\n";
	//enc_ptr->loadfile("G_array_forward.txt", 0);
	return new FP_Encoder(in, vflag);
}

void noMoreMemory()
{
	cerr << "Unable to satisfy request for memory\n";
	abort();
}
