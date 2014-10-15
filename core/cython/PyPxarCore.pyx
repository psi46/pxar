# distutils: language = c++
from libcpp cimport bool
from libc.stdint cimport uint8_t, int8_t, uint16_t, int16_t, int32_t, uint32_t
from libcpp.string cimport string
from libcpp.pair cimport pair
from libcpp.vector cimport vector
from libcpp.map cimport map
import numpy

cimport PyPxarCore

FLAG_FORCE_SERIAL   = int(_flag_force_serial)
FLAG_CALS           = int(_flag_cals)
FLAG_XTALK          = int(_flag_xtalk)
FLAG_RISING_EDGE    = int(_flag_rising_edge)
FLAG_DISABLE_DACCAL = int(_flag_disable_daccal)
FLAG_NOSORT         = int(_flag_nosort)
FLAG_CHECK_ORDER    = int(_flag_check_order)
FLAG_FORCE_UNMASKED = int(_flag_force_unmasked)

cdef class Pixel:
    cdef pixel *thisptr      # hold a C++ instance which we're wrapping
    def __cinit__(self, address = None, data = None): # default to None to mimick overloading of constructor
        if address is not None and data is not None:
            self.thisptr = new pixel(address, data)
        else:
            self.thisptr = new pixel()
    def __dealloc__(self):
        del self.thisptr
    def __str__(self):
        s = "ROC " + str(self.roc)
        s += " [" + str(self.column) + "," + str(self.row) + "," + str(self.value) + "] "
        return s
    cdef fill(self, pixel p):
        self.thisptr.setRoc(p.roc())
        self.thisptr.setColumn(p.column())
        self.thisptr.setRow(p.row())
        self.thisptr.setValue(p.value())
    cdef c_clone(self, pixel* p):
        del self.thisptr
        self.thisptr = p
    property roc:
        def __get__(self): return self.thisptr.roc()
        def __set__(self, roc): self.thisptr.setRoc(roc)
    property column:
        def __get__(self): return self.thisptr.column()
        def __set__(self, column): self.thisptr.setColumn(column)
    property row:
        def __get__(self): return self.thisptr.row()
        def __set__(self, row): self.thisptr.setRow(row)
    property value:
        def __get__(self): return self.thisptr.value()
        def __set__(self, value): self.thisptr.setValue(value)

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
        def __get__(self): return self.thisptr.column()
        def __set__(self, column): self.thisptr.setColumn(column)
    property row:
        def __get__(self): return self.thisptr.row()
        def __set__(self, row): self.thisptr.setRow(row)
    property trim:
        def __get__(self): return self.thisptr.trim()
        def __set__(self, trim): self.thisptr.setTrim(trim)
    property enable:
        def __get__(self): return self.thisptr.enable()
        def __set__(self, enable): self.thisptr.setEnable(enable)
    property mask:
        def __get__(self): return self.thisptr.mask()
        def __set__(self, mask): self.thisptr.setMask(mask)


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
        def __get__(self): return self.thisptr.enable()
        def __set__(self, enable): self.thisptr.setEnable(enable)

cdef class PxEvent:
    cdef Event *thisptr      # hold a C++ instance which we're wrapping
    def __cinit__(self):
            self.thisptr = new Event()
    def __dealloc__(self):
        del self.thisptr
    def __str__(self):
        s = "====== " + hex(self.header) + " ====== "
        for px in self.pixels:
            s += str(px)
        s += " ====== " + hex(self.trailer) + " ======\n"
        return str(s)
    cdef c_clone(self, Event* p):
        del self.thisptr
        self.thisptr = p
    cdef fill(self, Event ev):
        self.thisptr.header = ev.header
        self.thisptr.trailer = ev.trailer
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

cdef class PyPxarCore:
    cdef pxarCore *thisptr # hold the C++ instance
    def __cinit__(self, usbId = "*", logLevel = "INFO"):
        self.thisptr = new pxarCore(usbId, logLevel)
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
            sd.push_back(pair[string,uint8_t](key,int(value)))
        for key, value in power_settings.items():
            ps.push_back((key,float(value)))
        for item in enumerate(pg_setup):
            pgs.push_back(pair[string, uint8_t ](item[1][0],int(item[1][1])))
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
            ps.push_back((key,float(value)))
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
                td.at(idx).push_back(pair[string,uint8_t](key,int(value,0) if isinstance(value,str) else int(value)))
        for idx, rocDAC in enumerate(rocDACs):
            rd.push_back(vector[pair[string,uint8_t]]())
            for key, value in rocDAC.items():
                rd.at(idx).push_back(pair[string,uint8_t](key,int(value,0) if isinstance(value,str) else int(value)))
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

    def getTbmDACs(self, int tbmid):
        return self.thisptr._dut.getTbmDACs(tbmid)
  
    def updateTrimBits(self, trimming, int rocid):
        cdef vector[pixelConfig] v
        cdef pixelConfig pc
        #for idx, col, row, trim in enumerate(trimming):
        for line in xrange(len(trimming)):
            pc = pixelConfig(trimming[line][0][0], trimming[line][1][0], trimming[line][2][0])
            v.push_back(pc)
        self.thisptr._dut.updateTrimBits(v, rocid)
    
    def info(self):
        self.thisptr._dut.info()

    def setROCEnable(self, int rocid, bool enable):
        self.thisptr._dut.setROCEnable(rocid, enable)

    def setTBMEnable(self, int tbmid, bool enable):
        self.thisptr._dut.setTBMEnable(tbmid, enable)

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
    def getNMaskedPixels(self, int rocid):
        return self.thisptr._dut.getNMaskedPixels(rocid)
    def getNEnabledPixels(self, int rocid):
        return self.thisptr._dut.getNEnabledPixels(rocid)
    def getNEnabledTbms(self):
        return self.thisptr._dut.getNEnabledTbms()
    def getNEnabledRocs(self):
        return self.thisptr._dut.getNEnabledRocs()
    def getNTbms(self):
        return self.thisptr._dut.getNTbms()
    def getNRocs(self):
        return self.thisptr._dut.getNRocs()
    def getTbmType(self):
        return self.thisptr._dut.getTbmType()
    def getRocType(self):
        return self.thisptr._dut.getRocType()
    #def programDUT(self):
        #return self.thisptr.programDUT()
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
        r = self.thisptr.getPulseheightVsDAC(dacName, dacStep, dacMin, dacMax, flags, nTriggers)
        dac_steps = list()
        for d in xrange(r.size()):
            pixels = list()
            for pix in range(r[d].second.size()):
                p = r[d].second[pix]
                px = Pixel()
                px.fill(p)
                pixels.append(px)
            dac_steps.append(pixels)
        return numpy.array(dac_steps)

    def getEfficiencyVsDAC(self, string dacName, int dacStep, int dacMin, int dacMax, int flags = 0, int nTriggers = 16):
        cdef vector[pair[uint8_t, vector[pixel]]] r
        r = self.thisptr.getEfficiencyVsDAC(dacName, dacStep, dacMin, dacMax, flags, nTriggers)
        dac_steps = list()
        for d in xrange(r.size()):
            pixels = list()
            for pix in range(r[d].second.size()):
                p = r[d].second[pix]
                px = Pixel()
                px.fill(p)
                pixels.append(px)
            dac_steps.append(pixels)
        return numpy.array(dac_steps)

    def getEfficiencyVsDACDAC(self, string dac1name, uint8_t dac1step, uint8_t dac1min, uint8_t dac1max, string dac2name, uint8_t dac2step, uint8_t dac2min, uint8_t dac2max, uint16_t flags = 0, uint32_t nTriggers=16):
        cdef vector[pair[uint8_t, pair[uint8_t, vector[pixel]]]] r
        r = self.thisptr.getEfficiencyVsDACDAC(dac1name, dac1step, dac1min, dac1max, dac2name, dac2step, dac2min, dac2max, flags, nTriggers)
        # Return the linearized matrix with all pixels:
        dac_steps = list()
        for d in xrange(r.size()):
            pixels = list()
            for pix in xrange(r[d].second.second.size()):
                p = r[d].second.second[pix]
                px = Pixel()
                px.fill(p)
                pixels.append(px)
            dac_steps.append(pixels)
        return numpy.array(dac_steps)

    def getThresholdVsDAC(self, string dac1Name, uint8_t dac1Step, uint8_t dac1Min, uint8_t dac1Max, string dac2Name, uint8_t dac2Step, uint8_t dac2Min, uint8_t dac2Max, threshold, uint16_t flags = 0, uint32_t nTriggers=16):
        cdef vector[pair[uint8_t, vector[pixel]]] r
        r = self.thisptr.getThresholdVsDAC(dac1Name, dac1Step, dac1Min, dac1Max, dac2Name, dac2Step, dac2Min, dac2Max, threshold, flags, nTriggers)
        dac_steps = list()
        for d in xrange(r.size()):
            pixels = list()
            for pix in range(r[d].second.size()):
                p = r[d].second[pix]
                px = Pixel()
                px.fill(p)
                pixels.append(px)
            dac_steps.append(pixels)
        return numpy.array(dac_steps)

    def getPulseheightVsDACDAC(self, string dac1name, uint8_t dac1step, uint8_t dac1min, uint8_t dac1max, string dac2name, uint8_t dac2step, uint8_t dac2min, uint8_t dac2max, uint16_t flags = 0, uint32_t nTriggers=16):
        cdef vector[pair[uint8_t, pair[uint8_t, vector[pixel]]]] r
        r = self.thisptr.getPulseheightVsDACDAC(dac1name, dac1step, dac1min, dac1max, dac2name, dac2step, dac2min, dac2max, flags, nTriggers)
        # Return the linearized matrix with all pixels:
        dac_steps = list()
        for d in xrange(r.size()):
            pixels = list()
            for pix in xrange(r[d].second.second.size()):
                p = r[d].second.second[pix]
                px = Pixel()
                px.fill(p)
                pixels.append(px)
            dac_steps.append(pixels)
        return numpy.array(dac_steps)

    def getPulseheightMap(self, int flags, int nTriggers):
        cdef vector[pixel] r
        r = self.thisptr.getPulseheightMap(flags, nTriggers)
        pixels = list()
        for p in r:
            px = Pixel()
            px.fill(p)
            pixels.append(px)
        return pixels

    def getEfficiencyMap(self, int flags, int nTriggers):
        cdef vector[pixel] r
        r = self.thisptr.getEfficiencyMap(flags, nTriggers)
        pixels = list()
        for p in r:
            px = Pixel()
            px.fill(p)
            pixels.append(px)
        return pixels

    def getThresholdMap(self, string dacName, uint8_t dacStep, uint8_t dacMin, uint8_t dacMax, uint8_t threshold, int flags, int nTriggers):
        cdef vector[pixel] r
        r = self.thisptr.getThresholdMap(dacName, dacStep, dacMin, dacMax, threshold, flags, nTriggers)
        pixels = list()
        for p in r:
            px = Pixel()
            px.fill(p)
            pixels.append(px)
        return pixels

    def setExternalClock(self, bool enable):
        return self.thisptr.setExternalClock(enable)

    def setClockStretch(self, uint8_t src, uint16_t delay, uint16_t width):
        self.thisptr.setClockStretch(src, delay, width)

    def setSignalMode(self, string signal, uint8_t mode):
        self.thisptr.setSignalMode(signal, mode)

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

    def daqGetRawEvent(self):
        cdef rawEvent r
        hits = []
        r = self.thisptr.daqGetRawEvent()
        for i in range(r.data.size()):
            hits.append(r.data[i])
        return hits

    def daqGetBuffer(self):
        cdef vector[uint16_t] r
        r = self.thisptr.daqGetBuffer()
        return r

    def daqGetRawEventBuffer(self):
        # Since we're just returning the 16bit ints as rawEvent in python,
        # this is the same as dqGetBuffer:
        return self.thisptr.daqGetBuffer()

    def daqGetReadback(self):
        cdef vector[vector[uint16_t]] r
        r = self.thisptr.daqGetReadback()
        return r

    def daqStop(self):
        return self.thisptr.daqStop()

cimport regdict
cdef class PyRegisterDictionary:
    cdef regdict.RegisterDictionary *thisptr      # hold a C++ instance which we're wrapping
    def __cinit__(self):
        self.thisptr = regdict.getInstance()
    def __dealloc__(self):
        self.thisptr = NULL
    def getAllROCNames(self):
        names = []
        cdef vector[string] v = self.thisptr.getAllROCNames()
        for i in xrange(v.size()):
            names.append(v.at(i))
        return names
    def getAllDTBNames(self):
        names = []
        cdef vector[string] v = self.thisptr.getAllDTBNames()
        for i in xrange(v.size()):
            names.append(v.at(i))
        return names
    def getAllTBMNames(self):
        names = []
        cdef vector[string] v = self.thisptr.getAllTBMNames()
        for i in xrange(v.size()):
            names.append(v.at(i))
        return names

cimport probedict
cdef class PyProbeDictionary:
    cdef probedict.ProbeDictionary *thisptr      # hold a C++ instance which we're wrapping
    def __cinit__(self):
        self.thisptr = probedict.getInstance()
    def __dealloc__(self):
        self.thisptr = NULL
    def getAllAnalogNames(self):
        names = []
        cdef vector[string] v = self.thisptr.getAllAnalogNames()
        for i in xrange(v.size()):
            names.append(v.at(i))
        return names
    def getAllDigitalNames(self):
        names = []
        cdef vector[string] v = self.thisptr.getAllDigitalNames()
        for i in xrange(v.size()):
            names.append(v.at(i))
        return names
    def getAllNames(self):
        names = []
        cdef vector[string] v = self.thisptr.getAllDigitalNames()
        for i in xrange(v.size()):
            names.append(v.at(i))
        v = self.thisptr.getAllAnalogNames()
        for i in xrange(v.size()):
            names.append(v.at(i))
        return names


