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





//---------new code for wrapping the code into gnuradio
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
   //{
	//	
	//	if(DEBUG)
	//	{
	//		;
			//printf("%d ", uchar_out[i]);
	//	}
	//}
	//cout << endl;
	//ofstream fout("./hard_out");
	//cout << "hard decision for first batch of 2209*8 bits" << endl;
	
	/*for(j = 0; j < 8; j++)
	{
		for(i = 2209*j; i < 2209*(j+1); i ++)
		{
			temp_in = (float(uchar_in[i]) - 128)/32;
			fout << (temp_in > 0 ? 1:0); 
		}
		fout << endl;
	}
	fout.close();
	*/
	//cout << endl;
	//cin >> temp_in;
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
			temp_in = -temp_in*8; // chage sign and mul channel value est
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
	ptr = new FP_Decoder;
	// TC: it is extremely important to set the information bit index
	cout << "loading info index\n";
	ptr->setInfoIndex("array_infoindx.txt", 1);
	//cout << "the pointer is " << ptr <<endl; 
	return ptr;
}

extern "C" void destroy_dec_obj( FP_Decoder* object )
{
  delete object;
}

FP_Encoder* create_enc_obj()
{
	//FP_Decoder *ptr;
	//cout << "new FP_Decoder\n";
	//ptr = new FP_Decoder;
	//cout << "the pointer is " << ptr <<endl; 
	FP_Encoder *enc_ptr = new FP_Encoder;
	//cout << "calling loadfile\n";
	enc_ptr->loadfile("G_array_forward.txt", 0);
	return enc_ptr;
}

void destroy_enc_obj( FP_Encoder* object )
{
  delete object;
}




void noMoreMemory()
{
	cerr << "Unable to satisfy request for memory\n";
	abort();
}

/*
int ArrayLDPC_Debug(double EbN0_dB)
{
	//2209 - 231 = 1978
	//1978 / 8 = 247, 1978 % 8 = 2
	unsigned char InfoStream[248] = "OMG how long should this string be to make it 248, just imagine that. \
								   I guess it's still not long enough. Let's see. This is a testing string \
									for a lot of characters so that we have some random bit stream that's\
									correct";
	char sInfoStream[248] = "OMG how long should this string be to make it 248, just imagine that. \
								   I guess it's still not long enough. Let's see. This is a testing string \
									for a lot of characters so that we have some random bit stream that's\
									correct";
	//--- unused debugging varibles  
	//char InfoStream[248] = "";
	//int InfoBit[INFO_LENGTH];
	//int Codewordtemp[CWD_LENGTH];
	//double Receive[CWD_LENGTH];
	//--- unused debugging varibles
	int i, j;
	int LLR_fp[CWD_LENGTH];
	//int pckerror = 0;
	double biterror = 0;
	long Counter = 0;
	long Clk = 0;

	double LLR[CWD_LENGTH];
	double db = 1.0;
	double Rate = 0;
	//double EbN0_dB = 4;
	double snr = 0;
	double sigma;
	double blkerror = 0; 
	double pckerror = 0;
	int info_indx[INFO_LENGTH];
	
	
	FP_Decoder Decoder;
	FP_Encoder Encoder("G_array_forward.txt", 0);
	
	Rate = Decoder.getRate();
	snr = 2*pow(10.0, EbN0_dB/10)*Rate;			// Eb/N0	to snr
	sigma = sqrt(1/snr);
	cout << "SNR is " << 10*log10(snr) << " dB" << endl;
	biterror = 0;
	Clk = 0;

	//---- debugging for char stream output
	//char out[277] = "";
	//out = new char[553];
	// working now
	//Encoder.encode(InfoStream, out, 248);
	//---- 
	//char*(InfoStr)
	Decoder.setInfoBit(sInfoStream, 248);
	//ofstream FileInfoIdx("array_infoindx.txt");
	for(i = 0; i < INFO_LENGTH; i++)
	{
		info_indx[i] = Encoder.getInfoIndex(i);
		//FileInfoIdx << info_indx[i] << " ";
	}
	//FileInfoIdx.close();
	//Decoder.setInfoIndex(info_indx);
	cout << "testing setInfoIndex from file\n";
	
	Decoder.setInfoIndex("array_infoindx.txt");
	exit(1);
	//for(i = 0; i < CWD_LENGTH; i++)
	//		Codewordtemp[i] = Encoder.getCodeword(i);
	//Decoder.setCodeword(Codewordtemp);
	//cout << "true codeword checksum:" << Decoder.check() <<endl;
	while(pckerror < 100)
	{
		blkerror = 0;

		//for(i = 247; i >= 0; i--)
		//	cout << std::bitset<8>(InfoStream[i]) << ", ";
		//cout <<endl;
		Encoder.encode(InfoStream, 248);
		
		//cout << "codeword checksum:" << Decoder.check_fp(Codewordtemp) <<endl;
		//cout << "codeword checksum: (0 is pass, 1 is not pass):" << Decoder.check_fp(Codeword)<<endl;
		for(i = 0; i < CWD_LENGTH; i++)
		{
			//-- BPSK modulation => 0 -> 1, 1 -> -1
			// Using Box-Muller method to generate Gaussian noise
			LLR[i] = 2*snr*(1 - 2*Encoder.getCodeword(i) + Normal(0, sigma));
			//-- noiseless case
			//LLR[i] = 2*snr*(1 - 2*Encoder.getCodeword(i));
			// Using Wallace method to generate Gaussian noise
			//LLR[i] = 2*EbN0*(1 + Wallace(0, sigma));
			LLR_fp[i] = int(LLR[i]*(1<<FRAC_WIDTH));
		}
		Decoder.setState(PCV);
		
		
		//cout << "checking hard output before decoding:" << Decoder.hardDecision(LLR_fp) << endl;
		//blkerror = Decoder.decode(LLR);
		//cout << Decoder.decode_fixpoint(LLR_fp) << " total iterations" <<endl;
		Decoder.decode_fixpoint(LLR_fp);
		Decoder.resetBER();
		blkerror = Decoder.calculateBER();
		if(blkerror > 0)
			pckerror++;
		biterror += blkerror;
		Counter++;
	}
	cout << biterror << " " << pckerror << " " << Counter << endl
		<< " FER: " << pckerror/Counter << " BER: " << biterror/Counter/CWD_LENGTH << endl;
	//Decoder.getInfoBit();
	return 0;
}



int ArrayLDPC_PerfTest(double db_start, double db_end, double db_step, char* Filename)
{
	char Filename2[40]="";
	int InfoBit[INFO_LENGTH];
	int i, j;
	int CurrentInd = 0;
	int LLR_fp[CWD_LENGTH];
	//int PckError = 0;
	double BitError = 0;
	long Counter = 0;
	long Clk = 0;

	double Receive[CWD_LENGTH];
	double LLR[CWD_LENGTH];
	double snr;
	double db = 1.0;
	double Rate = 0;
	double EbN0_dB = 4;
	double EbN0 = 0;
	double Sigma;
	double BlkError = 0; 
	double PckError = 0;
	double **MemBank; //Memory bank
	bool per_flag;

	class FP_Decoder Decoder;
	class ControlFSM FSM;

	ofstream FilePtr(Filename);
	if(!FilePtr)
	{
		cerr << "failed to open " << Filename << endl;
		//system("pause");
		exit(0);
	}
	FilePtr.close();
	//strcat(Filename2, "log_");
	//strcat(Filename2, Filename);
	//strcat(Filename2, ".txt");
	sprintf(Filename2, "%s_log.txt", Filename);
	ofstream LogPtr(Filename2);
	if(!LogPtr)
	{
		cerr << "failed to open " << Filename2 << endl;
		system("pause");
		exit(0);
	}
	LogPtr.close();

	
	//cout << "SNR? ";
	//cin >> EbN0_dB;
	EbN0_dB = db_start;
	Rate = Decoder.getRate();
	EbN0 = 2*pow(10.0, EbN0_dB/10)*Rate;			// Eb/N0	
	Sigma = sqrt(1/EbN0);
	BitError = 0;
	Clk = 0;
	while(PckError < 100)
	{
		BlkError = 0;
		for(i = 0; i < CWD_LENGTH; i++)
		{
			//-- All zero codeword
			// Using Box-Muller method to generate Gaussian noise
			LLR[i] = 2*EbN0*(1 + Normal(0, Sigma));
			// Using Wallace method to generate Gaussian noise
			//LLR[i] = 2*EbN0*(1 + Wallace(0, Sigma));
			LLR_fp[i] = int(LLR[i]*(1<<FRAC_WIDTH));
			//Debug
			//LLR_fp[i] = i % 47;
		}
		Decoder.setState(PCV);
		//BlkError = Decoder.decode(LLR);
		BlkError = Decoder.decode_fixpoint(LLR_fp);
		if(BlkError > 0)
			PckError++;
		BitError += BlkError;
		Counter++;
	}
	cout << BitError << " " << PckError << " " << Counter << endl
		<< " FER: " << PckError/Counter << " BER: " << BitError/Counter/CWD_LENGTH << endl;
	//Decoder.getInfoBit();
	return 0;
}


int DecodeTrial(double EbN0_dB, int MaxPckNum)
{
	double LLR[CWD_LENGTH];
	int LLR_fp[100][CWD_LENGTH];
	int i, j; 
	double snr, sigma, Rate = 0;; 
	class FP_Decoder Decoder;


	//EbN0_dB = db;
	Rate = Decoder.getRate();
	snr = 2*pow(10.0, EbN0_dB/10)*Rate;			// Eb/N0	to snr in BPSK
	sigma = sqrt(1/snr);

	//---- linux version timing 
	int sr=0,trials = MaxPckNum,framebits=CWD_LENGTH;
	//time the running time
	struct rusage start,finish;
	double extime;
	// generate 100 block of random noise
	for(i = 0; i < 100; i ++)
	{
		
		for(j = 0; j < CWD_LENGTH; j++)
		{
			//-- All zero codeword
			// Using Box-Muller method to generate Gaussian noise
			LLR[j] = 2*snr*(1 + Normal(0, sigma));
			// Using Wallace method to generate Gaussian noise
			//LLR[i] = 2*EbN0*(1 + Wallace(0, Sigma));
			LLR_fp[i][j] = int(LLR[j]*(1<<FRAC_WIDTH));
		}
	}
	getrusage(RUSAGE_SELF,&start);
	for(i = 0; i < MaxPckNum; i ++ )
	{
		//BlkError = 0;
		Decoder.setState(PCV);
		// cyclically decode different blocks of noisy all zero codeword
		Decoder.decode_fixpoint(LLR_fp[i%100]);
		//Counter++;
	}

	getrusage(RUSAGE_SELF,&finish);
	extime = finish.ru_utime.tv_sec - start.ru_utime.tv_sec + 1e-6*(finish.ru_utime.tv_usec - start.ru_utime.tv_usec);
	printf("Execution time for %d %d-bit frames: %.2f sec\n",trials, framebits,extime);
	printf("decoder speed: %g bits/s\n",trials*framebits/extime);
	//cout << BitError << " " << PckError << " " << Counter << endl
	//	<< " FER: " << PckError/Counter << " BER: " << BitError/Counter/CWD_LENGTH << endl;
	//Decoder.getInfoBit();
	return 0;
}

int EncodeTrial(unsigned char *info, int MaxPacket)
{
	class FP_Encoder Encoder("G_array_forward.txt", 1);
	int i;

	//---- linux version timing 
	int sr=0,trials = MaxPacket,framebits=CWD_LENGTH;
	//time the running time
	struct rusage start,finish;
	double extime;
	
	getrusage(RUSAGE_SELF,&start);
	for(i = 0; i < MaxPacket; i++)
	{
		Encoder.encode(info, 248);
	}

	getrusage(RUSAGE_SELF,&finish);
	extime = finish.ru_utime.tv_sec - start.ru_utime.tv_sec + 1e-6*(finish.ru_utime.tv_usec - start.ru_utime.tv_usec);
	printf("Execution time for %d %d-bit frames: %.2f sec\n",trials, framebits,extime);
	cout << "encoder speed: " << trials*framebits/extime << " bits/s" << endl;
	return 0;
}


*/
