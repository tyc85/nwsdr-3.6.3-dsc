#ifndef ENCODER_H
#define ENCODER_H

//--------------------- Encoder class
class FP_Encoder
{
public:
	//FP_Encoder();
	FP_Encoder(const char*, int);
	~FP_Encoder();
	
	//--- should change to unsigned char
	int encode(unsigned char *, unsigned char *, int);
	int encode(unsigned char *, int);
	int encode(char *, int);
	int encode(unsigned int *, int);
	//void check_codeword();
	void loadfile(const char *m, int);
	int getCodeword(int addr){ return Codeword[addr];};
	int getInfoIndex(int addr){return InfoIndex[addr];};
	void loadCodeword(unsigned char*, int); // second param is # of bits
	void loadCodeword_intr(unsigned char*, int);
private:
	int Codeword[NUM_VAR];
	
	//------------- for array ldpc code
	/*
	//231 is the row dimension (reduced from 235). improvement: allocate dynamically
	int ChkDeg[231]; 
	int VarDeg[NUM_VAR];
	//1078 is the max check deg. improvement: allocate dynamically
	int G_mlist[231][1078];
	*/
	//------------- for general ldpc code
	int ChkDeg[972]; 
	//540 is the max check deg. improvement: allocate dynamically
	int G_mlist[972][540];
	//------------- end
	int Gdim_row;
	int Gdim_col;
	
	int check_fp(int*); // check whether the input vector pass the parity check matrix
	class ROM CodeROM;
	// hard code test
	unsigned int ColumnFlag[NUM_VAR];
	unsigned int InfoBuffer[INFO_LENGTH];
	unsigned int ParityIndex[NUM_VAR - INFO_LENGTH];
	unsigned int InfoIndex[INFO_LENGTH];
	
	

	//---- need some verification
	//unsigned int *InfoBuffer;
	//unsigned int *ParityIndex;
	//unsigned int *InfoIndex;
	//unsigned int **GeneratorMat;
};

#endif
