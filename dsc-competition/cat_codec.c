/* Test the Reed-Solomon codecs
 * for various block sizes and with random data and random error patterns
 *
 * Copyright 2002 Phil Karn, KA9Q
 * May be used under the terms of the GNU Lesser General Public License (LGPL)

Revised by Dongning Guo on 20130305
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include "fec.h"

#define N 255
#define K 223

/* calculate the total codeword length */
int cat_codelength(int nbytes)
{
  int ncodeword;
  ncodeword = (nbytes+K-1)/K;
  return nbytes + ncodeword * (N-K);
  /* return nh*(H+N-K) + nl*(L+N-K); */
}

/* given the codelength, calculate the original source sequence length */
int cat_nbytes(int codelength)
{
  int ncodeword;
  ncodeword = (codelength+N-1)/N;
  return codelength - ncodeword * (N-K);
  /*  return nh*(H+N-K) + nl*(L+N-K); */
}

/* input is nbytes of unsigned chars in block,
   block must be of cat_codelength to hold all codewords */
int cat_encode(unsigned char* block, int nbytes)
{
  int ncodeword;
  int nh;
  int nl;
  int H;
  int L;
  int i;
  unsigned char temp[N];

  ncodeword = (nbytes+K-1)/K;
  H = (nbytes+ncodeword-1)/ncodeword;
  nl = ncodeword * H - nbytes;
  nh = ncodeword - nl;
  L = H - 1;

  /* Input divided into chucks of length H followed by chuncks of length L,
     total length is nbytes
     total number of chunks is ncodeword
     H = L+1 */

  /* Start from the last chunck, a shorter chuck if any */
  for( i=ncodeword-1; i>=nh; i-- ) {
    /* input -> temp */
    memcpy(&temp[K-L], &block[nh*H+(i-nh)*L], L);
    /* pad zeros at front */
    memset(temp,0,K-L);
    /* encode temp */
    encode_rs_8(temp, &temp[K], 0);
    /* temp -> output */
    memcpy(&block[i*(H+N-K)+nh-i], &temp[K-L], L+N-K);
  }
    
  /* Next treat the remaining longer chuncks */
  for( i=nh-1; i>=0; i--){
    /* input -> temp */
    memcpy(&temp[K-H], &block[i*H], H);
    /* pad zeros at front */
    memset(temp,0,K-H);
    /* encode temp */
    encode_rs_8(temp, &temp[K], 0);
    /* temp -> output */
    memcpy(&block[i*(H+N-K)], &temp[K-H], H+N-K);
  }
	return 1;
}

/* input must be of cat_codelength that contains all codewords 
   output is nbytes of unsigned chars in block */
int cat_decode(unsigned char* block, int nbytes)
{
  int ncodeword;
  int nh;
  int nl;
  int H;
  int L;
  int i;
  unsigned char temp[N];

  ncodeword = (nbytes+K-1)/K;
  H = (nbytes+ncodeword-1)/ncodeword;
  nl = ncodeword * H - nbytes;
  nh = ncodeword - nl;
  L = H - 1;

  /* Input divided into chucks of length H followed by chuncks of length L,
     total length is nbytes
     total number of chunks is ncodeword
     H = L+1 */

  /* Start from the first chunck, a longer chuck */
  for( i=0; i<nh; i++ ) {
    /* input -> temp */
    memcpy(&temp[K-H], &block[i*(H+N-K)], H+N-K);
    /* pad zeros at front */
    memset(temp,0,K-H);
    /* encode temp */
    decode_rs_8(temp, NULL, 0, 0);
    /* temp -> output */
    memcpy(&block[i*H], &temp[K-H], H);
  }
    
  /* Next treat the remaining shorter chuncks, if any */
  for( i=nh; i<ncodeword; i++){
    /* input -> temp */
  memcpy(&temp[K-L], &block[i*(H+N-K)+nh-i], L+N-K);
/*    memcpy(&temp[K-L], &block[nh*(H+N-K)+i*(L+N-K)], L+N-K);*/
    /* pad zeros at front */
    memset(temp,0,K-L);
    /* encode temp */
    decode_rs_8(temp, NULL, 0, 0);
    /* temp -> output */
    memcpy(&block[nh*H+(i-nh)*L], &temp[K-L], L);
  }
  return 1;
}
