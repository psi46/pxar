#!/bin/sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../lib
export PYTHONPATH=$PYTHONPATH:../lib

# Run some script requiring python lib and pxar core lib:
python test.py
