# distutils: language = c++
# this file contains the cython c declarations for the register dictionary
from libcpp.vector cimport vector
from libcpp.string cimport string

cdef extern from "dictionaries.h" namespace "pxar":
    cdef cppclass RegisterDictionary:
        vector[string] getAllROCNames()
        vector[string] getAllDTBNames()
        vector[string] getAllTBMNames()


cdef extern from "dictionaries.h" namespace "pxar::RegisterDictionary":
        RegisterDictionary* getInstance()
