#!/bin/sh
PXARDIR="$(readlink -f $(dirname $0)/..)"

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PXARDIR/lib
export PYTHONPATH=$PYTHONPATH:$PXARDIR/lib

# Run the cmd script requiring python lib and pxar core lib:
python $PXARDIR/python/cmdline.py $*
