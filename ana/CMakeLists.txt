# Build util library

INCLUDE_DIRECTORIES(. ${PROJECT_SOURCE_DIR}/util)

SET (ANALIB_SOURCES
PixUtil.cc
PixInitFunc.cc
PHCalibration.cc
anaFullTest.cc
anaGainPedestal.cc
anaScurve.cc
)

# fill list of header files 
set(ANALIB_HEADERS
PixUtil.hh
PixInitFunc.hh
PHCalibration.hh
anaFullTest.hh
anaGainPedestal.hh
anaScurve.hh
)

SET(MY_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/core/api ${PROJECT_SOURCE_DIR}/core/utils ${PROJECT_SOURCE_DIR}/ana ${PROJECT_SOURCE_DIR}/util ${ROOT_INCLUDE_DIR} )

# set everything needed for the root dictonary and create the
# dictionary
set(ANALIB_DICTIONARY ${CMAKE_CURRENT_BINARY_DIR}/anaDict.cxx) 
ROOT_GENERATE_DICTIONARY_OLD("${ANALIB_HEADERS}" "${ANALIB_DICTIONARY}" "${MY_INCLUDE_DIRECTORIES}")


# create a shared library
ADD_LIBRARY( pxarana SHARED ${ANALIB_SOURCES} ${ANALIB_DICTIONARY} )
# link against our core library, the root stuff, and the USB libs
target_link_libraries(pxarana ${PROJECT_NAME} ${ROOT_LIBRARIES} ${FTDI_LINK_LIBRARY} )

# install the lib in the appropriate directory
INSTALL(TARGETS pxarana
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
