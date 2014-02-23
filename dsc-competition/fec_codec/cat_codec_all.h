#ifndef FEC_CODEC_H
#define FEC_CODEC_H
// seems like extern is not really necessary for these C functions. 
// 
//rs codec 
extern int cat_codelength(int nbytes);
extern int cat_nbytes(int codelength);
extern int cat_encode(unsigned char* block, int nbytes);
extern int cat_decode(unsigned char* block, int nbytes);

// cc codec
extern int cc3_codelength(int nbytes, int incclen);
extern int cc3_nbytes(int codelength, int outcclen);
extern void cc3_encode(unsigned char* block, int nbytes, int incclen);
extern void cc3_decode(unsigned char* block, int codelength, int outcclen);
//extern int 

#endif
