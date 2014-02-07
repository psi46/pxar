# Add source files for the library

INCLUDE_DIRECTORIES(rpc usb hal utils)

FILE(GLOB LIB_SOURCE_FILES 
  "rpc/*.cpp"
  "api/*.cc"
  "utils/*.cc"
  )

option(BUILD_dummydtb "Replace HAL with dummy implementation ('virtual DTB')?" OFF)
IF(BUILD_dummydtb)
  SET(LIB_SOURCE_FILES ${LIB_SOURCE_FILES} "hal/dummy_hal.cc")
ELSE(BUILD_dummydtb)
  SET(LIB_SOURCE_FILES ${LIB_SOURCE_FILES} "hal/hal.cc")
ENDIF(BUILD_dummydtb)

FILE(GLOB LIB_HEADER_FILES
  "rpc/*.h"
  "usb/*.h"
  "api/*.h"
  "hal/*.h"
  "utils/*.h"
  )

IF(USE_FTD2XX)
  FILE(GLOB SOURCE_FILES_FTDI "usb/USBInterface.libftd2xx.cc")
ELSE(USE_FTD2XX)
  FILE(GLOB SOURCE_FILES_FTDI "usb/USBInterface.libftdi.cc")
ENDIF(USE_FTD2XX)
SET(LIB_SOURCES ${LIB_SOURCE_FILES} ${SOURCE_FILES_FTDI})

ADD_LIBRARY( ${PROJECT_NAME} SHARED ${LIB_SOURCES} )

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${FTDI_LINK_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${LIBUSB_1_LIBRARIES})

INSTALL(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
