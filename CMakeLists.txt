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

# Build options and their defaults:
OPTION(USE_FTD2XX "Use the proprietary FTDI library instead of the open source version" ON)


#######################################
# Check prerequisities for pXar build #
#######################################

# Additional packages to be searched for by cmake
LIST( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake )

# Check for libusb-1.0 package:
FIND_PACKAGE(libusb-1.0 REQUIRED)
INCLUDE_DIRECTORIES(SYSTEM ${LIBUSB_1_INCLUDE_DIRS})

# Include packages for threading:
FIND_PACKAGE(Threads)

# Find the FTDI chip drivers, either the open source or proprietary one,
# depending on the build option we set. Use the other as fallback:
FIND_PACKAGE(FTD2XX)
FIND_PACKAGE(FTDI)

IF(NOT FTDI_FOUND AND NOT FTD2XX_FOUND)
  # We have none of the two options available, this doesn't work!
  MESSAGE(FATAL_ERROR "No FTDI library found. Provide either libftdi or libftd2xx!")
ELSE(NOT FTDI_FOUND AND NOT FTD2XX_FOUND)
  # One of the options is available, let's figure out which one to use!
  IF(USE_FTD2XX AND FTD2XX_FOUND)
    # We want FTD2XX, we have FTD2XX.
    MESSAGE(STATUS "Using libFTD2XX.")
    INCLUDE_DIRECTORIES(SYSTEM ${FTD2XX_INCLUDE_DIR})
  ENDIF(USE_FTD2XX AND FTD2XX_FOUND)

  IF(USE_FTD2XX AND NOT FTD2XX_FOUND)
    # We want FTD2XX but it's not there. So let's take FTDI.
    SET(USE_FTD2XX FALSE)
    MESSAGE(STATUS "Using libFTDI as fallback. LibFTD2XX could not be found.")
    ADD_DEFINITIONS(-DHAVE_LIBFTDI)
    INCLUDE_DIRECTORIES(SYSTEM ${FTDI_INCLUDE_DIR})
  ENDIF(USE_FTD2XX AND NOT FTD2XX_FOUND)

  IF(NOT USE_FTD2XX AND FTDI_FOUND)
    # We want FTDI, and we have it.
    MESSAGE(STATUS "Using libFTDI.")
    ADD_DEFINITIONS(-DHAVE_LIBFTDI)
    INCLUDE_DIRECTORIES(SYSTEM ${FTDI_INCLUDE_DIR})
  ENDIF(NOT USE_FTD2XX AND FTDI_FOUND)

  IF(NOT USE_FTD2XX AND NOT FTDI_FOUND)
    # We want FTDI but we only have FTD2XX, so let's take that instead.
    SET(USE_FTD2XX TRUE)
    MESSAGE(STATUS "Using libFTD2XX as fallback. LibFTDI could not be found.")
    INCLUDE_DIRECTORIES(SYSTEM ${FTD2XX_INCLUDE_DIR})
  ENDIF(NOT USE_FTD2XX AND NOT FTDI_FOUND)
ENDIF(NOT FTDI_FOUND AND NOT FTD2XX_FOUND)


# Find Python libraries
FIND_PACKAGE(PythonLibs)
INCLUDE_DIRECTORIES(SYSTEM ${PYTHON_INCLUDE_PATH})

# ROOT provides, well, ROOT:
FIND_PACKAGE(ROOT REQUIRED)
INCLUDE_DIRECTORIES(SYSTEM ${ROOT_INCLUDE_DIR})


MESSAGE( STATUS "Build Type: " ${CMAKE_BUILD_TYPE} )
MESSAGE( STATUS "Installing to: " ${PROJECT_SOURCE_DIR} )

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)



##################################################
# Preparing the build of the shared pXar library #
##################################################

# Include directories
INCLUDE_DIRECTORIES( core/api)

# Build main pXar API library
ADD_SUBDIRECTORY(core)

# Build the tests and test library
ADD_SUBDIRECTORY(tests)

# Build the pXar executable
ADD_SUBDIRECTORY(main)

# Build the pXar GUI
ADD_SUBDIRECTORY(gui)
