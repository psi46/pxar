# Build pXar executables:
INCLUDE_DIRECTORIES( . )

# ROOT provides, well, ROOT:
FIND_PACKAGE(ROOT REQUIRED)
INCLUDE_DIRECTORIES(SYSTEM ${ROOT_INCLUDE_DIR})

SET (pxarGui_SOURCES
ConfigParameters.cc
PixGui.cc
PixSetup.cc
PixTab.cc
PixParTab.cc
PixTest.cc
PixTestAlive.cc
PixTestFactory.cc
PixTestGainCalibration.cc
PixTestParameters.cc
SysCommand.cc
#pXar.cc
  )

# fill list of header files 
set(pxarGui_HEADERS
ConfigParameters.hh
#GlobalConstants.hh
PixGui.hh
#PixLog.hh
PixSetup.hh
PixParTab.hh
PixTab.hh
PixTest.hh
PixTestAlive.hh
PixTestFactory.hh
PixTestGainCalibration.hh
PixTestParameters.hh
SysCommand.hh
)

SET(MY_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/main ${PROJECT_SOURCE_DIR}/core/api ${ROOT_INCLUDE_DIR} )

# set everything needed for the root dictonary and create the
# dictionary
set(pxarGui_DICTIONARY ${CMAKE_CURRENT_BINARY_DIR}/pxarGuiDict.cxx) 
ROOT_GENERATE_DICTIONARY_OLD("${pxarGui_HEADERS}" "${pxarGui_DICTIONARY}" "${MY_INCLUDE_DIRECTORIES}")

ADD_LIBRARY( pxarui SHARED ${pxarGui_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/pxarGuiDict.cxx )
target_link_libraries(pxarui ${PROJECT_NAME} ${ROOT_LIBRARIES} ${FTDI_LINK_LIBRARY} )


add_executable(pxarGui pXar.cc ${pxarGui_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/pxarGuiDict.cxx)
target_link_libraries(pxarGui ${PROJECT_NAME} ${ROOT_LIBRARIES} ${FTDI_LINK_LIBRARY} )

INSTALL(TARGETS pxarui pxarGui
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
