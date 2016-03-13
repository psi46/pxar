#!/usr/bin/env python2
"""
Simple DAQ Python Script Using the pxarCore API.
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
    parser = argparse.ArgumentParser(prog=progName, description="A Simple DAQ using the pxarCore API.")
    parser.add_argument('--dir', '-d', metavar="DIR", help="The directory with all required config files.")
    parser.add_argument('--verbosity', '-v', metavar="LEVEL", default="INFO", help="The output verbosity set in the pxarCore API.")
    args = parser.parse_args(argv)

    api = PxarStartup(args.dir,args.verbosity)

    # prepare the device by turning of calibrates and turning of mask bits:
    api.testAllPixels(0)
    api.maskAllPixels(0)
    api.HVon()
    
    # enable external triggers:
    api.daqTriggerSource("random_dir",100000)

    # start the DAQ
    api.daqStart(0)

    print "Starting massive module readout test..."
    
    # collect data:
    evt = 0
    words = 0
    while 1:
        try:
            data = api.daqGetEvent()
            evt += 1
            mystats = api.getStatistics()
            words += mystats.info_words_read
            error = True if mystats.errors != 0 else False
        except RuntimeError:
            pass
        except KeyboardInterrupt:
            print "KeyboardInterrupt caught"
            print "#################################################################"
            print "Total: " + str(words) + " words in " + str(evt) + " events."
            print "#################################################################"
            raise
        if error:
            print "Found error after " + str(words) + " words (event " + str(evt) + ")"
        if evt%250000 == 0:
            print "Event " + str(evt)
            
    # stop the DAQ:
    api.daqStop()
    
    print "Done"



if __name__ == "__main__":
    sys.exit(main())
