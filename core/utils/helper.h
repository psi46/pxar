/* This file contains helper classes which are used by API and DUT
   implementations to search API objects for certain properties */

#ifndef PXAR_HELPER_H
#define PXAR_HELPER_H

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

#include <algorithm>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

namespace pxar {
  /** Delay helper function
   *  Uses usleep() to wait the given time in milliseconds
   */
  void inline mDelay(uint32_t ms) {
    // Wait for the given time in milliseconds:
#ifdef WIN32
    Sleep(ms);
#else
    usleep(ms*1000);
#endif
  }

  /** Helper class to search vectors of pixelConfig, rocConfig and tbmConfig for 'enable' bit
   */
  class configEnableSet
  {
    const bool _isEnable;

  public:
  configEnableSet(const bool pEnable) : _isEnable(pEnable) {}

    template<class ConfigType>
      bool operator()(const ConfigType &config) const
      {
	return config.enable() == _isEnable;
      }
  };

  /** Helper class to search vectors of pixelConfig for 'mask' bit
   */
  class configMaskSet
  {
    const bool _isMasked;

  public:
  configMaskSet(const bool pMask) : _isMasked(pMask) {}

    template<class ConfigType>
      bool operator()(const ConfigType &config) const
      {
	return config.mask() == _isMasked;
      }
  };

  /** Helper class to search vectors of pixel or pixelConfig for 'column' and 'row' (and 'roc_id') values
   */
  class findPixelXY
  {
    const uint8_t _column;
    const uint8_t _row;
    const uint8_t _roc;
    const bool _check_roc;

  public:
  findPixelXY(const uint8_t pColumn, const uint8_t pRow)
    : _column(pColumn), _row(pRow), _roc(0), _check_roc(false) {}
  findPixelXY(const uint8_t pColumn, const uint8_t pRow, const uint8_t pRoc) 
    : _column(pColumn), _row(pRow), _roc(pRoc), _check_roc(true) {}

    template<class ConfigType>
      bool operator()(const ConfigType &config) const
      {
	if(_check_roc) return (config.row() == _row) && (config.column() == _column) && (config.roc() == _roc);
	return (config.row() == _row) && (config.column() == _column);
      }
  };


  /** Helper class to search vectors of pixel or pixelConfig for
      'column' and 'row' values BEYOND the values given in the constructor
  */
  class findPixelBeyondXY
  {
    const uint8_t _column;
    const uint8_t _row;

  public:
  findPixelBeyondXY(const uint8_t pColumn, const uint8_t pRow) : _column(pColumn), _row(pRow) {}

    template<class ConfigType>
      bool operator()(const ConfigType &config) const
      {
	return (config.row() > _row) || (config.column() > _column);
      }
  };

  /** Helper class to search vectors of rocConfigs in order to find the correct i2c_address
   */
  class findRoc
  {
    const uint8_t _i2c_address;

  public:
  findRoc(const uint8_t pI2cAddress) : _i2c_address(pI2cAddress) {}

    template<class ConfigType>
      bool operator()(const ConfigType &config) const
      {
	return (config.i2c_address == _i2c_address);
      }
  };

  /** Helper to compare the pixel configuration of rocConfigs
  */
  bool inline comparePixelConfiguration(const std::vector<pixelConfig> pxA, const std::vector<pixelConfig> pxB) {

    // Check the number of enabled pixels:
    if(pxA.size() != pxB.size()) return false;

    // Check the single pixels:
    for(std::vector<pixelConfig>::const_iterator pixit = pxA.begin(); pixit != pxA.end(); pixit++){
      if(std::count_if(pxB.begin(), pxB.end(), findPixelXY(pixit->column(),pixit->row())) != 1) { return false; }
    }
    return true;
  }

  /** Helper function to recover the ADC sign of analog data words
   */
  static int16_t expandSign(uint16_t x) { return (x & 0x0800) ? static_cast<int16_t>(x) - 4096 : static_cast<int16_t>(x); }

  /** Helper function to return a printed list of an integer vector, used to shield
   *  debug code from being executed if debug level is not sufficient
   */
  template <typename T>
    std::string listVector(std::vector<T> vec, bool hex = false, bool sign = false) {
    std::stringstream os;
    if(hex) { os << std::hex; }
    for(typename std::vector<T>::iterator it = vec.begin(); it != vec.end(); ++it) {
      if(sign) { os << expandSign(*it & 0x0fff) << " "; }
      else {
	if(hex) os << std::setw(4) << std::setfill('0');
	os << static_cast<int>(*it) << " ";
      }
    }
    if(hex) { os << std::dec; }
    return os.str();
  }

  /** Helper function to return a printed list of flags
   */
  std::string inline listFlags(uint32_t flags) {
    std::stringstream os;

    // No flags given:
    if(flags == 0) return "(0) ";

    if((flags&FLAG_FORCE_SERIAL) != 0) { os << "FLAG_FORCE_SERIAL, "; flags -= FLAG_FORCE_SERIAL; }
    if((flags&FLAG_CALS) != 0) { os << "FLAG_CALS, "; flags -= FLAG_CALS; }
    if((flags&FLAG_XTALK) != 0) { os << "FLAG_XTALK, "; flags -= FLAG_XTALK; }
    if((flags&FLAG_RISING_EDGE) != 0) { os << "FLAG_RISING_EDGE, "; flags -= FLAG_RISING_EDGE; }
    if((flags&FLAG_FORCE_MASKED) != 0) { os << "FLAG_FORCE_MASKED (obsolete), "; flags -= FLAG_FORCE_MASKED; }
    if((flags&FLAG_DISABLE_DACCAL) != 0) { os << "FLAG_DISABLE_DACCAL, "; flags -= FLAG_DISABLE_DACCAL; }
    if((flags&FLAG_NOSORT) != 0) { os << "FLAG_NOSORT, "; flags -= FLAG_NOSORT; }
    if((flags&FLAG_CHECK_ORDER) != 0) { os << "FLAG_CHECK_ORDER, "; flags -= FLAG_CHECK_ORDER; }
    if((flags&FLAG_FORCE_UNMASKED) != 0) { os << "FLAG_FORCE_UNMASKED, "; flags -= FLAG_FORCE_UNMASKED; }

    if(flags != 0) os << "Unknown flag: " << flags;
    return os.str();
  }

}
#endif
