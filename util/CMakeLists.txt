# Build util library

# ROOT provides, well, ROOT:
FIND_PACKAGE(ROOT REQUIRED)
INCLUDE_DIRECTORIES(SYSTEM ${ROOT_INCLUDE_DIR})

# for now, we also need includes from the "main" folder
INCLUDE_DIRECTORIES(.)

SET (UTILLIB_SOURCES
ConfigParameters.cc
PixUtil.cc
PixInitFunc.cc
PixSetup.cc
PixTestParameters.cc
)

# fill list of header files 
set(UTILLIB_HEADERS
ConfigParameters.hh
PixUtil.hh
PixInitFunc.hh
PixSetup.hh
PixTestParameters.hh
)

SET(MY_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/core/api ${PROJECT_SOURCE_DIR}/util ${ROOT_INCLUDE_DIR} )

# set everything needed for the root dictonary and create the
# dictionary
set(UTILLIB_DICTIONARY ${CMAKE_CURRENT_BINARY_DIR}/utilDict.cxx) 
ROOT_GENERATE_DICTIONARY_OLD("${UTILLIB_HEADERS}" "${UTILLIB_DICTIONARY}" "${MY_INCLUDE_DIRECTORIES}")


# create a shared library
ADD_LIBRARY( pxarutil SHARED ${UTILLIB_SOURCES} ${UTILLIB_DICTIONARY} )
# link against our core library, the UI lib (for now), the root stuff, and the USB libs
target_link_libraries(pxarutil ${PROJECT_NAME} ${ROOT_LIBRARIES} ${FTDI_LINK_LIBRARY} )

# install the lib in the appropriate directory
INSTALL(TARGETS pxarutil
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
