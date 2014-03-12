#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "parameters.h"
#include "init_field.h"
#include "get_msg.h"
#include "encode.h"
#include "lose_packets.h"
#include "decode.h" 
#include "compare_msg.h"
#include "parameters.h"
void cat_encode_cauchy(unsigned char* in, unsigned char* out, int pkt_length)
{
  int i, j, iter, Nrec, TNrec, seed, return_code ;
  static UNSIGNED COLBIT, BIT[15] ;
  static UNSIGNED *ExptoFE, *FEtoExp;
  //pkt_length is in unit of byte, Nsegs*Lfield = 360
  static unsigned int message[Mlen]; //Nsegs*Lfield*4 bytes
  unsigned int *msg;
  //Plentot = Plen + 1
  //include a 4-byte of pkt number in each packet (one unsigned integer)
  static unsigned int packets[Npackets*Plentot];
  unsigned int word;
  // need to pack unsigne char into unsigned int;
  	/*for(i = 0; i < Mlen; i ++)
  	{
    word = 0;
    for(j = 0; j < 4; j ++)
    {
	 	word = (word << 8) | in[i*4+j];
    }
    message[i] = word;
  	}
  	*/
   //memcpy(
	msg = (unsigned int *)in;

	ExptoFE = (unsigned int *) calloc(TableLength+Lfield, sizeof(UNSIGNED));
	if (!(ExptoFE)) {printf("\ndriver: ExptoFE malloc failed\n"); exit(434); }
	FEtoExp = (unsigned int *) calloc(TableLength, sizeof(UNSIGNED));
	if (!(FEtoExp)) {printf("\ndriver: FEtoExp malloc failed\n"); exit(434); }
  
	Init_field(&COLBIT, BIT, ExptoFE, FEtoExp);
	Encode(COLBIT, BIT, ExptoFE, FEtoExp, packets, msg);
  
  //pack packets into unsigned char now!!Plentot*Mpackets
	/*for(i = 0; i < Plentot*Mpackets; i++)
	{
		for(j = 0; j < 4; j ++)
		{
			out[i*4 + j] =  (packets[i] >> (8*(3-j))) & 0x000000ff;
		}
	}*/
	memcpy( out, packets, Plentot*Npackets*4 );
	//printf("packets[1] is %x\n", packets[1]);
	//printf("in[4:7] is %x%x%x%x\n", out[4], out[5], out[6], out[7]);

}
int cat_msg_compare(unsigned char *in1, unsigned char *in2, int pkt_length)
{
	int i, j;
	unsigned int word1, word2;
	unsigned int msg1[Mlen], msg2[Mlen]; //Mlen = Plen*Mpakcets
	for(i = 0; i < Mlen; i ++)	
	{
		word1 = 0;
		for(j = 0; j < 4; j ++)
		{	 
			word1 = (word1 << 8) | in1[i*4+j]; 
    	}
		msg1[i] = word1;
	}

	for(i = 0; i < Mlen; i ++)	
	{
		word2 = 0;
		for(j = 0; j < 4; j ++)
		{	 
			word2 = (word2 << 8) | in2[i*4+j]; 
    	}
		msg2[i] = word2;
	}
	Compare_msg(msg1, msg2);
	printf("compare message done!\n");

	return 0;
}

void cat_decode_cauchy(unsigned char* in, unsigned char* out, int num_rec)
{
	int i, j, iter, Nrec, TNrec, seed, return_code ;
	unsigned int COLBIT, BIT[15] ;
	unsigned int rec_message[Mlen];//Plen=[Nsegs*Lfield]
	unsigned int rec_packets[Npackets*Plentot];
	//unsigned int *rec_message, *rec_packets;
	unsigned int *ExptoFE, *FEtoExp;
	unsigned int word;
	/*
	rec_message = (unsigned int *) calloc(Nsegs*Lfield, sizeof(int));
	if (!(rec_message)) {printf("\ndecode: rec_message malloc failed\n"); exit(1); }
	rec_packets = (unsigned int *) calloc(Plentot*num_rec, sizeof(int));
	if (!(rec_packets)) {printf("\ndecode: rec_packets malloc failed\n"); exit(1); }
	*/
	ExptoFE = (unsigned int *) calloc(TableLength+Lfield, sizeof(UNSIGNED));
	if (!(ExptoFE)) {printf("\ndriver: ExptoFE malloc failed\n"); exit(434); }
	FEtoExp = (unsigned int *) calloc(TableLength, sizeof(UNSIGNED));
	if (!(FEtoExp)) {printf("\ndriver: FEtoExp malloc failed\n"); exit(434); }
	
	Init_field(&COLBIT, BIT, ExptoFE, FEtoExp);
	memcpy( rec_packets, in, num_rec*Plentot*4 );
	//total number
	/*for(i = 0; i < num_rec*Plentot; i ++)
	{
		word = 0;
		for(j = 0; j < 4; j ++)
		{
			word = (word << 8) | in[i*4 + j]; 
		}
		rec_packets[i] = word;
	}*/
	// need to pack uchar in to uint rec_pack
	return_code = Decode(COLBIT, BIT, ExptoFE, FEtoExp, rec_packets, &num_rec, rec_message);
	memcpy( out, rec_message, Mlen*4 );
	/*for(i = 0; i < Mlen; i ++)
	{
		for(j = 0; j < 4; j ++)
		{
			out[i*4 + j] =   (rec_message[i] >> (8*(3-j))) & 0x000000ff;
			//printf("%x", (rec_message[i] >> (8*(3-j))) & 0x000000ff);
			//printf("%x", out[i*4+j]);
		}
	}*/
	i = 0;
	//printf("\n%d th rec_message is %x\n in is %x\n%x\n%x\n%x\n", i, rec_message[i], out[0], out[1], out[2], out[3]);	
	if (return_code == 0) 
	{
		;
		//printf("Done with decoding\n");
	}

}


