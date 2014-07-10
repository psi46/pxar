#!/usr/bin/env python2
"""
Simple Example Python Script Using the Pxar API.
"""
import PyPxarCore
from PyPxarCore import Pixel, PixelConfig, PyPxarCore, PyRegisterDictionary, PyProbeDictionary
from numpy import set_printoptions, nan
from pxar_helpers import * # arity decorator, PxarStartup, PxarConfigFile, PxarParametersFile and others

import os # for file system cmds
import sys

def main(argv=None):
    if argv is None:
        argv = sys.argv
        progName = os.path.basename(argv.pop(0))

    # command line argument parsing
    import argparse
    parser = argparse.ArgumentParser(prog=progName, description="A Simple Command Line Interface to the pxar API.")
    parser.add_argument('--dir', '-d', metavar="DIR", help="The directory with all required config files.")
    parser.add_argument('--verbosity', '-v', metavar="LEVEL", default="INFO", help="The output verbosity set in the pxar API.")
    args = parser.parse_args(argv)

    api = PxarStartup(args.dir,args.verbosity)

    # get pulseheight vs DAC
    hits = api.getPulseheightVsDAC("vcal", 1, 15, 32)

    # print output
    # uncomment the next line for full output
    # set_printoptions(threshold=nan)
    print hits

    print "Done"



if __name__ == "__main__":
    sys.exit(main())
