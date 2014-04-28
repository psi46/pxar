# Build util library

INCLUDE_DIRECTORIES(. ${PROJECT_SOURCE_DIR}/ana)

SET (UTILLIB_SOURCES
ConfigParameters.cc
PixSetup.cc
PixTestParameters.cc
)

# fill list of header files 
set(UTILLIB_HEADERS
ConfigParameters.hh
PixSetup.hh
PixTestParameters.hh
)

SET(MY_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/core/api ${PROJECT_SOURCE_DIR}/core/utils ${PROJECT_SOURCE_DIR}/util )

# create a shared library
ADD_LIBRARY( pxarutil SHARED ${UTILLIB_SOURCES} ${UTILLIB_DICTIONARY} )
# link against our core library, the root stuff, and the USB libs
target_link_libraries(pxarutil pxarana ${PROJECT_NAME} ${FTDI_LINK_LIBRARY} )

# install the lib in the appropriate directory
INSTALL(TARGETS pxarutil
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
