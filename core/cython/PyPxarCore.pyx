# distutils: language = c++
from libcpp cimport bool
from libc.stdint cimport uint8_t, int8_t, uint16_t, int16_t, int32_t, uint32_t
from libcpp.string cimport string
from libcpp.pair cimport pair
from libcpp.vector cimport vector
from libcpp.map cimport map
import numpy

cimport PyPxarCore

cdef class Pixel:
    cdef pixel *thisptr      # hold a C++ instance which we're wrapping
    def __cinit__(self, address = None, data = None): # default to None to mimick overloading of constructor
        if address is not None and data is not None:
            self.thisptr = new pixel(address, data)
        else:
            self.thisptr = new pixel()
    def __dealloc__(self):
        del self.thisptr
    cdef fill(self, pixel p):
        self.thisptr.roc_id = p.roc_id
        self.thisptr.column = p.column
        self.thisptr.row = p.row
        self.thisptr.setValue(p.getValue())
    cdef c_clone(self, pixel* p):
        del self.thisptr
        self.thisptr = p
    property roc_id:
        def __get__(self): return self.thisptr.roc_id
        def __set__(self, roc_id): self.thisptr.roc_id = roc_id
    property column:
        def __get__(self): return self.thisptr.column
        def __set__(self, column): self.thisptr.column = column
    property row:
        def __get__(self): return self.thisptr.row
        def __set__(self, row): self.thisptr.row = row
    def setValue(self, double value):
        self.thisptr.setValue(value)
    def getValue(self):
        return self.thisptr.getValue()

cdef class PixelConfig:
    cdef pixelConfig *thisptr      # hold a C++ instance which we're wrapping
    def __cinit__(self, column = None, row = None, trim = None): # default to None to mimick overloading of constructor
        if column is not None and row is not None and trim is not None:
            self.thisptr = new pixelConfig(column, row, trim)
        else:
            self.thisptr = new pixelConfig()
    def __dealloc__(self):
        del self.thisptr
    cdef c_clone(self, pixelConfig* p):
        del self.thisptr
        thisptr = p
    property column:
        def __get__(self): return self.thisptr.column
        def __set__(self, column): self.thisptr.column = column
    property row:
        def __get__(self): return self.thisptr.row
        def __set__(self, row): self.thisptr.row = row
    property trim:
        def __get__(self): return self.thisptr.trim
        def __set__(self, trim): self.thisptr.trim = trim
    property enable:
        def __get__(self): return self.thisptr.enable
        def __set__(self, enable): self.thisptr.enable = enable
    property mask:
        def __get__(self): return self.thisptr.mask
        def __set__(self, mask): self.thisptr.mask = mask


cdef class RocConfig:
    cdef rocConfig *thisptr      # hold a C++ instance which we're wrapping
    def __cinit__(self):
            self.thisptr = new rocConfig()
    def __dealloc__(self):
        del self.thisptr
    property column:
        def __get__(self): 
            r = list()
            for p in self.thisptr.pixels:
                P = PixelConfig()
                P.c_clone(&p)
                r.append(P)
            return r
        def __set__(self, value): 
            cdef vector[pixelConfig] v
            cdef PixelConfig pc
            for pc in value:
                v.push_back( <pixelConfig> pc.thisptr[0])
            self.thisptr.pixels = v
    property dacs:
        def __get__(self): return self.thisptr.dacs
        def __set__(self, value):
            cdef map[uint8_t, uint8_t] m
            for key in value.iterkeys():
                m[key] = value[key]
            self.thisptr.dacs = m
    property type:
        def __get__(self): return self.thisptr.type
        def __set__(self, value): self.thisptr.type = value
    property enable:
        def __get__(self): return self.thisptr.enable
        def __set__(self, enable): self.thisptr.enable = enable

cdef class PxEvent:
    cdef Event *thisptr      # hold a C++ instance which we're wrapping
    def __cinit__(self):
            self.thisptr = new Event()
    def __dealloc__(self):
        del self.thisptr
    cdef c_clone(self, Event* p):
        del self.thisptr
        self.thisptr = p
    cdef fill(self, Event ev):
        self.thisptr.header = ev.header
        self.thisptr.trailer = ev.trailer
        self.thisptr.numDecoderErrors = ev.numDecoderErrors
        for px in ev.pixels:
            self.thisptr.pixels.push_back(px)
    property pixels:
        def __get__(self): 
            r = list()
            for p in self.thisptr.pixels:
                P = Pixel()
                P.fill(p)
                r.append(P)
            return r
        def __set__(self, value): 
            cdef vector[pixel] v
            cdef Pixel px
            for px in value:
                v.push_back( <pixel> px.thisptr[0])
            self.thisptr.pixels = v
    property header:
        def __get__(self): return self.thisptr.header
        def __set__(self, value): self.thisptr.header = value
    property trailer:
        def __get__(self): return self.thisptr.trailer
        def __set__(self, trailer): self.thisptr.trailer = trailer
    property numDecoderErrors:
        def __get__(self): return self.thisptr.numDecoderErrors
        def __set__(self, errors): self.thisptr.numDecoderErrors = errors

cdef class PyPxarCore:
    cdef pxarCore *thisptr # hold the C++ instance
    def __cinit__(self, usbId = "*", logLevel = "INFO"):
        self.thisptr = new pxarCore(usbId, logLevel)
        self.dut = None
    def __dealloc__(self):
        del self.thisptr
    def initTestboard(self,sig_delays, power_settings, pg_setup):
        """ Initializer method for the testboard
        Parameters are dictionaries in the form {"name":value}:
        sig_delays = signal delays
        power_settings = voltage/current limit settings
        pg_setup = initial pattern generator setup
        """
        cdef vector[pair[string, uint8_t]] sd
        cdef vector[pair[string, double]] ps
        cdef vector[pair[string, uint8_t ]] pgs
        # type conversions for fixed-width integers need to
        # be handled very explicitly: creating pairs to push into vects
        for key, value in sig_delays.items():
            sd.push_back(pair[string,uint8_t](key,value))
        for key, value in power_settings.items():
            ps.push_back((key,value))
        for item in enumerate(pg_setup):
            pgs.push_back(pair[string, uint8_t ](item[1][0],item[1][1]))
        return self.thisptr.initTestboard(sd, ps, pgs)
    def setTestboardPower(self, power_settings):
        """ Initializer method for the testboard
        Parameters are dictionaries in the form {"name":value}:
        power_settings = voltage/current limit settings
        """
        cdef vector[pair[string, double]] ps
        # type conversions for fixed-width integers need to
        # be handled very explicitly: creating pairs to push into vects
        for key, value in power_settings.items():
            ps.push_back((key,value))
        self.thisptr.setTestboardPower(ps)
    def setTestboardDelays(self, sig_delays):
        """ Initializer method for the testboard
        Parameters are dictionaries in the form {"name":value}:
        sig_delays = signal delays
        """
        cdef vector[pair[string, uint8_t]] sd
        # type conversions for fixed-width integers need to
        # be handled very explicitly: creating pairs to push into vects
        for key, value in sig_delays.items():
            sd.push_back(pair[string,uint8_t](key,value))
        self.thisptr.setTestboardDelays(sd)
    def setPatternGenerator(self, pg_setup):
        """ Initializer method for the testboard
        Parameters are dictionaries in the form {"name":value}:
        pg_setup = initial pattern generator setup
        """
        cdef vector[pair[string, uint8_t ]] pgs
        # type conversions for fixed-width integers need to
        # be handled very explicitly: creating pairs to push into vects
        for item in enumerate(pg_setup):
            pgs.push_back(pair[string, uint8_t ](item[1][0],item[1][1]))
        self.thisptr.setPatternGenerator(pgs)
    def initDUT(self, hubId, tbmtype, tbmDACs, roctype, rocDACs, rocPixels):
        """ Initializer method for the DUT (attached devices)
        Parameters:
	hubId (int)
        tbmtype (string)
        tbmDACs (list of dictionaries (string,int), one for each TBM)
        roctype (string)
        rocDACs (list of dictionaries (string,int), one for each ROC)
        rocPixels (list of list of pixelConfigs, one list for each ROC)
        """
        cdef vector[vector[pair[string,uint8_t]]] td
        cdef vector[vector[pair[string,uint8_t]]] rd
        cdef vector[vector[pixelConfig]] rpcs
        cdef PixelConfig pc
        for idx, tbmDAC in enumerate(tbmDACs):
            td.push_back(vector[pair[string,uint8_t]]())
            for key, value in tbmDAC.items():
                td.at(idx).push_back(pair[string,uint8_t](key,value))
        for idx, rocDAC in enumerate(rocDACs):
            rd.push_back(vector[pair[string,uint8_t]]())
            for key, value in rocDAC.items():
                rd.at(idx).push_back(pair[string,uint8_t](key,value))
        for idx, rocPixel in enumerate(rocPixels):
            rpcs.push_back(vector[pixelConfig]())
            for pc in rocPixel:
                rpcs.at(idx).push_back(<pixelConfig> pc.thisptr[0])
        return self.thisptr.initDUT(hubId, tbmtype, td, roctype,rd,rpcs)
    def getVersion(self):
        return self.thisptr.getVersion()
    def testAllPixels(self, bool enable, rocid = None):
        if rocid is not None:
            self.thisptr._dut.testAllPixels(enable,rocid)
        else:
            self.thisptr._dut.testAllPixels(enable)
  
    def updateTrimBits(self, trimming, int rocid):
        cdef vector[pixelConfig] v
        cdef pixelConfig pc
        #for idx, col, row, trim in enumerate(trimming):
        for line in xrange(len(trimming)):
            pc = pixelConfig(trimming[line][0][0], trimming[line][1][0], trimming[line][2][0])
            v.push_back(pc)
        self.thisptr._dut.updateTrimBits(v, rocid)
    
    def testPixel(self, int col, int row, bool enable, rocid = None):
        if rocid is not None:
            self.thisptr._dut.testPixel(col, row, enable,rocid)
        else:
            self.thisptr._dut.testPixel(col, row, enable)
    def maskAllPixels(self, bool enable, rocid = None):
        if rocid is not None:
            self.thisptr._dut.maskAllPixels(enable,rocid)
        else:
            self.thisptr._dut.maskAllPixels(enable)
    def maskPixel(self, int col, int row, bool enable, rocid = None):
        if rocid is not None:
            self.thisptr._dut.maskPixel(col, row, enable,rocid)
        else:
            self.thisptr._dut.maskPixel(col, row, enable)
    def programDUT(self):
        return self.thisptr.programDUT()
    def status(self):
        return self.thisptr.status()
    def flashTB(self, string filename):
        return self.thisptr.flashTB(filename)
    def getTBia(self):
        return float(self.thisptr.getTBia())
    def getTBva(self):
        return float(self.thisptr.getTBva())
    def getTBid(self):
        return float(self.thisptr.getTBid())
    def getTBvd(self):
        return float(self.thisptr.getTBvd())
    def HVoff(self):
        self.thisptr.HVoff()
    def HVon(self):
        self.thisptr.HVon()
    def Poff(self):
        self.thisptr.Poff()
    def Pon(self):
        self.thisptr.Pon()
    def SignalProbe(self, string probe, string name):
        return self.thisptr.SignalProbe(probe, name)
    def setDAC(self, string dacName, uint8_t dacValue, rocid = None):
        if rocid is None:
            return self.thisptr.setDAC(dacName, dacValue)
        else:
            return self.thisptr.setDAC(dacName, dacValue, rocid)
    def getDACRange(self, string dacName):
        return self.thisptr.getDACRange(dacName)
    def setTbmReg(self, string regName, uint8_t regValue, tbmid = None):
        if tbmid is None:
            return self.thisptr.setTbmReg(regName, regValue)
        else:
            return self.thisptr.setTbmReg(regName, regValue, tbmid)
    def getPulseheightVsDAC(self, string dacName, int dacStep, int dacMin, int dacMax, int flags = 0, int nTriggers = 16):
        cdef vector[pair[uint8_t, vector[pixel]]] r
        #TODO understand why this returns empty data
        r = self.thisptr.getPulseheightVsDAC(dacName, dacStep, dacMin, dacMax, flags, nTriggers)
        hits = []
        #TODO not hardcode col, row
        #PYXAR expects a list for each from dacMin to dacMax for each activated pixel in DUT
        s = (52, 80, dacMax-dacMin+1)
        for i in range(self.dut.n_rocs):
            hits.append(numpy.zeros(s))
        for d in xrange(r.size()):
            for pix in range(r[d].second.size()):
                hits[r[d].second[pix].roc_id][r[d].second[pix].column][r[d].second[pix].row][d] = r[d].second[pix].getValue()
        return numpy.array(hits)

    def getEfficiencyVsDAC(self, string dacName, int dacStep, int dacMin, int dacMax, int flags = 0, int nTriggers = 16):
        cdef vector[pair[uint8_t, vector[pixel]]] r
        #TODO understand why this returns empty data
        r = self.thisptr.getEfficiencyVsDAC(dacName, dacStep, dacMin, dacMax, flags, nTriggers)
        hits = []
        #TODO not hardcode col, row
        #PYXAR expects a list for each from dacMin to dacMax for each activated pixel in DUT
        s = (52, 80, dacMax-dacMin+1)
        for i in range(self.dut.n_rocs):
            hits.append(numpy.zeros(s))
        for d in xrange(r.size()):
            for pix in range(r[d].second.size()):
                hits[r[d].second[pix].roc_id][r[d].second[pix].column][r[d].second[pix].row][d] = r[d].second[pix].getValue()
        return numpy.array(hits)

    def getThresholdVsDAC(self, string dac1Name, uint8_t dac1Step, uint8_t dac1Min, uint8_t dac1Max, string dac2Name, uint8_t dac2Step, uint8_t dac2Min, uint8_t dac2Max, threshold, uint16_t flags = 0, uint32_t nTriggers=16):
        cdef vector[pair[uint8_t, vector[pixel]]] r
        #TODO understand why this returns empty data
        r = self.thisptr.getThresholdVsDAC(dac1Name, dac1Step, dac1Min, dac1Max, dac2Name, dac2Step, dac2Min, dac2Max, threshold, flags, nTriggers)
        hits = []
        #TODO not hardcode col, row
        #PYXAR expects a list for each from dacMin to dacMax for each activated pixel in DUT
        s = (52, 80, dac2Max-dac2Min+1)
        for i in range(self.dut.n_rocs):
            hits.append(numpy.zeros(s))
        for d in xrange(r.size()):
            for pix in range(r[d].second.size()):
                hits[r[d].second[pix].roc_id][r[d].second[pix].column][r[d].second[pix].row][d] = r[d].second[pix].getValue()
        return numpy.array(hits)

    def getEfficiencyVsDACDAC(self, string dac1name, uint8_t dac1step, uint8_t dac1min, uint8_t dac1max, string dac2name, uint8_t dac2step, uint8_t dac2min, uint8_t dac2max, uint16_t flags = 0, uint32_t nTriggers=16):
        cdef vector[pair[uint8_t, pair[uint8_t, vector[pixel]]]] r
        r = self.thisptr.getEfficiencyVsDACDAC(dac1name, dac1step, dac1min, dac1max, dac2name, dac2step, dac2min, dac2max, flags, nTriggers)
        hits = []
        #TODO not hardcode col, row, check if indices make sense, currently not running!
        #This currently only returns one single pixel! The rest is lost...
        for d in xrange(r.size()):
            if r[d].second.second.size() > 0:
                hits.append(r[d].second.second[0].getValue())
            else:
                hits.append(0)
        return numpy.array(hits)

    def getPulseheightVsDACDAC(self, string dac1name, uint8_t dac1step, uint8_t dac1min, uint8_t dac1max, string dac2name, uint8_t dac2step, uint8_t dac2min, uint8_t dac2max, uint16_t flags = 0, uint32_t nTriggers=16):
        cdef vector[pair[uint8_t, pair[uint8_t, vector[pixel]]]] r
        r = self.thisptr.getPulseheightVsDACDAC(dac1name, dac1step, dac1min, dac1max, dac2name, dac2step, dac2min, dac2max, flags, nTriggers)
        hits = []
        #TODO not hardcode col, row, check if indices make sense, currently not running!
        #This currently only returns one single pixel! The rest is lost...
        for d in xrange(r.size()):
            if r[d].second.second.size() > 0:
                hits.append(r[d].second.second[0].getValue())
            else:
                hits.append(0)
        return numpy.array(hits)

    def getPulseheightMap(self, int flags, int nTriggers):
        cdef vector[pixel] r
        r = self.thisptr.getPulseheightMap(flags, nTriggers)
        #TODO wrap data refilling into single function, rather than copy paste
        s = self.dut.get_roc_shape()
        hits = []
        for i in xrange(self.dut.n_rocs):
            hits.append(numpy.zeros(s))
        for d in xrange(r.size()):
            hits[r[d].roc_id][r[d].column][r[d].row] = r[d].getValue()
        return numpy.array(hits)

    def getEfficiencyMap(self, int flags, int nTriggers):
        cdef vector[pixel] r
        r = self.thisptr.getEfficiencyMap(flags, nTriggers)
        #TODO wrap data refilling into single function, rather than copy paste
        s = self.dut.get_roc_shape()
        hits = []
        for i in xrange(self.dut.n_rocs):
            hits.append(numpy.zeros(s))
        for d in xrange(r.size()):
            hits[r[d].roc_id][r[d].column][r[d].row] = r[d].getValue()
        return numpy.array(hits)

    def getThresholdMap(self, string dacName, uint8_t dacStep, uint8_t dacMin, uint8_t dacMax, uint8_t threshold, int flags, int nTriggers):
        cdef vector[pixel] r
        #TODO wrap data refilling into single function, rather than copy paste
        r = self.thisptr.getThresholdMap(dacName, dacStep, dacMin, dacMax, threshold, flags, nTriggers)
        s = self.dut.get_roc_shape()
        hits = []
        for i in xrange(self.dut.n_rocs):
            hits.append(numpy.zeros(s))
        for d in xrange(r.size()):
            hits[r[d].roc_id][r[d].column][r[d].row] = r[d].getValue()
        return numpy.array(hits)

#    def int32_t getReadbackValue(self, string parameterName):

    def daqStart(self):
        return self.thisptr.daqStart()

    def daqStatus(self):
        return self.thisptr.daqStatus()

    def daqTrigger(self, uint32_t nTrig, uint16_t period = 0):
        self.thisptr.daqTrigger(nTrig,period)

    def daqTriggerLoop(self, uint16_t period):
        self.thisptr.daqTriggerLoop(period)

    def daqTriggerLoopHalt(self):
        self.thisptr.daqTriggerLoopHalt()

    def daqGetEvent(self):
        cdef Event r
        r = self.thisptr.daqGetEvent()
        p = PxEvent()
        p.fill(r)
        return p

    def daqGetEventBuffer(self):
        cdef vector[Event] r
        r = self.thisptr.daqGetEventBuffer()
        pixelevents = list()
        for event in r:
            p = PxEvent()
            p.fill(event)
            pixelevents.append(p)
        return pixelevents

    def daqGetBuffer(self):
        cdef vector[uint16_t] r
        r = self.thisptr.daqGetBuffer()
        return r

    def daqStop(self):
        return self.thisptr.daqStop()

