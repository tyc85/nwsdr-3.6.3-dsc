/* -*- c++ -*- */
/*
 * Copyright 2005,2006 Free Software Foundation, Inc.
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

#ifndef INCLUDED_GR_FRAMER_SINK_1_H
#define INCLUDED_GR_FRAMER_SINK_1_H

#include <gr_core_api.h>
#include <gr_sync_block.h>
#include <gr_msg_queue.h>

// TC: for decoder class
#include "ArrayLDPCMacro.h"

class gr_framer_sink_1;
typedef boost::shared_ptr<gr_framer_sink_1> gr_framer_sink_1_sptr;

GR_CORE_API gr_framer_sink_1_sptr
gr_make_framer_sink_1 (gr_msg_queue_sptr target_queue);

/*!
 * \brief Given a stream of bits and access_code flags, assemble packets.
 * \ingroup sink_blk
 *
 * input: stream of bytes from gr_correlate_access_code_bb
 * output: none.  Pushes assembled packet into target queue
 *
 * The framer expects a fixed length header of 2 16-bit shorts
 * containing the payload length, followed by the payload.  If the
 * 2 16-bit shorts are not identical, this packet is ignored.  Better
 * algs are welcome.
 *
 * The input data consists of bytes that have two bits used.
 * Bit 0, the LSB, contains the data bit.
 * Bit 1 if set, indicates that the corresponding bit is the
 * the first bit of the packet.  That is, this bit is the first
 * one after the access code.
 */
class GR_CORE_API gr_framer_sink_1 : public gr_sync_block
{
  friend GR_CORE_API gr_framer_sink_1_sptr
  gr_make_framer_sink_1 (gr_msg_queue_sptr target_queue);
  //------ TC: similar setlength method
  

 private:
  enum state_t {STATE_SYNC_SEARCH, STATE_HAVE_SYNC, STATE_HAVE_HEADER};

  static const int MAX_PKT_LEN    = 10000; //4096; // Xu: make it larger
  static const int HEADERBITLEN   = 32;
  

  gr_msg_queue_sptr  d_target_queue;		// where to send the packet when received
  state_t            d_state;
  unsigned int       d_header;			// header bits
  int		     d_headerbitlen_cnt;	// how many so far

  unsigned char      d_packet[MAX_PKT_LEN];	// assembled payload
  unsigned char	     d_packet_byte;		// byte being assembled
  int		     d_packet_byte_index;	// which bit of d_packet_byte we're working on
  int 		     d_packetlen;		// length of packet
  int                d_packet_whitener_offset;  // offset into whitener string to use
  int		     d_packetlen_cnt;		// how many so far

  //--------- new part start
  int info_packetlen;
  //Xu: For Soft CC decoding
  //void *handle;
  // Xu: allocate memory to decoded symbols packed in bytes
  unsigned char out_symbol[MAX_PKT_LEN]; 
  // Xu: allocate memory to for floating point input stream
  float pkt_symbol[MAX_PKT_LEN*8];
  int d_packetsym_cnt; // how many symbols are collected
  /*
  static const int RSLEN = 1670;  // in byte
  static const int MEMLEN = 8;
  static const int RATEINV = 3;
  static const int CCLEN = 5034;//(RSLEN + MEMLEN)*RATEINV; in bits?
  */
  //TC: For soft LDPC decoding
  void *handle_ldpc;
 
  FP_Decoder *(*get_obj)(void);
  FP_Decoder *(*get_obj_general)(const char*, const char*, int vflag);
  void (*del_obj)(FP_Decoder*);
  void (* decode_ldpc)(FP_Decoder *, unsigned char *, unsigned char *, int);
  int (* decode_ldpc_general)(FP_Decoder *, float *, unsigned char *, int);
  //int (* decode_ldpc_general)(FP_Decoder *, unsigned char *, unsigned char *, int);
  //static const int LDPCCLEN = 2209;// pad 1670 bits with zeros and encode
  //static const int LDPCINFOLEN = 1978;
  //TC: for channel coding mode: 
  // 0: hard decoding, no code
  // 3: soft with cc ldpc
  // 1: soft with arrya ldpc
  // 2: soft with wifi ldpc
  int cat_code_mode;
  enum CAT_CODE_MODE { ARRAY, WIFI, CC};
  //--------- SNR estimator
  float cat_snr[10];
  int cat_num;
  //--------- new part end 

 protected:
  FP_Decoder *p_decoder_ldpc;
 
  gr_framer_sink_1(gr_msg_queue_sptr target_queue);
  
  void enter_search();
  void enter_have_sync();
  void enter_have_header(int payload_len, int whitener_offset);
  
  bool header_ok()
  {
    // confirm that two copies of header info are identical
    return ((d_header >> 16) ^ (d_header & 0xffff)) == 0;
  }

  void header_payload(int *len, int *offset)
  {
    // header consists of two 16-bit shorts in network byte order
    // payload length is lower 12 bits
    // whitener offset is upper 4 bits
    *len = (d_header >> 16) & 0x0fff;
    *offset = (d_header >> 28) & 0x000f;
  }

 public:
  ~gr_framer_sink_1();
  void setlen(int in){d_packetlen = in;};
  void cat_setlength(int in) {d_packetlen = in;};
  void cat_setcode(int in){cat_code_mode = in;}

  int work(int noutput_items,
	   gr_vector_const_void_star &input_items,
	   gr_vector_void_star &output_items);
};

#endif /* INCLUDED_GR_FRAMER_SINK_1_H */
