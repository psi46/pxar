/**
 * pxar DUT class implementation
 */

#include "api.h"
#include "log.h"
#include "dictionaries.h"
#include <vector>
#include <algorithm>

using namespace pxar;

/* =========================================================================== */

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


/* =========================================================================== */

/** DUT class functions **/

void dut::info() {
  if (status()) {
    LOG(logINFO) << "The DUT currently contains the following objects:";

    LOG(logINFO) << std::setw(2) << tbm.size() << " TBMs (" << getNEnabledTbms() 
		 << " ON)";

    size_t nTBMs = 0;
    for(std::vector<tbmConfig>::iterator tbmIt = tbm.begin(); tbmIt != tbm.end(); tbmIt++) {
      LOG(logINFO) << "\tTBM " << nTBMs << ": " 
		   << (*tbmIt).dacs.size() << " DACs set";
      nTBMs++;
    }

    // We currently hide the possibility to enable pixels on some ROCs only,
    // so looking at ROC 0 as default is safe:
    LOG(logINFO) << std::setw(2) << roc.size() << " ROCs (" << getNEnabledRocs() 
		 << " ON) with " << roc.at(0).pixels.size() << " pixelConfigs (" 
		 << getNEnabledPixels() << " ON).";

    size_t nROCs = 0;
    for(std::vector<rocConfig>::iterator rocIt = roc.begin(); rocIt != roc.end(); rocIt++) {
      LOG(logINFO) << "\tROC " << nROCs << ": " 
		   << (*rocIt).dacs.size() << " DACs set, " 
		   << getNMaskedPixels(nROCs) << " pixels masked";
      
      nROCs++;
    }
  }
}

int32_t dut::getNEnabledPixels() {
  if (!status()) return 0;
  // loop over result, count enabled pixel
  int32_t count = 0;
  // We currently hide the possibility to enable pixels on some ROCs only,
  // so looking at ROC 0 as default is safe:
  for (std::vector<pixelConfig>::iterator it = roc.at(0).pixels.begin(); it != roc.at(0).pixels.end(); ++it){
    if (it->enable) count++;
  }
  return count;
}

int32_t dut::getNMaskedPixels(size_t rocid) {
  if (!status() || !(rocid < roc.size())) return 0;
  // loop over result, count masked pixel
  int32_t count = 0;

  for (std::vector<pixelConfig>::iterator it = roc.at(rocid).pixels.begin(); it != roc.at(rocid).pixels.end(); ++it){
    if (it->mask) count++;
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

int32_t dut::getNEnabledTbms() {
  if (!status()) return 0;
  // loop over result, count enabled TBMs
  int32_t count = 0;
  for (std::vector<tbmConfig>::iterator it = tbm.begin(); it != tbm.end(); ++it){
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
  if (!_initialized) return result;
  // search for rocs that have enable set
  for (std::vector<rocConfig>::iterator it = roc.begin(); it != roc.end(); ++it){
    if (it->enable) result.push_back(*it);
  }
  return result;
}

std::vector< tbmConfig > dut::getEnabledTbms() {
  std::vector< tbmConfig > result;
  if (!_initialized) return result;
  // search for rocs that have enable set
  for (std::vector<tbmConfig>::iterator it = tbm.begin(); it != tbm.end(); ++it){
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


uint8_t dut::getDAC(size_t rocId, std::string dacName) {

  if(status() && rocId < roc.size()) {  
    // Convert the name to lower case for comparison:
    std::transform(dacName.begin(), dacName.end(), dacName.begin(), ::tolower);

    // Get singleton DAC dictionary object:
    RegisterDictionary * _dict = RegisterDictionary::getInstance();

    // And get the register value from the dictionary object:
    uint8_t _register = _dict->getRegister(dacName);
    return roc[rocId].dacs[_register];
  }
  // FIXME throw error
  else return 0x0;
}

std::vector< std::pair<uint8_t,uint8_t> > dut::getDACs(size_t rocId) {

  if(status() && rocId < roc.size()) {
    std::vector< std::pair<uint8_t,uint8_t> > vec;

    for(std::map< uint8_t,uint8_t >::iterator it = roc.at(rocId).dacs.begin(); 
	it != roc.at(rocId).dacs.end(); ++it) {
      vec.push_back(*it);
    }
    return vec;
  }
  else return std::vector< std::pair<uint8_t,uint8_t> >();
}

void dut::printDACs(size_t rocId) {

  if(status() && rocId < roc.size()) {
    LOG(logINFO) << "Printing current DAC settings for ROC " << rocId << ":";
    for(std::map< uint8_t,uint8_t >::iterator it = roc.at(rocId).dacs.begin(); 
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

void dut:: setPixelMask(uint8_t rocid, uint8_t column, uint8_t row, bool mask) {

  if(status() && rocid < roc.size()) {
    // Find pixel with specified column and row
    std::vector<pixelConfig>::iterator it = std::find_if(roc.at(rocid).pixels.begin(),
							 roc.at(rocid).pixels.end(),
							 findPixelXY(column,row));
    // Set mask:
    if(it != roc.at(rocid).pixels.end()){
      it->mask = mask;
    } else {
      LOG(logWARNING) << "Pixel at column " << (int) column << " and row " << (int) row << " not found for ROC " << (int)(rocid)<< "!" ;
    }
  }
}

void dut::setPixelEnable(uint8_t column, uint8_t row, bool enable) {

  if(status()) {
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

  if(!_initialized || !_programmed) {
    LOG(logERROR) << "DUT structure not initialized/programmed yet!";
  }

  return (_initialized && _programmed);
}
