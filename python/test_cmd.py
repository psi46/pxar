import PyPxarCore
from PyPxarCore import Pixel, PixelConfig, PyPxarCore, PyRegisterDictionary, PyProbeDictionary
from functools import wraps # used in parameter verification decorator
from numpy import set_printoptions, nan

import cmd      # for command interface and parsing
import os # for file system cmds
import sys

# set up the DAC and probe dictionaries
dacdict = PyRegisterDictionary()
probedict = PyProbeDictionary()

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

def get_possible_filename_completions(text):
    head, tail = os.path.split(text.strip())
    if head == "": #no head
        head = "."
    files = os.listdir(head)
    return [ f for f in files if f.startswith(tail) ]
 
def extract_full_argument(line, endidx):
    newstart = line.rfind(" ", 0, endidx)
    return line[newstart:endidx]



class PxarCoreCmd(cmd.Cmd):
    """Simple command processor for the pxar core API."""
    fullOutput=False

    def __init__(self, api):
        cmd.Cmd.__init__(self)
        self.prompt = "pxar core =>> "
        self.intro  = "Welcome to the pxar core console!"  ## defaults to None
        self.api = api
    
    def do_EOF(self, line):
        """ clean exit when receiving EOF (Ctrl-D) """
        return True

    def do_switchFullOutput(self, line):
        """Switch between full and suppressed output of all pixels"""
        if self.fullOutput:
            set_printoptions(threshold=1000)
            self.fullOutput = False
        else:
            set_printoptions(threshold=nan)
            self.fullOutput = True

    @arity(0,0,[])
    def do_getVersion(self):
        """getVersion: returns the pxarcore library version"""
        print self.api.getVersion()
        
    def complete_getVersion(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_getVersion.__doc__, '']

    @arity(1,1,[str])
    def do_flashTB(self, filename):
        """flashTB [filename]: flash the DTB with new firmware"""
        self.api.flashTB(filename)
        
    def complete_flashTB(self, text, line, start_index, end_index):
        # tab-completion for the file path:
        try:
            # remove specific delimeters from the readline parser
            # to allow completion of filenames with dashes
            import readline
            delims = readline.get_completer_delims( )
            delims = delims.replace('-', '')
            readline.set_completer_delims(delims)
        except ImportError:
            pass
        return get_possible_filename_completions(extract_full_argument(line,end_index))

    @arity(0,0,[])
    def do_HVon(self):
        """HVon: switch High voltage for sensor bias on"""
        self.api.HVon()
        
    def complete_HVon(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_HVon.__doc__, '']

    @arity(0,0,[])
    def do_Poff(self):
        """Pon: switch DTB power output off"""
        self.api.Poff()
        
    def complete_Poff(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_Poff.__doc__, '']

    @arity(0,0,[])
    def do_Pon(self):
        """Pon: switch DTB power output on"""
        self.api.Pon()
        
    def complete_Pon(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_Pon.__doc__, '']

    @arity(0,0,[])
    def do_HVoff(self):
        """HVoff: switch High voltage for sensor bias off"""
        self.api.HVoff()
        
    def complete_HVoff(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_HVoff.__doc__, '']

    @arity(0,0,[])
    def do_getTBia(self):
        """getTBia: returns analog DTB current"""
        print "Analog Current: ", (self.api.getTBia()*1000), " mA"
        
    def complete_getTBia(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_getTBia.__doc__, '']

    @arity(0,0,[])
    def do_getTBva(self):
        """getTBva: returns analog DTB voltage"""
        print "Analog Voltage: ", self.api.getTBva(), " V"
        
    def complete_getTBva(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_getTBva.__doc__, '']

    @arity(0,0,[])
    def do_getTBid(self):
        """getTBid: returns digital DTB current"""
        print "Digital Current: ", (self.api.getTBid()*1000), " mA"
        
    def complete_getTBid(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_getTBid.__doc__, '']

    @arity(0,0,[])
    def do_getTBvd(self):
        """getTBvd: returns digital DTB voltage"""
        print "Digital Voltage: ", self.api.getTBvd(), " V"
        
    def complete_getTBvd(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_getTBvd.__doc__, '']

    @arity(0,2,[int, int])
    def do_getEfficiencyMap(self, flags = 0, nTriggers = 10):
        """getEfficiencyMap [flags = 0] [nTriggers = 10]: returns the efficiency map"""
        print self.api.getEfficiencyMap(flags,nTriggers)
        
    def complete_getEfficiencyMap(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_getEfficiencyMap.__doc__, '']

    @arity(0,2,[int, int])
    def do_getPulseheightMap(self, flags = 0, nTriggers = 10):
        """getPulseheightMap [flags = 0] [nTriggers = 10]: returns the pulseheight map"""
        print self.api.getPulseheightMap(flags,nTriggers)
        
    def complete_getPulseheightMap(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_getPulseheightMap.__doc__, '']

    @arity(1,3,[str, int, int])
    def do_getThresholdMap(self, dacname, dacstep = 1, ddacmin = 0, dacmax = 255, threshold = 50, flags = 0, nTriggers = 10):
        """getPulseheightMap [DAC name] [flags = 0] [nTriggers = 10]: returns the threshold map for the given DAC"""
        print self.api.getThresholdMap(dacname,1,0,255,50,flags,nTriggers)
        
    def complete_getThresholdMap(self, text, line, start_index, end_index):
        if text and len(line.split(" ")) <= 2: # first argument and started to type
            # list matching entries
            return [dac for dac in dacdict.getAllROCNames()
                        if dac.startswith(text)]
        else:
            if len(line.split(" ")) > 2:
                # return help for the cmd
                return [self.do_getThresholdMap.__doc__, '']
            else:
                # return all DACS
                return dacdict.getAllROCNames()

    @arity(2,2,[str, str])
    def do_SignalProbe(self, probe, name):
        """SignalProbe [probe] [name]: Switches DTB probe output [probe] to signal [name]"""
        self.api.SignalProbe(probe,name)

    def complete_SignalProbe(self, text, line, start_index, end_index):
        probes = ["d1","d2","a1","a2"]
        if len(line.split(" ")) <= 2: # first argument
            if text: # started to type
                # list matching entries
                return [pr for pr in probes
                        if pr.startswith(text)]
            else:
                # list all probes
                return probes
        elif len(line.split(" ")) <= 3: # second argument
            if text: # started to type
                return [pr for pr in probedict.getAllNames()
                        if pr.startswith(text)]
            else:
                # return all signals:
                return probedict.getAllNames()
        else:
            # return help for the cmd
            return [self.do_SignalProbe.__doc__, '']

    @arity(2,3,[str, int, int])
    def do_setDAC(self, dacname, value, rocid = None):
        """setDAC [DAC name] [value] [ROCID]: Set the DAC to given value for given roc ID"""
        self.api.setDAC(dacname, value, rocid)

    def complete_setDAC(self, text, line, start_index, end_index):
        if text and len(line.split(" ")) <= 2: # first argument and started to type
            # list matching entries
            return [dac for dac in dacdict.getAllROCNames()
                        if dac.startswith(text)]
        else:
            if len(line.split(" ")) > 2:
                # return help for the cmd
                return [self.do_setDAC.__doc__, '']
            else:
                # return all DACS
                return dacdict.getAllROCNames()

    @arity(1,1,[str])
    def do_getDACRange(self, dacname):
        """getDACRange [DAC name]: Get the valid value range for the given DAC"""
        print "DAC ", dacname, ": 0 -", self.api.getDACRange(dacname)

    def complete_getDACRange(self, text, line, start_index, end_index):
        if text and len(line.split(" ")) <= 2: # first argument and started to type
            # list matching entries
            return [dac for dac in dacdict.getAllROCNames()
                        if dac.startswith(text)]
        else:
            if len(line.split(" ")) > 2:
                # return help for the cmd
                return [self.do_getDACRange.__doc__, '']
            else:
                # return all DACS
                return dacdict.getAllROCNames()

    def do_quit(self, arg):
        """quit: terminates the application"""
        sys.exit(1)

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
api = PyPxarCore(usbId="*",logLevel="INFO")
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
