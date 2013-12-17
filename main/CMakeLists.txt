# Build pXar test exe:
ADD_EXECUTABLE(testpxar "pxar.cpp" "pxar.h" )
TARGET_LINK_LIBRARIES(testpxar ${PROJECT_NAME})

INCLUDE_DIRECTORIES( . )


SET (pxarGui_SOURCES
ConfigParameters.cc
PixGui.cc
PixLog.cc
PixModule.cc
PixRoc.cc
PixSetup.cc
PixTab.cc
PixTbm.cc
PixTest.cc
PixTestAlive.cc
PixTestFactory.cc
PixTestGainCalibration.cc
PixTestParameters.cc
SysCommand.cc
TBInterface.cc
TBVirtualInterface.cc
TestRange.cc
pxar.cc
  )

# fill list of header files 
set(pxarGui_HEADERS
ConfigParameters.hh
#GlobalConstants.hh
PixGui.hh
#PixLog.hh
PixModule.hh
PixRoc.hh
PixSetup.hh
PixTab.hh
PixTbm.hh
PixTest.hh
PixTestAlive.hh
PixTestFactory.hh
PixTestGainCalibration.hh
PixTestParameters.hh
SysCommand.hh
TBInterface.hh
TBVirtualInterface.hh
TestRange.hh
)

SET(MY_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/main ${PROJECT_SOURCE_DIR}/core/api ${ROOT_INCLUDE_DIR} )

# set everything needed for the root dictonary and create the
# dictionary
set(pxarGui_DICTIONARY ${CMAKE_CURRENT_BINARY_DIR}/pxarGuiDict.cxx) 
ROOT_GENERATE_DICTIONARY_OLD("${pxarGui_HEADERS}" "${pxarGui_DICTIONARY}" "${MY_INCLUDE_DIRECTORIES}")

add_executable(pxarGui ${pxarGui_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/pxarGuiDict.cxx)

target_link_libraries(pxarGui ${PROJECT_NAME} ${ROOT_LIBRARIES} ${FTDI_LIBRARY} )

INSTALL(TARGETS pxarGui testpxar
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
