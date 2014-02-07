# distutils: language = c++
from libc.stdint cimport uint8_t, int8_t, uint16_t, int16_t, int32_t, uint32_t
from libcpp.vector cimport vector
from libcpp.map cimport map
from libcpp.pair cimport pair
from libcpp.string cimport string
from libcpp cimport bool

cdef extern from "api.h" namespace "pxar":
    cdef cppclass pixel:
        uint8_t roc_id
        uint8_t column
        uint8_t row
        int32_t value
        pixel()
        pixel(int32_t address, int32_t data)
        void decode(int32_t address)

cdef extern from "api.h" namespace "pxar":
    cdef cppclass pixelConfig:
        uint8_t trim
        uint8_t column
        uint8_t row
        bool mask
        bool enable
        pixelConfig()
        pixelConfig(uint8_t column, uint8_t row, uint8_t trim)

cdef extern from "api.h" namespace "pxar":
    cdef cppclass rocConfig:
        vector[pixelConfig] pixels
        map[uint8_t, uint8_t] dacs
        uint8_t type
        bool enable
        rocConfig()

cdef extern from "api.h" namespace "pxar":
    cdef cppclass tbmConfig:
        map[uint8_t, uint8_t] dacs
        uint8_t type
        bool enable
        tbmConfig()

cdef extern from "api.h" namespace "pxar":
    cdef cppclass pxarCore:
        pxarCore(string usbId, string logLevel) except +
        string getVersion()
        bool initTestboard(vector[pair[string, uint8_t] ] sig_delays, 
                           vector[pair[string, double] ] power_settings, 
                           vector[pair[uint16_t, uint8_t]] pg_setup) except +
        bool initDUT(string tbmtype,
                     vector[vector[pair[string,uint8_t]]] tbmDACs,
                     string roctype,
                     vector[vector[pair[string,uint8_t]]] rocDACs,
                     vector[vector[pixelConfig]] rocPixels) except +

        bool programDUT()
        bool status()
        bool flashTB(string filename)
        double getTBia()
        double getTBva()
        double getTBid()
        double getTBvd()
        void HVoff()
        void HVon()
        void Poff()
        void Pon()
        bool SignalProbe(string probe, string name)
        bool setDAC(string dacName, uint8_t dacValue, uint8_t rocid)
        bool setDAC(string dacName, uint8_t dacValue)
        uint8_t getDACRange(string dacName)
        bool setTbmReg(string regName, uint8_t regValue, uint8_t tbmid)
        bool setTbmReg(string regName, uint8_t regValue)
        vector[pair[uint8_t, vector[pixel]]] getPulseheightVsDAC(string dacName, uint8_t dacMin, uint8_t dacMax, uint16_t flags, uint32_t nTriggers) 
        vector[pair[uint8_t, vector[pixel]]] getEfficiencyVsDAC(string dacName, uint8_t dacMin, uint8_t dacMax, uint16_t flags, uint32_t nTriggers)
        vector[pair[uint8_t, vector[pixel]]] getThresholdVsDAC(string dacName, uint8_t dacMin, uint8_t dacMax, uint16_t flags, uint32_t nTriggers)
        vector[pair[uint8_t, pair[uint8_t, vector[pixel]]]] getPulseheightVsDACDAC(string dac1name, uint8_t dac1min, uint8_t dac1max, string dac2name, uint8_t dac2min, uint8_t dac2max, uint16_t flags, uint32_t nTriggers)
        vector[pair[uint8_t, pair[uint8_t, vector[pixel]]]] getEfficiencyVsDACDAC(string dac1name, uint8_t dac1min, uint8_t dac1max, string dac2name, uint8_t dac2min, uint8_t dac2max, uint16_t flags, uint32_t nTriggers)
        vector[pair[uint8_t, pair[uint8_t, vector[pixel]]]] getThresholdVsDACDAC(string dac1name, uint8_t dac1min, uint8_t dac1max, string dac2name, uint8_t dac2min, uint8_t dac2max, uint16_t flags, uint32_t nTriggers)
        vector[pixel] getPulseheightMap(uint16_t flags, uint32_t nTriggers)
        vector[pixel] getEfficiencyMap(uint16_t flags, uint32_t nTriggers)
        vector[pixel] getThresholdMap(string dacName, uint16_t flags, uint32_t nTriggers)
        int32_t getReadbackValue(string parameterName)
        bool daqStart(vector[pair[uint16_t, uint8_t]] pg_setup)
        bool daqStatus()
        void daqTrigger(uint32_t nTrig)
        void daqTriggerLoop(uint16_t period)
        vector[uint16_t] daqGetBuffer()
        vector[pixel] daqGetEvent()
        bool daqStop()

