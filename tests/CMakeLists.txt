# Build tests library

# for now, we also need includes from the "util" folder
INCLUDE_DIRECTORIES( . ${PROJECT_SOURCE_DIR}/util ${PROJECT_SOURCE_DIR}/ana)

SET (TESTLIB_SOURCES
PixTest.cc
PixTestDaq.cc
PixTestXray.cc
PixTestHighRate.cc
PixTestCurrentVsDac.cc
PixTestIV.cc
PixTestAlive.cc
PixTestTbm.cc
PixTestDacScan.cc
PixTestDacDacScan.cc
PixTestScurves.cc
PixTestGainPedestal.cc
PixTestTrim.cc
PixTestSetup.cc
PixTestPattern.cc
PixTestPretest.cc
PixTestTiming.cc
PixTestFactory.cc
PixTestPh.cc
PixTestPhOptimization.cc
PixTestPhOpt.cc
PixTestBBMap.cc
PixTestBareModule.cc
PixTestFullTest.cc
)

# fill list of header files
set(TESTLIB_HEADERS
PixTest.hh
PixTestDaq.hh
PixTestXray.hh
PixTestHighRate.hh
PixTestCurrentVsDac.hh
PixTestIV.hh
PixTestAlive.hh
PixTestTbm.hh
PixTestDacScan.hh
PixTestDacDacScan.hh
PixTestScurves.hh
PixTestGainPedestal.hh
PixTestTrim.hh
PixTestSetup.hh
PixTestPattern.hh
PixTestPretest.hh
PixTestTiming.hh
PixTestFactory.hh
PixTestPh.hh
PixTestPhOptimization.hh
PixTestPhOpt.hh
PixTestBBMap.hh
PixTestBareModule.hh
PixTestFullTest.hh
)

SET(MY_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/core/api ${PROJECT_SOURCE_DIR}/core/utils ${PROJECT_SOURCE_DIR}/tests ${PROJECT_SOURCE_DIR}/util ${PROJECT_SOURCE_DIR}/ana ${ROOT_INCLUDE_DIR} )

# set everything needed for the root dictonary and create the
# dictionary
set(TESTLIB_DICTIONARY ${CMAKE_CURRENT_BINARY_DIR}/testsDict.cxx)
ROOT_GENERATE_DICTIONARY_OLD("${TESTLIB_HEADERS}" "${TESTLIB_DICTIONARY}" "${MY_INCLUDE_DIRECTORIES}")


# create a shared library
ADD_LIBRARY( pxartests SHARED ${TESTLIB_SOURCES} ${TESTLIB_DICTIONARY} )
# link against our core library, the util lib, the root stuff, and the USB libs
target_link_libraries(pxartests pxarutil pxarana ${PROJECT_NAME} ${ROOT_LIBRARIES} ${FTDI_LINK_LIBRARY} ${DEVICES_LINK_LIBRARY})

# install the lib in the appropriate directory
INSTALL(TARGETS pxartests
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
