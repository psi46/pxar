# distutils: language = c++
# this file contains the cython c declarations for the probe dictionary
from libcpp.vector cimport vector
from libcpp.string cimport string

cdef extern from "dictionaries.h" namespace "pxar":
    cdef cppclass TriggerDictionary:
        vector[string] getAllNames()



cdef extern from "dictionaries.h" namespace "pxar::TriggerDictionary":
        TriggerDictionary* getInstance()
