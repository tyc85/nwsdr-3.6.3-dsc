/* -*- c++ -*- */
/*
 * Copyright 2004,2010,2012 Free Software Foundation, Inc.
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

#include <digital_bytes_to_syms.h>
#include <gr_io_signature.h>
#include <assert.h>

static const int BITS_PER_BYTE = 8;
static const int FSK4_PER_BYTE = 4; // DG
static const float constellation[4] = {-3, -1, 3, 1}; // DG

// DG
void digital_bytes_to_syms::set_fsk4 ()
{
  fsk4 = 1;
}

digital_bytes_to_syms_sptr
digital_make_bytes_to_syms ()
{
  return gnuradio::get_initial_sptr(new digital_bytes_to_syms ());
}

digital_bytes_to_syms::digital_bytes_to_syms ()
  : gr_sync_interpolator ("bytes_to_syms",
			  gr_make_io_signature (1, 1, sizeof (unsigned char)),
			  gr_make_io_signature (1, 1, sizeof (float)),
			  BITS_PER_BYTE)
{
  fsk4 = 0;
  assert (fsk4 == 1);
}

int
digital_bytes_to_syms::work (int noutput_items,
			     gr_vector_const_void_star &input_items,
			     gr_vector_void_star &output_items)
{
  const unsigned char *in = (unsigned char *) input_items[0];
  float *out = (float *) output_items[0];

  if (fsk4==0)    {
  assert (noutput_items % BITS_PER_BYTE == 0);
  
  for (int i = 0; i < noutput_items / BITS_PER_BYTE; i++) {
    int x = in[i];

    *out++ = (((x >> 7) & 0x1) << 1) - 1;
    *out++ = (((x >> 6) & 0x1) << 1) - 1;
    *out++ = (((x >> 5) & 0x1) << 1) - 1;
    *out++ = (((x >> 4) & 0x1) << 1) - 1;
    *out++ = (((x >> 3) & 0x1) << 1) - 1;
    *out++ = (((x >> 2) & 0x1) << 1) - 1;
    *out++ = (((x >> 1) & 0x1) << 1) - 1;
    *out++ = (((x >> 0) & 0x1) << 1) - 1;
  }
    }
  else    {
  assert (noutput_items % FSK4_PER_BYTE == 0);
  
  for (int i = 0; i < noutput_items / FSK4_PER_BYTE; i++) {
    int x = in[i];

    *out++ = constellation[(x >> 6) & 0x3];
    *out++ = constellation[(x >> 4) & 0x3];
    *out++ = constellation[(x >> 2) & 0x3];
    *out++ = constellation[(x >> 0) & 0x3];
  }
    }

  return noutput_items;
}



