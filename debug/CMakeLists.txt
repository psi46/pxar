# Build pXar test exe:
ADD_EXECUTABLE(testpxar "pxar.cpp" "pxar.h" )
TARGET_LINK_LIBRARIES(testpxar ${PROJECT_NAME} ${FTDI_LINK_LIBRARY} )

ADD_EXECUTABLE(takedata "takedata.cpp" "pxar.h" )
TARGET_LINK_LIBRARIES(takedata ${PROJECT_NAME} ${FTDI_LINK_LIBRARY} )

INCLUDE_DIRECTORIES( . )

INSTALL(TARGETS testpxar takedata
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

# also copy the ftd2xx dll if on win32
if(WIN32 AND FTD2XX_DLL)
  # copy needed FTD2XX dll file to build directory so that executable can be run from there as well
  add_custom_command(TARGET testpxar POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${FTD2XX_DLL}
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR})
  # copy FTD2XX DLL into bin directory when running INSTALL step
  install(CODE "copy_files(\"${FTD2XX_DLL}\" \"${CMAKE_INSTALL_PREFIX}/bin\")")
endif()
