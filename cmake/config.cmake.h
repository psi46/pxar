#pragma once

// CMake uses config.cmake.h to generate config.h within the build folder.
#ifndef PXAR_CONFIG_H
#define PXAR_CONFIG_H

#define PACKAGE_NAME "@CMAKE_PROJECT_NAME@"
#define PACKAGE_VERSION "@PXAR_LIB_VERSION@"
#define PACKAGE_STRING PACKAGE_NAME " " PACKAGE_VERSION
#define PACKAGE_BUGREPORT "hn-cms-pixel-psi46-testboard@cern.ch"
#define PACKAGE_FIRMWARE_URL "https://github.com/psi46/pixel-dtb-firmware/tree/master/FLASH"
#define PACKAGE_FIRMWARE "v@PXAR_FW_VERSION@"

#endif
