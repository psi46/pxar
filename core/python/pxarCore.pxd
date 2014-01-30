from libc.stdint cimport uint8_t, int8_t, uint16_t, int16_t, int32_t, uint32_t

cdef extern from "api.h" namespace "pxar":
    cdef cppclass pixel:
        uint8_t roc_id
        uint8_t column
        uint8_t row
        int32_t value
        pixel()
        pixel(int32_t address, int32_t data)
        void decode(int32_t address)
