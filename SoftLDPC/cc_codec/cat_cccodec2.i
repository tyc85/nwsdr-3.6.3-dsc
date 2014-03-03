%module cat_cccodec2

%{
extern void cc2_encode(unsigned char* block, int nbytes, int rs_length);
extern void cc2_decode(unsigned char* block, int codelength, int cclen);
int cc2_codelength(int nbytes, int rs_length);
int cc2_nbytes(int codelength, int cclen);
%}

extern void cc2_encode(unsigned char* block, int nbytes, int rs_length);
extern void cc2_decode(unsigned char* block, int codelength, int cclen);
int cc2_codelength(int nbytes, int rs_length);
int cc2_nbytes(int codelength, int cclen);
