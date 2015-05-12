# Build pXar test exe:

INCLUDE_DIRECTORIES( . )

INCLUDE_DIRECTORIES(../core/hal)

ADD_EXECUTABLE(flash "flash.cc")
TARGET_LINK_LIBRARIES(flash ${PROJECT_NAME} ${FTDI_LINK_LIBRARY} )

ADD_EXECUTABLE(testpxar "pxar.cpp" "pxar.h" )
TARGET_LINK_LIBRARIES(testpxar ${PROJECT_NAME} ${FTDI_LINK_LIBRARY} ${DEVICES_LINK_LIBRARY})

ADD_EXECUTABLE(pxardaq "pxardaq.cc" "pxar.h" )
TARGET_LINK_LIBRARIES(pxardaq ${PROJECT_NAME} ${FTDI_LINK_LIBRARY} )

ADD_EXECUTABLE(decode "decoder.cc")
TARGET_LINK_LIBRARIES(decode ${PROJECT_NAME})

INSTALL(TARGETS testpxar pxardaq flash decode
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

# also copy the ftd2xx dll if on win32
if(WIN32 AND FTD2XX_DLL)
  # copy needed FTD2XX dll file to build directory so that executable can be run from there as well
  add_custom_command(TARGET testpxar POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${FTD2XX_DLL}
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
  # copy FTD2XX DLL into bin directory when running INSTALL step
  # WARNING: the "FILE(COPY)" command requires CMake 2.8 or later (our compatability macro does not work in the install cmake script)
  install(CODE "FILE(COPY \"${FTD2XX_DLL}\" DESTINATION \"${CMAKE_INSTALL_PREFIX}/bin\")")
endif()
