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


#define MAXBYTES 1000000
#define MEMLEN 6 
#define RATE (1.0/2.0)
#define RATEINV 2 


int main(int argc,char *argv[]){
  int i;
  int toterrcnt;

  int incclen = 2400;
  const int nbytes = 1200;  // must be multiples of incclen
  int outcclen = (incclen+MEMLEN)*RATEINV;
  int codelength;
  int ccnbytes;

  unsigned char infobits[nbytes];
  unsigned char block[MAXBYTES];
  //unsigned char bits[MAXBYTES];

  for (i = 0; i < nbytes; i++)
  {
    //infobits[i] = (i < nbytes) ? (random() & 1) : 0;
    infobits[i] = random() & 0xff;
    //printf("%d ", infobits[i]);
  }
  memcpy(block, infobits, nbytes);
  //printf("infobits is %d, block is %d \n",infobits[0],block[0]);

  // Encode
  cc2_encode(block, nbytes, incclen);

  codelength = cc2_codelength(nbytes, incclen);
  ccnbytes = cc2_nbytes(codelength, outcclen);
  
  //printf("outcclen is  %d \n",outcclen);
  //printf("codelength in byte is %d \n",codelength);
  //printf("nbytes is %d, calculated nbytes is %d \n",nbytes, ccnbytes);
  // Decode
  cc2_decode(block, codelength, outcclen);

  // Compare
  toterrcnt = 0;
  for(i = 0; i < nbytes; i++)
  {
    if (block[i] != infobits[i]){
      printf ("error in Convolutional Codec!");
      //printf("input is %d, output is %d \n",infobits[i],block[i]);
      toterrcnt ++;
    }
  }
  //printf ("\n total err is %d \n", toterrcnt);
}
