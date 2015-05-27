/* This file contains helper classes and functions used bu the DTB emulator. */

#ifndef PXAR_DUMMY_HELPER_H
#define PXAR_DUMMY_HELPER_H

/** Cannot use stdint.h when running rootcint on WIN32 */
#if ((defined WIN32) && (defined __CINT__))
typedef int int32_t;
typedef short int int16_t;
typedef unsigned int uint32_t;
typedef unsigned short int uint16_t;
typedef unsigned char uint8_t;
#else
#include <stdint.h>
#endif

#ifdef WIN32
#ifdef __CINT__
#include <Windows4Root.h>
#else
#include <Windows.h>
#endif // __CINT__
#else
#include <unistd.h>
#endif // WIN32

#include "api.h"
#include "datatypes.h"
#include <stdlib.h>

namespace pxar {
  
  pxar::pixel getNoiseHit(uint8_t rocid, size_t i, size_t j);
  bool isInTornadoRegion(size_t dac1min, size_t dac1max, size_t dac1, size_t dac2min, size_t dac2max, size_t dac2);
  void fillEvent(pxar::Event * evt, uint8_t rocid, size_t col, size_t row, uint32_t flags);
  void fillRawEvent(pxar::rawEvent * evt, uint8_t rocid, size_t col, size_t row, uint32_t flags);
  void fillRawData(std::vector<uint16_t> &data, uint8_t tbm, uint8_t nrocs, size_t col, size_t row, uint32_t flags = 0);
  
}

#endif
