#ifndef COMMON_H
#define COMMON_H

#include <iostream>
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
	double BRAM[RAM_DEPTH]; //save memory see if it's faster
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
