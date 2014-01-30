# distutils: language = c++
from libc.stdint cimport uint8_t, int8_t, uint16_t, int16_t, int32_t, uint32_t
from libcpp.vector cimport vector
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
    cdef cppclass pxarCore:
        pxarCore(string usbId, string logLevel)
        string getVersion()
        bool initTestboard(vector[pair[string,uint8_t] ] sig_delays, vector[pair[string,double] ] power_settings, vector[pair[uint16_t, uint8_t] ] pg_setup)

