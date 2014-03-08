#!/bin/sh
echo "Creating wrapper..."
swig -python cat_cccodec6.i
echo "Compiling..."
python setup.py build_ext --inplace
echo "Done"
