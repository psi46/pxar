#!/usr/bin/env python2
"""
Helper classes and functions useful when interfacing the pxar API with Python.
"""
import PyPxarCore
from PyPxarCore import Pixel, PixelConfig, PyPxarCore, PyRegisterDictionary, PyProbeDictionary
from functools import wraps # used in parameter verification decorator ("arity")
import os # for file system cmds
import sys

# "arity": decorator used for parameter parsing/verification on each cmd function call
# Usually, the cmd module only passes a single string ('line') with all parameters;
# this decorator divides and verifies the types of each parameter.
def arity(n, m, cs=[]): # n = min number of args, m = max number of args, cs = types
    def __temp1(f):
        @wraps(f) # makes sure the docstring of the orig. function is passed on
        def __temp2(self, text):
            ps = filter(lambda p: p, text.split(" "))
            if len(ps) < n:
                print "Error: this command needs %d arguments (%d given)" % (n, len(ps))
                return
            if len(ps) > m:
                print "Error: this command takes at most %d arguments (%d given)" % (m, len(ps))
                return
            # now verify the type
            try:
                ps = [ c(p) for c, p in zip(cs, ps) ]
            except ValueError as e:
                print "Error: '" + str(p) + "' does not have " + str(c)
                return
            f(self, *ps)
        return __temp2
    return __temp1

def print_data(fullOutput,data,stepsize=1):
    for idac, dac in enumerate(data):
        s = "DAC " + str(idac*stepsize) + ": "
        if fullOutput:
            for px in dac:
                s += str(px)
        else:
            s += str(len(dac)) + " pixels"
        print s

def get_possible_filename_completions(text):
    head, tail = os.path.split(text.strip())
    if head == "": #no head
        head = "."
    files = os.listdir(head)
    return [ f for f in files if f.startswith(tail) ]
 
def extract_full_argument(line, endidx):
    newstart = line.rfind(" ", 0, endidx)
    return line[newstart:endidx]

class PxarConfigFile:
    """ class that loads the old-style config files of psi46expert """
    def __init__(self, f):
        self.config = {}
        import shlex
        thisf = open(f)
        try:
            for line in thisf:
                if not line.startswith("--"):
                    parts = shlex.split(line)
                    if len(parts) == 2:
                        self.config[parts[0].lower()] = parts[1]
        finally:
            thisf.close()
    def show(self):
        print self.config
    def get(self, opt, default = None):
        if default:
            return self.config.get(opt.lower(),default)
        else:
            return self.config[opt.lower()]

class PxarParametersFile:
    """ class that loads the old-style parameters files of psi46expert """
    def __init__(self, f):
        self.config = {}
        import shlex
        thisf = open(f)
        try:
            for line in thisf:
                if not line.startswith("--"):
                    parts = shlex.split(line)
                    if len(parts) == 3:
                        # ignore the first part (index/line number)
                        self.config[parts[1].lower()] = parts[2]
                    elif len(parts) == 2:
                        self.config[parts[0].lower()] = parts[1]
        finally:
            thisf.close()
    def show(self):
        print self.config
    def get(self, opt, default = None):
        if default:
            return self.config.get(opt.lower(),default)
        else:
            return self.config[opt.lower()]
    def getAll(self):
        return self.config

def PxarStartup(directory, verbosity):
    if not directory or not os.path.isdir(directory):
        print "Error: no or invalid configuration directory specified!"
        sys.exit(404)
    
    config = PxarConfigFile('%sconfigParameters.dat'%(os.path.join(directory,"")))
    tbparameters = PxarParametersFile('%s%s'%(os.path.join(directory,""),config.get("tbParameters")))

    # Power settings:
    power_settings = {
        "va":config.get("va",1.9),
        "vd":config.get("vd",2.6),
        "ia":config.get("ia",1.190),
        "id":config.get("id",1.10)}

    # Pattern Generator for single ROC operation:
    if int(config.get("nTbms")) == 0:
        pg_setup = (
            ("PG_RESR",25),
            ("PG_CAL",106),
            ("PG_TRG",16),
            ("PG_TOK",0))
    else:
        pg_setup = (
            ("PG_REST",15),
            ("PG_CAL",106),
            ("PG_TRG",0))

    # Start an API instance from the core pxar library
    api = PyPxarCore(usbId=config.get("testboardName"),logLevel=verbosity)
    print api.getVersion()
    if not api.initTestboard(pg_setup = pg_setup, 
                             power_settings = power_settings,
                             sig_delays = tbparameters.getAll()):
        print "WARNING: could not init DTB -- possible firmware mismatch."
        print "Please check if a new FW version is available"
        exit

    
    tbmDACs = []
    for tbm in range(int(config.get("nTbms"))):
        for n in range(2):
            tbmparameters = PxarParametersFile('%s%s'%(os.path.join(directory,""),config.get("tbmParameters") + "_C" + str(tbm) + ("a" if n%2 == 0 else "b") + ".dat"))
            tbmDACs.append(tbmparameters.getAll())

    print "Have DAC config for " + str(len(tbmDACs)) + " TBM cores:"
    for idx, tbmDAC in enumerate(tbmDACs):
        for key in tbmDAC:
            print "  TBM " + str(idx/2) + ("a" if idx%2 == 0 else "b") + " dac: " + str(key) + " = " + str(tbmDAC[key])

    # init pixel list
    pixels = list()
    for column in range(0, 52):
        for row in range(0, 80):
            p = PixelConfig(column,row,15)
            p.mask = False
            pixels.append(p)

    rocDacs = []
    rocPixels = list()
    for roc in xrange(int(config.get("nrocs"))):
        dacconfig = PxarParametersFile('%s%s_C%i.dat'%(os.path.join(directory,""),config.get("dacParameters"),roc))
        rocDacs.append(dacconfig.getAll())
        rocPixels.append(pixels)

    print "And we have just initialized " + str(len(pixels)) + " pixel configs to be used for every ROC!"

    api.initDUT(int(config.get("hubId",31)),config.get("tbmType","tbm08"),tbmDACs,config.get("rocType"),rocDacs,rocPixels)

    api.testAllPixels(True)
    print "Now enabled all pixels"

    print "pxar API is now started and configured."
    return api
