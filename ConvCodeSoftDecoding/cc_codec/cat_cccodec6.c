/* Test viterbi decoder speeds */
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <memory.h>
#include <sys/time.h>
#include <sys/resource.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include "fec.h"

#define MAXBYTES 300000
#define MEMLEN 14
#define RATE (1.0/6.0)
#define RATEINV 6 

/* calculate the total length in bytes */
int cc6_codelength(int nbytes, int incclen)
{
  int chunks;
  int outbitlen, outbytelen;
  

  if( (nbytes*8)%incclen !=0)
	printf("error: nbytes*8 is not multiples of incclen");

  chunks = nbytes*8/incclen;

  outbitlen = chunks * (incclen+MEMLEN) * RATEINV;
 
  
  if (outbitlen %8 ==0)
	outbytelen = outbitlen /8;
  else 
	outbytelen = (outbitlen / 8) +1;

  return outbytelen;
  /* return nh*(H+N-K) + nl*(L+N-K); */
}


/* given the codelength in byte, calculate the original source sequence length */
int cc6_nbytes(int codelength, int outcclen)
{
  int chunks;
  int incclen;
  int nbytes;

  chunks = ( codelength*8 ) / outcclen;
  incclen = outcclen / RATEINV - MEMLEN;

  nbytes = chunks*incclen/8;

  return nbytes;
}


void cc6_encode(unsigned char* block, int nbytes, int incclen)
{
	// Length of input block is an array of bytes (unsigned char)
	// Every byte corresponds to 8 bits
	// Total length in bits is nbytes*8
	// Input to cc_encode: a sequence of bits of length nbytes*8
	// Encode: Pad 0's at the end of bit sequence
	// 	   Divide the input bits into chunks of incclen bits 
        //         Encode every chunk
	// Output from cc_encode: Pad zeros at the end of the coded sequence to make it multiple of 8
	//			  chunks of bits of length (incclen+6)*2 

	int chunks;
	int i;
	int n;
	int sr;
	int outcclen;
	int infobit;
	int outbytelen; // output length in bytes
	int byteind;
	int bitind;

	unsigned char symbols[MAXBYTES] = {0};

	
	outcclen = (incclen+MEMLEN) * RATEINV;
	outbytelen = cc6_codelength(nbytes, incclen);
   	
	if( (nbytes*8)%incclen !=0)
		printf("error: nbytes*8 is not multiples of incclen");

 	chunks = nbytes*8/incclen;


	sr = 0;
	for(n=0;n<chunks;n++){
		for(i=0;i<incclen+MEMLEN;i++){
  			// Last MEMLEN bits are tail-biting
			byteind = (int) ( (n*incclen + i) /8);
			bitind =  (n*incclen + i) % 8;

			if ( n*incclen + i +1 > nbytes*8){ 
				// Exceeds the input length of block
				infobit = 0;
			}
			else{
				infobit = (block[byteind]>> (7 - bitind)) & 1;
				infobit = (i < incclen) ? infobit : 0;
			}
			//printf("%d ",infobit);
			sr = (sr << 1) | infobit;
			
			byteind =  ( n*outcclen+RATEINV*i+0)/8;
			bitind = ( n*outcclen+RATEINV*i+0)%8;
			symbols[byteind] = symbols[byteind] + (parity(sr & V615POLYA)<<bitind);
			byteind = ( n*outcclen+RATEINV*i+1)/8;
			bitind = ( n*outcclen+RATEINV*i+1)%8;
			symbols[byteind] = symbols[byteind] + (parity(sr & V615POLYB)<<bitind);

			byteind =  ( n*outcclen+RATEINV*i+2)/8;
			bitind = ( n*outcclen+RATEINV*i+2)%8;
			symbols[byteind] = symbols[byteind] + (parity(sr & V615POLYC)<<bitind);
			byteind = ( n*outcclen+RATEINV*i+3)/8;
			bitind = ( n*outcclen+RATEINV*i+3)%8;
			symbols[byteind] = symbols[byteind] + (parity(sr & V615POLYD)<<bitind);

			byteind =  ( n*outcclen+RATEINV*i+4)/8;
			bitind = ( n*outcclen+RATEINV*i+4)%8;
			symbols[byteind] = symbols[byteind] + (parity(sr & V615POLYE)<<bitind);
			byteind = ( n*outcclen+RATEINV*i+5)/8;
			bitind = ( n*outcclen+RATEINV*i+5)%8;
			symbols[byteind] = symbols[byteind] + (parity(sr & V615POLYF)<<bitind);

			
			//symbols[n*outcclen + 2*i+0] = parity(sr & V27POLYA);
			//symbols[n*outcclen + 2*i+1] = parity(sr & V27POLYB);
 			//printf("%d %d ",parity(sr & V27POLYA), parity(sr & V27POLYB) );
		}
	}

	memcpy(block,symbols,outbytelen);
}

void cc6_decode(unsigned char* block, int codelength, int outcclen){
        /*
		input block is an array of bytes
		each byte corresponds to 8 bits
		block contains multiple chunks of length outcclen
        */
	int chunks;
	int i;
	int j;
	int byteind;
	int bitind;
	unsigned char data[MAXBYTES];
        unsigned char insymbol[MAXBYTES*8];
	void *vp;
	int incclen;
	unsigned char mask;
	int decbit;

	//printf("Enter Decoder! \n");
	chunks = codelength*8/ outcclen;
  	incclen = outcclen/ RATEINV - MEMLEN;

        // BPSK mapping 
	//printf("BPSK here \n");
	
        for(i=0; i< codelength*8; i++){
		byteind = i /8;
		bitind = i %8;
		insymbol[i] = ( (block[byteind] >> bitind) & 1) ==0? 1: -1;
		//printf("%d ", (block[byteind] >>  bitind) & 1);
	}

	//printf("codelength is %d, chunks is %d \n", codelength, chunks);
	if((vp = create_viterbi615(incclen)) == NULL){
		printf("create_viterbi615 failed\n");
		exit(1);
	}

        
    	for(i=0;i<chunks;i++){

	    init_viterbi615(vp,0);

	    /* Decode block */
	    update_viterbi615_blk(vp,insymbol+i*outcclen,incclen+MEMLEN);
	      
	    /* Do Viterbi chainback */
	    chainback_viterbi615(vp,data,incclen,0);

	   // printf("\n decoded data is :");
	    for(j=0; j<incclen; j++){
		byteind = (i*incclen+j) / 8;
		bitind = (i*incclen+j) % 8;

		mask = (0x80 >> bitind);
		decbit = (data[j/8] & 0x80) >> 7;
		block[byteind]  = (block[byteind] & ~mask) + (decbit << (7-bitind));
		data[j/8] = (data[j/8] << 1);
	    	
                //printf("%d ", decbit);
             
	    }
	}

}
