########################################################
# CMake file for the pXar framework
CMAKE_MINIMUM_REQUIRED(VERSION 2.4 FATAL_ERROR)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)
########################################################

# Project name
PROJECT( pxar )

# Library Version
SET(PXAR_LIB_VERSION "0.0.1")
SET(PXAR_LIB_SOVERSION "1")

SET(GCC_COMPILE_FLAGS "-Wall -Wextra -g -Wno-deprecated -pedantic -Wno-long-long")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COMPILE_FLAGS}" )

SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fno-inline -fdiagnostics-show-option -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wmissing-include-dirs -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wsign-promo -Wstrict-null-sentinel -Wswitch-default -Wundef" CACHE STRING "Debug options." FORCE )
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -Wall"  CACHE STRING "Relwithdebinfo options." FORCE )

# Set the correct build type and allow command line options:
IF(!CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Relwithdebinfo)
ENDIF(!CMAKE_BUILD_TYPE)

SET(INSTALL_PREFIX "${PROJECT_SOURCE_DIR}" CACHE PATH "Prefix prepended to install directories")
SET(CMAKE_INSTALL_PREFIX "${INSTALL_PREFIX}" CACHE INTERNAL "Prefix prepended to install directories" FORCE)



#######################################
# Check prerequisities for pXar build #
#######################################

# Additional packages to be searched for by cmake
LIST( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake )

# Check for libusb-1.0 package:
FIND_PACKAGE(libusb-1.0 REQUIRED)
INCLUDE_DIRECTORIES(${LIBUSB_1_INCLUDE_DIRS})

# Include packages for threading:
FIND_PACKAGE(Threads)

# Find the FTDI chip drivers, either the open source or proprietary ones,
# depending on the build option we set:
IF(USE_FTD2XX)
  FIND_PACKAGE(FTD2XX REQUIRED)
  INCLUDE_DIRECTORIES(${FTD2XX_INCLUDE_DIR})
ELSE(USE_FTD2XX)
  FIND_PACKAGE(FTDI REQUIRED)
  ADD_DEFINITIONS(-DHAVE_LIBFTDI)
  INCLUDE_DIRECTORIES(${FTDI_INCLUDE_DIR})
ENDIF(USE_FTD2XX)

# SWIG provides the wrapper generation for our Python interface:
FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})
SET(CMAKE_SWIG_FLAGS "")

# Find Python libraries
FIND_PACKAGE(PythonLibs)
INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

# ROOT provides, well, ROOT:
FIND_PACKAGE(ROOT REQUIRED)


MESSAGE( STATUS "Build Type: " ${CMAKE_BUILD_TYPE} )
MESSAGE( STATUS "Installing to: " ${PROJECT_SOURCE_DIR} )

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)



##################################################
# Preparing the build of the shared pXar library #
##################################################

# Include directories
#INCLUDE_DIRECTORIES( ./include )

# Build main pXar API library
ADD_SUBDIRECTORY(src)



#########################################################
# Preparation for SWIG wrapper Python module generation #
#########################################################

#SET_SOURCE_FILES_PROPERTIES(example.i PROPERTIES CPLUSPLUS ON)
#SET_SOURCE_FILES_PROPERTIES(example.i PROPERTIES SWIG_FLAGS "-includeall")
#SWIG_ADD_MODULE(example python example.i example.cxx)
#SWIG_LINK_LIBRARIES(example ${PYTHON_LIBRARIES})
