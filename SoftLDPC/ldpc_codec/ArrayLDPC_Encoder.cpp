#include <iostream>
#include <fstream>
#include <bitset>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "rngs.h"
#include "rvgs.h"
//#include "MemoryMacro.h"
//#include "Memory.h"
//#include "ArrayLDPC.h"
#include "ArrayLDPCMacro.h"
#include "wifi_encoder.h"
using namespace std;
//--------- notes -----------
// 1. don't really need the posterior stored. just the hard decision would be enough
// 2. adding a function that will instantiate a class and return the pointer
// 3. use perfermance test's functions to pass in the pointer
//---------------------------

FP_Encoder::FP_Encoder(const char *in, int vflag)
{
	if(vflag)
		cout << "in enc constructor, loading encoder file\n"; 
	loadfile(in, vflag);//cout << "constructor of FP_Encoder \n";
}


//-------------- Encoder part starts
FP_Encoder::~FP_Encoder()
{
	;
	//int i;
	//delete [] ParityIndex;
	//delete [] InfoIndex;
	//for(i = 0; i < Gdim_col; i++ )
	//	delete [] GeneratorMat[i];
	//
	//delete [] InfoBuffer;
	//delete [] GeneratorMat;
}
void FP_Encoder::loadfile(const char* Filename, int flag)
{
	//flag == 1 : verbose, flag == 0 supress message
	if(flag) 
		cout << "reading file " << Filename << endl;
	try{
		//ifstream FileStr("G_array_zindx.txt");
		ifstream FileStr(Filename);
		FileStr.exceptions ( ifstream::failbit | ifstream::badbit );
		if(1)// alist file format, current test mode
		{
			int vnum, cnum, vdeg_max, cdeg_max;
			int par_count = 0, info_count = 0;
			FileStr >> vnum;
			
			FileStr >> cnum;
			Gdim_col = cnum;
			Gdim_row = vnum - cnum;
			FileStr >> vdeg_max;
			FileStr >> cdeg_max;
			int i, j;
			for(i = 0; i < vnum; i++)
			{
				 FileStr >> ColumnFlag[i];
				 if(ColumnFlag[i] == 1)
				 {
					 ParityIndex[par_count] = i;
					 par_count++;
				 }
				 else
				 {
					 InfoIndex[info_count] = i;
					 info_count++;
				 }
			}
			for(i = 0; i < cnum;i++)
			{
				 FileStr >> ChkDeg[i];
			}
			//----- if var deg information is include need dummy input
			//for(i = 0; i < vnum; i++)
			//{
			//	for(j = 0; j < VarDeg[i]; j++)
			//		FileStr >> dummy;
			//}
			for(i = 0; i < cnum; i++)
			{
				for(j = 0; j < ChkDeg[i]; j++)
					FileStr >> G_mlist[i][j];
			}
		}
		else // binary file format (the generator matrix in binary)
		{
			//int par_num, info_num;
			int i, j;
			FileStr >> Gdim_row; // row dimension
			//cout << Gdim_row << endl;
			//info_num; //
			FileStr >> Gdim_col;
			for(i = 0; i < Gdim_row; i++)
				FileStr >> InfoIndex[i];
			for(i = 0; i < Gdim_col; i++)
				FileStr >> ParityIndex[i];
			for(i = 0; i < Gdim_row; i++)
			{
				for(j = 0; j < Gdim_col; j++)
				{
					// this is the transposed version, i.e., the true parity part of a systematic generator matrix
					//FileStr >> GeneratorMat[i][j];
					;
				}
			}
		}
		FileStr.close();
	}

	catch (ifstream::failure e) {
		cout << "Exception opening/reading file " << e.what() << endl;
		//system("pause");
		exit(0);
	}
	catch (std::bad_alloc& ba)
	{
		std::cerr << "bad_alloc caught: " << ba.what() << '\n';
		exit(0);
	}
	if(flag)
		cout << "encoder initialized" << endl;
}


// NOTE!!! MUST INITIALIZE OUT TO BE ALL ZERO!!!!
void FP_Encoder::loadCodeword_intr(unsigned char* out, int bit_pos)
{
	int i, j;
	int char_size = sizeof(char)*8;
	// bit_pos goes from 0 to 7
	//cout << "bit pos: " << bit_pos << endl;
	for(i = 0; i < CWD_LENGTH; i++)
	{
		out[i] = out[i] + (Codeword[i] << bit_pos);	
		//cout << out[i];
		//cout << std::bitset<8>(out[i]);
	}
	//cout << endl;
}
//--- easy version
int FP_Encoder::encode(unsigned int *in, int in_len) //in_len in bit=>encoding 8*info_length bits
{
	int i, j, k, counter = 0;
	int num_blks;
	int char_size = sizeof(char)*8;
	unsigned int temp;

	//cout << "in int input encoder\n";
	//num_blks = in_len*char_size/INFO_LENGTH + (in_len*char_size % INFO_LENGTH > 0? 1:0);
	if(in_len != Gdim_row)
	{
		cout << "in_len in encode does not match the G_dim_row\n";
		cout << "in_len is " << in_len << " and Gdim_row is " << Gdim_row << endl;
	}
	for(i = 0; i < Gdim_row; i++)
	{
		Codeword[InfoIndex[i]] = in[i];
		//cout << Codeword[InfoIndex[i]];
	}
	//cout << "parities " << endl;

	for(i = 0; i < Gdim_col; i++)
	{
		temp = 0;
		for(j = 0; j < ChkDeg[i]; j++)
		{
			if(ColumnFlag[G_mlist[i][j]] == 0)
			{
				temp ^= Codeword[G_mlist[i][j]];
				//cout << G_mlist[i][j] << " ";
			}
		}
		Codeword[ParityIndex[i]] = temp;
		//cout << Codeword[ParityIndex[i]];
	}
	//cout << endl;
	return 0;
}

int FP_Encoder::encode(char *in, int in_len)
{
	int i, j, k, counter = 0;
	int out_len = 2209;  // hard coded for now
	int num_blks;
	int char_size = sizeof(char)*8;
	unsigned int temp;


	num_blks = in_len*char_size/INFO_LENGTH + (in_len*char_size % INFO_LENGTH > 0? 1:0);
	

	//for(i = 0; i<num_blks; i++)
	//{
		// unload the information bits from the char array till the 
		// second last element
	for(i = 0; i < in_len-1; i++)
	{
		for(j = 0; j < char_size; j++)// per byte
		{	
			InfoBuffer[counter] = (in[i] >> j) & 1;
			counter ++;
		}
	}
	// unload the remaining bits from the last element 
	for(j = 0; j < Gdim_row % char_size; j++)
	{
		InfoBuffer[counter] = (in[in_len-1] >> j) & 1;
		counter ++;
	}

	// NOTE for improvement: can ignore InfoBuffer and just use systematic codewords
	for(i = 0; i < Gdim_row; i++)
	{
		Codeword[InfoIndex[i]] = InfoBuffer[i];
	}

	for(i = 0; i < Gdim_col; i++)
	{
		temp = 0;
		for(j = 0; j < ChkDeg[i]; j++)
		{
			if(ColumnFlag[G_mlist[i][j]] == 0)
			{
				temp ^= Codeword[G_mlist[i][j]];
				//cout << G_mlist[i][j] << " ";
			}
		}
		Codeword[ParityIndex[i]] = temp;
	}
	return out_len;
}


// an overloaded function that outputs simple integer array: working!
int FP_Encoder::encode(unsigned char *in, int in_len) //in_len in byte
{
	int i, j, k, counter = 0;
	int out_len = 2209;  // hard coded for now
	int num_blks;
	int char_size = sizeof(char)*8;
	unsigned int temp;

	// ideally in_len is matching Gdim_col/8
	// if not matching just pad zero
	// output codeword bitwise stored in output
	// calculate the number of codeword blocks to output
	num_blks = in_len*char_size/INFO_LENGTH + (in_len*char_size % INFO_LENGTH > 0? 1:0);
	
	// already allocated in the top block. unsafe to allocate the memory in the function
	//out = new int[out_len*num_blks]; 

	//for(i = 0; i<num_blks; i++)
	//{
		// unload the information bits from the char array till the 
		// second last element
	
	/*if(in_len != Gdim_row)
	{
		cout << "in_len in encode does not match the G_dim_row\n";
		cout << "in_len is " << in_len << " and Gdim_row is " << Gdim_row << endl;
	}*/
	for(i = 0; i < in_len-1; i++)
	{
		for(j = 0; j < char_size; j++)// per byte
		{	
			InfoBuffer[counter] = (in[i] >> j) & 1;
			counter ++;
		}
	}
	// unload the remaining bits from the last element 
	for(j = 0; j < Gdim_row % char_size; j++)
	{
		InfoBuffer[counter] = (in[in_len-1] >> j) & 1;
		counter ++;
	}
	//}
	// NOTE for improvement: can ignore InfoBuffer and just use systematic codewords
	for(i = 0; i < Gdim_row; i++)
	{
		Codeword[InfoIndex[i]] = InfoBuffer[i];
	}
	//c = uG : each column is a parity bit.
	/*for(i = 0; i < Gdim_col; i++)
	{
		temp = 0;
		for(j = 0; j < Gdim_row; j++)
		{
			temp ^= (GeneratorMat[j][i] & InfoBuffer[j]);
		}
		Codeword[ParityIndex[i]] = temp;
	}*/
	for(i = 0; i < Gdim_col; i++)
	{
		temp = 0;
		for(j = 0; j < ChkDeg[i]; j++)
		{
			if(ColumnFlag[G_mlist[i][j]] == 0)
			{
				temp ^= Codeword[G_mlist[i][j]];
				//cout << G_mlist[i][j] << " ";
			}
		}
		Codeword[ParityIndex[i]] = temp;
	}
	//for(i = 0; i < CWD_LENGTH; i++)
	//	cout << Codeword[i];
	//cin >> out_len;
	return out_len;
}

void FP_Encoder::loadCodeword(unsigned char* out, int out_len_byte)// out_len in byte
{
	int i, j;
	int char_size = sizeof(char)*8;
	//out_len gives the number of bits to output
	// out_len_byte - 1 because the last few bits of the last elemetn 
	// is not the codeword
	for(i = 0; i < out_len_byte-1; i++)
	{
		out[i] = 0;
		for(j = 0; j < char_size; j ++)
			out[i] = (out[i] << j) | Codeword[i*8+j]; 
	}
	// shift out the rest of the bits
	for(j = 0; j < CWD_LENGTH % char_size; j ++)
		out[i] = (out[i] << j) | Codeword[i*8+j]; 
}



//--------------------
// not done yet
int FP_Encoder::encode(unsigned char *in, unsigned char *out, int in_len)
{
	int i, j, k, counter = 0;
	int num_blks;
	int out_len;
	int char_size = sizeof(char)*8;
	unsigned int temp;

	// ideally in_len is matching Gdim_col/8
	// if not matching just pad zero
	// output codeword bitwise stored in output
	// calculate the number of codeword blocks to output
	num_blks = in_len*char_size/INFO_LENGTH + (in_len*char_size % INFO_LENGTH > 0? 1:0);

	for(i = 0; i < in_len-1; i++)
	{
		for(j = 0; j < char_size; j++)// per byte
		{	
			InfoBuffer[counter] = (in[i] >> j) & 1;
			counter ++;
		}
	}
	// unload the remaining bits from the last element 
	for(j = 0; j < Gdim_row % char_size; j++)
	{
		InfoBuffer[counter] = (in[in_len-1] >> j) & 1;
		counter ++;
	}
	//}
	// NOTE for improvement: can ignore InfoBuffer and just use systematic codewords
	for(i = 0; i < Gdim_row; i++)
	{
		Codeword[InfoIndex[i]] = InfoBuffer[i];
	}
	//c = uG : each column is a parity bit.
	/*for(i = 0; i < Gdim_col; i++)
	{
		temp = 0;
		for(j = 0; j < Gdim_row; j++)
		{
			temp ^= (GeneratorMat[j][i] & InfoBuffer[j]);
		}
		Codeword[ParityIndex[i]] = temp;
	}*/
	for(i = 0; i < Gdim_col; i++)
	{
		temp = 0;
		for(j = 0; j < ChkDeg[i]; j++)
		{
			if(ColumnFlag[G_mlist[i][j]] == 0)
			{
				temp ^= Codeword[G_mlist[i][j]];
				//cout << G_mlist[i][j] << " ";
			}
		}
		Codeword[ParityIndex[i]] = temp;
	}
	out_len = num_blks*NUM_VAR/8 + (num_blks*NUM_VAR % 8 > 0? 1:0);
	//out = new char[out_len];
	// stuff the codeword into an array of char
	cout << out;
	counter = 0;
	for(i = 0; i < 276; i++)
	{
		//cout << "out[i] = " << std::bitset<8>(out[i]) << endl;
		// increase the index by one every 8 bits (one byte)
		for(j = 0 ;j < 8; j++)
		{
			//cout << 0xff;
			out[i] = out[i] ^ (Codeword[i*8 + j] << j);
			//cout << "codeword " << j << "=" << Codeword[j] << endl;
			//cout << std::bitset<8>(out[i]) << endl;
		// shift the result to the left one (stuffing in)
			//out[i] = (out[i] & 0xff) >> 1;
			//cout << std::bitset<8>(out[i]) << endl;
		}
		cout << std::bitset<8>(out[i]) << endl;
		for(j = 7; j >= 0; j--)
			cout << Codeword[i*8 + j];
		cout << endl;
	}
	for(j = 0 ;j < 2209 % 8; j++)
	{
		out[i] = out[i] ^ (Codeword[i*8 + j] << j);
	}
	//for(i)
	//for(i = 0; i < 277; i++)
	//{
	//		cout << std::bitset<8>(out[i]) << ", ";
	//		for(j = 0; j < 8; j++)
	//			cout << Codeword[i*8 + j];
	//		cout << endl;
	//}
	cout <<endl;
	// returning the padding length (or should I return the codeword length?)
	return out_len/num_blks;
}

