/* -*- c++ -*- */
/*
 * Copyright 2006,2007,2010,2012 Free Software Foundation, Inc.
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

#include <digital_map_bb.h>
#include <gr_io_signature.h>
#include <stdio.h>
//#include <gr_math.h>

digital_map_bb_sptr
digital_make_map_bb (const std::vector<int> &map)
{
  return gnuradio::get_initial_sptr(new digital_map_bb (map));
}

// Xu
//std::vector<int> io_sizes;
//io_sizes.push_back(sizeof(unsigned char));
//io_sizes.push_back(sizeof(gr_complex));
// TC: second port is complex numbers
static int ios[] = {sizeof(unsigned char), sizeof(gr_complex)};
static std::vector<int> iosig(ios, ios+sizeof(ios)/sizeof(int));
digital_map_bb::digital_map_bb (const std::vector<int> &map)
  : gr_sync_block ("map_bb",
		   gr_make_io_signaturev (1, 2, iosig), // Xu: Make the input output port number = 2
		   gr_make_io_signaturev (1, 2, iosig))
		   //gr_make_io_signature (1, 1, sizeof (unsigned char)), // Commented by Xu
		   //gr_make_io_signature (1, 1, sizeof (unsigned char)))
{

  for (int i = 0; i < 0x100; i++)
    d_map[i] = i;

  unsigned int size = std::min((size_t) 0x100, map.size());
  for (unsigned int i = 0; i < size; i++)
    d_map[i] = map[i];
}

int
digital_map_bb::work (int noutput_items,
		      gr_vector_const_void_star &input_items,
		      gr_vector_void_star &output_items)
{
  const unsigned char *in = (const unsigned char *) input_items[0];
  unsigned char *out = (unsigned char *) output_items[0];
  
  // Xu: Use the second port to output complex samples
  //printf("symbol mapper: here");
  
  const gr_complex *in_symbol;
  gr_complex *out_symbol;
  if(input_items.size() == 2)
    in_symbol = (const gr_complex*) input_items[1];

  if(output_items.size() == 2)
    out_symbol = (gr_complex*) output_items[1];
  
  

  for (int i = 0; i < noutput_items; i++){
    out[i] = d_map[in[i]];

    
    // XU: Use the second port to output complex samples
    
    //printf("out char is %c", out[i]);
    if(output_items.size() == 2){
      out_symbol[i] = in_symbol[i];
    }
    
  }

  return noutput_items;
}
