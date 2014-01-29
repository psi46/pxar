# Build pXar test exe:
ADD_EXECUTABLE(testpxar "pxar.cpp" "pxar.h" )
TARGET_LINK_LIBRARIES(testpxar ${PROJECT_NAME} ${FTDI_LINK_LIBRARY} )

INCLUDE_DIRECTORIES( . )

INSTALL(TARGETS testpxar
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

# also copy the ftd2xx dll if on win32
if(WIN32 AND FTD2XX_DLL)
  add_custom_command(TARGET testpxar POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${FTD2XX_DLL}
    ${CMAKE_INSTALL_PREFIX}/bin)
endif()
