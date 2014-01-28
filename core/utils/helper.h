/* This file contains helper classes which are used by API and DUT
   implementations to search API objects for certain properties */

#ifndef PXAR_HELPER_H
#define PXAR_HELPER_H



#ifdef WIN32
#include <Windows.h> 
#else
#include <sys/time.h>
#endif

/* ============================================================================== */
/** Helper function to handle sleep routines across multiple platforms 
 */
namespace util
{
  void mSleep(unsigned ms);

  std::tm localtime( std::time_t t );
} // namespace util
/* =========================================================================== */

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
    return config.enable == _isEnable;
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
    return config.mask == _isMasked;
  }
};

/** Helper class to search vectors of pixel or pixelConfig for 'column' and 'row' values
 */
class findPixelXY
{
  const uint8_t _column;
  const uint8_t _row;

public:
  findPixelXY(const uint8_t pColumn, const uint8_t pRow) : _column(pColumn), _row(pRow) {}

  template<class ConfigType>
  bool operator()(const ConfigType &config) const
  {
    return (config.row == _row) && (config.column == _column);
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
    return (config.row > _row) || (config.column > _column);
  }
};


#endif
