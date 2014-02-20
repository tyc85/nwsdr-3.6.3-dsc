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
//#include <windows.h>
//-----------------------
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
//-----------------------


using std::endl;
using std::cout;
using std::cin;
using std::ofstream;

//---------------------------- Non in-class code
void noMoreMemory()
{
	cerr << "Unable to satisfy request for memory\n";
	abort();
}
int ArrayLDPC_Debug(double EbN0_dB)
{
	//2209 - 231 = 1978
	//1978 / 8 = 247, 1978 % 8 = 2
	char InfoStream[248] = "OMG how long should this string be to make it 248, just imagine that. \
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
	
	class FP_Decoder Decoder;
	class FP_Encoder Encoder("G_array_forward.txt", 0);
	
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

	Decoder.setInfoBit(InfoStream, 248);
	for(i = 0; i < INFO_LENGTH; i++)
	{
		info_indx[i] = Encoder.getInfoIndex(i);
	}
	Decoder.setInfoIndex(info_indx);
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

int EncodeTrial(char *info, int MaxPacket)
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


int main(int argc, char *argv[])
{
	//char Filename[30];
	double db_start, db_end, db_step;
	int i = 0;
	char InfoStream[248] = "OMG how long should this string be to make it 248, just imagine that. \
								   I guess it's still not long enough. Let's see. This is a testing string \
									for a lot of characters so that we have some random bit stream that's\
									correct";
	
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

