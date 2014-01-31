# distutils: language = c++
# distutils: sources = Rectangle.cpp
from libcpp.pair cimport pair
from libcpp.vector cimport vector
from libc.stdint cimport uint16_t


cdef extern from "Rectangle.h" namespace "shapes":
    cdef cppclass Rectangle:
        Rectangle(vector[int]) except +
        #Rectangle(vector[uint16_t]) except +
        Rectangle(vector[uint16_t] ) except +
        Rectangle(pair[int, int], pair[int, int]) except +
        int x0, y0, x1, y1
        int getLength()
        int getHeight()
        int getArea()
        void move(int, int)

cdef class PyRectangle:
    cdef Rectangle *thisptr      # hold a C++ instance which we're wrapping
    def __cinit__(self, int x0, int y0, int x1, int y1):
        # 1: works!
        #self.thisptr = new Rectangle((x0,x1),(y0,y1))
        
        # 2: works too
        #cdef pair[int,int] x = (x0,x1)
        #cdef pair[int,int] y = (y0,y1)
        #self.thisptr = new Rectangle(x,y)

        # 3: doesn't work: error: no matching function for call
        #cdef vector[int] vect = xrange(x0,y0,x1,y1)
        #self.thisptr = new Rectangle(vect)

        # 4: doesn't work: vector.from_py:37:13: 'uint16_t' is not a type identifier
        #cdef vector[uint16_t] vect = xrange(x0,y0,x1,y1)
        #self.thisptr = new Rectangle(vect)

        # 5: works ?!?
        cdef vector[uint16_t] vect
        vect.push_back(<uint16_t> x0)
        vect.push_back(<uint16_t> y0)
        vect.push_back(<uint16_t> x1)
        vect.push_back(<uint16_t> y1)
        self.thisptr = new Rectangle(vect)

        # 6: doesn't work
        #cdef vector[uint16_t] vect = xrange(x0,y0,x1,y1)
        #self.thisptr = new Rectangle(vect)


    def __dealloc__(self):
        del self.thisptr
    def getLength(self):
        return self.thisptr.getLength()
    def getHeight(self):
        return self.thisptr.getHeight()
    def getArea(self):
        return self.thisptr.getArea()
    def move(self, dx, dy):
        self.thisptr.move(dx, dy)
