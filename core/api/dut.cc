/**
 * pxar DUT class implementation
 */

#include "api.h"
#include "log.h"
#include "dictionaries.h"
#include "helper.h"
#include <vector>
#include <algorithm>

using namespace pxar;

// DUT class functions

void dut::info() {
  if (status()) {
    LOG(logINFO) << "The DUT currently contains the following objects:";

    LOG(logINFO) << std::setw(2) << tbm.size() << " TBM Cores (" << getNEnabledTbms() 
		 << " ON)";

    for(std::vector<tbmConfig>::iterator tbmIt = tbm.begin(); tbmIt != tbm.end(); tbmIt++) {
      LOG(logINFO) << "\tTBM Core " 
		   << ((tbmIt-tbm.begin())%2 == 0 ? "alpha" : "beta " )
		   << " (" << static_cast<int>(tbmIt - tbm.begin()) << "): " 
		   << (*tbmIt).dacs.size() << " registers set";
    }

    // We currently hide the possibility to enable pixels on some ROCs only,
    // so looking at ROC 0 as default is safe:
    LOG(logINFO) << std::setw(2) << roc.size() << " ROCs (" << getNEnabledRocs() 
		 << " ON) with " << roc.at(0).pixels.size() << " pixelConfigs";

    for(std::vector<rocConfig>::iterator rocIt = roc.begin(); rocIt != roc.end(); rocIt++) {
      LOG(logINFO) << "\tROC " << static_cast<int>(rocIt-roc.begin()) << ": " 
		   << (*rocIt).dacs.size() << " DACs set, Pixels: " 
		   << getNMaskedPixels(static_cast<int>(rocIt-roc.begin())) << " masked, "
		   << getNEnabledPixels(static_cast<int>(rocIt-roc.begin())) << " active.";
    }
  }
}

size_t dut::getNEnabledPixels(uint8_t rocid) {
  if (!_initialized || rocid >= roc.size()) return 0;
  return std::count_if(roc.at(rocid).pixels.begin(),roc.at(rocid).pixels.end(),configEnableSet(true));
}

size_t dut::getNEnabledPixels() {
  if (!_initialized) return 0;
  size_t nenabled = 0;
  // Loop over all ROCs
  for (std::vector<rocConfig>::iterator rocit = roc.begin() ; rocit != roc.end(); ++rocit){
    nenabled += std::count_if(rocit->pixels.begin(),rocit->pixels.end(),configEnableSet(true));
  }
  return nenabled;
}

size_t dut::getNMaskedPixels(uint8_t rocid) {
  if (!_initialized || rocid >= roc.size()) return 0;
  return std::count_if(roc.at(rocid).pixels.begin(),roc.at(rocid).pixels.end(),configMaskSet(true));
}

size_t dut::getNMaskedPixels() {
  if (!_initialized) return 0;
  size_t nmasked = 0;
  // Loop over all ROCs
  for (std::vector<rocConfig>::iterator rocit = roc.begin() ; rocit != roc.end(); ++rocit){
    nmasked += std::count_if(rocit->pixels.begin(),rocit->pixels.end(),configMaskSet(true));
  }
  return nmasked;
}

size_t dut::getNEnabledRocs() {
  if (!status()) return 0;
  // loop over result, count enabled ROCs
  size_t count = 0;
  for (std::vector<rocConfig>::iterator it = roc.begin(); it != roc.end(); ++it){
    if (it->enable()) count++;
  }
  return count;
}

size_t dut::getNRocs() {
  return roc.size();
}

std::string dut::getRocType() {
  if(roc.empty()) return "";

  // Get singleton DAC dictionary object:
  DeviceDictionary * _dict = DeviceDictionary::getInstance();

  // And get the device name from the dictionary object:
  // Returning the type of ROC 0 should be fine since we're not mixing types.
  return _dict->getName(roc.front().type);
}

std::string dut::getTbmType() {
  if(tbm.empty()) return "";

  // Get singleton DAC dictionary object:
  DeviceDictionary * _dict = DeviceDictionary::getInstance();

  // And get the device name from the dictionary object:
  return _dict->getName(tbm.front().type);
}

size_t dut::getNEnabledTbms() {
  if (!status()) return 0;
  // loop over result, count enabled TBMs
  size_t count = 0;
  for (std::vector<tbmConfig>::iterator it = tbm.begin(); it != tbm.end(); ++it){
    if (it->enable) count++;
  }
  return count;
}

size_t dut::getNTbms() {
  return tbm.size();
}


std::vector< pixelConfig > dut::getEnabledPixels(size_t rocid) {

  std::vector< pixelConfig > result;

  // Check if DUT is allright and the roc we are looking at exists:
  if (!status() || !(rocid < roc.size())) return result;

  // Search for pixels that have enable set
  for (std::vector<pixelConfig>::iterator it = roc.at(rocid).pixels.begin(); it != roc.at(rocid).pixels.end(); ++it){
    if (it->enable()) result.push_back(*it);
  }
  return result;
}

std::vector< pixelConfig > dut::getEnabledPixels() {

  std::vector< pixelConfig > result;

  // Check if DUT is allright and the roc we are looking at exists:
  if (!status()) return result;

  // Loop over all ROCs
  for (std::vector<rocConfig>::iterator rocit = roc.begin() ; rocit != roc.end(); ++rocit){
    // Search for pixels that have enable set
    for (std::vector<pixelConfig>::iterator it = rocit->pixels.begin(); it != rocit->pixels.end(); ++it){
      if (it->enable()) result.push_back(*it);
    }
  }
  return result;
}

std::vector< pixelConfig > dut::getMaskedPixels(size_t rocid) {

  std::vector< pixelConfig > result;

  // Check if DUT is allright and the roc we are looking at exists:
  if (!status() || !(rocid < roc.size())) return result;

  // Search for pixels that have enable set
  for (std::vector<pixelConfig>::iterator it = roc.at(rocid).pixels.begin(); it != roc.at(rocid).pixels.end(); ++it){
    if (it->mask()) result.push_back(*it);
  }
  return result;
}

std::vector< pixelConfig > dut::getMaskedPixels() {

  std::vector< pixelConfig > result;

  // Check if DUT is allright and the roc we are looking at exists:
  if (!status()) return result;

  // Loop over all ROCs
  for (std::vector<rocConfig>::iterator rocit = roc.begin() ; rocit != roc.end(); ++rocit){
    // Search for pixels that have enable set
    for (std::vector<pixelConfig>::iterator it = rocit->pixels.begin(); it != rocit->pixels.end(); ++it){
      if (it->mask()) result.push_back(*it);
    }
  }
  return result;
}

std::vector< bool > dut::getEnabledColumns(size_t roci2c) {

  std::vector< bool > result(52,false);

  // Check if DUT is allright and the roc we are looking at exists:
  if (!status()) return result;

  for(std::vector<rocConfig>::iterator rocit = roc.begin(); rocit != roc.end(); ++rocit){
    if(rocit->i2c_address == roci2c) {
      // Search for pixels that have enable set
      for(std::vector<pixelConfig>::iterator it = rocit->pixels.begin(); it != rocit->pixels.end(); ++it){
	if (it->enable()) result.at((*it).column()) = true;
      }
    }
  }
  return result;
}

std::vector< rocConfig > dut::getEnabledRocs() {
  std::vector< rocConfig > result;
  if (!_initialized) return result;
  // search for rocs that have enable set
  for (std::vector<rocConfig>::iterator it = roc.begin(); it != roc.end(); ++it){
    if (it->enable()) result.push_back(*it);
  }
  return result;
}

std::vector< uint8_t > dut::getEnabledRocIDs() {

  std::vector< uint8_t > result = std::vector<uint8_t>();

  if (!_initialized) return result;

  // search for rocs that have enable set
  for (std::vector<rocConfig>::iterator it = roc.begin(); it != roc.end(); ++it){
    if (it->enable()) result.push_back(static_cast<uint8_t>(it - roc.begin()));
  }
  return result;
}

std::vector< uint8_t > dut::getEnabledRocI2Caddr() {

  std::vector< uint8_t > result = std::vector<uint8_t>();

  if (!_initialized) return result;

  // search for rocs that have enable set
  for (std::vector<rocConfig>::iterator it = roc.begin(); it != roc.end(); ++it){
    if (it->enable()) result.push_back(it->i2c_address);
  }
  return result;
}

std::vector< uint8_t > dut::getRocI2Caddr() {

  std::vector< uint8_t > result = std::vector<uint8_t>();

  if (!_initialized) return result;

  // search for rocs that have enable set
  for (std::vector<rocConfig>::iterator it = roc.begin(); it != roc.end(); ++it){
    result.push_back(it->i2c_address);
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
  if(it != roc.at(0).pixels.end()) { return it->enable(); }
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
    uint8_t _register = _dict->getRegister(dacName, ROC_REG);
    return roc[rocId].dacs[_register];
  }
  throw InvalidConfig("Could not identify DAC " + dacName);
  return 0x0;
}

std::vector< std::pair<std::string,uint8_t> > dut::getDACs(size_t rocId) {

  if(status() && rocId < roc.size()) {
    std::vector< std::pair<std::string,uint8_t> > vec;

    // Get singleton DAC dictionary object:
    RegisterDictionary * _dict = RegisterDictionary::getInstance();

    for(std::map< uint8_t,uint8_t >::iterator it = roc.at(rocId).dacs.begin(); 
	it != roc.at(rocId).dacs.end(); ++it) {
      vec.push_back(std::make_pair(_dict->getName(it->first,ROC_REG),it->second));
    }
    return vec;
  }
  else return std::vector< std::pair<std::string,uint8_t> >();
}

std::vector< std::pair<std::string,uint8_t> > dut::getTbmDACs(size_t tbmId) {

  if(status() && tbmId < tbm.size()) {
    std::vector< std::pair<std::string,uint8_t> > vec;

    // Get singleton DAC dictionary object:
    RegisterDictionary * _dict = RegisterDictionary::getInstance();

    for(std::map< uint8_t,uint8_t >::iterator it = tbm.at(tbmId).dacs.begin(); 
	it != tbm.at(tbmId).dacs.end(); ++it) {
      // We need to strip the core identifier in order to look up the register name from the dictionary (&0x0F):
      vec.push_back(std::make_pair(_dict->getName((it->first&0x0F),TBM_REG),it->second));
    }
    return vec;
  }
  else return std::vector< std::pair<std::string,uint8_t> >();
}

void dut::printDACs(size_t rocId) {

  if(status() && rocId < roc.size()) {
    // Get singleton DAC dictionary object:
    RegisterDictionary * _dict = RegisterDictionary::getInstance();


    LOG(logINFO) << "Printing current DAC settings for ROC " << rocId << ":";
    for(std::map< uint8_t,uint8_t >::iterator it = roc.at(rocId).dacs.begin(); 
	it != roc.at(rocId).dacs.end(); ++it) {
      LOG(logINFO) << "DAC " << _dict->getName(it->first,ROC_REG) << " (" << static_cast<int>(it->first) << ") = " << static_cast<int>(it->second);
    }
  }
}

void dut::setROCEnable(size_t rocId, bool enable) {

  // Check if ROC exists:
  if(rocId < roc.size())
    // Set its status to the desired value:
    roc[rocId].setEnable(enable);
}

void dut::setTBMEnable(size_t tbmId, bool enable) {

  // Check if TBM exists:
  if(tbmId < tbm.size())
    // Set its status to the desired value:
    tbm[tbmId].enable = enable;
}

void dut:: maskPixel(uint8_t column, uint8_t row, bool mask) {

  if(status()) {
    // Loop over all ROCs
    for (std::vector<rocConfig>::iterator rocit = roc.begin() ; rocit != roc.end(); ++rocit){
      // Find pixel with specified column and row
      std::vector<pixelConfig>::iterator it = std::find_if(rocit->pixels.begin(),
							   rocit->pixels.end(),
							   findPixelXY(column,row));
      // Set enable bit
      if(it != rocit->pixels.end()) {
	it->setMask(mask);
      } else {
	LOG(logWARNING) << "Pixel at column " << static_cast<int>(column) << " and row " << static_cast<int>(row) << " not found for ROC " << static_cast<int>(rocit - roc.begin()) << "!" ;
      }
    }
  }
}

void dut:: maskPixel(uint8_t column, uint8_t row, bool mask, uint8_t rocid) {

  if(status() && rocid < roc.size()) {
    // Find pixel with specified column and row
    std::vector<pixelConfig>::iterator it = std::find_if(roc.at(rocid).pixels.begin(),
							 roc.at(rocid).pixels.end(),
							 findPixelXY(column,row));
    // Set mask:
    if(it != roc.at(rocid).pixels.end()){
      it->setMask(mask);
    } else {
      LOG(logWARNING) << "Pixel at column " << static_cast<int>(column) << " and row " << static_cast<int>(row) << " not found for ROC " << static_cast<int>(rocid)<< "!" ;
    }
  }
}

void dut::testPixel(uint8_t column, uint8_t row, bool enable) {

  if(status()) {
    // Loop over all ROCs
    for (std::vector<rocConfig>::iterator rocit = roc.begin() ; rocit != roc.end(); ++rocit){
      // Find pixel with specified column and row
      std::vector<pixelConfig>::iterator it = std::find_if(rocit->pixels.begin(),
							   rocit->pixels.end(),
							   findPixelXY(column,row));
      // Set enable bit
      if(it != rocit->pixels.end()) {
	it->setEnable(enable);
      } else {
	LOG(logWARNING) << "Pixel at column " << static_cast<int>(column) << " and row " << static_cast<int>(row) << " not found for ROC " << static_cast<int>(rocit - roc.begin())<< "!" ;
      }
    }
  }
}

void dut::testPixel(uint8_t column, uint8_t row, bool enable, uint8_t rocid) {

  if(status() && rocid < roc.size()) {
    // Find pixel with specified column and row
    std::vector<pixelConfig>::iterator it = std::find_if(roc.at(rocid).pixels.begin(),
							 roc.at(rocid).pixels.end(),
							 findPixelXY(column,row));
    // Set mask:
    if(it != roc.at(rocid).pixels.end()){
      it->setEnable(enable);
    } else {
      LOG(logWARNING) << "Pixel at column " << static_cast<int>(column) << " and row " << static_cast<int>(row) << " not found for ROC " << static_cast<int>(rocid)<< "!" ;
    }
  }
}

void dut::maskAllPixels(bool mask) {

  if(status()) {
    LOG(logDEBUGAPI) << "Set mask bit to " << static_cast<int>(mask) << " for all pixels on all ROCs.";
    // Loop over all ROCs
    for (std::vector<rocConfig>::iterator rocit = roc.begin() ; rocit != roc.end(); ++rocit){
      // loop over all pixel, set enable according to parameter
      for (std::vector<pixelConfig>::iterator pixelit = rocit->pixels.begin() ; pixelit != rocit->pixels.end(); ++pixelit){
	pixelit->setMask(mask);
      }
    }
  }
}

void dut::maskAllPixels(bool mask, uint8_t rocid) {

  if(status() && rocid < roc.size()) {
    LOG(logDEBUGAPI) << "Set mask bit to " << static_cast<int>(mask) << " for all pixels on ROC " << static_cast<int>(rocid);
    // loop over all pixel, set enable according to parameter
    for (std::vector<pixelConfig>::iterator pixelit = roc.at(rocid).pixels.begin() ; pixelit != roc.at(rocid).pixels.end(); ++pixelit){
      pixelit->setMask(mask);
    }
  }
}

void dut::testAllPixels(bool enable, uint8_t rocid) {

  if(status() && rocid < roc.size()) {
    LOG(logDEBUGAPI) << "Set enable bit to " << static_cast<int>(enable) << " for all pixels on ROC " << static_cast<int>(rocid);
    // loop over all pixel, set enable according to parameter
    for (std::vector<pixelConfig>::iterator pixelit = roc.at(rocid).pixels.begin() ; pixelit != roc.at(rocid).pixels.end(); ++pixelit){
      pixelit->setEnable(enable);
    }
  }
}

void dut::testAllPixels(bool enable) {

  if(status()) {
    LOG(logDEBUGAPI) << "Set enable bit to " << static_cast<int>(enable) << " for all pixels on all ROCs";
    // Loop over all ROCs
    for (std::vector<rocConfig>::iterator rocit = roc.begin() ; rocit != roc.end(); ++rocit){
      // loop over all pixel, set enable according to parameter
      for (std::vector<pixelConfig>::iterator pixelit = rocit->pixels.begin() ; pixelit != rocit->pixels.end(); ++pixelit){
	pixelit->setEnable(enable);
      }
    }
  }
}

bool dut::updateTrimBits(pixelConfig trimming, uint8_t rocid) {

  if(status() && rocid < roc.size()) {

    // Find the pixel in the given ROC pixels vector:
    std::vector<pixelConfig>::iterator px = std::find_if(roc.at(rocid).pixels.begin(),
							 roc.at(rocid).pixels.end(),
							 findPixelXY(trimming.column(),trimming.row()));
    // Pixel was not found:
    if(px == roc.at(0).pixels.end()) return false;
    // Pixel was found, set the new trimming values:
    px->setTrim(trimming.trim());
    return true;
  }
  else { return false; }
}

bool dut::updateTrimBits(uint8_t column, uint8_t row, uint8_t trim, uint8_t rocid) {

  if(status() && rocid < roc.size()) {

    // Find the pixel in the given ROC pixels vector:
    std::vector<pixelConfig>::iterator px = std::find_if(roc.at(rocid).pixels.begin(),
							 roc.at(rocid).pixels.end(),
							 findPixelXY(column,row));
    // Pixel was not found:
    if(px == roc.at(0).pixels.end()) return false;
    // Pixel was found, set the new trimming values:
    px->setTrim(trim);
    return true;
  }
  else { return false; }
}

bool dut::updateTrimBits(std::vector<pixelConfig> trimming, uint8_t rocid) {

  if(status() && rocid < roc.size()) {
    // Loop over all trimbit pixelConfigs we got as parameter:
    for (std::vector<pixelConfig>::iterator it = trimming.begin(); it != trimming.end(); ++it){

      // Find the pixel in the given ROC pixels vector:
      std::vector<pixelConfig>::iterator px = std::find_if(roc.at(rocid).pixels.begin(),
							   roc.at(rocid).pixels.end(),
							   findPixelXY(it->column(),it->row()));
      // Pixel was not found:
      if(px == roc.at(0).pixels.end()) return false;
      // Pixel was found, set the new trimming values:
      px->setTrim(it->trim());
    }
    return true;
  }
  else { return false; }
}

bool dut::status() {

  if(!_initialized || !_programmed) {
    LOG(logERROR) << "DUT structure not initialized/programmed yet!";
  }

  return (_initialized && _programmed);
}
