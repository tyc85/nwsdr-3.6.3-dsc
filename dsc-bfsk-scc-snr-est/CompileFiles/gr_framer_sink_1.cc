/* -*- c++ -*- */
/*
 * Copyright 2004,2006,2010 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gr_framer_sink_1.h>
#include <gr_io_signature.h>
#include <cstdio>
#include <stdexcept>
#include <string.h>
#include <iostream>

#define VERBOSE 0

// Xu: Include .so
#include <dlfcn.h>
// Xu: Added a function to set d_len

void gr_framer_sink_1::setlen(int len)
{
  d_len = len;
}

void gr_framer_sink_1::setdec(char dec)
{
  d_dec = dec;
  
}

inline void
gr_framer_sink_1::enter_search()
{
  if (VERBOSE)
    fprintf(stderr, "@ enter_search\n");

  d_state = STATE_SYNC_SEARCH;
}

inline void
gr_framer_sink_1::enter_have_sync()
{
  if (VERBOSE)
    fprintf(stderr, "@ enter_have_sync\n");

  d_state = STATE_HAVE_SYNC;
  d_header = 0;
  d_headerbitlen_cnt = 0;
}

inline void
gr_framer_sink_1::enter_have_header(int payload_len, int whitener_offset)
{
  if (VERBOSE)
    fprintf(stderr, "@ enter_have_header (payload_len = %d) (offset = %d)\n", payload_len, whitener_offset);

  d_state = STATE_HAVE_HEADER;
  d_packetlen = payload_len;
  d_packet_whitener_offset = whitener_offset;
  d_packetlen_cnt = 0;
  d_packet_byte = 0;
  d_packet_byte_index = 0;

  // Xu: Hard code d_packetlen
    d_packetsym_cnt = 0;
  if (d_len >0)
     d_packetlen = d_len; //5034;
  //printf ("d_packetlen is %d \n", d_packetlen);
}

gr_framer_sink_1_sptr
gr_make_framer_sink_1(gr_msg_queue_sptr target_queue) 
{
  return gnuradio::get_initial_sptr(new gr_framer_sink_1(target_queue)); 
}

// Xu: Make two input port, the second one passes the soft values
static int is[] = {sizeof(unsigned char), sizeof(float)};
static std::vector<int> isig(is, is+sizeof(is)/sizeof(int));

gr_framer_sink_1::gr_framer_sink_1(gr_msg_queue_sptr target_queue) 
  : gr_sync_block ("framer_sink_1",
		   //gr_make_io_signature (1, 1, sizeof(unsigned char)), Commented by Xu
		   gr_make_io_signaturev (1, 2, isig), // The second input port inputs float soft info
		   //gr_make_io_signature (1, 2, sizeof(unsigned char)), // fixed point info
		   gr_make_io_signature (0, 0, 0)),
    d_target_queue(target_queue)
{
  enter_search();

  d_len = 0; // Xu: initilize d_len = 0  

  // Xu: For soft decoding
  d_dec = 'h';
  handle = dlopen ("/lib/cat_cccodec3.so", RTLD_LAZY);
  if (!handle) {
    fputs (dlerror(), stderr);
    exit(1);
   }
  for(int k= 0; k<10; k++){
    cat_snr[k] = 0; 
  }
  //cat_snr = {0};
  cat_num = 0;
}

gr_framer_sink_1::~gr_framer_sink_1 ()
{
   // Xu
  dlclose(handle);
}



int
gr_framer_sink_1::work (int noutput_items,
			gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items)
{
  const unsigned char *in = (const unsigned char *) input_items[0];
  int count=0;

  // Xu: Used for SNR Estimator
  //const float AC_CODE[64]={1,0,1,0,1,1,0,0,1,1,0,1,1,1,0,1,1,0,0,0,0,1,0,0,1,1,1,0,0,0,1,0,1,1,1,1,0,0,1,0,1,0,0,0,1,1,0,0,0,0,1,0,0,0,0,0,1,1,1,1,1,1,0,0}; // default access code
  // new access code
  const float AC_CODE[64]={0 ,0 ,0 ,0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1 ,1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0};

  float h=0 ,s=0,noise=0,snr=0,snr_sum=0;
  int p,j; 

  // Xu: Make the second input port pass the soft values
  //const float *in_soft_symbol; // float point
  const float *in_symbol; // float point
  if(input_items.size() == 2){
    in_symbol = (const float*) input_items[1]; // float point
    //in_symbol = (const unsigned char*) input_items[1]; // fixed point
  }
 

  if (VERBOSE)
    fprintf(stderr,">>> Entering state machine\n");

  while (count < noutput_items){
    switch(d_state) {

    case STATE_SYNC_SEARCH:    // Look for flag indicating beginning of pkt
      if (VERBOSE)
	fprintf(stderr,"SYNC Search, noutput=%d\n", noutput_items);

      while (count < noutput_items) {
	if (in[count] & 0x2){  // Found it, set up for header decode

          // Xu: SNR Estimator
	  if(input_items.size() == 2){
		  for(p=64;p>0;p--){
		    //printf("%f\n", in_soft_symbol[count-p]);
		    if(AC_CODE[64-p]>0)
		       s=s+in_symbol[count-p];
		    else 
		       s=s-in_symbol[count-p];
		    }
		  h = s/64;
		  for(p=64;p>0;p--){
		    if(AC_CODE[64-p]>0)
		      noise+=(h-in_symbol[count-p])*(h-in_symbol[count-p]);
		    else
		      noise+=(h+in_symbol[count-p])*(h+in_symbol[count-p]);
		  }
		  //printf("noise=%f\n",noise);
		  snr=64*h*h/noise;
		  cat_snr[cat_num%10]=snr;
		  if(cat_num<10){
		    for(j=0;j<cat_num;j++){
		      snr_sum += cat_snr[j];
		    }
		    snr_sum /=cat_num+1;}
		  else{
		    for(j=0;j<10;j++){
		      snr_sum += cat_snr[j];
		    }
		    snr_sum /=10;}
		  cat_num++;
		  printf("snr=%f\n",10*log10(snr_sum));
          }

          //

	  enter_have_sync();
	  break;
	}
	count++;
      }
      break;

    case STATE_HAVE_SYNC:
      if (VERBOSE)
	fprintf(stderr,"Header Search bitcnt=%d, header=0x%08x\n",
		d_headerbitlen_cnt, d_header);

      while (count < noutput_items) {	// Shift bits one at a time into header
	d_header = (d_header << 1) | (in[count++] & 0x1);
	if (++d_headerbitlen_cnt == HEADERBITLEN) {

	  if (VERBOSE)
	    fprintf(stderr, "got header: 0x%08x\n", d_header);

	  // we have a full header, check to see if it has been received properly
	  //if (header_ok()){ // Xu
	    int payload_len;
	    int whitener_offset;
	    header_payload(&payload_len, &whitener_offset);
	    enter_have_header(payload_len, whitener_offset);

	    if (d_packetlen == 0){	    // check for zero-length payload
	      // build a zero-length message
	      // NOTE: passing header field as arg1 is not scalable
	      gr_message_sptr msg =
		gr_make_message(0, d_packet_whitener_offset, 0, 0);

	      d_target_queue->insert_tail(msg);		// send it
	      msg.reset();  				// free it up

	      enter_search();
	    }
	  //}
	  //else
	    //enter_search();				// bad header
	  break;					// we're in a new state
	}
      }
      break;

    case STATE_HAVE_HEADER:
      if (VERBOSE)
	fprintf(stderr,"Packet Build\n");

      if (d_dec=='s'){ 
		
	      // Xu: Soft decoding starts here!
	      //printf("soft decoding starts \n"); 
	      //void *handle;
	      void (* softdecode)(const unsigned char *, unsigned char *, int , int);
	      char *error;
	      int info_packetlen;
              float snr_data_based, sum_sig = 0, sum_noise = 0;  //Zhiyi : snr estimator based on data
	
	      *(void **)(&softdecode)= dlsym(handle, "cc3_softdecode");
	      
	      info_packetlen = RSLEN* d_packetlen / CCLEN;
	      
	      
	      while (count < noutput_items){

		// Zhiyi: Snr estimate
		sum_sig += in_symbol[count]*in_symbol[count];
                if (in_symbol[count]>0){
                    sum_noise += (1-in_symbol[count])*(1-in_symbol[count]);
                }
                else {
                    sum_noise += (-1-in_symbol[count])*(-1-in_symbol[count]);
                }

		//pkt_symbol[d_packetsym_cnt++] = in_symbol[count++];
		// Convert float point to fixed point           
                
		pkt_symbol[d_packetsym_cnt] = 127.5 + 32*in_symbol[count++]; 
		if( pkt_symbol[d_packetsym_cnt] <0)
			pkt_symbol[d_packetsym_cnt] = 0;
		else if (pkt_symbol[d_packetsym_cnt] > 255)
			pkt_symbol[d_packetsym_cnt] = 255;
			

		d_packetsym_cnt ++;
		//printf("pkt_symbol is %u \n", pkt_symbol[d_packetsym_cnt-1]);
 		

		if (d_packetsym_cnt== d_packetlen*8){ // Collect all symbols
			// Zhiyi: snr estimator
                        snr_data_based = 10*log10(sum_sig/sum_noise);
                        printf("snr based on data =%f\n", snr_data_based);

			// Decode here!
	      		(*softdecode)(pkt_symbol, out_symbol, d_packetlen, CCLEN);

			if ((error = dlerror()) != NULL)  {
				fputs(error, stderr);
				exit(1);
			}

			gr_message_sptr msg = gr_make_message(0, d_packet_whitener_offset, 0, info_packetlen);
				memcpy(msg->msg(), out_symbol, info_packetlen);

			d_target_queue->insert_tail(msg);		// send it
			msg.reset();  				// free it up

			enter_search();
			break;
		}
	      }
	      //dlclose(handle);
	      break;
	}
	else{ // Conventional Decoding here
		
	      while (count < noutput_items) {   // shift bits into bytes of packet one at a time
		//printf("Test hard decoding of fixed point symbols \n"); // Xu

		d_packet_byte = (d_packet_byte << 1) | (in[count++] & 0x1);
		if (d_packet_byte_index++ == 7) {	  	// byte is full so move to next byte
		  d_packet[d_packetlen_cnt++] = d_packet_byte;
		  d_packet_byte_index = 0;

		  if (d_packetlen_cnt == d_packetlen){		// packet is filled

		    // build a message
		    // NOTE: passing header field as arg1 is not scalable
		    gr_message_sptr msg =
		      gr_make_message(0, d_packet_whitener_offset, 0, d_packetlen_cnt);
		    memcpy(msg->msg(), d_packet, d_packetlen_cnt);

		    d_target_queue->insert_tail(msg);		// send it
		    msg.reset();  				// free it up

		    enter_search();
		    break;
		  }
		}
	      }
	      break;
    } // if d_len
    default:
      assert(0);

    } // switch

  }   // while

  return noutput_items;
}
