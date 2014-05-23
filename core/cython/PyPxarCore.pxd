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
    cdef cppclass dut:
        dut()
        info()
        int32_t getNEnabledPixels(uint8_t rocid)
        int32_t getNMaskedPixels(uint8_t rocid)
        int32_t getNEnabledTbms()
        int32_t getNTbms()
        int32_t getNEnabledRocs()
        int32_t getNRocs()
        vector[ uint8_t ] getEnabledRocI2Caddr()
        vector[pixelConfig] getEnabledPixels(size_t rocid)
        vector[rocConfig] getEnabledRocs()
        vector[uint8_t] getEnabledRocIDs()
        vector[tbmConfig] getEnabledTbms()
        bool getPixelEnabled(uint8_t column, uint8_t row)
        bool getAllPixelEnable()
        bool getModuleEnable()
        pixelConfig getPixelConfig(size_t rocid, uint8_t column, uint8_t row)
        uint8_t getDAC(size_t rocId, string dacName)
        vector[pair[string,uint8_t]] getDACs(size_t rocId)
        vector[pair[string,uint8_t]] getTbmDACs(size_t tbmId)
        void printDACs(size_t rocId)
        void setROCEnable(size_t rocId, bool enable)
        void setTBMEnable(size_t tbmId, bool enable)
        void testPixel(uint8_t column, uint8_t row, bool enable)
        void testPixel(uint8_t column, uint8_t row, bool enable, uint8_t rocid)
        void maskPixel(uint8_t column, uint8_t row, bool mask)
        void maskPixel(uint8_t column, uint8_t row, bool mask, uint8_t rocid)
        void testAllPixels(bool enable)
        void testAllPixels(bool enable, uint8_t rocid)
        void maskAllPixels(bool mask, uint8_t rocid)
        void maskAllPixels(bool mask)
        bool updateTrimBits(vector[pixelConfig] trimming, uint8_t rocid)
        bool updateTrimBits(uint8_t column, uint8_t row, uint8_t trim, uint8_t rocid)
        bool updateTrimBits(pixelConfig trim, uint8_t rocid)
        bool status()
        bool _initialized
        bool _programmed
        vector[bool] getEnabledColumns(size_t rocid)
        vector[rocConfig] roc
        vector[tbmConfig] tbm
        map[uint8_t,uint8_t] sig_delays
        double va, vd, ia, id
        vector[pair[uint16_t,uint8_t]] pg_setup

cdef extern from "api.h" namespace "pxar":
    cdef cppclass Event:
        Event()
        void Clear()
        uint16_t header
        uint16_t trailer
        vector[pixel] pixels

cdef extern from "api.h" namespace "pxar":
    cdef cppclass rawEvent:
        rawEvent()
        void SetStartError()
        void SetEndError()
        void SetOverflow()
        void ResetStartError()
        void ResetEndError()
        void ResetOverflow()
        void Clear()
        bool IsStartError()
        bool IsEndError()
        bool IsOverflow()
        unsigned int GetSize()
        void Add(uint16_t value)
        vector[uint16_t] data


cdef extern from "api.h" namespace "pxar":
    cdef cppclass pxarCore:
        pxarCore(string usbId, string logLevel) except +
        dut* _dut
        string getVersion()
        bool initTestboard(vector[pair[string, uint8_t] ] sig_delays, 
                           vector[pair[string, double] ] power_settings, 
                           vector[pair[uint16_t, uint8_t]] pg_setup) except +
        bool initDUT(uint8_t hubId,
	             string tbmtype,
                     vector[vector[pair[string,uint8_t]]] tbmDACs,
                     string roctype,
                     vector[vector[pair[string,uint8_t]]] rocDACs,
                     vector[vector[pixelConfig]] rocPixels) except +

        bool programDUT() except +
        bool status()
        bool flashTB(string filename) except +
        double getTBia()
        double getTBva()
        double getTBid()
        double getTBvd()
        void HVoff()
        void HVon()
        void Poff()
        void Pon()
        bool SignalProbe(string probe, string name) except +
        bool setDAC(string dacName, uint8_t dacValue, uint8_t rocid) except +
        bool setDAC(string dacName, uint8_t dacValue) except +
        uint8_t getDACRange(string dacName) except +
        bool setTbmReg(string regName, uint8_t regValue, uint8_t tbmid) except +
        bool setTbmReg(string regName, uint8_t regValue) except +
        vector[pair[uint8_t, vector[pixel]]] getPulseheightVsDAC(string dacName, uint8_t dacMin, uint8_t dacMax, uint16_t flags, uint16_t nTriggers)  except +
        vector[pair[uint8_t, vector[pixel]]] getEfficiencyVsDAC(string dacName, uint8_t dacMin, uint8_t dacMax, uint16_t flags, uint16_t nTriggers) except +
        vector[pair[uint8_t, vector[pixel]]] getThresholdVsDAC(string dacName, string dac2Name, uint8_t dac2Min, uint8_t dac2Max, uint16_t flags, uint16_t nTriggers) except +
        vector[pair[uint8_t, vector[pixel]]] getThresholdVsDAC(string dac1Name, uint8_t dac1Min, uint8_t dac1Max, string dac2Name, uint8_t dac2Min, uint8_t dac2Max, uint16_t flags, uint16_t nTriggers) except +
        vector[pair[uint8_t, pair[uint8_t, vector[pixel]]]] getPulseheightVsDACDAC(string dac1name, uint8_t dac1min, uint8_t dac1max, string dac2name, uint8_t dac2min, uint8_t dac2max, uint16_t flags, uint16_t nTriggers) except +
        vector[pair[uint8_t, pair[uint8_t, vector[pixel]]]] getEfficiencyVsDACDAC(string dac1name, uint8_t dac1min, uint8_t dac1max, string dac2name, uint8_t dac2min, uint8_t dac2max, uint16_t flags, uint16_t nTriggers) except +
        vector[pixel] getPulseheightMap(uint16_t flags, uint16_t nTriggers) except +
        vector[pixel] getEfficiencyMap(uint16_t flags, uint16_t nTriggers) except +
        vector[pixel] getThresholdMap(string dacName, uint16_t flags, uint16_t nTriggers) except +
        vector[pixel] getThresholdMap(string dacName, uint8_t dacMin, uint8_t dacMax, uint16_t flags, uint16_t nTriggers) except +
        int32_t getReadbackValue(string parameterName) except +
        bool daqStart(vector[pair[uint16_t, uint8_t]] pg_setup) except +
        bool daqStatus() except +
        void daqTrigger(uint32_t nTrig) except +
        void daqTriggerLoop(uint16_t period) except +
        Event daqGetEvent() except +
        rawEvent daqGetRawEvent() except +
        vector[rawEvent] daqGetRawBuffer() except +
        vector[Event] daqGetEventBuffer() except +
        bool daqStop() except +
