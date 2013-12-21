/**
 * pxar DUT class implementation
 */

#include "api.h"
#include "log.h"
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
  if (!status()) return 0;
  // loop over result, count enabled pixel
  int32_t count = 0;
  for (std::vector<pixelConfig>::iterator it = roc.at(0).pixels.begin(); it != roc.at(0).pixels.end(); ++it){
    if (it->enable) count++;
  }
  return count;
}

int32_t dut::getNEnabledRocs() {
  if (!status()) return 0;
  // loop over result, count enabled ROCs
  int32_t count = 0;
  for (std::vector<rocConfig>::iterator it = roc.begin(); it != roc.end(); ++it){
    if (it->enable) count++;
  }
  return count;
}


std::vector< pixelConfig > dut::getEnabledPixels(size_t rocid) {

  std::vector< pixelConfig > result;

  // Check if DUT is allright and the roc we are looking at exists:
  if (!status() || !(rocid < roc.size())) return result;

  // Search for pixels that have enable set
  for (std::vector<pixelConfig>::iterator it = roc.at(rocid).pixels.begin(); it != roc.at(rocid).pixels.end(); ++it){
    if (it->enable) result.push_back(*it);
  }
  return result;
}


std::vector< rocConfig > dut::getEnabledRocs() {
  std::vector< rocConfig > result;
  if (!status()) return result;
  // search for rocs that have enable set
  for (std::vector<rocConfig>::iterator it = roc.begin(); it != roc.end(); ++it){
    if (it->enable) result.push_back(*it);
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
 if (!status()) return false;
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
 if (!status()) return false;
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

  pixelConfig result; // initialized with 0 by constructor
  if (!status()) return result;
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
    LOG(logINFO) << "Printing current DAC settings for ROC " << rocId << ":";
    for(std::vector< std::pair<uint8_t,uint8_t> >::iterator it = roc.at(rocId).dacs.begin(); 
	it != roc.at(rocId).dacs.end(); ++it) {
      LOG(logINFO) << "DAC" << (int)it->first << " = " << (int)it->second;
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

  // loop over all rocs (not strictly needed but used for consistency)
  for (std::vector<rocConfig>::iterator rocit = roc.begin() ; rocit != roc.end(); ++rocit){
    // find pixel with specified column and row
    std::vector<pixelConfig>::iterator it = std::find_if(rocit->pixels.begin(),
							 rocit->pixels.end(),
							 findPixelXY(column,row));
    // set enable
    if(it != rocit->pixels.end()){
      it->enable = enable;
    } else {
      LOG(logWARNING) << "Pixel at column " << (int) column << " and row " << (int) row << " not found for ROC " << (int) (rocit - roc.begin())<< "!" ;
    }
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

  if(!_initialized) {
    LOG(logERROR) << "DUT structure not initialized yet!";
  }

  return _initialized;
}
