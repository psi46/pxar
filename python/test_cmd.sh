#!/bin/sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../lib
export PYTHONPATH=$PYTHONPATH:../lib

# Run the cmd script requiring python lib and pxar core lib:
python test_cmd.py
