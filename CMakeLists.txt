########################################################
# CMake file for the pXar framework
CMAKE_MINIMUM_REQUIRED(VERSION 2.4 FATAL_ERROR)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)
########################################################

# Project name
PROJECT( pxar )

# Check if this is a source tarball build
IF(NOT IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)
  SET(SOURCE_PACKAGE 1)
ENDIF(NOT IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)

# Set package version
IF(NOT SOURCE_PACKAGE)
  # Get the version from last git tag plus numer of additional commits:
  EXEC_PROGRAM(git ${CMAKE_CURRENT_SOURCE_DIR} ARGS describe --tags HEAD | "sed 's,^release-,,;s,-,+,;s,-,~,;'" OUTPUT_VARIABLE PXAR_LIB_VERSION) 
  MESSAGE("-- Determined API version ${PXAR_LIB_VERSION}.")
ELSE(NOT SOURCE_PACKAGE)
  # If we don't have git we take the hard-set version.
  # UPDATE THIS BEFORE RELEASING A SOURCE CODE TARBALL!
  SET(PXAR_LIB_VERSION "0.0.2")
ENDIF(NOT SOURCE_PACKAGE)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake.h" "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")

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

# init the variable that will later hold the FTDI library name we link against
SET(FTDI_LINK_LIBRARY "")

IF(NOT FTDI_FOUND AND NOT FTD2XX_FOUND)
  # We have none of the two options available, this doesn't work!
  MESSAGE(FATAL_ERROR "No FTDI library found. Provide either libftdi or libftd2xx!")
ELSE(NOT FTDI_FOUND AND NOT FTD2XX_FOUND)
  # One of the options is available, let's figure out which one to use!
  IF(USE_FTD2XX AND FTD2XX_FOUND)
    # We want FTD2XX, we have FTD2XX.
    MESSAGE(STATUS "Using libFTD2XX.")
    SET(FTDI_LINK_LIBRARY ${FTD2XX_LIBRARY})
    INCLUDE_DIRECTORIES(SYSTEM ${FTD2XX_INCLUDE_DIR})
  ENDIF(USE_FTD2XX AND FTD2XX_FOUND)

  IF(USE_FTD2XX AND NOT FTD2XX_FOUND)
    # We want FTD2XX but it's not there. So let's take FTDI.
    SET(USE_FTD2XX FALSE)
    MESSAGE(STATUS "Using libFTDI as fallback. LibFTD2XX could not be found.")
    ADD_DEFINITIONS(-DHAVE_LIBFTDI)
    SET(FTDI_LINK_LIBRARY ${FTDI_LIBRARY})
    INCLUDE_DIRECTORIES(SYSTEM ${FTDI_INCLUDE_DIR})
  ENDIF(USE_FTD2XX AND NOT FTD2XX_FOUND)

  IF(NOT USE_FTD2XX AND FTDI_FOUND)
    # We want FTDI, and we have it.
    MESSAGE(STATUS "Using libFTDI.")
    ADD_DEFINITIONS(-DHAVE_LIBFTDI)
    SET(FTDI_LINK_LIBRARY ${FTDI_LIBRARY})
    INCLUDE_DIRECTORIES(SYSTEM ${FTDI_INCLUDE_DIR})
  ENDIF(NOT USE_FTD2XX AND FTDI_FOUND)

  IF(NOT USE_FTD2XX AND NOT FTDI_FOUND)
    # We want FTDI but we only have FTD2XX, so let's take that instead.
    SET(USE_FTD2XX TRUE)
    MESSAGE(STATUS "Using libFTD2XX as fallback. LibFTDI could not be found.")
    INCLUDE_DIRECTORIES(SYSTEM ${FTD2XX_INCLUDE_DIR})
    SET(FTDI_LINK_LIBRARY ${FTD2XX_LIBRARY})
  ENDIF(NOT USE_FTD2XX AND NOT FTDI_FOUND)
ENDIF(NOT FTDI_FOUND AND NOT FTD2XX_FOUND)


# Find Python libraries
#FIND_PACKAGE(PythonLibs)
#INCLUDE_DIRECTORIES(SYSTEM ${PYTHON_INCLUDE_PATH})

# set the path to which we will install later: default project home, can be changed using
# cmake -DINSTALL_PREFIX=/some/dir ..
SET(INSTALL_PREFIX "${PROJECT_SOURCE_DIR}" CACHE PATH "Prefix prepended to install directories")
SET(CMAKE_INSTALL_PREFIX "${INSTALL_PREFIX}" CACHE INTERNAL "Prefix prepended to install directories" FORCE)

MESSAGE( STATUS "Build Type: " ${CMAKE_BUILD_TYPE} )
MESSAGE( STATUS "Installing to: " ${CMAKE_INSTALL_PREFIX} )

# SET THE RPATH SO THAT OUR EXECUTABLES FIND THE LIBRARY EVEN WHEN INSTALLED INTO NON-DEFAULT LOCATIONS
# see http://www.cmake.org/Wiki/CMake_RPATH_handling
# enable @rpath in the install name for any shared library being built
# note: it is planned that a future version of CMake will enable this by default
set(CMAKE_MACOSX_RPATH 1)
# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
# *ALTERNATIVELY*: USE RELATIVE PATHS
# see http://www.semipol.de/archives/356
# for all binaries created in a CMake project:
#SET(CMAKE_INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/")
# for certain targets
#SET_TARGET_PROPERTIES(target 1 target2 ...
#    PROPERTIES INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/")
# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
# the RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")


##################################################
# Preparing the build of the shared pXar library #
##################################################

# Include directories
INCLUDE_DIRECTORIES( core/api)

# Build main pXar API library
ADD_SUBDIRECTORY(core)

option(BUILD_pxarui  "Compile pXar UI, tests and executables (requires ROOT)?" OFF)
IF(BUILD_pxarui)

  # Build the tests and test library
  ADD_SUBDIRECTORY(tests)

  # Build the pXar executable
  ADD_SUBDIRECTORY(main)

  # Build the pXar GUI
  ADD_SUBDIRECTORY(gui)

ENDIF(BUILD_pxarui)

option(BUILD_pxardebug  "Compile pXar debug executable?" ON)
IF(BUILD_pxardebug)

  # Build the pXar debug executable
  ADD_SUBDIRECTORY(debug)

ENDIF(BUILD_pxardebug)


##################################################
# Doxygen target to generate API documentation   #
##################################################
FIND_PACKAGE(Doxygen)
IF(DOXYGEN_FOUND)
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile @ONLY)
  ADD_CUSTOM_TARGET(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc
    COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
ENDIF(DOXYGEN_FOUND)