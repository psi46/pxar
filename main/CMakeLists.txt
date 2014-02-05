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
PixTestParameters.cc
SysCommand.cc
)

# fill list of header files 
set(pXar_HEADERS
ConfigParameters.hh
PixInitFunc.hh
PixUtil.hh
PixGui.hh
PixSetup.hh
PixParTab.hh
PixMonitor.hh
PixTab.hh
PixTest.hh
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
PixTestFactory.hh
PixTestGainCalibration.hh
PixTestParameters.hh
SysCommand.hh
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
