#!/bin/sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../../lib
export PYTHONPATH=$PYTHONPATH:./build/lib.linux-x86_64-2.7

# build lib
python setup.py build_ext

# Run some script requiring python lib and pxar core lib:
python test.py
