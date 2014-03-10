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

#if HAVE_GETOPT_LONG
struct option Options[] = {
  {"frame-length",1,NULL,'l'},
  {"frame-count",1,NULL,'n'},
  {"ebn0",1,NULL,'e'},
  {"gain",1,NULL,'g'},
  {"verbose",0,NULL,'v'},
  {"force-altivec",0,NULL,'a'},
  {"force-port",0,NULL,'p'},
  {"force-mmx",0,NULL,'m'},
  {"force-sse",0,NULL,'s'},
  {"force-sse2",0,NULL,'t'},
  {NULL},
};
#endif


#define MAXBYTES 10000
#define MEMLEN 8 
#define RATE (1.0/3.0)
#define RATEINV 3 


int main(int argc,char *argv[]){
  int i;
  int toterrcnt;

  int incclen = 300;
  const int nbytes = 1200;  // must be multiples of incclen
  int outcclen = (incclen+MEMLEN)*RATEINV;
  int codelength;
  int ccnbytes;

  int byteind = 0;
  int bitind = 0;

  unsigned char infobytes[nbytes];
  unsigned char block[MAXBYTES];
  //unsigned char bits[MAXBYTES];
  unsigned char insymbol[MAXBYTES*8];
  unsigned char decsymbol[MAXBYTES];
  
  printf("Start testing 1/3 Convolutional codes with constraint length 9 \n");
  for (i = 0; i < nbytes; i++)
  {
    //infobytes[i] = (i < nbytes) ? (random() & 1) : 0;
    infobytes[i] = random() & 0xff;
    //printf("%d ", infobytes[i]);
  }
  memcpy(block, infobytes, nbytes);
  //printf("infobytes is %d, block is %d \n",infobytes[0],block[0]);

  // Encode
  cc3_encode(block, nbytes, incclen);

  codelength = cc3_codelength(nbytes, incclen);
  ccnbytes = cc3_nbytes(codelength, outcclen);
  
  // Map the bits to fixed point values
  
   for(i=0; i< codelength*8; i++){
	byteind = i /8;
	bitind = 7 - (i %8);
	//bitind = i %8;
	// -1 is 255 in unsigned char, which luckily corresponds to the actuall implementation
	insymbol[i] =  ( (block[byteind] >> bitind) & 1) ==0? 0: 255; 
	//printf("%d ", (block[byteind] >>  bitind) & 1);
    }

  //printf("outcclen is  %d \n",outcclen);
  //printf("codelength in byte is %d \n",codelength);
  //printf("nbytes is %d, calculated nbytes is %d \n",nbytes, ccnbytes);

  // Test hard Decoding
  cc3_decode(block, codelength, outcclen);

  // Test soft decoding
  
  printf("start soft decoding! \n");
  cc3_softdecode(insymbol, decsymbol, codelength, outcclen);

  // Compare
  toterrcnt = 0;
  for(i = 0; i < nbytes; i++)
  {
    if (block[i] != infobytes[i]){
      printf ("error in Convolutional Codec!");
      //printf("input is %d, output is %d \n",infobytes[i],block[i]);
      toterrcnt ++;
    }
  }
  printf ("\n total err from hard decoding is %d \n", toterrcnt);

  // Compare output from soft decoder
  
  toterrcnt = 0;
  for(i = 0; i < nbytes; i++)
  {
    if (decsymbol[i] != infobytes[i]){
      printf ("error in Convolutional soft decoding!");
      //printf("input is %d, output is %d \n",infobytes[i],block[i]);
      toterrcnt ++;
    }
  }
  printf ("\n total err from soft decoding is %d \n", toterrcnt);
  


}
