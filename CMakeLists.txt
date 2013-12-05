########################################################
# CMake file for the pXar framework
CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
########################################################

# Project name
PROJECT( PXAR )

# Library Version
SET(PXAR_LIB_VERSION "0.0.1")
SET(CPXAR_LIB_SOVERSION "1")

SET(GCC_COMPILE_FLAGS "-Wall -Wextra -g -Wno-deprecated -ansi -pedantic -Wno-long-long")
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

# Can't remember right now: do we need libusb-1.0?
#FIND_PACKAGE(libusb-1.0 REQUIRED)

# Find the FTDI chip drivers, either the open source or proprietary ones,
# depending on the build option we set:
IF(USE_FTD2XX)
  FIND_PACKAGE(FTD2XX REQUIRED)
ELSE(USE_FTD2XX)
  FIND_PACKAGE(FTDI REQUIRED)
ENDIF(USE_FTD2XX)

# SWIG provides the wrapper generation for our Python interface:
FIND_PACKAGE(SWIG REQUIRED)

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
INCLUDE_DIRECTORIES( ./include )

# Build main pXar API library
add_subdirectory(src)


