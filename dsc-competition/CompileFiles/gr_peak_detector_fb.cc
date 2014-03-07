/* -*- c++ -*- */
/*
 * Copyright 2007,2010 Free Software Foundation, Inc.
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

// WARNING: this file is machine generated. Edits will be overwritten

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gr_peak_detector_fb.h>
#include <gr_io_signature.h>
#include <string.h>
#include <cstdio>

gr_peak_detector_fb_sptr
gr_make_peak_detector_fb (float threshold_factor_rise,
		     float threshold_factor_fall,
		     int look_ahead, float alpha)
{
  return gnuradio::get_initial_sptr (new gr_peak_detector_fb (threshold_factor_rise,
				  threshold_factor_fall,
				  look_ahead, alpha));
}

gr_peak_detector_fb::gr_peak_detector_fb (float threshold_factor_rise,
		float threshold_factor_fall,
		int look_ahead, float alpha)
  : gr_sync_block ("peak_detector_fb",
		   gr_make_io_signature (1, 1, sizeof (float)),
		   gr_make_io_signature (1, 1, sizeof (char))),
    d_threshold_factor_rise(threshold_factor_rise),
    d_threshold_factor_fall(threshold_factor_fall),
    d_look_ahead(look_ahead), d_avg_alpha(alpha), d_avg(0), d_found(0)
{
	peak_val = 0.00001; //-(float)INFINITY;
}

int
gr_peak_detector_fb::work (int noutput_items,
	      gr_vector_const_void_star &input_items,
	      gr_vector_void_star &output_items)
{
  float *iptr = (float *) input_items[0];
  char *optr = (char *) output_items[0];

  memset(optr, 0, noutput_items*sizeof(char));

  //float peak_val = -(float)INFINITY; // Commented by Xu Chen, make it a private variable
  int peak_ind = 0;
  unsigned char state = 0;
  int i = 0;

  //printf("noutput_items %d\n",noutput_items);
  while(i < noutput_items) {
    //printf("i: %d iptr[i]: %f davg: %f, noutput_items: %d \n", i, iptr[i], d_avg, noutput_items);
    if(state == 0) {  // below threshold
      // Modified by Xu Chen
      //if(iptr[i] > d_avg*d_threshold_factor_rise) {
      if(iptr[i] > d_avg*d_threshold_factor_rise && iptr[i] > peak_val/2.0) { // Xu Chen: In order to reduce the false alarm
	state = 1;

	// Added by Xu Chen
	//printf("Previous peak is %f iptr[i] is %f davg is %f \n", peak_val, iptr[i], d_avg);
	peak_val = iptr[i];
	peak_ind = i;
      }
      else {
	d_avg = (d_avg_alpha)*iptr[i] + (1-d_avg_alpha)*d_avg;
	i++;
      }
    }
    else if(state == 1) {  // above threshold, have not found peak
      //printf("Entered State 1: %f  i: %d  noutput_items: %d\n", iptr[i], i, noutput_items);
      //if (iptr[i] > 0.25)
	// printf("i: %d iptr[i]: %f is greater than 0.9 \n", i, iptr[i]);	
	
      if(iptr[i] > peak_val) {
	peak_val = iptr[i];
	peak_ind = i;
	d_avg = (d_avg_alpha)*iptr[i] + (1-d_avg_alpha)*d_avg;
	i++;
	//printf("Find Peak i: %d peak_val: %f davg: %f, noutput_items: %d \n", i, peak_val, d_avg, noutput_items);
      }
      else if (iptr[i] > d_avg*d_threshold_factor_fall) {
	d_avg = (d_avg_alpha)*iptr[i] + (1-d_avg_alpha)*d_avg;
	i++;

	//printf("At : %d peak_val: %f davg: %f, noutput_items: %d \n", i, peak_val, d_avg, noutput_items);
      }
      else {
	optr[peak_ind] = 1;
	state = 0;

	//printf("Leaving  State 1: Peak: %f  Peak Ind: %d   i: %d  noutput_items: %d, davg: %f \n",
	//peak_val, peak_ind, i, noutput_items, d_avg);

	// Modified by Xu Chen
	//peak_val = -(float)INFINITY;
	
      }
    }
  }

  if(state == 0) {
    //printf("Leave in State 0, produced %d\n",noutput_items);
    return noutput_items;
  }
  else {   // only return up to passing the threshold
    //printf("Leave in State 1, only produced %d of %d\n",peak_ind,noutput_items);
    return peak_ind+1;
  }
}
