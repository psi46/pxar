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
triggerdict = PyTriggerDictionary()

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
        elif(gui and not guiAvailable):
            print "No GUI available (missing ROOT library)"

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

    def varyDelays(self,tindelay,toutdelay,verbose=False):
        self.api.setTestboardDelays({"tindelay":tindelay,"toutdelay":toutdelay})
        self.api.daqStart()
        self.api.daqTrigger(1, 500)

        try:
            rawEvent = self.api.daqGetRawEvent()
        except RuntimeError:
            pass

        if verbose: print "raw Event:\t\t",rawEvent
        nCount = 0
        for i in rawEvent:
            i = i & 0x0fff
            if i & 0x0800:
                i -= 4096
            rawEvent[nCount] = i
            nCount += 1
        if verbose: print "converted Event:\t",rawEvent
        self.api.daqStop()
        return rawEvent

    def converted_raw_event(self):
        event = []
        try:
            event = self.api.daqGetRawEvent()
        except RuntimeError:
            pass
        count = 0
        for i in event:
            i &= 0x0fff
            if i & 0x0800:
                i -= 4096
            event[count] = i
            count += 1
        return event

    def set_clock(self, value):
        # sets all the delays to the right value if you want to change clk
        self.api.setTestboardDelays({"clk": value})
        self.api.setTestboardDelays({"ctr": value})
        self.api.setTestboardDelays({"sda": value + 15})
        self.api.setTestboardDelays({"tin": value + 5})

    def enable_pix(self, row=5, col=12, roc=0):
        self.api.testAllPixels(0)
        self.api.maskAllPixels(1)
        self.api.testPixel(row, col, 1, roc)
        self.api.maskPixel(row, col, 0, roc)

    def address_level_scan(self):
        self.api.daqStart()
        self.api.daqTrigger(1000, 500)  # choose here how many triggers you want to send (crucial for the time it takes)
        plotdata = zeros(1024)
        try:

            while True:
                pos = -3
                dat = self.api.daqGetRawEvent()
                for i in dat:
                    # REMOVE HEADER
                    i &= 0x0fff
                    # Remove PH from hits:
                    if pos == 5:
                        pos = 0
                        continue
                    # convert negatives
                    if i & 0x0800:
                        i -= 4096
                    plotdata[500 + i] += 1
                    pos += 1
        except RuntimeError:
            pass
        self.api.daqStop()
        return plotdata

##########################################################################################################################

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

    @arity(1,1,[str])
    def do_setReportingLevel(self, logLevel):
        """setReportingLevel [logLevel]: Set another Reporting Level"""
        self.api.setReportingLevel(logLevel)

    @arity(0,0,[])
    def do_getReportingLevel(self):
        """getReportingLevel: Print the Reporting Level"""
        self.api.getReportingLevel()


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
    def do_run(self, filename):
        """run [filename]: loads a list of commands to be executed on the pxar cmdline"""
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

    def complete_run(self, text, line, start_index, end_index):
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

    @arity(0,1,[int])
    def do_daqStart(self, flags = 0):
        """daqStart: starts a new DAQ session"""
        self.api.daqStart(flags)

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

    @arity(1,2,[str,int])
    def do_daqTriggerSource(self, source, rate = 0):
        """daqTriggerSource [source] [rate]: select the trigger source to be used for the DAQ session, the rate is valid for periodic and random triggers"""
        if self.api.daqTriggerSource(source,rate):
            print "Trigger source \"" + source + "\" selected."
        else:
            print "DAQ returns faulty state."

    def complete_daqTriggerSource(self, text, line, start_index, end_index):
        # return help for the cmd
        if text and len(line.split(" ")) <= 2: # first argument and started to type
            # list matching entries
            return [trg for trg in triggerdict.getAllNames()
                        if trg.startswith(text)]
        else:
            if len(line.split(" ")) > 2:
                # return help for the cmd
                return [self.do_daqTriggerSource.__doc__, '']
            else:
                # return all trigger sources
                return triggerdict.getAllNames()

    @arity(1,1,[str])
    def do_daqSingleSignal(self, signal):
        """daqSingleSignal [signal]: send a single signal to the DUT"""
        if self.api.daqSingleSignal(signal):
            print "Trigger signal \"" + signal + "\" sent to DUT."
        else:
            print "Trigger signal lookup failed."

    def complete_daqSingleSignal(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_daqSingleSignal.__doc__, '']

    @arity(0,0,[])
    def do_daqStop(self):
        """daqStop: stops the running DAQ session"""
        self.api.daqStop()

    def complete_daqStop(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_daqStop.__doc__, '']

    @arity(0,2,[int, float])
    def do_daqTrigger(self, ntrig=1, period=0):
        """daqTrigger [ntrig = 1] [period = 0]: sends ntrig patterns to the device"""
        self.api.daqTrigger(ntrig, int(period))

    def complete_daqTrigger(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_daqTrigger.__doc__, '']

    @arity(0,3,[int, int, int])
    def do_setPG(self, n_trig=5, t=30, tbm_id=0):
        """
        Sets up a multi trigger pattern generator for ROC testing
        :param n_trig: [=5] number of accumulated triggers
        :param t: [=30] time between first two calibrates
        :param tbm_id: [=0]
        """
        print 'Set up pattern generator with {0} calibrates/triggers per loop!'.format(n_trig)
        pgcal = self.api.getRocDACs(tbm_id)['wbc'] + 4
        if n_trig == 1:
            pg_setup = (("PG_RESR", 25), ("PG_CAL",pgcal + 2), ("PG_TRG",16), ("PG_TOK",0))
        else:
            double_cal = (("PG_CAL",pgcal - t + 1), ("PG_CAL", t))
            single_cal = (("PG_TRG",pgcal - 2 * t), ("PG_CAL", t))
            tok_delay = (("PG_TOK", 255), ('DELAY', 255), ('DELAY', 255))
            n_trig -= 2
            pg_setup = (("PG_RESR", 25),) + double_cal + single_cal * n_trig + (("PG_TRG", pgcal - t + 1),) + (("PG_TRG", 255),) + (n_trig + 1) * tok_delay  + (("PG_TOK", 0),)
        try:
            self.api.setPatternGenerator(pg_setup)
        except RuntimeError, err:
            print err

    def complete_setPG(self):
        # return help for the cmd
        return [self.do_setPG.__doc__, '']

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
        try:
            data = self.api.daqGetEvent()
            self.plot_eventdisplay(data)
        except RuntimeError, err:
            print err

    def complete_daqGetEvent(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_daqGetEvent.__doc__, '']

    @arity(0,0,[])
    def do_daqGetEventBuffer(self):
        """daqGetEventBuffer: read all decoded events from the DTB buffer"""
        try:
            data = self.api.daqGetEventBuffer()
            self.plot_eventdisplay(data)
        except RuntimeError:
            pass

    def complete_daqGetEventBuffer(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_daqGetEventBuffer.__doc__, '']

    @arity(0,0,[])
    def do_daqGetRawEvent(self):
        """daqGetRawEvent: read one raw event from the event buffer"""
        try:
            dat = self.api.daqGetRawEvent()
            s = ""
            for i in dat:
                s += '{0:03x}'.format(i) + " "
            print s
        except RuntimeError:
            pass

    def complete_daqGetRawEvent(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_daqGetRawEvent.__doc__, '']

    @arity(0,0,[])
    def do_getStatistics(self):
        """getStatistics: print full statistics accumulated during last DAQ session"""
        dat = self.api.getStatistics()
        dat.dump

    def complete_getStatistics(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_getStatistics.__doc__, '']

    @arity(0,0,[])
    def do_daqGetBuffer(self):
        """daqGetBuffer: read full raw data DTB buffer"""
        try:
            dat = self.api.daqGetBuffer()
            s = ""
            for i in dat:
                if i & 0x0FF0 == 0x07f0:
                    s += "\n"
                    s += '{:04x}'.format(i) + " "
                    print s
        except RuntimeError:
            pass

    def complete_daqGetBuffer(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_daqGetBuffer.__doc__, '']

    @arity(0,0,[])
    def do_daqGetReadback(self):
        """daqGetReadback: return all ROC readback values for the last DAQ session"""
        dat = self.api.daqGetReadback()
        for iroc, roc in enumerate(dat):
            print "ROC " + str(iroc) + ": (" + str(len(roc)) + " values)"
            s = ""
            for i in roc:
                s += '{:04x}'.format(i) + " "
            print s

    def complete_daqGetReadback(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_daqGetReadback.__doc__, '']

    @arity(1,1,[int])
    def do_daqGetXORsum(self, channel):
        """daqGetXORsum: return all DESER400 XOR sum values from the last DAQ session fo channel [channel]"""
        dat = self.api.daqGetXORsum(channel)
        print dat

    def complete_daqGetXORsum(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_daqGetXORsum.__doc__, '']

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

    @arity(0,0,[])
    def do_analogLevelScan(self):
        """analogLevelScan: scan the ADC levels of an analog ROC"""
        self.api.daqStart()
        self.api.daqTrigger(5000,500)
        plotdata = self.address_level_scan()
        plot = Plotter.create_th1(plotdata, -512, +512, "Address Levels", "ADC", "#")
        self.window.histos.append(plot)
        self.window.update()

    def complete_analogLevelScan(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_analogLevelScan.__doc__, '']

    @arity(2,3,[str, str, int])
    def do_setSignalMode(self, signal, mode, speed = 0):
        """setSignalMode [signal] [mode] [speed]: Set the DTB signal to given mode (normal, low, high, random). The [speed] parameter is only necessary for random signal mode."""
        self.api.setSignalMode(signal, mode, speed)

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

    @arity(2,3,[str, str, int])
    def do_SignalProbe(self, probe, name, channel = 0):
        """SignalProbe [probe] [name] [channel]: Switches DTB probe output [probe] to signal [name]. The [channel] parameter can be used to select the deserializer DAQ channel"""
        self.api.SignalProbe(probe,name, channel)

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

    @arity(2,3,[str, int, int])
    def do_setTbmReg(self, regname, value, tbmid = None):
        """setTbmReg [Reg. name] [value] [TBMID]: Set the register to given value for given TBM ID"""
        self.api.setTbmReg(regname, value, tbmid)

    def complete_setTbmReg(self, text, line, start_index, end_index):
        if text and len(line.split(" ")) <= 2: # first argument and started to type
            # list matching entries
            return [dac for dac in dacdict.getAllTBMNames()
                        if dac.startswith(text)]
        else:
            if len(line.split(" ")) > 2:
                # return help for the cmd
                return [self.do_setTbmReg.__doc__, '']
            else:
                # return all DACS
                return dacdict.getAllTBMNames()

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

    @arity(0,1,[int])
    def do_getRocDACs(self, tbmid=0):
        """getRocDACs [id]: get the currently programmed register/DAC settings for ROC #id"""
        dacs = self.api.getRocDACs(tbmid)
        for dac, value in dacs.iteritems():
            print '{dac}: {value}'.format(dac=dac.rjust(10), value=value)
        return dacs

    def complete_getRocDACs(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_getRocDACs.__doc__, '']

    @arity(0,0,[])
    def do_info(self):
        """info: print pxarCore DUT info"""
        self.api.info()

    def complete_info(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_info.__doc__, '']

    @arity(0,0,[])
    def do_getEnabledRocIDs(self):
        """info: print ROC IDs of all enabled ROCs"""
        data = self.api.getEnabledRocIDs()
        print data

    def complete_getEnabledRocIDs(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_getEnabledRocIDs.__doc__, '']

    @arity(0,0,[])
    def do_getEnabledRocI2Caddr(self):
        """info: print ROC IDs of all enabled ROCs"""
        data = self.api.getEnabledRocI2Caddr()
        print data

    def complete_getEnabledRocI2Caddr(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_getEnabledRocI2Caddr.__doc__, '']

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

    @arity(0,0,[])
    def do_findAnalogueTBDelays(self):
        """findAnalogueTBDelays: configures tindelay and toutdelay"""
        print ""
        bestTin = 10    #default value if algorithm should fail
        print "scan tindelay:"
        print "tindelay\ttoutdelay\trawEvent[0]"
        for tin in range(5,20):
            rawEvent = self.varyDelays(tin, 20,verbose=False)
            print str(tin)+"\t\t20\t\t"+str(rawEvent[0])
            if (rawEvent[0] < -100):    #triggers for UB, the first one should always be UB
                bestTin = tin
                break
        print ""
        bestTout = 20   #default value if algorithm should fail
        tout = bestTin+5
        print "scan toutdelay"
        print "tindelay\ttoutdelay\trawEvent[-1]"
        for i in range (15):
            rawEvent = self.varyDelays(bestTin, tout,verbose=False)
            print str(bestTin)+"\t\t"+str(tout)+"\t\t"+str(rawEvent[-1])
            if rawEvent[-1] > 20:   #triggers for PH, the last one should always be a pos PH
                bestTout = tout
                break
            tout -= 1
        print ""
        self.api.setTestboardDelays({"tindelay":bestTin,"toutdelay":bestTout})
        print "set tindelay to:  ", bestTin
        print "set toutdelay to: ", bestTout

    def complete_findAnalogueTBDelays(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_findAnalogueTBDelays.__doc__, '']

    @arity(0,4,[int, int, int, str])
    def do_wbcScan(self, minWBC = 90, maxWBC = 255, maxTriggers = 10, triggersignal = "extern"):
        """ do_wbcScan [minWBC] [maxWBC] [maxTriggers] [signal]: sets the values of wbc from minWBC until it finds the wbc which has more than 90% filled events or it reaches 255 (default minWBC 90)"""

        self.api.daqTriggerSource(triggersignal)
        self.api.HVon()

        wbcScan = []
        print "wbc \t yield \t err \t pixel"

        # loop over wbc
        for wbc in range (minWBC,maxWBC):
            self.api.setDAC("wbc", wbc)
            self.api.daqStart()
            nHits       = 0
            nTriggers   = 0

            #loop until you find maxTriggers
            while nTriggers < maxTriggers:
                try:
                    data = self.api.daqGetEvent()
                    if len(data.pixels) > 0:
                       nHits += 1
                    nTriggers += 1
                except RuntimeError:
                    pass

            hitYield = 100*nHits/maxTriggers
            stats = self.api.getStatistics()
            wbcScan.append(hitYield)
            print '{0:03d}'.format(wbc),"\t", '{0:3.0f}%'.format(hitYield),"\t", '{0:3d}'.format(stats.errors),"\t", '{0:3d}'.format(stats.info_pixels_valid)

            self.api.daqStop()

        # Turn HV off again.
        self.api.HVoff()

        if(self.window):
            self.window = PxarGui( ROOT.gClient.GetRoot(), 1000, 800 )
            plot = Plotter.create_tgraph(wbcScan, "wbc scan", "wbc", "evt/trig [%]", minWBC)
            self.window.histos.append(plot)
            self.window.update()

    def complete_wbcScan(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_wbcScan.__doc__, '']

    @arity(0,4,[int, int, int, str])
    def do_latencyScan(self, minlatency = 50, maxlatency = 100, triggers = 10, triggersignal = "extern"):
        """ do_latencyScan [min] [max] [triggers] [signal]: scan the trigger latency from min to max with set number of triggers)"""

        self.api.testAllPixels(0,None)
        self.api.HVon()

        latencyScan = []
        print "latency \tyield"

        # loop over latency
        for latency in range (minlatency,maxlatency):
            delay = {}
            delay["triggerlatency"] = latency
            self.api.setTestboardDelays(delay)
            self.api.daqTriggerSource(triggersignal)
            self.api.daqStart()
            nHits       = 0
            nTriggers   = 0

            #loop until you find maxTriggers
            while nTriggers < triggers:
                try:
                    data = self.api.daqGetEvent()
                    if len(data.pixels) > 0:
                       nHits += 1
                    nTriggers += 1
                except RuntimeError:
                    pass

            hitYield = 100*nHits/triggers
            latencyScan.append(hitYield)
            print '{0:03d}'.format(latency),"\t", '{0:3.0f}%'.format(hitYield)
            self.api.daqStop()

        if(self.window):
            self.window = PxarGui( ROOT.gClient.GetRoot(), 1000, 800 )
            plot = Plotter.create_tgraph(latencyScan, "latency scan", "trigger latency", "evt/trig [%]", minlatency)
            self.window.histos.append(plot)
            self.window.update()

    def complete_latencyScan(self, text, line, start_index, end_index):
        # return help for the cmd
        return [self.do_latencyScan.__doc__, '']

    @arity(0, 3, [int, int, int])
    def do_find_clk_delay(self, n_rocs=1, min_val=0, max_val=25):
        """find the best clock delay setting """
        # variable declarations
        cols = [0, 2, 4, 6, 8, 10]
        rows = [44, 41, 38, 35, 32, 29]  # special pixel setting for splitting
        n_triggers = 100
        n_levels = len(cols)
        clk_x = []
        levels_y = []
        mean_value = []
        spread_black = []
        header = [0, 0]

        # find the address levels
        print "get level splitting:"
        for roc in range(n_rocs):
            self.api.maskAllPixels(1, roc)
            self.api.testAllPixels(0, roc)
            levels_y.append([])
            mean_value.append([])
            spread_black.append([])
            for i in range(len(cols)):
                levels_y[roc].append([])
                mean_value[roc].append(0)
                self.api.testPixel(cols[i], rows[i], 1, roc)
                self.api.maskPixel(cols[i], rows[i], 0, roc)
            # active pixel for black level spread
            self.api.testPixel(15, 59, 1, roc)
            self.api.maskPixel(15, 59, 0, roc)

            for clk in range(min_val, max_val):
                # clear mean values
                for i in range(n_levels):
                    mean_value[roc][i] = 0
                if not roc:
                    clk_x.append(clk)
                self.set_clock(clk)
                self.api.daqStart()
                self.api.daqTrigger(n_triggers, 500)
                sum_spread = 0
                for k in range(n_triggers):
                    event = self.converted_raw_event()
                    # black level spread
                    spread_j = 0
                    for j in range(5):
                        try:
                            spread_j += abs(event[1 + roc * 3] - event[3 + roc * 3 + n_levels * 6 + j])
                        except IndexError:
                            spread_j = 99
                            break
                    sum_spread += spread_j / 5
                    # level split
                    stop_loop = False
                    for j in range(len(cols)):
                        try:
                            mean_value[roc][j] += event[5 + roc * 3 + j * 6]
                        except IndexError:
                            mean_value[roc][j] = 0
                            stop_loop = True
                            break
                    if stop_loop:
                        break
                spread_black[roc].append(sum_spread / float(n_triggers))
                for i in range(n_levels):
                    levels_y[roc][i].append(mean_value[roc][i] / float(n_triggers))
                print '\rclk-delay:', "{0:2d}".format(clk), 'black lvl spread: ', "{0:2.2f}".format(spread_black[roc][clk]),
                sys.stdout.flush()
                self.api.daqStop()
            self.api.maskAllPixels(1, roc)
            self.api.testAllPixels(0, roc)
        print

        # find the best phase
        spread = []
        for i in range(len(levels_y[0][0])):
            sum_level = 0
            sum_spread = 0
            for j in range(n_levels):
                sum_level += levels_y[0][j][i]
            for j in range(n_levels):
                if levels_y[0][j][i] != 0:
                    sum_spread += abs(sum_level / n_levels - levels_y[0][j][i])
                else:
                    sum_spread = 99 * n_levels
                    break
            spread.append(sum_spread / n_levels)
        best_clk = 99
        min_spread = 99
        for i in range(len(spread)):
            if spread[i] < min_spread:
                min_spread = spread[i]
                best_clk = i
        print
        print 'best clk: ', best_clk
        names = ['clk\t\t\t', 'level spread:\t\t', 'black level spread:\t']
        infos = [clk_x, spread, spread_black[0]]
        for i in range(3):
            print names[i],
            for j in range(-2, 3):
                if not i:
                    print infos[i][best_clk + j], '\t',
                else:
                    print '{0:2.2f}'.format(infos[i][best_clk + j]), '\t',
            print
        self.set_clock(best_clk)

        # get an averaged header for the lvl margins
        self.api.daqStart()
        self.api.daqTrigger(n_triggers, 500)
        for i in range(n_triggers):
            event = self.converted_raw_event()
            header[0] += event[0]
            header[1] += event[1]
        self.api.daqStop()
        header[0] /= n_triggers
        header[1] /= n_triggers

        # save the data to file (optional)
        f = open('levels_header.txt', 'w')
        f.write(str(header[0]) + "\n" + str(header[1]))
        f.close()
        file_name = []
        for i_roc in range(n_rocs):
            file_name.append('levels_roc' + str(i_roc) + '.txt')
            f = open(file_name[i_roc], 'w')
            for i in range(n_levels):
                for j in levels_y[i_roc][i]:
                    f.write(str(j) + ' ')
                f.write("\n")
            for i in clk_x:
                f.write(str(i) + ' ')
            f.close()
        print 'saved the levels to file(s)'
        for name in file_name:
            print name

        # plot address levels
        self.enable_pix(5, 12)
        self.window = PxarGui(ROOT.gClient.GetRoot(), 800, 800)
        plotdata = self.address_level_scan()
        plot = Plotter.create_th1(plotdata, -512, +512, "Address Levels", "ADC", "#")
        self.window.histos.append(plot)
        self.window.update()

    def complete_find_clk_delay(self):
        # return help for the cmd
        return [self.do_find_clk_delay.__doc__, '']

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
    parser.add_argument('--run', '-r', metavar="FILE", help="Load a cmdline script to be executed before entering the prompt.")
    args = parser.parse_args(argv)

    api = PxarStartup(args.dir,args.verbosity)

    # start the cmd line
    prompt = PxarCoreCmd(api,args.gui)
    # run the startup script if requested
    if args.run:
        prompt.do_run(args.run)
    # start user interaction
    prompt.cmdloop()

if __name__ == "__main__":
    sys.exit(main())
