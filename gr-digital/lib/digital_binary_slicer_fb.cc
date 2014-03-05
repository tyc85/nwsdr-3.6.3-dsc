/* -*- c++ -*- */
/*
 * Copyright 2006,2010,2011 Free Software Foundation, Inc.
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

#include <digital_binary_slicer_fb.h>
#include <gr_io_signature.h>
#include <gr_math.h>
#include <stdexcept>
#include <iostream>
#include <gr_sync_interpolator.h>

#define BITS_PER_SAMPLE (1)
digital_binary_slicer_fb_sptr
digital_make_binary_slicer_fb ()
{
  return gnuradio::get_initial_sptr(new digital_binary_slicer_fb ());
}

digital_binary_slicer_fb::digital_binary_slicer_fb ()
  : gr_sync_interpolator ("binary_slicer_fb",
		   gr_make_io_signature (1, 1, sizeof (float)),
		   gr_make_io_signature (1, 1, sizeof (unsigned char)),
                   BITS_PER_SAMPLE )
{
  th1=2; //1.57079;   // pi over 2
  th2=-th1;
  pam4=0;
}


void digital_binary_slicer_fb::setpam4 ()
{
  pam4=1;
  set_interpolation(2);
  std::cout << "pam4=1; interp=2\n";
}

void digital_binary_slicer_fb::set_th1 (float th)
{
  th1=th;
  th2=-th1;
}

int
digital_binary_slicer_fb::work (int noutput_items,
				gr_vector_const_void_star &input_items,
				gr_vector_void_star &output_items)
{
  const float *in = (const float *) input_items[0];
  unsigned char *out = (unsigned char *) output_items[0];
  float inval;  

if (pam4==0)
{
  for (int i = 0; i < noutput_items; i++){
    out[i] = gr_binary_slicer(in[i]);
  }
}  // pam4==0
else
{
  assert (noutput_items % 2 == 0);
  for (int i = 0; i < noutput_items/2; i++){
    inval =in[i];
    if (inval >=0.0)
     *out++=1;
    else
     *out++=0; 
    if ((inval>th2) && (inval < th1))
     *out++=1;
    else
     *out++=0;
  } // i loop
} // pam4 != 0
  
  return noutput_items;
}
