%module cat_cccodec6

%{
extern void cc6_encode(unsigned char* block, int nbytes, int rs_length);
extern void cc6_decode(unsigned char* block, int codelength, int cclen);
int cc6_codelength(int nbytes, int rs_length);
int cc6_nbytes(int codelength, int cclen);
%}

extern void cc6_encode(unsigned char* block, int nbytes, int rs_length);
extern void cc6_decode(unsigned char* block, int codelength, int cclen);
int cc6_codelength(int nbytes, int rs_length);
int cc6_nbytes(int codelength, int cclen);
