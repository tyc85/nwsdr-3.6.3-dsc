
//#ifdef HAVE_CONFIG_H
//#include "config.h"
//#endif

#include <digital_bytes_to_pam4.h>
#include <gr_io_signature.h>
#include <assert.h>

static const int PAM4_PER_BYTE = 4;
static const float constellation[4] = {-3, -1, 3, 1};

digital_bytes_to_pam4_sptr
digital_make_bytes_to_pam4 ()
{
  return gnuradio::get_initial_sptr(new digital_bytes_to_pam4 ());
}

digital_bytes_to_pam4::digital_bytes_to_pam4 ()
  : gr_sync_interpolator ("bytes_to_pam4",
			  gr_make_io_signature (1, 1, sizeof (unsigned char)),
			  gr_make_io_signature (1, 1, sizeof (float)),
			  PAM4_PER_BYTE)
{
}

int
digital_bytes_to_pam4::work (int noutput_items,
			     gr_vector_const_void_star &input_items,
			     gr_vector_void_star &output_items)
{
  const unsigned char *in = (unsigned char *) input_items[0];
  float *out = (float *) output_items[0];

  assert (noutput_items % PAM4_PER_BYTE == 0);
  
  for (int i = 0; i < noutput_items / PAM4_PER_BYTE; i++) {
    int x = in[i];

    *out++ = constellation[(x >> 6) & 0x3];
    *out++ = constellation[(x >> 4) & 0x3];
    *out++ = constellation[(x >> 2) & 0x3];
    *out++ = constellation[(x >> 0) & 0x3];

    //    *out++ = (((x >> 6) & 0x3) << 1) - 3;
    //    *out++ = (((x >> 4) & 0x3) << 1) - 3;
    //    *out++ = (((x >> 2) & 0x3) << 1) - 3;
    //    *out++ = (((x >> 0) & 0x3) << 1) - 3;
  }

  return noutput_items;
}



