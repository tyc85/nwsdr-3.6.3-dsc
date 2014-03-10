#!/usr/bin/env python

"""
setup.py file for SWIG 
"""

from distutils.core import setup, Extension


cat_cccodec_module = Extension('_cat_cccodec',
                           sources=['cat_cccodec_wrap.c', 'cat_cccodec.c',
                             'viterbi27_mmx.c', 'viterbi27_sse.c', 'viterbi27_sse2.c', 'viterbi29_mmx.c', 'viterbi29_sse.c', 'viterbi29_sse2.c', 'viterbi39_sse2.c', 'viterbi39_sse.c', 'viterbi39_mmx.c', 'viterbi615_mmx.c', 'viterbi615_sse.c', 'viterbi615_sse2.c', 'dotprod_mmx.c', 'dotprod_mmx_assist.c', 'dotprod_sse2.c', 'dotprod_sse2_assist.c', 'peakval_mmx.c', 'peakval_mmx_assist.c', 'peakval_sse.c', 'peakval_sse_assist.c', 'peakval_sse2.c', 'peakval_sse2_assist.c', 'sumsq.c', 'sumsq_port.c', 'sumsq_sse2.c', 'sumsq_sse2_assist.c', 'sumsq_mmx.c', 'sumsq_mmx_assist.c', 'cpu_features.c', 'cpu_mode_x86.c', 'fec.c', 'sim.c', 'viterbi27.c', 'viterbi27_port.c', 'viterbi29.c', 'viterbi29_port.c', 
'viterbi39.c', 'viterbi39_port.c', 
'viterbi615.c', 'viterbi615_port.c', 'encode_rs_char.c', 'encode_rs_int.c', 'encode_rs_8.c', 
'decode_rs_char.c', 'decode_rs_int.c', 'decode_rs_8.c', 
'init_rs_char.c', 'init_rs_int.c', 'ccsds_tab.c', 
'encode_rs_ccsds.c', 'decode_rs_ccsds.c', 'ccsds_tal.c', 
'dotprod.c', 'dotprod_port.c', 
'peakval.c', 'peakval_port.c', 
'sumsq.c', 'sumsq_port.c'],
                           )

setup (name = 'cat_cccodec',
       version = '0.1',
       author      = "SWIG Docs",
       description = """Simple swig example from docs""",
       ext_modules = [cat_cccodec_module],
       py_modules = ["cat_cccodec"],
       )