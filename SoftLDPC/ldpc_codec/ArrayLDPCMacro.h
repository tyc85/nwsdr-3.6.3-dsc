#ifndef ARRAY_MACRO_H
#define ARRAY_MACRO_H


#include <fstream>
#include <iostream>
#include <math.h>
//#include "common_classes.h"
//#include "wifi_ldpc_const.h"
//---------- Shared constants
//#define ALLZERO
//#define INFO_LENGTH 192
//#define XMITTED_BIT 384
//#define CWD_LENGTH 608
//#define CWD_LENGTH 576
//#define CIRCULANT_SIZE 32
#define DEBUG 0

using namespace std;

enum Simulation {MAX_ITER = 18, NUM_PEEK = 1000000, SEED = 100};
enum CodeWifi {
		NUM_VAR = 1944, NUM_CHK = 972, NUM_CGRP = 12, NUM_VGRP = 24, CHK_DEG = 8, VAR_DEG = 11,
		P = 81, CIR_SIZE = 81, INFO_LENGTH = 972, CWD_LENGTH = 1944, 
		INFO_LENGTH_BYTE = 122, CWD_LENGTH_BYTE = 244};
//enum Code {
		//NUM_VAR = 2209, NUM_CHK = 235, NUM_CGRP = 5, VAR_DEG = 5, NUM_VGRP = 47, 
		//CHK_DEG = 47, P = 47, CIR_SIZE = 47, INFO_LENGTH = 1978, CWD_LENGTH = 2209,
		//INFO_LENGTH_BYTE = 248, CWD_LENGTH_BYTE = 277};
enum RAM_Const {
		RAM_WIDTH = 32, RAM_SLICE = 8, RAM_DEPTH = NUM_CGRP*CIR_SIZE};
enum Precision {
		WIDTH_MASK = 0x000000ff, //8 bit mask
		SIGN_MASK = 0x00000080, //not really used yet, the 8th bit mask
		//INT_WIDTH = 4, 
		//FRAC_WIDTH = 3, 
		//INT_WIDTH_NOISE = 4, 
		//FRAC_WIDTH_NOISE = 12
		INT_WIDTH = 4, 
		FRAC_WIDTH = 4, 
		INT_WIDTH_NOISE = 4, 
		FRAC_WIDTH_NOISE = 6
};
enum StateFSM {IDLE, PCV, V2C, SXOR, C2V, SIMEND};

class ROM
{
public:
	ROM()
	{
		RowDeg = NUM_VGRP;
		ColDeg = NUM_CGRP;
		CirSize = P;
		NumVar = NUM_VAR;
		NumChk = NUM_CHK;
		int i, j;
		for(i = 0; i < NUM_CGRP; i++)
		{
			for(j = 0; j < NUM_VGRP; j++)
			{
				CirShift[i][j] = (i*j) % CirSize;
			}
		}
		CodeRate = 1 - double(NUM_CGRP*CirSize -NUM_CGRP+1)/(CirSize*CirSize);
		//cout <<"Array code with circulant size "  << P <<", blocklength " 
		//	  << NUM_VAR << ", code rate " << getRate() << endl;
	};
	//~ROM();
	double getRate(){ return CodeRate; };
	int getCirShift(int Chk, int Var){ return CirShift[Chk][Var]; };
private:
	int RowDeg;	// Set as variable to accommodate irregular code for future
	int ColDeg;
	int CirSize;
	int Width;
	int Slice;
	int Depth;
	int Address;
	int CirShift[NUM_CGRP][NUM_VGRP];
	int DataBus;
	int WrE;
	int RdE;
	int NumVar;
	int NumChk;
	double CodeRate;
};

//!!!!! remember to remove the double type BRAM
class Memory
{
public:
	//Memory();
	//{
	//	set_new_handler(noMoreMemory);
	//	BRAM = new int[RAM_DEPTH];//CHK_DEG*CIR_SIZE
	//};
	//~Memory(){ delete [] BRAM; };
	int rdData(){	return BRAM_fp[Address]; };
	double getData(){	return BRAM[Address]; };
	void wrtData(int in){ BRAM_fp[Address] = in; };
	void wrtData(double in){ std::cout << "using float ram\n"; BRAM[Address] = in; };
	void setAddress(int Addr){ Address = Addr; };
private:
	int Address;
	int BRAM_fp[RAM_DEPTH];
	double BRAM[2]; //save memory see if it's faster
	int DataBus;
	int WrE;
	int RdE;
};


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



#endif

