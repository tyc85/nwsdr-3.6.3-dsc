#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "rngs.h"
#include "rvgs.h"
//#include "Memory.h"
//#include "ArrayLDPC.h"
#include "ArrayLDPCMacro.h"
using namespace std;
//--------- notes -----------
// 1. don't really need the posterior stored. just the hard decision would be enough
// 
//---------------------------
//---- new part
// new version that outputs character stream
// need to fixed the char to unsigned char, some compiling error occur
// may be due to the addition of signed integers? => change ot unsigned int

int FP_Decoder::decode_general_fp(const int *LLR)
{
	
	//int Counter; 
	static int i, j, k;
	//int VarShift[NUM_VGRP];
	static int VarAddr;
	static int ChkAddr;
	//static int Shift;
	static int Iteration;
	static int BankSelect;
	static int deg;
	static int MV2C[CHK_DEG];
	static int MC2V[VAR_DEG];
	static int Forward[CHK_DEG];
	static int Backward[CHK_DEG];
	
	//Initialize the Edge RAM with channel values from variable node 
	//double accum[CWD_LENGTH];
	static int accum;
	static int addr_count[INFO_LENGTH]; // count the number of check vnode added for each chk
	
	if(!hardDecision_general(LLR)) 
	{
		if(0)
			cout << "check sum pass before decoding (returning)" << endl;	
		return 0;
	}
	
	
	if(getState() == PCV)
	{
		for(i = 0; i < NUM_CGRP; i++)
		{
			for(j = 0; j < CIR_SIZE; j++)
			{
				ChkAddr = j + i*CIR_SIZE;
				deg =  cdeg[ChkAddr];
				for(k = 0; k < deg; k++)
				{
					//Shift = CodeROM.getCirShift(k, i);
					// the kth variable node addr of current cnode
					VarAddr = clist[ChkAddr][k];
					BankSelect = k; //simply which vgroup we are at
					EdgeRAM[BankSelect].setAddress(ChkAddr);	
					EdgeRAM[BankSelect].wrtData(LLR[VarAddr]);
				}
			}
		}
	}
	Iteration = 0;
	setState(C2V);
	//cout << "start iteration 1\n";
	while(Iteration < MAX_ITER)
	{
		//-- Process each check group at a time. 
		if(getState() != C2V)
			break;
		for(i = 0; i < NUM_CGRP; i++)
		{
			//-- Check node parallel process denoted in for loop (loop through all check nodes in one group)
			for(j = 0; j < CIR_SIZE; j++)
			{
				ChkAddr = j + i*CIR_SIZE;
				deg = cdeg[ChkAddr];
				for(k = 0; k < deg; k++)
				{
					//This is simply for better read of the code. Not necessary.
					//Shift = CodeROM.getCirShift(i, k);
					//-- Prepare all the V2C message
					BankSelect = k;
					EdgeRAM[BankSelect].setAddress(ChkAddr); 
					MV2C[k] = EdgeRAM[BankSelect].rdData();	
				}
				//-- Do binary operation of the sxor: forward backward computation
				Forward[0] = MV2C[0];
				Backward[deg-1] = MV2C[deg-1];
				for(k = 1; k < deg; k++)
				{
					Forward[k] = sxor(Forward[k-1], MV2C[k]);
					Backward[deg - k - 1] = sxor(Backward[deg - k], MV2C[deg - 1 - k]);
				}
				//-- make MV2C temp memory to store all the computed MC2V message from one check node
				//-- Compute the first and last MC2V and write back to RAM
				k = 0; // 1st// a bit redundant
				BankSelect = k;
				MV2C[k] = Backward[k+1]; // redundant
				//ChkAddr = j + i*CIR_SIZE; //already done
				EdgeRAM[BankSelect].setAddress(ChkAddr); 
				EdgeRAM[BankSelect].wrtData(MV2C[k]);

				//MV2C[CHK_DEG-1] = Forward[CHK_DEG-1];
				k = deg-1; // last
				BankSelect = k;
				MV2C[k] = Forward[k-1];
				//ChkAddr = j + i*CIR_SIZE; //already done
				EdgeRAM[BankSelect].setAddress(ChkAddr); 
				EdgeRAM[BankSelect].wrtData(MV2C[k]);
				
				//-- Compute the MV2C message and write back to the RAM
				for(k = 1; k < deg-1; k++)
				{
					MV2C[k] = sxor(Forward[k-1], Backward[k+1]);
					BankSelect = k;
					EdgeRAM[BankSelect].setAddress(ChkAddr); 
					EdgeRAM[BankSelect].wrtData(MV2C[k]);
					//if(MV2C[k] > 8)
					//	cout << MV2C[k];
				}
			}
		}

		//---- still need this part of the code...
		for(i = 0; i < INFO_LENGTH; i++)
			addr_count[i] = 0;
		
		setState(V2C);
		for(i = 0; i < NUM_VGRP; i++)
		{
			for(j = 0; j < CIR_SIZE; j++)
			{
				VarAddr = i*CIR_SIZE + j;
				accum = 0;
				deg = vdeg[VarAddr];
				// Accumulate all the MC2V message
				for(k = 0; k < deg; k++)
				{
					//-- new code
					// kth cnode index for the current vnode
					ChkAddr = vlist[VarAddr][k];
					//Select which bank of RAM is active
					BankSelect = addr_count[ChkAddr];  // vgroup index 
					EdgeRAM[BankSelect].setAddress(ChkAddr);	
					MC2V[k] = EdgeRAM[BankSelect].rdData();
					accum = accum + MC2V[k];
				}
				// Add in the channel value
				accum = accum + LLR[VarAddr];
				wrtPost(VarAddr, accum);
				for(k = 0; k < deg; k++)
				{
					//Shift = CodeROM.getCirShift(k, i);
					ChkAddr = vlist[VarAddr][k];
					//Select which bank of RAM is active
					BankSelect = addr_count[ChkAddr];  // vgroup index
					EdgeRAM[BankSelect].setAddress(ChkAddr);	
					EdgeRAM[BankSelect].wrtData(accum - MC2V[k]);
					addr_count[ChkAddr]++;
				}
			}
		}
		Iteration++;

		if(!checkPost_fp_general())
		{
			//cout << "iter = " << Iteration << endl;
			setState(IDLE);
			//return Iteration;
		}
		else
			setState(C2V);
	}
	//cout << "iter = " << Iteration << endl;
	return Iteration;
}

int FP_Decoder::checkPost_fp_general()
{
	int i, j, k;
	int shift;
	unsigned int deg;
	static unsigned int varaddr;
	static unsigned int chkaddr;
	static unsigned int checksum, checksumtemp;
	// full check
	for(i = 0; i < CWD_LENGTH; i++)
	{
		DecodedCodeword[i] = getPost_fp(i) > 0 ? 0:1;
	}

	for(i = 0; i < NUM_CGRP; i ++)
	{
		for(j = 0; j < CIR_SIZE; j++)
		{
			checksum = 0;
			//checksumtemp = 0;
			chkaddr = j + i*CIR_SIZE;
			deg = cdeg[chkaddr];
			for(k = 0; k < deg; k++)
			{

				varaddr = clist[chkaddr][k];
				checksum ^= (DecodedCodeword[varaddr]);
				//checksumtemp ^= (TrueCodeword[varaddr]);	
			}
			if(checksum != 0)
				return 1; // check sum does not pass
		}
	}
	return 0; // check sum pass
}


FP_Decoder::FP_Decoder(const char* in, int vflag)
{
	setInfoIndex(in, vflag);
}


void FP_Decoder::loadInfoBit_intr(unsigned char* out, int bit_pos)
{
	int i, j; 
	//static int VarAddr;
	// first bit is in the LSB
	//currently hard coded for information length
	// in_len should be 1670 byte ?  
	// and the rest of the info is padded to be zero
	//cout << "in loadInfoBit_inter, decoded info are:\n";
	for(i = 0; i < INFO_LENGTH; i++)
	{
		//---
		//for bit_pos = 0, 1, 2, MASK = 1111_1110, 1111_1101, 1111_1011
		//---
		//VarAddr = i*8 + bit_pos;
		out[i] = out[i] & (0xff - (1 << (7-bit_pos))); 
		out[i] = out[i] + (DecodedCodeword[InfoIndex[i]] << (7-bit_pos));	
		//cout << out[i] << endl;
		//cout << std::bitset<8>(out[i]);
	}

}

//---- load decoded info bit into output char array
void FP_Decoder::loadInfoBit(unsigned char* output, int in_len_byte)
{
	int i, j; 
}

int FP_Decoder::hardDecision_general(const int *in)
{
	static int i, j, k, checksum, shift;
	int varaddr;
	int chkaddr;
	int deg;
	for(i = 0; i < CWD_LENGTH; i++)
	{
		DecodedCodeword[i] = in[i] > 0 ? 0:1;
	}
	for(i = 0; i < NUM_CGRP; i ++)
	{
		for(j = 0; j < CIR_SIZE; j++)
		{
			checksum = 0;
			chkaddr = j + i*CIR_SIZE;
			deg = cdeg[chkaddr];
			for(k = 0; k < deg; k++)
			{

				varaddr = clist[chkaddr][k];
				checksum ^= (DecodedCodeword[varaddr]);
			}
			if(checksum != 0)
			{
				return 1; // check sum does not pass
			}
		}
	}
	return 0;
}


int FP_Decoder::hardDecision(const int *in)
{
	static int i, j, k, checksum, shift;
	unsigned int varaddr;
	for(i = 0; i < CWD_LENGTH; i++)
	{
		DecodedCodeword[i] = in[i] > 0 ? 0:1;
	}
	for(i = 0; i < NUM_CGRP; i ++)
	{
		for(j = 0; j < CIR_SIZE; j++)
		{
			checksum = 0;
			for(k = 0; k < NUM_VGRP; k++)
			{
				shift = CodeROM.getCirShift(i, k);
				varaddr = (shift + j)% CIR_SIZE + k*NUM_VGRP;
				checksum ^= (DecodedCodeword[varaddr]);
			}
			if(checksum != 0)
			{
				//cout << endl;
				//cout << "check number " << i*CIR_SIZE + j << "not pass\n";
				//cout << "var addr is " << varaddr << " decode result: " << DecodedCodeword[varaddr] << endl;
				//cout << "hard dec in hardDecision is:\n";
				//for(int l = 0; l < CWD_LENGTH; l++)
				//	cout << DecodedCodeword[l];
				//cout << endl;
				return 1; // check sum does not pass
			}
		}
	}
	return 0;
}


//---- only for performance test, set the information bits to calculate BER
void FP_Decoder::setInfoBit(char* in, int in_len)
{
	int i, j;
	int char_size = sizeof(char)*8;
	int counter = 0;
	if(0)//all zeo debug
	{
		for(i = 0; i < INFO_LENGTH; i++)
			TrueInfoBit[i] = 0;
		return;
	}
	for(i = 0; i < in_len-1; i++)
	{
		for(j = 0; j < char_size; j++)// per byte
		{	
			TrueInfoBit[counter] = (in[i] >> j) & 1;
			counter ++;
		}
	}
	// unload the remaining bits from the last element 
	for(j = 0; j < INFO_LENGTH % char_size; j++)
	{
		TrueInfoBit[counter] = (in[in_len-1] >> j) & 1;
		counter ++;
	}
}

void FP_Decoder::setCodeword(int* in)
{
	int i;
	for(i = 0; i < CWD_LENGTH; i++)
	{
		TrueCodeword[i] = in[i];
	}
}


//---- only perform parity check on a vector
int FP_Decoder::check_fp(int *in)
{
	int i, j, k, shift;
	unsigned int varaddr, chkaddr;
	unsigned int checksum;
	for(i = 0; i < NUM_CGRP; i ++)
	{
		for(j = 0; j < CIR_SIZE; j++)
		{
			checksum = 0;
			chkaddr = i*CIR_SIZE + j;
			for(k = 0; k < NUM_VGRP; k++)
			{
				shift = CodeROM.getCirShift(i, k);
				varaddr = j + k*NUM_VGRP + shift;
				checksum ^= in[varaddr];		
			}
			if(checksum != 0)
				return 1; //check sum no pass
		}
	}
	return 0; //check sum pass
}

int FP_Decoder::check()
{
	int i, j, k, shift;
	unsigned int varaddr, chkaddr;
	unsigned int checksum;
	
	for(i = 0; i < NUM_CGRP; i ++)
	{
		for(j = 0; j < CIR_SIZE; j++)
		{
			checksum = 0;
			chkaddr = i*CIR_SIZE + j;
			for(k = 0; k < NUM_VGRP; k++)
			{
				// key problem: differences between shift back and shift forward: my program used 
				// shift forward, but standard array code use shift back
				// before changing decoder, try changing the generation of H and G
				shift = CodeROM.getCirShift(i, k);
				//if(j - shift < 0)
				//	dummy = j - shift + CIR_SIZE;
				//else
				//	dummy = j - shift;
				//dummy = (shift + j)% CIR_SIZE;
				varaddr = (shift + j)% CIR_SIZE  + k*NUM_VGRP;
				//if(varaddr != clist[chkaddr][k])
				//	cout << "mistmatch!" << endl;
				checksum ^= TrueCodeword[varaddr];		
			}
			if(checksum != 0)
				return 1; //check sum no pass
		}
	}
	
	return 0; //check sum pass
}

//---- use posterior to determined decoded codeword and perform check sum
int FP_Decoder::checkPost_fp()
{
	int i, j, k;
	//int temp[NUM_VAR];
	int shift;
	unsigned int varaddr;
	unsigned int checksum = 0, checksumtemp = 0;

	// full check
	for(i = 0; i < CWD_LENGTH; i++)
	{
		DecodedCodeword[i] = getPost_fp(i) > 0 ? 0:1;
	}

	for(i = 0; i < NUM_CGRP; i ++)
	{
		for(j = 0; j < CIR_SIZE; j++)
		{
			checksum = 0;
			checksumtemp = 0;
			for(k = 0; k < NUM_VGRP; k++)
			{
				shift = CodeROM.getCirShift(i, k);
				varaddr = (shift + j)% CIR_SIZE + k*NUM_VGRP;
				checksum ^= (DecodedCodeword[varaddr]);
				checksumtemp ^= (TrueCodeword[varaddr]);	
			}
			if(checksum != 0)
				return 1; // check sum does not pass
		}
	}
	return 0; // check sum pass
}


void FP_Decoder::ReadH(const char *filename, int vflag)
{
	int i, j;
	//-------------- H matrix check
	ifstream filestr(filename);
	if(vflag)
	{
		cout << "reading H file for genearl ldpc code from " << filename 
		<< endl;
	}
	//int dummy;
	int par_count = 0, info_count = 0;
	filestr >> vnum;
	filestr >> cnum;
	filestr >> vdeg_max;
	filestr >> cdeg_max;
	for(i = 0; i < vnum; i++)
	{
		filestr >> vdeg[i];
	}
	for(i = 0; i < cnum; i++)
	{
		filestr >> cdeg[i];
	}
	for(i = 0; i < vnum; i++)
	{
		for(j = 0; j < vdeg[i]; j++)
			filestr >> vlist[i][j];
	}
	for(i = 0; i < cnum; i++)
	{
		for(j = 0; j < cdeg[i]; j++)
			filestr >> clist[i][j];
	}
	filestr.close();
	if(vflag)
		cout << "finished reading H file for genearl ldpc code\n";
	//-------------- H matrix check end
}
//---- no mod
int FP_Decoder::sxor(int x, int y){		
	int v1, v2;
	int sum;
	int diff;
	int part1;
	int part2;
	int i;
	v1 = abs(x);
	v2 = abs(y);
	sum = (v1+v2) & WIDTH_MASK;
	diff = abs(v1-v2) & WIDTH_MASK;
	part1 = Constant - (sum >> 2); //divided by 4
	part1 = (part1 > 0) ? part1:0;
	part2 = Constant - (diff >> 2); 
	part2 = (part2 > 0) ? part2:0;
	//cout << sgn(x)*sgn(y)*(min(v1, v2) + part1 - part2) << endl;
	return sgn(x)*sgn(y)*(min(v1, v2) + part1 - part2);
}


// specify the info index
void FP_Decoder::setInfoIndex(int* in)
{
	int i = 0;
	for(i = 0; i < INFO_LENGTH; i ++)
	{
		InfoIndex[i] = in[i];
	}
}

// specify the info index from file
void FP_Decoder::setInfoIndex(const char* in, int vflag = 0)
{
	std::ifstream File(in);
	//int i = 0;
	if(vflag)
		cout << "reading index set from file " << in << endl;
	if(!File)
	{	
		cout <<"fail reading index file\n";
		std::cerr<<"fail to open info index file";
		exit(1);
	}
	for(int i = 0; i < INFO_LENGTH; i ++)
	{
		File >> InfoIndex[i];
		//cout << InfoIndex[i] << " ";
	}
	//cout << endl;
	if(vflag)
		cout << "info index set from file " << in << endl;
	
}

int FP_Decoder::calculateBER()
{
	int i = 0; 
	//cout << "bit error position: ";
	for(i = 0; i < INFO_LENGTH; i++)
	{
		if(DecodedCodeword[InfoIndex[i]] != TrueInfoBit[i])
		{
			BitError ++;
			//cout << i << " "; 
		}
		//cout << endl;
	}
	return BitError;
	//cout << endl;
}

double FP_Decoder::sxor(double x, double y){		
	double v1, v2;
	double sum_abs, diff_abs;
	v1 = fabs(x);
	v2 = fabs(y);
	sum_abs = v1+v2;
	diff_abs = fabs(v1-v2);
	return sgn(x)*sgn(y)*(min(v1, v2) + log(1 + exp(-sum_abs)) - log(1 + exp(-diff_abs)) );
}


/// error version...
int FP_Decoder::decode(const double *LLR)
{
	//int Counter; 
	int i, j, k;
	//int VarShift[NUM_VGRP];
	int VarAddr;
	int ChkAddr;
	int Shift;
	int Iteration = 0;
	double MV2C[CHK_DEG];
	double MC2V[VAR_DEG];
	double Forward[CHK_DEG];
	double Backward[CHK_DEG];
	
	//Initialize the Edge RAM with channel values from variable node 
	double temp;

	// for debugging
	double Received[NUM_VAR];
	for(i = 0; i < NUM_VAR; i++)
	{
		Received[i] = LLR[i];
	}
	for(i = 0; i < NUM_VGRP; i++)
	{
		for(j = 0; j < CIR_SIZE; j++)
		{
			VarAddr = i*CIR_SIZE + j;
			for(k = 0; k < VAR_DEG; k++)
			{
				Shift = CodeROM.getCirShift(k, i);
				ChkAddr = (j + Shift) % CIR_SIZE;
				EdgeRAM[k].setAddress(ChkAddr + i*CIR_SIZE);	
				EdgeRAM[k].wrtData(LLR[VarAddr]);
				//temp = EdgeRAM[k].getData();
			}
		}
	}
	while(Iteration < MAX_ITER)
	{
		//-- Process each check group at a time. 
		for(i = 0; i < NUM_CGRP; i++)
		{
			//-- Check node parallel process denoted in for loop (loop through all check nodes in one group)
			for(j = 0; j < CIR_SIZE; j++)
			{
				
				for(k = 0; k < CHK_DEG; k++)
				{
					//This is simply for better read of the code. Not necessary.
					//Shift = CodeROM.getCirShift(i, k);
					//-- Prepare all the V2C message
					//VarAddr = k*47 + Shift;
					VarAddr = j + k*CHK_DEG;
					EdgeRAM[i].setAddress(VarAddr); 
					MV2C[k] = EdgeRAM[i].getData();	
				}
				//-- Do binary operation of the sxor: forward backward computation
				Forward[0] = MV2C[0];
				Backward[CHK_DEG-1] = MV2C[CHK_DEG-1];
				for(k = 1; k < CHK_DEG-1; k++)
				{
					Forward[k] = sxor(Forward[k-1], MV2C[k]);
					Backward[CHK_DEG - k - 1] = sxor(Backward[CHK_DEG - k], MV2C[CHK_DEG - 1 - k]);
				}
				//-- make MV2C temp memory to store all the computed MC2V message from one check node
				//-- Compute the first and last MC2V and write back to RAM
				k = 0; // 1st
				MV2C[k] = Backward[k+1];
				VarAddr = j + k*CHK_DEG;
				EdgeRAM[i].setAddress(VarAddr); 
				EdgeRAM[i].wrtData(MV2C[k]);

				//MV2C[CHK_DEG-1] = Forward[CHK_DEG-1];
				k = CHK_DEG-1; // last
				MV2C[k] = Forward[k-1];
				VarAddr = j + k*CHK_DEG;
				EdgeRAM[i].setAddress(VarAddr); 
				EdgeRAM[i].wrtData(MV2C[k]);
				
				//-- Compute the MV2C message and write back to the RAM
				for(k = 1; k < CHK_DEG-1; k++)
				{
					MV2C[k] = sxor(Forward[k-1], Backward[k+1]);
					VarAddr = j + k*CHK_DEG;
					EdgeRAM[i].setAddress(VarAddr); 
					EdgeRAM[i].wrtData(MV2C[k]);
					if(MV2C[k] > 8)
						cout << MV2C[k];
				}
				
			}
		}
		//-- Check node group process complete

		//-- Prepare MC2V and compute the posteriori and new MV2C
		for(i = 0; i < NUM_VGRP; i++)
		{
			for(j = 0; j < CIR_SIZE; j++)
			{
				VarAddr = i*CIR_SIZE + j;
				temp = 0;
				// Accumulate all the MC2V message
				for(k = 0; k < VAR_DEG; k++)
				{
					//-- old code
					//Shift = CodeROM.getCirShift(k, i);
					//ChkAddr = (j + Shift) % CIR_SIZE;
					//EdgeRAM[k].setAddress(ChkAddr + i*CIR_SIZE);	
					//MC2V[k] = EdgeRAM[k].getData();
					//temp = temp + MC2V[k];
					//-- new code
					EdgeRAM[j].setAddress(i + k*CIR_SIZE);	
					MC2V[k] = EdgeRAM[j].getData();
					temp = temp + MC2V[k];
				}
				// Add in the channel value
				temp = temp + LLR[VarAddr];
				wrtPost(VarAddr, temp);
				for(k = 0; k < VAR_DEG; k++)
				{
					Shift = CodeROM.getCirShift(k, i);
					ChkAddr = (j + Shift) % CIR_SIZE;
					EdgeRAM[k].setAddress(ChkAddr + i*CIR_SIZE);	
					EdgeRAM[k].wrtData(temp - MC2V[k]);
					//temp = EdgeRAM[k].getData();
				}
			}
		}
		Iteration++;
		// If there is no error then break (cannot detect codeword error)
		// early termination part
		//if(!checkPost())
		//{
		//	return 0;
		//}
	}
	//checkPost();
	return BitError;
}

int FP_Decoder::decode(const int *LLR, char* out)
{
	int i, j;
	// !!!! Assuming out is all zero?
	decode_fixpoint(LLR);
	for(i = 0; i < 276; i++)
	{
		
		// NOTE: this should solve the problem where out might not be null
		// since DecodedCodeword is either 0 or 1, erasing all other bits
		j = 0;
		//out[i] = DecodedCodeword[i*8 + j];
		for(j = 1; j < 8; j++)
		{
			;//out[i] = out[i] + (DecodedCodeword[i*8 + j] << j);
		}
	}
	i = 277;
	j = 0;
	//out[i] = DecodedCodeword[i*8 + j];
	for(j = 1; j < 2209 % 8; j++)
	{
		;//out[i] = out[i] + (DecodedCodeword[i*8 + j] << j);
	}
}

//-------------- Decoder part starts
int FP_Decoder::decode_fixpoint(const int *LLR)
{
	int Iteration; 
	int i, j, k;
	//---- 
	int var_add, chk_add, cir_shift;
	int temp;
	//---- 
	static int VarAddr;
	static int ChkAddr;
	static int Shift;
	static int Accum[CIR_SIZE];
	static int BankSelect;
	static int Reg_v2c_pre[CIR_SIZE][CHK_DEG];
	static int Reg_c2v[CIR_SIZE][CHK_DEG];
	static int Reg_c2v_pre[CIR_SIZE][VAR_DEG]; // actually don't need pre?
	//static int Reg_v2c[CHK_DEG];
	static int Forward[CIR_SIZE][CHK_DEG];
	static int Backward[CIR_SIZE][CHK_DEG];


	if(!hardDecision(LLR)) 
	{
		//resetBER();
		//calculateBER();
		if(DEBUG)
			cout << "check sum pass before decoding (returning)" << endl;	
		//return BitError;
		
		return 0;
	}
	//else
	//{
		//resetBER();
		//calculateBER();
		//cout << BitError << " bit errors before decoding start" << endl;
	//}
	
	/*----
	The BRCM is check oriented. The memory address specity which check it is, 
	E.g. EdgeRAM[i].setAddress(j) specity the ith edge of the jth check node. 
	-----*/
	if(FSM.getState() == PCV) // prepare channel value state is on
	{
		if(DEBUG)
		{
			cout << "in pcv process\n";
		}
		// this is a totally rewritted code
		// writing channel value to each edge that connects with the each vnode
		for(i = 0; i < NUM_CGRP; i++) // go through all groups of cnode
		{
			for(k = 0; k < CIR_SIZE; k++) // for each cnode in each cgroup
			{
				ChkAddr = k + i*CIR_SIZE;
				//-- Parallel Process: writing one elem to each bank
				for(j = 0; j < CHK_DEG; j++)	
				{
					
					Shift = CodeROM.getCirShift(i, j);
					VarAddr = (k + Shift) % CIR_SIZE + j*CIR_SIZE;// if bug verify this address
					BankSelect = j; //simply which vgroup we are at
					EdgeRAM[BankSelect].setAddress(ChkAddr);	
					EdgeRAM[BankSelect].wrtData(LLR[VarAddr]);
					if(DEBUG)
					{
						;
						//printf("(bank, chkaddr, value) = (%d, %d, %d)\n", BankSelect, ChkAddr, EdgeRAM[BankSelect].rdData());
					}

				}
			}
		}
		FSM.setState(C2V);	// go to check to variable state
	}

	Iteration = 0;
	while(Iteration < MAX_ITER && FSM.getState() == C2V)
	{
		//-- Process one check group at a time. 
		for(i = 0; i < NUM_CGRP; i++)
		{
			//-- CIR_SIZE banks of RAM and serially retrieve CHK_DEG messages from the memory
			for(j = 0; j < CIR_SIZE; j++)
			{
				ChkAddr = j + i*CIR_SIZE;
				//-- Check node parallel process denoted in for loop 
				//-- (loop through all check nodes in one group)
				for(k = 0; k < CHK_DEG; k++)		
				{
					//-- Prepare all the V2C message
					
					//VarAddr = (k + Shift) % CIR_SIZE + j*CIR_SIZE;
					BankSelect = k;
					EdgeRAM[BankSelect].setAddress(ChkAddr); 
					Reg_v2c_pre[j][k] = EdgeRAM[BankSelect].rdData();	
				}
			}
			//-- Do binary operation of the sxor: forward backward computation
			for(j = 0; j < CIR_SIZE; j++)	//-- Parallel process
			{
				Forward[j][0] = Reg_v2c_pre[j][0];
				Backward[j][CHK_DEG-1] = Reg_v2c_pre[j][CHK_DEG-1];
			}
			//-- keeping this part as it is...
			for(k = 1; k < CHK_DEG-1; k++) //-- Serial process?
			{
				for(j = 0; j < CIR_SIZE; j++)	//-- Parallel process?
				{
					Forward[j][k] = sxor(Forward[j][k-1], Reg_v2c_pre[j][k]);
					Backward[j][CHK_DEG - k - 1] 
					= sxor(Backward[j][CHK_DEG - k], Reg_v2c_pre[j][CHK_DEG - 1 - k]);
				}
			}
			//-- Compute MC2V and accumulate the posteriori:
			//-- NOTE: can ignore the buffer Reg_c2v in software
			//-- 1. Compute the first and last MC2V and write back to RAM
			for(j = 0; j < CIR_SIZE; j++)	//-- Parallel process
			{
				k = 0; // 1st
				Reg_c2v[j][k] = Backward[j][k+1];
				ChkAddr = j + i*CIR_SIZE;
				BankSelect = k;
				EdgeRAM[BankSelect].setAddress(ChkAddr); 
				EdgeRAM[BankSelect].wrtData(Reg_c2v[j][k]);

				//Reg_c2v[CHK_DEG-1] = Forward[CHK_DEG-1];
				k = CHK_DEG-1; // last
				Reg_c2v[j][k] = Forward[j][k-1];
				ChkAddr = j + i*CIR_SIZE;
				BankSelect = k;
				EdgeRAM[BankSelect].setAddress(ChkAddr); 
				EdgeRAM[BankSelect].wrtData(Reg_c2v[j][k]);
			}
			//-- 2. Compute rest of the MC2V message and write back to the RAM
			for(k = 1; k < CHK_DEG-1; k++)
			{							
				for(j = 0; j < CIR_SIZE; j++) //-- Parallel process
				{
					Reg_c2v[j][k] = sxor(Forward[j][k-1], Backward[j][k+1]);
					//VarAddr = k + i*CHK_DEG;
					//EdgeRAM[j].setAddress(VarAddr); 
					//EdgeRAM[j].wrtData(Reg_c2v[j][k]);
					ChkAddr = j + i*CIR_SIZE;
					BankSelect = k;
					EdgeRAM[BankSelect].setAddress(ChkAddr); 
					EdgeRAM[BankSelect].wrtData(Reg_c2v[j][k]);
				}
			}
		}


		FSM.setState(V2C);
		//-- Check node group process complete

		//-- Prepare MC2V and compute the posteriori and new MV2C
		//if(FSM.getState() == V2C)	
		for(i = 0; i < NUM_VGRP; i++)
		{
			for(j = 0; j < CIR_SIZE; j++)//-- Parallel process
			{
				Accum[j] = 0;
			}
			for(k = 0; k < VAR_DEG; k++)	//-- Serially accumulate
			{
				// Accumulate all the MC2V message
				for(j = 0; j < CIR_SIZE; j++)	//-- Parallel process
				{
					VarAddr = i*CIR_SIZE + j;
					Shift = CodeROM.getCirShift(k, i);
					//Select which bank of RAM is active
					BankSelect = i;  // vgroup index 	
					//  need to deduce ChkAddr here: the right shift is upshift => minus
					temp = j - Shift;
					if(temp < 0)
						temp += CIR_SIZE;
					//The Address within the selected bank of RAM
					ChkAddr = temp + k*CIR_SIZE;	
					EdgeRAM[BankSelect].setAddress(ChkAddr);	
					Reg_c2v_pre[j][k] = EdgeRAM[BankSelect].rdData();
					Accum[j] = Accum[j] + Reg_c2v_pre[j][k];
				}
			}
			for(j = 0; j < CIR_SIZE; j++)	//-- Parallel process
			{
				VarAddr = i*CIR_SIZE + j;
				if(DEBUG)
				{
					;
					//printf("(addr, accum, llr) = (%d, %d, %d)\n", VarAddr, Accum[j], LLR[j]);
				}
				// Add in the channel value (fixed point)
				Accum[j] = Accum[j] + LLR[VarAddr];
				// Update the Posteriori
				wrtPost(VarAddr, Accum[j]);
				
			}
				
			for(k = 0; k < VAR_DEG; k++) //-- Write back to RAM the MV2C (serial)
			{
				for(j = 0; j < CIR_SIZE; j++) //-- Parallel process
				{
					Shift = CodeROM.getCirShift(k, i);
					BankSelect = i; // the vgroup index is the BankSelect
					//  need to deduce ChkAddr here
					temp = j - Shift;
					if(temp < 0)
						temp += CIR_SIZE;
					//The Address within the selected bank of RAM
					ChkAddr = temp + k*CIR_SIZE;	
					EdgeRAM[BankSelect].setAddress(ChkAddr);	
					EdgeRAM[BankSelect].wrtData(Accum[j] - Reg_c2v_pre[j][k]);
				}
			}
		}
		Iteration++;
		// If there is no error then break (cannot detect codeword error)
		if(!checkPost_fp())
		{
			if(DEBUG)
			{
				cout << "check sum of decoder output pass at iteration "
			  << Iteration << endl;
			}
			//cout << "iter=" << Iteration << endl;
			FSM.setState(IDLE);
		}
		else
		{
			FSM.setState(C2V);
			//-- for debugging
			if(DEBUG)
			{
				;
				//resetBER();
				//calculateBER();
				//cout << "finished iteration " << Iteration << endl;
				//cout << "posteriors are: \n";
				/*
				for(i = 0; i < CWD_LENGTH;i++)
				{
					cout << getPost_fp(i) << " ";
				}
				cout << endl;
				*/
			}
		}
	}// While Iteration loop end
	//checkPost_fp();
	// return total number of iterations
	if(DEBUG)
	{
		;
		//cout << "check sum not pass after iterations: " << Iteration << endl;
		/*
		for(i = 0; i < CWD_LENGTH;i++)
		{
			cout << getPost_fp(i) << " ";
		}
		cout << endl;
		*/
	}
	//cout << "iter=" << Iteration << endl;
	return Iteration;
}
/* !!!!!!!!!
need to modify to really check the parity check matrix!!!
not urgent yet
 
int FP_Decoder::checkPost()
{
	int i = 0;
	BitError = 0;
	double temp = 0;
	for(i = 0; i < CWD_LENGTH; i++)
	{
		//temp = getPost(i);
		if(getPost(i) < 0)
			BitError++;
	}
	if(BitError != 0)
		return 1;
	else
		return 0;
}
*/
