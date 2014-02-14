# Build test library

# ROOT provides, well, ROOT:
FIND_PACKAGE(ROOT REQUIRED)
INCLUDE_DIRECTORIES(SYSTEM ${ROOT_INCLUDE_DIR})

# for now, we also need includes from the "main" folder
INCLUDE_DIRECTORIES( . ${PROJECT_SOURCE_DIR}/main)

# search pattern for our source files
# DOESN'T WORK DUE TO COMPILER ERRORS FROM ONE OF THE SOURCE FILES -- FIX, THEN UNCOMMENT
#FILE(GLOB TESTLIB_SOURCE_FILES 
#  "*.cc"
#  )
# TEMP WORKAROUND:
SET (TESTLIB_SOURCE_FILES "PixTestSetCalDel.cc")

# create a shared library
ADD_LIBRARY( pxartest SHARED ${TESTLIB_SOURCE_FILES} )
# link against our core library, the UI lib (for now), the root stuff, and the USB libs
target_link_libraries(pxartest pxarui ${PROJECT_NAME} ${ROOT_LIBRARIES} ${FTDI_LINK_LIBRARY} )

# install the lib in the appropriate directory
INSTALL(TARGETS pxartest
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
