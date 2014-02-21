/* -*- c++ -*- */
/*
 * Copyright 2005,2010 Free Software Foundation, Inc.
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <gr_unpack_k_bits_bb.h>
#include <gr_io_signature.h>
#include <stdexcept>
#include <stdio.h>

gr_unpack_k_bits_bb_sptr gr_make_unpack_k_bits_bb (unsigned k)
{
  return gnuradio::get_initial_sptr(new gr_unpack_k_bits_bb (k));
}

// Xu : Make two input and output ports to pass raw complex samles
static int os[] = {sizeof(unsigned char), sizeof(float)};
static std::vector<int> osig(os, os+sizeof(os)/sizeof(int));

static int is[] = {sizeof(unsigned char), sizeof(gr_complex)};
static std::vector<int> isig(is, is+sizeof(is)/sizeof(int));

gr_unpack_k_bits_bb::gr_unpack_k_bits_bb (unsigned k)
  : gr_sync_interpolator ("unpack_k_bits_bb",
        gr_make_io_signaturev (1, 2, isig), // Modified by Xu
			  gr_make_io_signaturev (1, 2, osig),
			  //gr_make_io_signature (1, 1, sizeof (unsigned char)),
			  //gr_make_io_signature (1, 1, sizeof (unsigned char)),
			  k),
    d_k (k)
{
  if (d_k == 0)
    throw std::out_of_range ("interpolation must be > 0");
}

gr_unpack_k_bits_bb::~gr_unpack_k_bits_bb ()
{
}

int
gr_unpack_k_bits_bb::work (int noutput_items,
			   gr_vector_const_void_star &input_items,
			   gr_vector_void_star &output_items)
{
  const unsigned char *in = (const unsigned char *) input_items[0];
  unsigned char *out = (unsigned char *) output_items[0];

  // Xu: Use the second port to output complex samples
  //printf("symbol mapper: here");
  
  const gr_complex *in_symbol;
  float *out_symbol;

  if(input_items.size() == 2)
    in_symbol = (const gr_complex*) input_items[1];

  if(output_items.size() == 2)
    out_symbol = (float*) output_items[1];
  


  int n = 0;
  for (unsigned int i = 0; i < noutput_items/d_k; i++){
    unsigned int t = in[i];
    for (int j = d_k - 1; j >= 0; j--)
      out[n++] = (t >> j) & 0x01;

      // Xu: Calculate the metrics for each bit
      if(output_items.size() == 2){
        // Assume BPSK, i.e., d_k = 1
        out_symbol[i] = real(in_symbol[i]) ;
        //printf("real part output from unpack_k_bits is %f", out_symbol[i]);
      }
  }

  assert(n == noutput_items);
  return noutput_items;
}
