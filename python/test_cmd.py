import PyPxarCore
from PyPxarCore import Pixel, PixelConfig, PyPxarCore, PyDictionary
from functools import wraps # used in parameter verification decorator

import cmd      # for command interface and parsing
import sys

dacdict = PyDictionary()

# decorator used for parameter parsing/verification on each cmd function call
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

class PxarCoreCmd(cmd.Cmd):
    """Simple command processor for the pxar core API."""

    def __init__(self, api):
        cmd.Cmd.__init__(self)
        self.prompt = "pxar core =>> "
        self.intro  = "Welcome to the pxar core console!"  ## defaults to None
        self.api = api
    
    def do_EOF(self, line):
        """ clean exit when receiving EOF (Ctrl-D) """
        return True

    @arity(2,3,[str, int, int])
    def do_setDAC(self, dacname, value, rocid = None):
        """setDAC [DAC name] [value] [ROCID]
        Set the DAC to given value for given roc ID"""
        self.api.setDAC(dacname, value, rocid)

    def complete_setDAC(self, text, line, start_index, end_index):
        if text and len(line.split(" ")) <= 2: # first argument and started to type
            # list matching entries
            return [dac for dac in dacdict.getAllROCNames()
                        if dac.startswith(text)]
        else:
            if len(line.split(" ")) > 2:
                # return help for the cmd, removing line breaks and double spaces
                return [self.do_setDAC.__doc__.replace('\n',': ').replace('  ',' '), '']
            else:
                # return all DACS
                return dacdict.getAllROCNames()

    def do_quit(self, arg):
        sys.exit(1)

    def help_quit(self):
        print "syntax: quit",
        print "-- terminates the application"

    # shortcuts
    do_q = do_quit

# collect some basic settings

# Signal delays
sig_delays = {
"clk":2,
"ctr":2,
"sda":17,
"tin":7,
"deser160phase":4}

# Power settings:
power_settings = {
"va":1.9,
"vd":2.6,
"ia":1.190,
"id":1.10}

# Pattern Generator for single ROC operation:
pg_setup = (
("PG_RESR",25),  
("PG_CAL",101+5),
("PG_TRG",16),   
("PG_TOK",0))    

# Start an API instance from the core pXar library
api = PyPxarCore(usbId="*",logLevel="DEBUGAPI")
print api.getVersion()
if not api.initTestboard(pg_setup = pg_setup, 
                         power_settings = power_settings, 
                         sig_delays = sig_delays):
    print "WARNING: could not init DTB -- possible firmware mismatch."
    print "Please check if a new FW version is available"
    exit

tbmDACs = [{
        "clear":0xF0,       # Init TBM, Reset ROC
        "counters":0x01,    # Disable PKAM Counter
        "mode":0xC0,        # Set Mode = Calibration
        "pkam_set":0x10,    # Set PKAM Counter
        "delays":0x00,      # Set Delays
        "temperature": 0x00 # Turn off Temperature Measurement
        }]

print "Have DAC config for " + str(len(tbmDACs)) + " TBMs:"
for idx, tbmDAC in enumerate(tbmDACs):
    for key in tbmDAC:
        print "  TBM dac: " + str(key) + " = " + str(tbmDAC[key])


dacs = {
    "Vdig":7,
    "Vana":84,
    "Vsf":30,
    "Vcomp":12,
    "VwllPr":60,
    "VwllSh":60,
    "VhldDel":230,
    "Vtrim":29,
    "VthrComp":86,
    "VIBias_Bus":1,
    "Vbias_sf":6,
    "VoffsetOp":40,
    "VOffsetRO":129,
    "VIon":120,
    "Vcomp_ADC":100,
    "VIref_ADC":91,
    "VIbias_roc":150,
    "VIColOr":50,
    "Vcal":220,
    "CalDel":122,
    "CtrlReg":4,
    "WBC":100
    }

pixels = list()
for column in range(0, 51):
    for row in range(0, 79):
        p = PixelConfig(column,row,15)
        p.mask = False
        pixels.append(p)

print "And we have just initialized " + str(len(pixels)) + " pixel configs to be used for every ROC!"

rocPixels = list()
rocDacs = list()
for rocs in range(0,15):
    rocPixels.append(pixels)
    rocDacs.append(dacs)

api.initDUT(0,"tbm08",tbmDACs,"psi46digv2",rocDacs,rocPixels)

api.testAllPixels(True)
print " enabled all pixels"
#api.getPulseheightVsDAC("vcal", 15, 32)

PxarCoreCmd(api).cmdloop()
