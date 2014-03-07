#ifndef DECODER_H
#define DECODER_H
#include <iostream>


#include "ArrayLDPCMacro.h"

//--------------------- Decoder class
class FP_Decoder
{
public:
	FP_Decoder(const char*, int);
	//FP_Decoder();
	//new public functions after general fixed point decoding
	void ReadH(const char*, int);
	int checkPost_fp_general();
	int decode_general_fp(const int*);
	//-----
	int decode_fixpoint(const int *LLR);
	int decode(const double *LLR);
	int decode(const int *LLR, char* out);
	int sgn(double);
	int sgn(int);
	int sxor(int, int);
	int fmin(int, int);
	int fmax(int, int);
	int check();
	int check_fp(int*); // check whether the input vector pass the parity check matrix
	int checkPost();
	int checkPost_fp();
	int getPost_fp(int Addr){ return Posteriori_fp[Addr]; };
	int getState(){ return FSM.getState();  };
	void setState(int in){ FSM.setState(in);  };
	void setInfoBit(char*, int); // set info bit for a given arrary
	
	void loadInfoBit(unsigned char*, int);
	void loadInfoBit_intr(unsigned char* out, int bit_pos);
	void setInfoIndex(const char*, int); // set info index by loading from a file
	void setInfoIndex(int *);// set info index for a given arrary
	void setCodeword(int *);
	//---- temp inline functions
	int getInfoIndex(int in){ return InfoIndex[in];}
	//----
	int hardDecision(const int *);  // perform hardecision o input and verfiy check sum
	int hardDecision_general(const int *in);
	int calculateBER();
	void resetBER(){ BitError = 0;};

	double fmin(double, double);
	double getPost(int Addr){ std::cout << "using float getpost\n"; return Posteriori[Addr]; };
	double getRate(){ return CodeROM.getRate(); };
	double sxor(double, double);
	void wrtPost(int Addr, int in){ Posteriori_fp[Addr] = in;};
	void wrtPost(int Addr, double in){ std::cout << "using float post\n"; Posteriori[Addr] = in;};

	
private:
	class ROM CodeROM;
	class ControlFSM FSM;
	class Memory EdgeRAM[CIR_SIZE];
	int DecodedCodeword[CWD_LENGTH];
	int TrueCodeword[CWD_LENGTH];
	int TrueInfoBit[INFO_LENGTH];
	int InfoIndex[INFO_LENGTH];
	int Posteriori_fp[CWD_LENGTH];
	double Posteriori[CWD_LENGTH];
	int BitError;
	// shifting a fixed point fractional numbers to a binary representation
	static const int Constant = int((5.0/8.0)*(1 << FRAC_WIDTH));
	
	//---- for general code
	int vnum, cnum, vdeg_max, cdeg_max;
	int vdeg[CWD_LENGTH], cdeg[INFO_LENGTH], vlist[CWD_LENGTH][VAR_DEG], 
		 clist[INFO_LENGTH][CHK_DEG];
};

inline int FP_Decoder::sgn(double x){
	return (x > 0)?1:-1;
}

inline int FP_Decoder::sgn(int x){
	return (x > 0)?1:-1;
}

inline double FP_Decoder::fmin(double x, double y){		
	if(x <= y)
		return x;
	else
		return y;
}
inline int FP_Decoder::fmin(int x, int y){		
	if(x <= y)
		return x;
	else
		return y;
}

inline int FP_Decoder::fmax(int x, int y){		
	if(x >= y)
		return x;
	else
		return y;
}

#endif
