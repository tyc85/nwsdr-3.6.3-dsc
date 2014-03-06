
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <iostream>
#include "ArrayLDPCMacro.h"
#include "ArrayLDPC.h"
//#include <windows.h>
#include "PerfTest.h"
#include "rvgs.h"

using std::endl;
using std::cout;
using std::cin;

int ArrayLDPC_Debug_Wifi()
{
	//2209 - 231 = 1978
	//1978 / 8 = 247, 1978 % 8 = 2
	//char InfoStream[248] = "OMG how long should this string be to make it 248, just imagine that. \
								   I guess it's still not long enough. Let's see. This is a testing string \
									for a lot of characters so that we have some random bit stream that's\
									correct";
	//--- unused debugging varibles  
	//char InfoStream[243] = "";// all zero info, exactly 243 bytes for wifi code
	char InfoStream[122] = "OMG  how long   dd   should this string be to make it 243";
	//char InfoStream[122] = "";
	//int InfoBit[INFO_LENGTH];
	int Codewordtemp[CWD_LENGTH];
	//double Receive[CWD_LENGTH];
	//--- unused debugging varibles
	int i, j;
	int LLR_fp[CWD_LENGTH];
	//int pckerror = 0;
	double biterror = 0;
	long Counter = 0;

	double LLR[CWD_LENGTH];
	double db = 1.0;
	double Rate = 0;
	double EbN0_dB = 4;
	double snr = 0;
	double sigma;
	double blkerror = 0; 
	double pckerror = 0;
	int info_indx[INFO_LENGTH];
	
	class FP_Decoder Decoder("wifi_infoindx.txt", 1);
	class FP_Encoder Encoder("H_802.11_IndZerog.txt", 1);
	
	EbN0_dB = 3;
	//Rate = Decoder.getRate();
	
	cout << "EbNo in dB? ";
	cin >> EbN0_dB;
	snr = 2*pow(10.0, EbN0_dB/10)*0.5;			// Eb/N0	to snr
	sigma = sqrt(1/snr);
	cout << "SNR is " << 10*log10(snr) << " dB" << endl;
	biterror = 0;

	//---- debugging
	//char out[277] = "";
	//out = new char[553];
	// working now
	//Encoder.encode(InfoStream, out, 248);
	//---- 
	// set the true information bits in the decoder to calculate BER
	Decoder.setInfoBit(InfoStream, 122);
	//for(i = 0; i < INFO_LENGTH; i++)
	//{
	//	//info_indx[i] = Encoder.getInfoIndex(i);
	//	// for wifi code it's systematic due to staircase structure
	//	info_indx[i] = i;
	//}
	// NEED IMPROVEMENT! SHOULD BE ABLE TO HAVE SYSTEMATIC BITS
	/*for(i = 0; i < INFO_LENGTH; i++)
	{
		info_indx[i] = Encoder.getInfoIndex(i);
	}*/
	//reinterpret_cast<unsigned char*>()
	Encoder.encode(InfoStream, 122);
	//Decoder.setInfoIndex(info_indx);
	//for(i = 0; i < CWD_LENGTH; i++)
	//		Codewordtemp[i] = Encoder.getCodeword(i);
	//Decoder.setCodeword(Codewordtemp);
	//cout << "true codeword checksum:" << Decoder.check() <<endl;

	Decoder.ReadH("H_802.11_IndZero.txt", 1);
	while(pckerror < 100)
	{
		blkerror = 0;

		//for(i = 247; i >= 0; i--)
		//	cout << std::bitset<8>(InfoStream[i]) << ", ";
		//cout <<endl;
		//WIFI code
		
		//cout << "codeword checksum:" << Decoder.check_fp(Codewordtemp) <<endl;
		//cout << "codeword checksum: (0 is pass, 1 is not pass):" << Decoder.check_fp(Codeword)<<endl;
		for(i = 0; i < CWD_LENGTH; i++)
		{
			//-- BPSK modulation => 0 -> 1, 1 -> -1
			// Using Box-Muller method to generate Gaussian noise
			LLR[i] = 2*snr*(1 - 2*Encoder.getCodeword(i) + Normal(0, sigma));
			// -- all zero
			//LLR[i] = 2*snr*(1 + Normal(0, sigma));
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
		//Decoder.decode(LLR);
		Decoder.decode_general_fp(LLR_fp);
		Decoder.resetBER();
		blkerror = Decoder.calculateBER();
		if(blkerror > 0)
			pckerror++;
		biterror += blkerror;
		if(Counter % 1000 == 0)
			cout << biterror << endl;
		Counter++;
	}
	cout << biterror << " " << pckerror << " " << Counter << endl
		<< " FER: " << pckerror/Counter << " BER: " << biterror/Counter/CWD_LENGTH << endl;
	//Decoder.getInfoBit();
	return 0;
}

int main(int argc, char *argv[])
{
	//char Filename[30];
	double db_start, db_end, db_step;
	int i = 0;
	if(argc == 5)
	{
		printf("%.1f, %.1f, %.1f, %s\n", atof(argv[1]),atof(argv[2]),atof(argv[3]),argv[4]);
		//ArrayLDPC_PerfTest(atof(argv[1]), atof(argv[2]), atof(argv[3]), argv[4]);
	}
	else if(1)
	{
		ArrayLDPC_Debug_Wifi();
	}
	else
	{
		long MaxPckNum;
		printf("argc = %d \n time trial with: manual input\n", argc);
		cout << "EbN0 in dB: ";
		cin >> db_start;
		cout << "simulate how many packets? ( " << 2209 << " is the codeword length ): ";
		cin >> MaxPckNum;

		//---- code timging for windows
		/*
		__int64 ctr1 = 0, ctr2 = 0, freq = 0;
      // Start timing the code.
      if(QueryPerformanceCounter((LARGE_INTEGER *) &ctr1) != 0)
		{
			// Do what ever you do, what ever you need to time...
			
			// Finish timing the code.
			ArrayLDPC_TimeTrial(db_start, MaxPckNum, "timing.txt");
         QueryPerformanceCounter((LARGE_INTEGER *) &ctr2);
         QueryPerformanceFrequency((LARGE_INTEGER *) &freq);

         // Print the time spent in microseconds to the console.

         std::cout << ((ctr2 - ctr1) * 1.0 / freq) << "  seconds" << std::endl;
			std::cout << 2209*MaxPckNum/((ctr2 - ctr1) * 1.0 / freq) << " bits per second" << std::endl;
      }
      */
		
	}

	return 0;
}
