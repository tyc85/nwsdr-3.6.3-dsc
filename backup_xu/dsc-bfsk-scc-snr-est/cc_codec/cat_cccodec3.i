%module cat_cccodec3

%{
extern void cc3_encode(unsigned char* block, int nbytes, int rs_length);
extern void cc3_decode(unsigned char* block, int codelength, int cclen);
extern void cc3_softdecode();
int cc3_codelength(int nbytes, int rs_length);
int cc3_nbytes(int codelength, int cclen);
%}

extern void cc3_encode(unsigned char* block, int nbytes, int rs_length);
extern void cc3_decode(unsigned char* block, int codelength, int cclen);
extern void cc3_softdecode();
int cc3_codelength(int nbytes, int rs_length);
int cc3_nbytes(int codelength, int cclen);
