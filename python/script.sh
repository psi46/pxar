#!/bin/sh
PXARDIR="$(readlink -f $(dirname $0)/..)"

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PXARDIR/lib
export PYTHONPATH=$PYTHONPATH:$PXARDIR/lib

# Run the example script using python and pxar core lib:
python $PXARDIR/python/script.py $*
