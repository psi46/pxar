#!/bin/sh
export PYTHONPATH=$PYTHONPATH:./build/lib.linux-x86_64-2.7

rm -rf build

# build lib
python setup.py build_ext

# Run some script requiring python lib and pxar core lib:
python test.py
