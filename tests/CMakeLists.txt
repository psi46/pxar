# Build tests library

# for now, we also need includes from the "util" folder
INCLUDE_DIRECTORIES( . ${PROJECT_SOURCE_DIR}/util)

SET (TESTLIB_SOURCES
PixTest.cc
PixTestDaq.cc
PixTestCurrentVsDac.cc
PixTestAlive.cc
PixTestTbm.cc
PixTestDacScan.cc
PixTestDacDacScan.cc
PixTestPhDacScan.cc
PixTestScurves.cc
PixTestTrim.cc
PixTestSetup.cc
PixTestPretest.cc
PixTestFactory.cc
PixTestGainCalibration.cc
PixTestPh.cc
PixTestFullTest.cc
)

# fill list of header files 
set(TESTLIB_HEADERS
PixTest.hh
PixTestDaq.hh
PixTestCurrentVsDac.hh
PixTestAlive.hh
PixTestTbm.hh
PixTestDacScan.hh
PixTestDacDacScan.hh
PixTestPhDacScan.hh
PixTestScurves.hh
PixTestTrim.hh
PixTestSetup.hh
PixTestPretest.hh
PixTestGainCalibration.hh
PixTestFactory.hh
PixTestPh.hh
PixTestFullTest.hh
)

SET(MY_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/core/api ${PROJECT_SOURCE_DIR}/tests ${PROJECT_SOURCE_DIR}/util ${ROOT_INCLUDE_DIR} )

# set everything needed for the root dictonary and create the
# dictionary
set(TESTLIB_DICTIONARY ${CMAKE_CURRENT_BINARY_DIR}/testsDict.cxx) 
ROOT_GENERATE_DICTIONARY_OLD("${TESTLIB_HEADERS}" "${TESTLIB_DICTIONARY}" "${MY_INCLUDE_DIRECTORIES}")


# create a shared library
ADD_LIBRARY( pxartests SHARED ${TESTLIB_SOURCES} ${TESTLIB_DICTIONARY} )
# link against our core library, the util lib, the root stuff, and the USB libs
target_link_libraries(pxartests pxarutil ${PROJECT_NAME} ${ROOT_LIBRARIES} ${FTDI_LINK_LIBRARY} )

# install the lib in the appropriate directory
INSTALL(TARGETS pxartests
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
