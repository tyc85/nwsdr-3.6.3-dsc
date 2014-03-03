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
#include <stdio.h>

#define VERBOSE 0

// Xu: Include .so
#include <dlfcn.h>

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

  // Xu: Will hard code the d_packetlen in the real implementation
  d_packetsym_cnt = 0;
  //d_packetlen = 5034; 
  d_packetlen = 2209;
  //printf("packetlen is %d", d_packetlen);
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
       		   //gr_make_io_signaturev (1, 2, isig), // The second input port inputs float soft info
		   gr_make_io_signature (1, 2, sizeof(unsigned char)), // fixed point info 
		   //gr_make_io_signature (1, 1, sizeof(unsigned char)), // Commented by Xu
		   gr_make_io_signature (0, 0, 0)),
    d_target_queue(target_queue)
{
  enter_search();
  // Xu: For soft decoding
  handle_ldpc = dlopen ("/lib/_ldpc.so", RTLD_LAZY);
  if (!handle) 
  {
    fputs (dlerror(), stderr);
    exit(1);
  }
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

  // Xu: Make the second input port pass the soft values
  //const float *in_symbol; // float point
  const unsigned char *in_symbol; // fixed point
  if(input_items.size() == 2){
    //in_symbol = (const float*) input_items[1]; // float point
    in_symbol = (const unsigned char*) input_items[1]; // fixed point
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
	  // Commented by Xu: Go ahead and decode the packet even though the header is in error
	  if (header_ok()){
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
	  }
	  else
	    enter_search();				// bad header
	  break;					// we're in a new state
	}
      }
      break;

    case STATE_HAVE_HEADER:
      if (VERBOSE)
	fprintf(stderr,"Packet Build\n");

      // Xu: Include .so file 
      
      //void *handle;
      void (* softdecode)(const unsigned char *, unsigned char *, int , int);
      char *error;
      int info_packetlen;
 	  unsigned char tempchar;
	  int i,offset;

	  /*
      handle = dlopen ("/lib/cat_cccodec3.so", RTLD_LAZY);
      if (!handle) {
        fputs (dlerror(), stderr);
        exit(1);
       }
	  */
      *(void **)(&softdecode)= dlsym(handle, "cc3_softdecode");
  
	  info_packetlen = RSLEN* d_packetlen / CCLEN;
      while (count < noutput_items){

      
        pkt_symbol[d_packetsym_cnt++] = in_symbol[count++]; 
        //printf("pkt_symbol is %u \n", pkt_symbol[d_packetsym_cnt-1]);

		if (d_packetsym_cnt%8 ==0 ){
			offset = d_packetsym_cnt  -8;
   			/*
			printf("offset is %d, original pkt symbol is \n", offset);
			for (i=0;i<8, i++)
				printf("%u ",pkt_symbol[offset+i]);
			printf("\n");
			*/
			// reverse the symbols
			for (i=0; i<4; i++){
				tempchar = pkt_symbol[offset+i];
				pkt_symbol[offset + i] = pkt_symbol[offset+7-i];
				pkt_symbol[offset+7-i] = tempchar;
			}

			
			
		}
		

	   
		if (d_packetsym_cnt== d_packetlen*8){ // Collect all symbols

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
      ////
      /*
      while (count < noutput_items) {   // shift bits into bytes of packet one at a time
        
	
		// Xu: Hard decoding 
		//d_packet_byte = (d_packet_byte << 1) | ( (in_symbol[count++] >127.5?1:0) & 0x1); // Xu: Test the fixed point symbols
		//d_packet_byte = (d_packet_byte << 1) | ( (in_symbol[count++] >0?1:0) & 0x1); // Xu: Test the float point symbols
		d_packet_byte = (d_packet_byte << 1) | (in[count++] & 0x1); // Commented by Xu
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
*/
    default:
      assert(0);

    } // switch

  }   // while

  return noutput_items;
}
