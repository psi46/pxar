# Build pXar executables:
INCLUDE_DIRECTORIES( . )

# ROOT provides, well, ROOT:
FIND_PACKAGE(ROOT REQUIRED)
INCLUDE_DIRECTORIES(SYSTEM ${ROOT_INCLUDE_DIR})

SET (pXar_SOURCES
ConfigParameters.cc
PixInitFunc.cc
PixUtil.cc
PixGui.cc
PixSetup.cc
PixTab.cc
PixParTab.cc
PixMonitor.cc
PixTest.cc
PixTestFactory.cc
PixTestParameters.cc
PixTestPretest.cc
# PixTestSetTrim.cc
PixTestSetPh.cc
PixTestMap.cc
PixTestMapPh.cc
PixTestMapThr.cc
PixTestDacScanPix.cc
PixTestDacScanPh.cc
PixTestDacScanThr.cc
PixTestDacScanRoc.cc
PixTestDacScanCurrent.cc
PixTestDacDacScan.cc
PixTestDaq.cc
PixTestSetup.cc
PixTestTbm.cc
PixTestTrim.cc
PixTestScurves.cc
PixTestGainCalibration.cc
)

# fill list of header files 
set(pXar_HEADERS
ConfigParameters.hh
PixInitFunc.hh
PixUtil.hh
PixGui.hh
PixSetup.hh
PixTab.hh
PixParTab.hh
PixMonitor.hh
PixTest.hh
PixTestFactory.hh
PixTestParameters.hh
PixTestPretest.hh
# PixTestSetTrim.hh
PixTestSetPh.hh
PixTestMap.hh
PixTestMapPh.hh
PixTestMapThr.hh
PixTestDacScanPix.hh
PixTestDacScanPh.hh
PixTestDacScanThr.hh
PixTestDacScanRoc.hh
PixTestDacScanCurrent.hh
PixTestDacDacScan.hh
PixTestSetup.hh
PixTestDaq.hh
PixTestTbm.hh
PixTestTrim.hh
PixTestScurves.hh
PixTestGainCalibration.hh
)

SET(MY_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/main ${PROJECT_SOURCE_DIR}/core/api ${ROOT_INCLUDE_DIR} )

# set everything needed for the root dictonary and create the
# dictionary
set(pXar_DICTIONARY ${CMAKE_CURRENT_BINARY_DIR}/pXarDict.cxx) 
ROOT_GENERATE_DICTIONARY_OLD("${pXar_HEADERS}" "${pXar_DICTIONARY}" "${MY_INCLUDE_DIRECTORIES}")

ADD_LIBRARY( pxarui SHARED ${pXar_SOURCES} ${pXar_DICTIONARY} )
target_link_libraries(pxarui ${PROJECT_NAME} ${ROOT_LIBRARIES} ${FTDI_LINK_LIBRARY} )


add_executable(pXar pXar.cc )
target_link_libraries(pXar ${PROJECT_NAME} ${ROOT_LIBRARIES} ${FTDI_LINK_LIBRARY} pxarui)

INSTALL(TARGETS pxarui pXar
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
