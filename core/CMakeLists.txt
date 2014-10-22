# Add source files for the library

INCLUDE_DIRECTORIES(rpc usb ethernet hal utils)

SET(LIB_SOURCE_FILES
  # API
  "api/api.cc"
  "api/datatypes.cc"
  "api/dut.cc"
  )

# Build flag for Ethernet interface implementation:
option(INTERFACE_ETH "Build DTB Ethernet interface?" OFF)
# Build flag for USB interface implementation:
option(INTERFACE_USB "Build DTB USB interface?" ON)

# If both interfaces are disabled, build a Dummy DTB responding to API calls:
IF(NOT INTERFACE_USB AND NOT INTERFACE_ETH)
  # We only need the dummy HAL implementation for this:
  SET(LIB_SOURCE_FILES ${LIB_SOURCE_FILES}
    "hal/dummy_hal.cc"
    "hal/dummy_pipe.cc"
    )
  MESSAGE(STATUS "Building Dummy DTB (software DTB emulation for testing purposes)")
# We want to build a real interface, so add RPC and the HAL:
ELSE(NOT INTERFACE_USB AND NOT INTERFACE_ETH)
  SET(LIB_SOURCE_FILES ${LIB_SOURCE_FILES} 
    # RPC
    "rpc/rpc_calls.cpp"
    "rpc/rpc.cpp"
    "rpc/rpc_error.cpp"
    # HAL
    "hal/hal.cc"
    "hal/datapipe.cc"
    )
ENDIF(NOT INTERFACE_USB AND NOT INTERFACE_ETH)

IF(INTERFACE_ETH)
  # add Ethernet source files
  SET(LIB_SOURCE_FILES ${LIB_SOURCE_FILES} 
    "ethernet/EthernetInterface.cc"
    )
  MESSAGE(STATUS "Building DTB Ethernet interface.")
  # Add the libraries which need to be linked:
  SET(INTERFACE_LIBRARIES ${INTERFACE_LIBRARIES} ${PCAP_LIBRARIES})
ENDIF(INTERFACE_ETH)

IF(INTERFACE_USB)
  # add USB source files (depending on FTDI library used)
  IF(USE_FTD2XX)
    SET(LIB_SOURCE_FILES ${LIB_SOURCE_FILES}
      "usb/USBInterface.libftd2xx.cc"
      )
    MESSAGE(STATUS "Building DTB USB interface using FTD2XX.")
  ELSE(USE_FTD2XX)
    SET(LIB_SOURCE_FILES ${LIB_SOURCE_FILES}
      "usb/USBInterface.libftdi.cc"
      )
    MESSAGE(STATUS "Building DTB USB interface using FTDI.")
  ENDIF(USE_FTD2XX)
  # Add the libraries which need to be linked:
  SET(INTERFACE_LIBRARIES ${INTERFACE_LIBRARIES} ${FTDI_LINK_LIBRARY} ${LIBUSB_1_LIBRARIES})
ENDIF(INTERFACE_USB)


ADD_LIBRARY(${PROJECT_NAME} SHARED ${LIB_SOURCE_FILES})
# Link necessary libraries:
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} ${INTERFACE_LIBRARIES})

INSTALL(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

option(BUILD_python "Compile pxarcore python interface?" OFF)
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
