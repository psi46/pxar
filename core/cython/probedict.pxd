# distutils: language = c++
# this file contains the cython c declarations for the probe dictionary
from libcpp.vector cimport vector
from libcpp.string cimport string

cdef extern from "dictionaries.h" namespace "pxar":
    cdef cppclass ProbeDictionary:
        vector[string] getAllAnalogNames()
        vector[string] getAllDigitalNames()



cdef extern from "dictionaries.h" namespace "pxar::ProbeDictionary":
        ProbeDictionary* getInstance()
