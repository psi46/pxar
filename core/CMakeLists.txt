# Add source files for the library

INCLUDE_DIRECTORIES(rpc usb hal utils)

SET(LIB_SOURCE_FILES
  # API
  "api/api.cc"
  "api/datatypes.cc"
  "api/dut.cc"
  # HAL (w/o hal.cc, see below)
  )

# option to replace HAL implementation with "dummy" HAL (for testing of UI code w/o DTB)
option(BUILD_dummydtb "Replace HAL with dummy implementation ('virtual DTB')?" OFF)
IF(BUILD_dummydtb)
  SET(LIB_SOURCES ${LIB_SOURCE_FILES}
    "hal/dummy_hal.cc"
    "hal/dummy_pipe.cc"
    )

ELSE(BUILD_dummydtb)
  SET(LIB_SOURCE_FILES ${LIB_SOURCE_FILES} 
    # RPC
    "rpc/rpc_calls.cpp"
    "rpc/rpc.cpp"
    "rpc/rpc_error.cpp"
    # HAL
    "hal/hal.cc"
    "hal/datapipe.cc"
    )

  # add USB source files (depending on FTDI library used)
  IF(USE_FTD2XX)
    SET(SOURCE_FILES_FTDI "usb/USBInterface.libftd2xx.cc")
  ELSE(USE_FTD2XX)
    SET(SOURCE_FILES_FTDI "usb/USBInterface.libftdi.cc")
  ENDIF(USE_FTD2XX)

  SET(LIB_SOURCES ${LIB_SOURCE_FILES} ${SOURCE_FILES_FTDI})
ENDIF(BUILD_dummydtb)

ADD_LIBRARY( ${PROJECT_NAME} SHARED ${LIB_SOURCES} )

# link only the libraries needed:
IF(BUILD_dummydtb)
  TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
ELSE(BUILD_dummydtb)
  TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${FTDI_LINK_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${LIBUSB_1_LIBRARIES})
ENDIF(BUILD_dummydtb)

INSTALL(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

option(BUILD_python "Compile pXar python interface?" OFF)
IF(BUILD_python)
MESSAGE(STATUS "Will now configure the Cython pXar core interface")
# Include the CMake script UseCython.cmake. This defines add_cython_module().
# Instruction for use can be found at the top of cmake/UseCython.cmake.
include( UseCython )

if( NOT NUMPY_INCLUDE_DIR )
  find_package( PythonInterp )
  execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c "import numpy; print(numpy.get_include())"
    OUTPUT_VARIABLE _numpy_include
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  find_path( NUMPY_INCLUDE_DIR numpy/arrayobject.h
    HINTS ${_numpy_include} )
endif()
include_directories( ${NUMPY_INCLUDE_DIR} )

set( cxx_pyx_files
  cython/PyPxarCore.pyx
  )
set_source_files_properties( ${cxx_pyx_files}
  PROPERTIES CYTHON_IS_CXX TRUE )

cython_add_module( PyPxarCore cython/PyPxarCore.pyx )

set_target_properties( PyPxarCore
  PROPERTIES COMPILE_FLAGS "-Wno-strict-aliasing" )

target_link_libraries(PyPxarCore ${PROJECT_NAME})

INSTALL(TARGETS PyPxarCore
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)


ENDIF(BUILD_python)
