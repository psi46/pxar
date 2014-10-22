########################################################
# CMake file for the pXar framework
CMAKE_MINIMUM_REQUIRED(VERSION 2.4 FATAL_ERROR)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)
########################################################

# Project name
PROJECT( pxar )
# UPDATE THIS BEFORE RELEASING A SOURCE CODE TARBALL!
SET(PXAR_VERSION "1.6.5")
SET(PXAR_FW_VERSION "3.4")


#########################################
# pxar build options and their defaults #
#########################################

OPTION(BUILD_tools  "Compile pxar tools? (flash, testpxar...)" OFF)
OPTION(USE_FTD2XX "Use the proprietary FTDI library instead of the open source version" ON)
OPTION(BUILD_pxarui "Compile pXar UI, tests and executables (requires ROOT)?" ON)


########################################
# Setup the build environment for pxar #
########################################

# some macros are redefined to keep compatability with CMake 2.6
include(${PROJECT_SOURCE_DIR}/cmake/CMakeCompatibility.cmake)

# Additional packages to be searched for by cmake
LIST( APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake )

# Check if this is a source tarball build
IF(NOT IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)
  SET(SOURCE_PACKAGE 1)
ENDIF(NOT IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)

# Set package version
IF(NOT SOURCE_PACKAGE)
  # Get the version from last git tag plus number of additional commits:
  FIND_PACKAGE(Git QUIET)
  IF(GIT_FOUND)
    EXEC_PROGRAM(git ${CMAKE_CURRENT_SOURCE_DIR} ARGS describe --tags HEAD OUTPUT_VARIABLE PXAR_LIB_VERSION)
    STRING(REGEX REPLACE "^release-" "" PXAR_LIB_VERSION ${PXAR_LIB_VERSION})
    STRING(REGEX REPLACE "([v0-9.]+)-([0-9]+)-([A-Za-z0-9]+)" "\\1+\\2~\\3" PXAR_LIB_VERSION ${PXAR_LIB_VERSION})
    EXEC_PROGRAM(git ARGS status --porcelain ${CMAKE_CURRENT_SOURCE_DIR}/core OUTPUT_VARIABLE PXAR_CORE_STATUS)
    IF(PXAR_CORE_STATUS STREQUAL "")
      MESSAGE("-- Git: corelib directory is clean.")
    ELSE(PXAR_CORE_STATUS STREQUAL "")
      MESSAGE("-- Git: corelib directory is dirty:\n ${PXAR_CORE_STATUS}.")
    ENDIF(PXAR_CORE_STATUS STREQUAL "")
  ELSE(GIT_FOUND)
    SET(PXAR_LIB_VERSION ${PXAR_VERSION})
    MESSAGE("-- Git repository present, but could not find git executable.")
  ENDIF(GIT_FOUND)
ELSE(NOT SOURCE_PACKAGE)
  # If we don't have git we take the hard-set version.
  SET(PXAR_LIB_VERSION ${PXAR_VERSION})
  MESSAGE("-- Source tarball build - no repository present.")
ENDIF(NOT SOURCE_PACKAGE)
MESSAGE("-- Determined pxar corelib API version ${PXAR_LIB_VERSION}.")

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake.h" "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}")

# Set the correct build type and allow command line options:
# Set a default build type if none was specified
IF(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  MESSAGE(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
ENDIF()

SET(INSTALL_PREFIX "${PROJECT_SOURCE_DIR}" CACHE PATH "Prefix prepended to install directories")
SET(CMAKE_INSTALL_PREFIX "${INSTALL_PREFIX}" CACHE INTERNAL "Prefix prepended to install directories" FORCE)

include( cmake/Platform.cmake)


#######################################
# Check prerequisities for pXar build #
#######################################

# Include packages for threading:
FIND_PACKAGE(Threads)

# unless we are building only a dummy DTB instance, we need the FTI and USB libraries:
IF(NOT BUILD_dummydtb)
  # Find the FTDI chip drivers, either the open source or proprietary one,
  # depending on the build option we set. Use the other as fallback:
  FIND_PACKAGE(FTD2XX)
  FIND_PACKAGE(FTDI)

  # init the variable that will later hold the FTDI library name we link against
  SET(FTDI_LINK_LIBRARY "")

  IF(NOT FTDI_FOUND AND NOT FTD2XX_FOUND)
    # We have none of the two options available, this doesn't work!
    MESSAGE(FATAL_ERROR "No FTDI library found. Provide either libftdi or libftd2xx! Please refer to the documentation for detailed instructions.")
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

  # Check for libusb-1.0 package (not required on Windows w/ FTD2XX library):
  IF(NOT WIN32 OR NOT FTD2XX_FOUND)
    FIND_PACKAGE(libusb-1.0 REQUIRED)
    INCLUDE_DIRECTORIES(SYSTEM ${LIBUSB_1_INCLUDE_DIRS})
  ENDIF(NOT WIN32 OR NOT FTD2XX_FOUND)

ENDIF(NOT BUILD_dummydtb)


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
# Preparing the build of the shared pxar library #
##################################################

# Include directories
INCLUDE_DIRECTORIES(core/api core/utils)

# Always build main pxar API library;
ADD_SUBDIRECTORY(core)

# init the variable that will later hold the devices library name we might link against
SET(DEVICES_LINK_LIBRARY "")
# Add external devices directory (HV Power supplies)
ADD_SUBDIRECTORY(devices)
IF(DEVICES)
  ADD_DEFINITIONS(-DBUILD_HV)
  SET(DEVICES_LINK_LIBRARY devices)
  INCLUDE_DIRECTORIES(devices)
ENDIF(DEVICES)

# full UI and test suite (requires ROOT)
IF(BUILD_pxarui)
  # this UI and its libraries depend on ROOT:
  FIND_PACKAGE(ROOT REQUIRED)
  INCLUDE_DIRECTORIES(SYSTEM ${ROOT_INCLUDE_DIR})

  # Build the util library
  ADD_SUBDIRECTORY(util)

  # Build the analysis  library
  ADD_SUBDIRECTORY(ana)

  # Build the tests and test library
  ADD_SUBDIRECTORY(tests)

  # Build the usertests library
  ADD_SUBDIRECTORY(usertests)

  # Build the pXar executable
  ADD_SUBDIRECTORY(main)

  # Build the pXar GUI
  ADD_SUBDIRECTORY(gui)

ENDIF(BUILD_pxarui)

IF(BUILD_tools)
  # Build pxar tools
  ADD_SUBDIRECTORY(tools)
ENDIF(BUILD_tools)


#############################################
# Doxygen target to generate API reference  #
#############################################
FIND_PACKAGE(Doxygen)
IF(DOXYGEN_FOUND)
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/doc/reference/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/doc/reference/Doxyfile @ONLY)
  ADD_CUSTOM_TARGET(ref
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doc/reference/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc/reference
    COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
ENDIF(DOXYGEN_FOUND)

###############################################
# LaTeX target to compile the PDF user manual #
###############################################
# Information and the manual on the UseLATEX.cmake package can be found here
# http://www.cmake.org/Wiki/CMakeUserUseLATEX
# The code resides here:
# https://github.com/kmorel/UseLATEX

FIND_PACKAGE(LATEX)
IF(LATEX_COMPILER)
  MESSAGE(STATUS "Found LaTeX compiler.")
  INCLUDE(cmake/UseLATEX.cmake)
  ADD_LATEX_DOCUMENT(doc/usermanual/pxar-manual.tex 
    INPUTS 
    doc/usermanual/symbols.tex 
    doc/usermanual/introduction.tex
    doc/usermanual/hwOverview.tex 
    doc/usermanual/swOverview.tex
    doc/usermanual/tutorial.tex
    doc/usermanual/tests.tex
    doc/usermanual/usertests.tex
    doc/usermanual/references.tex
    NO_DEFAULT
    )
ELSE(LATEX_COMPILER)
  MESSAGE(STATUS "No LaTeX found, cannot compile user manual.")
ENDIF(LATEX_COMPILER)

#######################################################
# Print out system information useful for bug reports #
############################################### #######
include(PrintSystemInformation)
