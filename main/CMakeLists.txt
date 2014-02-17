# Build pXar executables:

INCLUDE_DIRECTORIES( . )
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/util ${PROJECT_SOURCE_DIR}/tests ${PROJECT_SOURCE_DIR}/gui )

SET (pXar_SOURCES
pXar.cc
)

# fill list of header files 
set(pXar_HEADERS
)

SET(MY_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/main ${PROJECT_SOURCE_DIR}/core/api ${ROOT_INCLUDE_DIR} )

# set everything needed for the root dictonary and create the
# dictionary
#set(pXar_DICTIONARY ${CMAKE_CURRENT_BINARY_DIR}/pXarDict.cxx) 
#ROOT_GENERATE_DICTIONARY_OLD("${pXar_HEADERS}" "${pXar_DICTIONARY}" "${MY_INCLUDE_DIRECTORIES}")

#ADD_LIBRARY( pxarui SHARED ${pXar_SOURCES} ${pXar_DICTIONARY} )
#ADD_LIBRARY( pxarui SHARED ${pXar_SOURCES} )
#target_link_libraries(pxarui pxartests ${PROJECT_NAME} ${ROOT_LIBRARIES} ${FTDI_LINK_LIBRARY} )


add_executable(pXar pXar.cc )
target_link_libraries(pXar ${PROJECT_NAME} ${ROOT_LIBRARIES} ${FTDI_LINK_LIBRARY} pxartests pxargui pxarutil)

INSTALL(TARGETS pXar
  RUNTIME DESTINATION bin
# LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
