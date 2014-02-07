# distutils: language = c++
from libcpp cimport bool
from libc.stdint cimport uint8_t, int8_t, uint16_t, int16_t, int32_t, uint32_t
from libcpp.string cimport string
from libcpp.pair cimport pair
from libcpp.vector cimport vector
from libcpp.map cimport map

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
    def decode(self, int address):
        self.thisptr.decode(address)
    property roc_id:
        def __get__(self): return self.thisptr.roc_id
        def __set__(self, roc_id): self.thisptr.roc_id = roc_id
    property column:
        def __get__(self): return self.thisptr.column
        def __set__(self, column): self.thisptr.column = column
    property row:
        def __get__(self): return self.thisptr.row
        def __set__(self, row): self.thisptr.row = row
    property value:
        def __get__(self): return self.thisptr.value
        def __set__(self, value): self.thisptr.value = value


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


cdef class DUT:
    cdef dut *thisptr # hold the C++ instance
    def __cinit__(self):
        self.thisptr = new dut()
    def __dealloc__(self):
        del self.thisptr
    cdef c_clone(self, dut* d):
        del self.thisptr
        thisptr = d
    def status(self):
        return self.thisptr.status()
    def testAllPixels(self, bool enable, rocid = None):
        if rocid is not None:
            self.thisptr.testAllPixels(enable,rocid)
        else:
            self.thisptr.testAllPixels(enable)

cdef class PyPxarCore:
    cdef pxarCore *thisptr # hold the C++ instance
    def __cinit__(self, usbId = "*", logLevel = "WARNING"):
        self.thisptr = new pxarCore(usbId, logLevel)
    def __dealloc__(self):
        del self.thisptr
    property dut:
        def __get__(self):
            d = DUT()
            d.c_clone(self.thisptr._dut)
            return d
    def initTestboard(self,sig_delays, power_settings, pg_setup):
        """ Initializer method for the testboard
        Parameters are dictionaries in the form {"name":value}:
        sig_delays = signal delays
        power_settings = voltage/current limit settings
        pg_setup = initial pattern generator setup
        """
        cdef vector[pair[string, uint8_t]] sd
        cdef vector[pair[string, double]] ps
        cdef vector[pair[uint16_t, uint8_t ]] pgs
        # type conversions for fixed-width integers need to
        # be handled very explicitly: creating pairs to push into vects
        for key, value in sig_delays.items():
            sd.push_back(pair[string,uint8_t](key,value))
        for key, value in power_settings.items():
            ps.push_back((key,value))
        for key, value in pg_setup.items():
            pgs.push_back(pair[uint16_t, uint8_t ](key,value))
        return self.thisptr.initTestboard(sd, ps, pgs)
    def initDUT(self, tbmtype, tbmDACs, roctype, rocDACs, rocPixels):
        """ Initializer method for the DUT (attached devices)
        Parameters:
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
        return self.thisptr.initDUT(tbmtype, td, roctype,rd,rpcs)
    def getVersion(self):
        return self.thisptr.getVersion()

    def programDUT(self):
        return self.thisptr.programDUT()
    def status(self):
        return self.thisptr.status()
    def flashTB(self, string filename):
        return self.thisptr.flashTB(filename)
    def getTBia(self):
        return self.thisptr.getTBia()
    def getTBva(self):
        return self.thisptr.getTBva()
    def getTBid(self):
        return self.thisptr.getTBid()
    def getTBvd(self):
        return self.thisptr.getTBvd()
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
    def getPulseheightVsDAC(self, string dacName, int dacMin, int dacMax, int flags = 0, int nTriggers = 16):
        cdef vector[pair[uint8_t, vector[pixel]]] r
        r = self.thisptr.getPulseheightVsDAC(dacName, dacMin, dacMax, flags, nTriggers)
        for d in range(r.size()):
            print d
        return "test"
#    def vector[pair[uint8_t, vector[pixel]]] getEfficiencyVsDAC(self, string dacName, uint8_t dacMin, uint8_t dacMax, uint16_t flags = 0, uint32_t nTriggers=16):
#    def vector[pair[uint8_t, vector[pixel]]] getThresholdVsDAC(self, string dacName, uint8_t dacMin, uint8_t dacMax, uint16_t flags = 0, uint32_t nTriggers=16):
#    def vector[pair[uint8_t, pair[uint8_t, vector[pixel]]]] getPulseheightVsDACDAC(self, string dac1name, uint8_t dac1min, uint8_t dac1max, string dac2name, uint8_t dac2min, uint8_t dac2max, uint16_t flags = 0, uint32_t nTriggers=16):
#    def vector[pair[uint8_t, pair[uint8_t, vector[pixel]]]] getEfficiencyVsDACDAC(self, string dac1name, uint8_t dac1min, uint8_t dac1max, string dac2name, uint8_t dac2min, uint8_t dac2max, uint16_t flags = 0, uint32_t nTriggers=16):
#    def vector[pair[uint8_t, pair[uint8_t, vector[pixel]]]] getThresholdVsDACDAC(self, string dac1name, uint8_t dac1min, uint8_t dac1max, string dac2name, uint8_t dac2min, uint8_t dac2max, uint16_t flags = 0, uint32_t nTriggers=16):
#    def vector[pixel] getPulseheightMap(self, uint16_t flags, uint32_t nTriggers):
#    def vector[pixel] getEfficiencyMap(self, uint16_t flags, uint32_t nTriggers):
#    def vector[pixel] getThresholdMap(self, string dacName, uint16_t flags, uint32_t nTriggers):
#    def int32_t getReadbackValue(self, string parameterName):
#    def bool daqStart(self, vector[pair[uint16_t, uint8_t]] pg_setup):
#    def bool daqStatus(self):
#    def void daqTrigger(self, uint32_t nTrig):
#    def void daqTriggerLoop(self, uint16_t period):
#    def vector[uint16_t] daqGetBuffer(self):
#    def vector[pixel] daqGetEvent(self):
#    def bool daqStop(self):

