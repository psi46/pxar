# - Find FTD2XX installation
# This module tries to find the libftd2xx installation on your system.
# Once done this will define
#
#  FTD2XX_FOUND - system has ftd2xx
#  FTD2XX_INCLUDE_DIR - ~ the ftd2xx include directory 
#  FTD2XX_LIBRARY - Link these to use ftd2xx

FIND_PATH(FTD2XX_INCLUDE_DIR 
NAMES   ftd2xx.h WinTypes.h
PATHS   /usr/local/include
        /usr/include
        /usr/include/libftd2xx
        /usr/local/include/libftd2xx
)


FIND_LIBRARY(FTDI_LIBRARY
NAMES ftd2xx
PATHS /usr/lib
      /usr/local/lib
)

IF (FTD2XX_LIBRARY)
  IF(FTD2XX_INCLUDE_DIR)
    set(FTD2XX_FOUND TRUE)
    MESSAGE(STATUS "Found libFTD2XX: ${FTD2XX_INCLUDE_DIR}, ${FTDI_LIBRARY}")
  ELSE(FTD2XX_INCLUDE_DIR)
    set(FTD2XX_FOUND FALSE)
    MESSAGE(STATUS "libFTD2XX headers NOT FOUND. Make sure to install the development headers!")
  ENDIF(FTD2XX_INCLUDE_DIR)
ELSE (FTD2XX_LIBRARY)
    set(FTD2XX_FOUND FALSE)
    MESSAGE(STATUS "libFTD2XX NOT FOUND.")
ENDIF (FTD2XX_LIBRARY)

set(FTD2XX_INCLUDE_DIR
    ${FTD2XX_INCLUDE_DIR}
)
