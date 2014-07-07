import PyPxarCore
from PyPxarCore import Pixel, PixelConfig, PyPxarCore, PyDictionary


import cmd      # for command interface and parsing
import argparse # for command line option parsing
import sys

# TODO: extend dict to get ROC_REG and filter for 'false' state
dacdict = PyDictionary()

class ArgumentParserError(Exception):
    def __init__(self, msg):
        self.msg = msg

# override the default exit of argparse when encountering wrong options:
class ThrowingArgumentParser(argparse.ArgumentParser):
    def error(self, message):
        sys.stderr.write('error: %s\n' % message)
        self.print_help()
        raise ArgumentParserError(message)

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

# TODO: ODER DECORATOR
# see: https://coderwall.com/p/jiczbg
#      http://www.rmi.net/~lutz/rangetest.html
    def do_setDAC(self, line):
        """setDAC [DAC name] [value] [-r [ROCID]]
        Set the DAC to given value for given roc ID"""
        
        parser = ThrowingArgumentParser()
        parser.add_argument("-r", "--roc", "--rocid", help="Set DAC only for ROC with specified ID", default=None, type=int, metavar="ROCID")
        parser.add_argument("dacname", help="Which DAC to set (given by name)")
        parser.add_argument("value", help="The value to set the DAC to", type=int)
        try:
            args = parser.parse_args(line.split())
            self.api.setDAC(args.dacname, args.value, args.roc)
        except ArgumentParserError, e: pass

    def complete_setDAC(self, text, line, start_index, end_index):
        if text:
            return [dac for dac in dacdict.getAllROCNames()
                    if dac.startswith(text)]
        else:
            return DACS

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
        #p.enable = True
        pixels.append(p)

print "And we have just initialized " + str(len(pixels)) + " pixel configs to be used for every ROC!"

rocPixels = list()
rocDacs = list()
for rocs in range(0,15):
    rocPixels.append(pixels)
    rocDacs.append(dacs)

api.initDUT(0,"tbm08",tbmDACs,"psi46digv2",rocDacs,rocPixels)

print "pixel 35 is " + str(pixels[35].enable)

api.testAllPixels(True)
print " enabled all pixels"
#api.getPulseheightVsDAC("vcal", 15, 32)

PxarCoreCmd(api).cmdloop()
