from libcpp cimport bool
from libcpp.vector cimport vector
from libc.stdint cimport uint8_t, int8_t, uint16_t, int16_t, int32_t, uint32_t
from libcpp.string cimport string

cimport pxarCore

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
