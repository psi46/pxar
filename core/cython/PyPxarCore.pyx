# distutils: language = c++
from libcpp cimport bool
from libc.stdint cimport uint8_t, int8_t, uint16_t, int16_t, int32_t, uint32_t
from libcpp.string cimport string
from libcpp.pair cimport pair
from libcpp.vector cimport vector

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
    cdef to_cobject(self, PixelConfig arg):
        self.thisptr = arg.thisptr
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
    

cdef class PyPxarCore:
    cdef pxarCore *thisptr # hold the C++ instance
    def __cinit__(self, usbId = "*", logLevel = "WARNING"):
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
    
