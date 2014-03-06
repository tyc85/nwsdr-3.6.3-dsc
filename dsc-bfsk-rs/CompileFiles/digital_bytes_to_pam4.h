/* -*- c++ -*- */
/*
 * Copyright 2004,2012 Free Software Foundation, Inc.
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
#ifndef INCLUDED_GR_BYTES_TO_PAM4_H
#define INCLUDED_GR_BYTES_TO_PAM4_H

#include <digital_api.h>
#include <gr_sync_interpolator.h>

class digital_bytes_to_pam4;
typedef boost::shared_ptr<digital_bytes_to_pam4> digital_bytes_to_pam4_sptr;

DIGITAL_API digital_bytes_to_pam4_sptr digital_make_bytes_to_pam4();

/*!
 * \brief Convert stream of bytes to stream of +/- 1 symbols
 * \ingroup converter_blk
 *
 * input: stream of bytes; output: stream of float
 *
 * This block is deprecated.
 *
 * The combination of gr_packed_to_unpacked_bb followed by
 * gr_chunks_to_symbols_bf or gr_chunks_to_symbols_bc handles the
 * general case of mapping from a stream of bytes into arbitrary float
 * or complex symbols.
 *
 * \sa gr_packed_to_unpacked_bb, gr_unpacked_to_packed_bb,
 * \sa gr_chunks_to_symbols_bf, gr_chunks_to_symbols_bc.
 */
class DIGITAL_API digital_bytes_to_pam4 : public gr_sync_interpolator
{
  friend DIGITAL_API digital_bytes_to_pam4_sptr
    digital_make_bytes_to_pam4();

  digital_bytes_to_pam4();

 public:
  int work (int noutput_items,
	    gr_vector_const_void_star &input_items,
	    gr_vector_void_star &output_items);
};

#endif /* INCLUDED_GR_BYTES_TO_PAM4_H */