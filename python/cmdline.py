#!/usr/bin/env python2
"""
Python Command Line Interface to the pxar API.
"""
import PyPxarCore
from PyPxarCore import *
from numpy import set_printoptions, nan, zeros
from pxar_helpers import * # arity decorator, PxarStartup, PxarConfigFile, PxarParametersFile and others

# Try to import ROOT:
guiAvailable = True
try:
    import ROOT
    ROOT.PyConfig.IgnoreCommandLineOptions = True
    from pxar_gui import PxarGui
    from pxar_plotter import Plotter
except ImportError:
    guiAvailable = False;
    pass

import cmd      # for command interface and parsing
import os # for file system cmds
import sys

# set up the DAC and probe dictionaries
dacdict = PyRegisterDictionary()
probedict = PyProbeDictionary()
    
class PxarCoreCmd(cmd.Cmd):
    """Simple command processor for the pxar core API."""

    def __init__(self, api, gui):
        cmd.Cmd.__init__(self)
        self.fullOutput=False
        self.prompt = "pxarCore =>> "
        self.intro  = "Welcome to the pxar core console!"  ## defaults to None
        self.api = api
        self.window = None
        if(gui and guiAvailable):
            self.window = PxarGui(ROOT.gClient.GetRoot(),800,800)

    def plot_eventdisplay(self,data):
        pixels = list()
        # Multiple events:
        if(isinstance(data,list)):
            if(not self.window):
                for evt in data:
                    print evt
                return
            for evt in data:
                for px in evt.pixels:
                    pixels.append(px)
        else:
            if(not self.window):
                print data
                return
            for px in data.pixels:
                pixels.append(px)
        self.plot_map(pixels,'Event Display',True)

    def plot_map(self,data,name,count=False):
        if(not self.window):
            print data
            return

        # Find number of ROCs present:
        module = False
        for px in data:
            if px.roc > 0:
                module = True
                break

        # Prepare new numpy matrix:
        d = zeros((416 if module else 52,160 if module else 80))

        for px in data:
            xoffset = 52*(px.roc%8) if module else 0
            yoffset = 80*int(px.roc/8) if module else 0
            # Flip the ROCs upside down:
            y = (px.row + yoffset) if (px.roc < 8) else (2*yoffset - px.row - 1)
            # Reverse order of the upper ROC row:
            x = (px.column + xoffset) if (px.roc < 8) else (415 - xoffset - px.column)
            d[x][y] += 1 if count else px.value

        plot = Plotter.create_th2(d, 0, 415 if module else 51, 0, 159 if module else 79, name, 'pixels x', 'pixels y', name)
        self.window.histos.append(plot)
        self.window.update()

    def plot_1d(self,data,name,dacname,min,max):
        if(not self.window):
            print_data(self.fullOutput,data,(max-min)/len(data))
            return

        # Prepare new numpy matrix:
        d = zeros(len(data))
        for idac, dac in enumerate(data):
            if(dac):
                d[idac] = dac[0].value

        plot = Plotter.create_th1(d, min, max, name, dacname, name)
        self.window.histos.append(plot)
        self.window.update()

    def plot_2d(self,data,name,dac1,step1,min1,max1,dac2,step2,min2,max2):
        if(not self.window):
            for idac, dac in enumerate(data):
                dac1 = min1 + (idac/((max2-min2)/step2+1))*step1
                dac2 = min2 + (idac%((max2-min2)/step2+1))*step2
                s = "DACs " + str(dac1) + ":" + str(dac2) + " - "
                for px in dac:
                    s += str(px)
                print s
            return
        
        # Prepare new numpy matrix:
        bins1 = (max1-min1)/step1+1
        bins2 = (max2-min2)/step2+1
        d = zeros((bins1,bins2))

        for idac, dac in enumerate(data):
            if(dac):
                bin1 = (idac/((max2-min2)/step2+1))
                bin2 = (idac%((max2-min2)/step2+1))
                d[bin1][bin2] = dac[0].value

        plot = Plotter.create_th2(d, min1, max1, min2, max2, name, dac1, dac2, name)
        self.window.histos.append(plot)
        self.window.update()

    def do_EOF(self, line):
        """ clean exit when receiving EOF (Ctrl-D) """
        return True

    def do_gui(self, line):
        """Open the ROOT results browser"""
        if not guiAvailable:
            print "No GUI available (missing ROOT library)"
            return
        if self.window:
            return
        self.window = PxarGui( ROOT.gClient.GetRoot(), 800, 800 )

    def do_switchFullOutput(self, line):
        """Switch between full and suppressed output of all pixels"""
        if self.fullOutput:
            set_printoptions(threshold=1000)
            self.fullOutput = False
        else:
            set_printoptions(threshold=nan)
            self.fullOutput = True

    @arity(0,0,[]) # decorator for argument verification defined in pxar_helpers.py
    def do_getVersion(self):
        """getVersion: returns the pxarcore library version"""
        print self.api.getVersion()
        
    def complete_getVersion(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_getVersion.__doc__, '']

    @arity(0,0,[])
    def do_status(self):
        """status: returns the pxarcore library status"""
        print self.api.status()
        
    def complete_status(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_status.__doc__, '']

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

    @arity(1,1,[str])
    def do_loadscript(self, filename):
        """loadscript [filename]: loads a list of commands to be executed on the pxar cmdline"""
        try:
            f = open(filename)
        except IOError:
            print "Error: cannot open file '" + filename + "'"
        try:
            for line in f:
                if not line.startswith("#") and not line.isspace():
                    print line.replace('\n', ' ').replace('\r', '')
                    self.onecmd(line)
        finally:
            f.close()
        
    def complete_loadscript(self, text, line, start_index, end_index):
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

    @arity(1,1,[int])
    def do_setExternalClock(self, enable):
        """setExternalClock [enable]: enables the external DTB clock input, switches off the internal clock. Only switches if external clock is present."""
        if self.api.setExternalClock(enable) is True:
            print "Switched to " + ("external" if enable else "internal") + " clock."
        else:
            print "Could not switch to " + ("external" if enable else "internal") + " clock!"

    def complete_setExternalClock(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_setExternalClock.__doc__, '']

    @arity(3,3,[int,int,int])
    def do_setClockStretch(self, src, delay, width):
        """setClockStretch [src] [delay] [width]: enables the clock stretch mechanism with the parameters given."""
        self.api.setClockStretch(src,delay,width)

    def complete_setClockStretch(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_setClockStretch.__doc__, '']

    @arity(0,0,[])
    def do_daqStart(self):
        """daqStart: starts a new DAQ session"""
        self.api.daqStart()

    def complete_daqStart(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_daqStart.__doc__, '']

    @arity(0,0,[])
    def do_daqStatus(self):
        """daqStatus: reports status of the running DAQ session"""
        if self.api.daqStatus(): 
            print "DAQ session is fine"
        else:
            print "DAQ session returns faulty state"

    def complete_daqStatus(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_daqStatus.__doc__, '']

    @arity(0,0,[])
    def do_daqStop(self):
        """daqStop: stops the running DAQ session"""
        self.api.daqStop()

    def complete_daqStop(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_daqStop.__doc__, '']

    @arity(1,2,[int, int])
    def do_daqTrigger(self, ntrig, period = 0):
        """daqTrigger [ntrig] [period = 0]: sends ntrig patterns to the device"""
        self.api.daqTrigger(ntrig,period)

    def complete_daqTrigger(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_daqTrigger.__doc__, '']

    @arity(1,1,[int])
    def do_daqTriggerLoop(self, period):
        """daqTriggerLoop [period]: starts trigger loop with given period (in BC/40MHz)"""
        self.api.daqTriggerLoop(period)

    def complete_daqTriggerLoop(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_daqTriggerLoop.__doc__, '']

    @arity(0,0,[])
    def do_daqTriggerLoopHalt(self):
        """daqTriggerLoopHalt: stops the trigger loop"""
        self.api.daqTriggerLoopHalt()

    def complete_daqTriggerLoopHalt(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_daqTriggerLoopHalt.__doc__, '']

    @arity(0,0,[])
    def do_daqGetEvent(self):
        """daqGetEvent: read one event from the event buffer"""
        data = self.api.daqGetEvent()
        self.plot_eventdisplay(data)

    def complete_daqGetEvent(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_daqGetEvent.__doc__, '']

    @arity(0,0,[])
    def do_daqGetEventBuffer(self):
        """daqGetEventBuffer: read all decoded events from the DTB buffer"""
        data = self.api.daqGetEventBuffer()
        self.plot_eventdisplay(data)

    def complete_daqGetEventBuffer(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_daqGetEventBuffer.__doc__, '']

    @arity(0,0,[])
    def do_daqGetRawEvent(self):
        """daqGetRawEvent: read one raw event from the event buffer"""
        dat = self.api.daqGetRawEvent()
        s = ""
        for i in dat:
            s += '{:03x}'.format(i) + " "
        print s

    def complete_daqGetRawEvent(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_daqGetRawEvent.__doc__, '']

    @arity(0,0,[])
    def do_daqGetBuffer(self):
        """daqGetBuffer: read full raw data DTB buffer"""
        dat = self.api.daqGetBuffer()
        s = ""
        for i in dat:
            if i & 0x0FF0 == 0x07f0:
                s += "\n"
            s += '{:04x}'.format(i) + " "
        print s

    def complete_daqGetBuffer(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_daqGetBuffer.__doc__, '']

    @arity(0,0,[])
    def do_daqGetReadback(self):
        """daqGetReadback: return all ROC readback values for the last DAQ session"""
        dat = self.api.daqGetReadback()
        for iroc, roc in enumerate(dat):
            print "ROC " + str(iroc) + ":"
            s = ""
            for i in roc:
                s += '{:04x}'.format(i) + " "
            print s

    def complete_daqGetReadback(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_daqGetReadback.__doc__, '']

    @arity(0,2,[int, int])
    def do_getEfficiencyMap(self, flags = 0, nTriggers = 10):
        """getEfficiencyMap [flags = 0] [nTriggers = 10]: returns the efficiency map"""
        data = self.api.getEfficiencyMap(flags,nTriggers)
        self.plot_map(data,"Efficiency")
        
    def complete_getEfficiencyMap(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_getEfficiencyMap.__doc__, '']

    @arity(0,2,[int, int])
    def do_getPulseheightMap(self, flags = 0, nTriggers = 10):
        """getPulseheightMap [flags = 0] [nTriggers = 10]: returns the pulseheight map"""
        data = self.api.getPulseheightMap(flags,nTriggers)
        self.plot_map(data,"Pulseheight")
        
    def complete_getPulseheightMap(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_getPulseheightMap.__doc__, '']

    @arity(1,7,[str, int, int, int, int, int, int])
    def do_getThresholdMap(self, dacname, dacstep = 1, dacmin = 0, dacmax = 255, threshold = 50, flags = 0, nTriggers = 10):
        """getThresholdMap [DAC name] [step size] [min] [max] [threshold] [flags = 0] [nTriggers = 10]: returns the threshold map for the given DAC"""
        data = self.api.getThresholdMap(dacname,dacstep,dacmin,dacmax,threshold,flags,nTriggers)
        self.plot_map(data,"Threshold " + dacname)
        
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

    @arity(4,6,[str, int, int, int, int, int])
    def do_getPulseheightVsDAC(self, dacname, dacstep, dacmin, dacmax, flags = 0, nTriggers = 10):
        """getPulseheightVsDAC [DAC name] [step size] [min] [max] [flags = 0] [nTriggers = 10]: returns the pulseheight over a 1D DAC scan"""
        data = self.api.getPulseheightVsDAC(dacname, dacstep, dacmin, dacmax, flags, nTriggers)
        self.plot_1d(data,"Pulseheight",dacname,dacmin,dacmax)

    def complete_getPulseheightVsDAC(self, text, line, start_index, end_index):
        if text and len(line.split(" ")) <= 2: # first argument and started to type
            # list matching entries
            return [dac for dac in dacdict.getAllROCNames()
                        if dac.startswith(text)]
        else:
            if len(line.split(" ")) > 2:
                # return help for the cmd
                return [self.do_getPulseheightVsDAC.__doc__, '']
            else:
                # return all DACS
                return dacdict.getAllROCNames()

    @arity(4,6,[str, int, int, int, int, int])
    def do_getEfficiencyVsDAC(self, dacname, dacstep, dacmin, dacmax, flags = 0, nTriggers = 10):
        """getEfficiencyVsDAC [DAC name] [step size] [min] [max] [flags = 0] [nTriggers = 10]: returns the efficiency over a 1D DAC scan"""
        data = self.api.getEfficiencyVsDAC(dacname, dacstep, dacmin, dacmax, flags, nTriggers)
        self.plot_1d(data,"Efficiency",dacname,dacmin,dacmax)

    def complete_getEfficiencyVsDAC(self, text, line, start_index, end_index):
        if text and len(line.split(" ")) <= 2: # first argument and started to type
            # list matching entries
            return [dac for dac in dacdict.getAllROCNames()
                        if dac.startswith(text)]
        else:
            if len(line.split(" ")) > 2:
                # return help for the cmd
                return [self.do_getEfficiencyVsDAC.__doc__, '']
            else:
                # return all DACS
                return dacdict.getAllROCNames()

    @arity(8,11,[str, int, int, int, str, int, int, int, int, int, int])
    def do_getThresholdVsDAC(self, dac1name, dac1step, dac1min, dac1max, dac2name, dac2step, dac2min, dac2max, threshold = 50, flags = 0, nTriggers = 10):
        """getThresholdVsDAC [DAC1 name] [step size 1] [min 1] [max 1] [DAC2 name] [step size 2] [min 2] [max 2] [threshold = 50] [flags = 0] [nTriggers = 10]: returns the threshold for DAC1 over a 1D DAC2 scan"""
        data = self.api.getThresholdVsDAC(dac1name, dac1step, dac1min, dac1max, dac2name, dac2step, dac2min, dac2max, threshold, flags, nTriggers)
        self.plot_1d(data,"Threshold " + dac1name,dac2name,dac2min,dac2max)

    def complete_getThresholdVsDAC(self, text, line, start_index, end_index):
        if text and len(line.split(" ")) <= 2: # first argument and started to type
            # list matching entries
            return [dac for dac in dacdict.getAllROCNames()
                        if dac.startswith(text)]
        elif text and len(line.split(" ")) == 6:
            # list matching entries
            return [dac for dac in dacdict.getAllROCNames()
                    if dac.startswith(text)]
        else:
            if (len(line.split(" ")) > 2 and len(line.split(" ")) < 6) or len(line.split(" ")) > 6:
                # return help for the cmd
                return [self.do_getThresholdVsDAC.__doc__, '']
            else:
                # return all DACS
                return dacdict.getAllROCNames()

    @arity(8,10,[str, int, int, int, str, int, int, int, int, int])
    def do_getPulseheightVsDACDAC(self, dac1name, dac1step, dac1min, dac1max, dac2name, dac2step, dac2min, dac2max, flags = 0, nTriggers = 10):
        """getPulseheightVsDACDAC [DAC1 name] [step size 1] [min 1] [max 1] [DAC2 name] [step size 2] [min 2] [max 2] [flags = 0] [nTriggers = 10]: returns the pulseheight over a 2D DAC1-DAC2 scan"""
        data = self.api.getPulseheightVsDACDAC(dac1name, dac1step, dac1min, dac1max, dac2name, dac2step, dac2min, dac2max, flags, nTriggers)
        self.plot_2d(data,"Pulseheight",dac1name, dac1step, dac1min, dac1max, dac2name, dac2step, dac2min, dac2max)

    def complete_getPulseheightVsDACDAC(self, text, line, start_index, end_index):
        if text and len(line.split(" ")) <= 2: # first argument and started to type
            # list matching entries
            return [dac for dac in dacdict.getAllROCNames()
                        if dac.startswith(text)]
        elif text and len(line.split(" ")) == 6:
            # list matching entries
            return [dac for dac in dacdict.getAllROCNames()
                    if dac.startswith(text)]
        else:
            if (len(line.split(" ")) > 2 and len(line.split(" ")) < 6) or len(line.split(" ")) > 6:
                # return help for the cmd
                return [self.do_getPulseheightVsDACDAC.__doc__, '']
            else:
                # return all DACS
                return dacdict.getAllROCNames()

    @arity(8,10,[str, int, int, int, str, int, int, int, int, int])
    def do_getEfficiencyVsDACDAC(self, dac1name, dac1step, dac1min, dac1max, dac2name, dac2step, dac2min, dac2max, flags = 0, nTriggers = 10):
        """getEfficiencyVsDACDAC [DAC1 name] [step size 1] [min 1] [max 1] [DAC2 name] [step size 2] [min 2] [max 2] [flags = 0] [nTriggers = 10]: returns the efficiency over a 2D DAC1-DAC2 scan"""
        data = self.api.getEfficiencyVsDACDAC(dac1name, dac1step, dac1min, dac1max, dac2name, dac2step, dac2min, dac2max, flags, nTriggers)
        self.plot_2d(data,"Efficiency",dac1name, dac1step, dac1min, dac1max, dac2name, dac2step, dac2min, dac2max)

    def complete_getEfficiencyVsDACDAC(self, text, line, start_index, end_index):
        if text and len(line.split(" ")) <= 2: # first argument and started to type
            # list matching entries
            return [dac for dac in dacdict.getAllROCNames()
                        if dac.startswith(text)]
        elif text and len(line.split(" ")) == 6:
            # list matching entries
            return [dac for dac in dacdict.getAllROCNames()
                    if dac.startswith(text)]
        else:
            if (len(line.split(" ")) > 2 and len(line.split(" ")) < 6) or len(line.split(" ")) > 6:
                # return help for the cmd
                return [self.do_getEfficiencyVsDACDAC.__doc__, '']
            else:
                # return all DACS
                return dacdict.getAllROCNames()

    @arity(2,2,[str, int])
    def do_setSignalMode(self, signal, mode):
        """setSignalMode [signal] [mode]: Set the DTB signal to given mode"""
        self.api.setSignalMode(signal, mode)

    def complete_setSignalMode(self, text, line, start_index, end_index):
        if text and len(line.split(" ")) <= 2: # first argument and started to type
            # list matching entries
            return [sig for sig in dacdict.getAllDTBNames()
                        if sig.startswith(text)]
        else:
            if len(line.split(" ")) > 2:
                # return help for the cmd
                return [self.do_setSignalMode.__doc__, '']
            else:
                # return all signals
                return dacdict.getAllDTBNames()

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
            p = "".join(line.split(" ")[1:2])
            if text: # started to type
                if p.startswith("a"):
                    return [pr for pr in probedict.getAllAnalogNames()
                            if pr.startswith(text)]
                elif p.startswith("d"):
                    return [pr for pr in probedict.getAllDigitalNames()
                            if pr.startswith(text)]
                else:
                    return [self.do_SignalProbe.__doc__, '']
            else:
                # return all signals:
                if p.startswith("a"):
                    return probedict.getAllAnalogNames()
                elif p.startswith("d"):
                    return probedict.getAllDigitalNames()
                else:
                    return [self.do_SignalProbe.__doc__, '']
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

    @arity(2,3,[str, int, int])
    def do_setTbmReg(self, regname, value, tbmid = None):
        """setTbmReg [register name] [value] [TBM ID]: Set the register to given value for given TBM ID"""
        self.api.setTbmReg(regname, value, tbmid)

    def complete_setTbmReg(self, text, line, start_index, end_index):
        if text and len(line.split(" ")) <= 2: # first argument and started to type
            # list matching entries
            return [reg for reg in dacdict.getAllTBMNames()
                        if reg.startswith(text)]
        else:
            if len(line.split(" ")) > 2:
                # return help for the cmd
                return [self.do_setTbmReg.__doc__, '']
            else:
                # return all registers
                return dacdict.getAllTBMNames()


    @arity(1,1,[int])
    def do_getTbmDACs(self, tbmid):
        """getTbmDACs [id]: get the currently programmed register settings for TBM #id"""
        print self.api.getTbmDACs(tbmid)

    def complete_getTbmDACs(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_getTbmDACs.__doc__, '']

    @arity(0,0,[])
    def do_info(self):
        """info: print pxarCore DUT info"""
        self.api.info()

    def complete_info(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_info.__doc__, '']

    @arity(2,2,[int, int])
    def do_setROCEnable(self, rocid, enable):
        """setROCEnable [ROC id] [enable]: enable/disable the ROC with given ID"""
        self.api.setROCEnable(rocid,enable)

    def complete_setROCEnable(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_setROCEnable.__doc__, '']

    @arity(2,2,[int, int])
    def do_setTBMEnable(self, tbmid, enable):
        """setTBMEnable [ROC id] [enable]: enable/disable the ROC with given ID"""
        self.api.setTBMEnable(tbmid,enable)

    def complete_setTBMEnable(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_setTBMEnable.__doc__, '']

    @arity(3,4,[int, int, int, int])
    def do_testPixel(self, col, row, enable, rocid = None):
        """testPixel [column] [row] [enable] [ROC id]: enable/disable testing of pixel"""
        self.api.testPixel(col,row,enable,rocid)

    def complete_testPixel(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_testPixel.__doc__, '']

    @arity(1,2,[int, int])
    def do_testAllPixels(self, enable, rocid = None):
        """testAllPixels [enable] [rocid]: enable/disable tesing for all pixels on given ROC"""
        self.api.testAllPixels(enable,rocid)

    def complete_testAllPixels(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_testAllPixels.__doc__, '']

    @arity(3,4,[int, int, int, int])
    def do_maskPixel(self, col, row, enable, rocid = None):
        """maskPixel [column] [row] [enable] [ROC id]: mask/unmask pixel"""
        self.api.maskPixel(col,row,enable,rocid)
        
    def complete_maskPixel(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_maskPixel.__doc__, '']

    @arity(1,2,[int, int])
    def do_maskAllPixels(self, enable, rocid = None):
        """maskAllPixels [enable] [rocid]: mask/unmask all pixels on given ROC"""
        self.api.maskAllPixels(enable,rocid)
        
    def complete_maskAllPixels(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_maskAllPixels.__doc__, '']

    @arity(1,1,[int])
    def do_getNEnabledPixels(self, rocid):
        """getNEnabledPixels [ROC id]: returns number of enabled pixels for ROC id"""
        print self.api.getNEnabledPixels(rocid)

    def complete_getNEnabledPixels(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_getNEnabledPixels.__doc__, '']

    @arity(0,0,[])
    def do_getTbmType(self):
        """getTbmType: returns device code for the TBM programmed"""
        print self.api.getTbmType()

    def complete_getTbmType(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_getTbmType.__doc__, '']

    @arity(0,0,[])
    def do_getRocType(self):
        """getRocType: returns device code for the ROCs programmed"""
        print self.api.getRocType()

    def complete_getRocType(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_getRocType.__doc__, '']

    @arity(1,1,[int])
    def do_getNMaskedPixels(self, rocid):
        """getNMaskedPixels [ROC id]: returns number of masked pixels for ROC id"""
        print self.api.getNMaskedPixels(rocid)

    def complete_getNMaskedPixels(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_getNMaskedPixels.__doc__, '']

    def do_quit(self, arg):
        """quit: terminates the application"""
        sys.exit(1)

    # shortcuts
    do_q = do_quit

def main(argv=None):
    if argv is None:
        argv = sys.argv
        progName = os.path.basename(argv.pop(0))

    # command line argument parsing
    import argparse
    parser = argparse.ArgumentParser(prog=progName, description="A Simple Command Line Interface to the pxar API.")
    parser.add_argument('--dir', '-d', metavar="DIR", help="The directory with all required config files.")
    parser.add_argument('--verbosity', '-v', metavar="LEVEL", default="INFO", help="The output verbosity set in the pxar API.")
    parser.add_argument('--gui', '-g', action="store_true", help="The output verbosity set in the pxar API.")
    parser.add_argument('--load', metavar="FILE", help="Load a cmdline script to be executed before entering the prompt.")
    args = parser.parse_args(argv)

    api = PxarStartup(args.dir,args.verbosity)

    # start the cmd line
    prompt = PxarCoreCmd(api,args.gui)
    # run the startup script if requested
    if args.load:
        prompt.do_loadscript(args.load)
    # start user interaction
    prompt.cmdloop()

if __name__ == "__main__":
    sys.exit(main())
