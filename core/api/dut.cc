/**
 * pxar DUT class implementation
 */

#include "api.h"
#include <iostream>
#include <vector>
#include <algorithm>

using namespace pxar;

/* ===================================================================================================== */

/** Helper class to search vectors of pixelConfig, rocConfig and dacConfig for 'enable' bit
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


/* ===================================================================================================== */

/** DUT class functions **/

int32_t dut::getNEnabledPixels() {
  if (!_initialized) return 0;
  // search for pixels that have enable set
  std::vector<pixelConfig>::iterator it = std::find_if(roc.at(0).pixels.begin(),
						       roc.at(0).pixels.end(),
						       configEnableSet(true));
  int32_t count = 0;
  // loop over result, count pixel
  while(it != roc.at(0).pixels.end()){
    count++;
    it++;
  }
  return count;
}

std::vector< pixelConfig > dut::getEnabledPixels(size_t rocid) {
  std::vector< pixelConfig > result;
  if (!_initialized) return result;
  // search for pixels that have enable set
  std::vector<pixelConfig>::iterator it = std::find_if(roc.at(rocid).pixels.begin(),
						       roc.at(rocid).pixels.end(),
						       configEnableSet(true));
  while(it != roc.at(rocid).pixels.end()){
    result.push_back(*it);
    it++;
  }
  return result;
}


std::vector< rocConfig > dut::getEnabledRocs() {
  std::vector< rocConfig > result;
  if (!_initialized) return result;
  // search for pixels that have enable set
  std::vector<rocConfig>::iterator it = std::find_if(roc.begin(),
						     roc.end(),
						     configEnableSet(true));
  while(it != roc.end()){
    result.push_back(*it);
    it++;
  }
  return result;
}


bool dut::getPixelEnabled(uint8_t column, uint8_t row) {
  std::vector<pixelConfig>::iterator it = std::find_if(roc.at(0).pixels.begin(),
						       roc.at(0).pixels.end(),
						       findPixelXY(column,row));
  if(it != roc.at(0).pixels.end())
    return it->enable;
  return false;
}

bool dut::getAllPixelEnable(){
 if (!_initialized) return false;
 // search for pixels that DO NOT have enable set
 std::vector<pixelConfig>::iterator it = std::find_if(roc.at(0).pixels.begin(),
						      roc.at(0).pixels.end(),
						      configEnableSet(false));
 if(it != roc.at(0).pixels.end())
   return false; // found a disabled pixel
 else
   return true; // all pixels are enabled
}


bool dut::getModuleEnable(){
 if (!_initialized) return false;
 // check that we have all 16 ROCs
 if (roc.size()<16) return false;
 // search for pixels that DO NOT have enable set
 std::vector<rocConfig>::iterator it = std::find_if(roc.begin(),
						      roc.end(),
						      configEnableSet(false));
 if(it != roc.end()) return false;
 // 16 ROCs are enabled:
 return true;
}


pixelConfig dut::getPixelConfig(size_t rocid, uint8_t column, uint8_t row) {
  pixelConfig result = {}; // init with 0s
  if (!_initialized) return result;
  // find pixel with specified column and row
  std::vector<pixelConfig>::iterator it = std::find_if(roc.at(rocid).pixels.begin(),
						       roc.at(rocid).pixels.end(),
						       findPixelXY(column,row));
  // if pixel found, set result accordingly
  if(it != roc.at(rocid).pixels.end()){
    result = *it;
  }
  return result;
}


uint8_t dut::getDAC(size_t rocId, std::string dacName) {}

std::vector<std::pair<uint8_t,uint8_t> > dut::getDACs(size_t rocId) {}

void dut::printDACs(size_t rocId) {
  if(rocId < roc.size()) {
      std::cout << "Printing current DAC settings for ROC " << rocId << ":" << std::endl;
      for(std::vector< std::pair<uint8_t,uint8_t> >::iterator it = roc.at(rocId).dacs.begin(); 
	  it != roc.at(rocId).dacs.end(); ++it) {
	std::cout << "DAC" << (int)it->first << " = " << (int)it->second << std::endl;
      }
    }
}

void dut::setROCEnable(size_t rocId, bool enable) {

  // Check if ROC exists:
  if(rocId < roc.size())
    // Set its status to the desired value:
    roc[rocId].enable = enable;
}

void dut::setTBMEnable(size_t tbmId, bool enable) {

  // Check if TBM exists:
  if(tbmId < tbm.size())
    // Set its status to the desired value:
    tbm[tbmId].enable = enable;
}

void dut::setPixelEnable(uint8_t column, uint8_t row, bool enable) {
  if (!_initialized) return;
  // loop over all rocs (not strictly needed but used for consistency)
  for (std::vector<rocConfig>::iterator rocit = roc.begin() ; rocit != roc.end(); ++rocit){
    // find pixel with specified column and row
    std::vector<pixelConfig>::iterator it = std::find_if(rocit->pixels.begin(),
							 rocit->pixels.end(),
							 findPixelXY(column,row));
    // set enable
    if(it != rocit->pixels.end())
      it->enable = enable;
  }
}

void dut::setAllPixelEnable(bool enable) {
  // loop over all rocs (not strictly needed but used for consistency)
  for (std::vector<rocConfig>::iterator rocit = roc.begin() ; rocit != roc.end(); ++rocit){
    // loop over all pixel, set enable according to parameter
    for (std::vector<pixelConfig>::iterator pixelit = rocit->pixels.begin() ; pixelit != rocit->pixels.end(); ++pixelit){
      pixelit->enable = enable;
    }
  }
}

bool dut::status() {

  if(!_initialized)
    std::cout << "DUT structure not initialized yet!" << std::endl;

  return _initialized;
}
