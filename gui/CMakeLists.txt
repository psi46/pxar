# Build GUI library

# we also need includes from the util and tests directories
INCLUDE_DIRECTORIES( . ${PROJECT_SOURCE_DIR}/util ${PROJECT_SOURCE_DIR}/tests)

SET (GUILIB_SOURCES
PixGui.cc
PixTab.cc
PixParTab.cc
PixMonitor.cc
)

# fill list of header files 
SET (GUILIB_HEADERS
PixGui.hh
PixTab.hh
PixParTab.hh
PixMonitor.hh
)

SET(MY_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/core/api ${PROJECT_SOURCE_DIR}/gui ${PROJECT_SOURCE_DIR}/util ${PROJECT_SOURCE_DIR}/tests ${ROOT_INCLUDE_DIR} )

# set everything needed for the root dictonary and create the
# dictionary
set(GUILIB_DICTIONARY ${CMAKE_CURRENT_BINARY_DIR}/guiDict.cxx) 
ROOT_GENERATE_DICTIONARY_OLD("${GUILIB_HEADERS}" "${GUILIB_DICTIONARY}" "${MY_INCLUDE_DIRECTORIES}")


# create a shared library
ADD_LIBRARY( pxargui SHARED ${GUILIB_SOURCES} ${GUILIB_DICTIONARY} )
# link against our core library, the UI lib (for now), the root stuff, and the USB libs
target_link_libraries( pxargui pxartests pxarutils ${PROJECT_NAME} ${ROOT_LIBRARIES} ${FTDI_LINK_LIBRARY} )

# install the lib in the appropriate directory
INSTALL(TARGETS pxargui
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
