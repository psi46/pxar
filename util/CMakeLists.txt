# Build util library

INCLUDE_DIRECTORIES(.)

SET (UTILLIB_SOURCES
ConfigParameters.cc
PixUtil.cc
PixSetup.cc
PixTestParameters.cc
)

# fill list of header files 
set(UTILLIB_HEADERS
ConfigParameters.hh
PixUtil.hh
PixSetup.hh
PixTestParameters.hh
)

SET(MY_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/core/api ${PROJECT_SOURCE_DIR}/core/utils ${PROJECT_SOURCE_DIR}/util ${ROOT_INCLUDE_DIR} )

# if we build the GUI we also need the ROOT dictionary
IF(BUILD_pxarui)
  # set everything needed for the root dictonary and create the
  # dictionary
  set(UTILLIB_DICTIONARY ${CMAKE_CURRENT_BINARY_DIR}/utilDict.cxx) 
  ROOT_GENERATE_DICTIONARY_OLD("${UTILLIB_HEADERS}" "${UTILLIB_DICTIONARY}" "${MY_INCLUDE_DIRECTORIES}")
  set(OTHER_LIBS ${ROOT_LIBRARIES})
ELSE(BUILD_pxarui)
  set(UTILLIB_DICTIONARY "")
  set(OTHER_LIBS "")
ENDIF(BUILD_pxarui)

# create a shared library
ADD_LIBRARY( pxarutil SHARED ${UTILLIB_SOURCES} ${UTILLIB_DICTIONARY} )
# link against our core library, the root stuff, and the USB libs
target_link_libraries(pxarutil ${PROJECT_NAME} ${OTHER_LIBS} ${FTDI_LINK_LIBRARY} )

# install the lib in the appropriate directory
INSTALL(TARGETS pxarutil
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
