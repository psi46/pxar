# Add source files for the library

INCLUDE_DIRECTORIES(rpc usb hal utils)

SET(LIB_SOURCE_FILES
  # RPC
  "rpc/rpc_calls.cpp"
  "rpc/rpc.cpp"
  "rpc/rpc_error.cpp"
  "rpc/rpc_io.cpp"
  # API
  "api/api.cc"
  "api/datatypes.cc"
  "api/dut.cc"
  # HAL (w/o hal.cc, see below)
  "hal/datapipe.cc"
  # Utils
  "utils/helper.cc"
  )

# option to replace HAL implementation with "dummy" HAL (for testing of UI code w/o DTB)
option(BUILD_dummydtb "Replace HAL with dummy implementation ('virtual DTB')?" OFF)
IF(BUILD_dummydtb)
  SET(LIB_SOURCE_FILES ${LIB_SOURCE_FILES} "hal/dummy_hal.cc")
ELSE(BUILD_dummydtb)
  SET(LIB_SOURCE_FILES ${LIB_SOURCE_FILES} "hal/hal.cc")
ENDIF(BUILD_dummydtb)

# add USB source files (depending on FTDI library used)
IF(USE_FTD2XX)
  SET(SOURCE_FILES_FTDI "usb/USBInterface.libftd2xx.cc")
ELSE(USE_FTD2XX)
  SET(SOURCE_FILES_FTDI "usb/USBInterface.libftdi.cc")
ENDIF(USE_FTD2XX)
SET(LIB_SOURCES ${LIB_SOURCE_FILES} ${SOURCE_FILES_FTDI})

ADD_LIBRARY( ${PROJECT_NAME} SHARED ${LIB_SOURCES} )

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${FTDI_LINK_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${LIBUSB_1_LIBRARIES})

INSTALL(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
