/**
 * pxar API class implementation
 */

#include "api.h"
#include "hal.h"
#include <iostream>
#include <algorithm>

using namespace pxar;

api::api() {
  //FIXME: once we have config file parsing, we need the DTB name here:
  _hal = new hal("*");

  // Get the DUT up and running:
  _dut = new dut();
  _dut->_initialized = false;
}

api::~api() {
  //delete _dut;
  delete _hal;
}

bool api::initTestboard() {
  //FIXME Anything we need to do here? Probably depends on how we get the config...
  _hal->initTestboard();
  return true;
}
  
bool api::initDUT(std::vector<std::pair<uint8_t,uint8_t> > dacVector) {

  // Check if the HAL is ready:
  if(!_hal->status()) return false;

  // FIXME read these information from the DUT object:
  // this is highly incomplete and is only a demonstration for single ROCs
  //for(nrocs)

  rocConfig newroc = {};
  newroc.dacs = dacVector;
  for (uint8_t column=1;column<=52;column++){
    for (uint8_t row=1;row<=80;row++){
      pixelConfig newpix = {};
      newpix.row = row;
      newpix.column = column;
      newpix.enable = true;
      newpix.mask = false;
      newpix.trim = column+row; // arbitray but varying values for debugging
      newroc.pixels.push_back(newpix);
    }
  }
  newroc.type = 0x0; //ROC_PSI46DIGV2;
  newroc.enable = true;
  _dut->roc.push_back(newroc);

  for(size_t n = 0; n < _dut->roc.size(); n++) {
    if(_dut->roc[n].enable) _hal->initROC(n,_dut->roc[n].dacs);
  }

  //if(TBM _hal->initTBM();
  _dut->_initialized = true;
  return true;
}


bool api::status() {
  if(_hal->status() && _dut->status()) return true;
  return false;
}



/** DTB fcuntions **/

bool api::flashTB(std::string filename) {
  return false;
}

double api::getTBia() {
  return _hal->getTBia();
}

double api::getTBva() {
  return _hal->getTBva();
}

double api::getTBid() {
  return _hal->getTBid();
}

double api::getTBvd() {
  return _hal->getTBvd();
}

  
/** TEST functions **/

bool api::setDAC(std::string dacName, uint8_t dacValue) {
  return false;
}

std::vector< std::pair<uint8_t, std::vector<pixel> > > api::getPulseheightVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
										uint32_t flags, uint32_t nTriggers) {}

std::vector< std::pair<uint8_t, std::vector<pixel> > > api::getEfficiencyVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
									       uint32_t flags, uint32_t nTriggers) {}

std::vector< std::pair<uint8_t, std::vector<pixel> > > api::getThresholdVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
									      uint32_t flags, uint32_t nTriggers) {}

std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > api::getPulseheightVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
													std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
													uint32_t flags, uint32_t nTriggers){}

std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > api::getEfficiencyVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
												       std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
												       uint32_t flags, uint32_t nTriggers) {}

std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > api::getThresholdVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
												      std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
												      uint32_t flags, uint32_t nTriggers) {}

std::vector<pixel> api::getPulseheightMap(uint32_t flags, uint32_t nTriggers) {}

std::vector<pixel> api::getEfficiencyMap(uint32_t flags, uint32_t nTriggers) {}

std::vector<pixel> api::getThresholdMap(uint32_t flags, uint32_t nTriggers) {}
  
int32_t api::getReadbackValue(std::string parameterName) {}

int32_t api::debug_ph(int32_t col, int32_t row, int32_t trim, int16_t nTriggers) {

  // Make sure our DUT is properly configured:
  if(!status()) return -1;
  return _hal->PH(col,row,trim,nTriggers);
}


/** DAQ functions **/
bool api::daqStart() {
  return false;
}
    
std::vector<pixel> api::daqGetEvent() {}

bool api::daqStop() {
  return false;
}



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


bool dut::getPixelEnabled(uint8_t column, uint8_t row) {
  std::vector<pixelConfig>::iterator it = std::find_if(roc.at(0).pixels.begin(),
						       roc.at(0).pixels.end(),
						       findPixelXY(column,row));
  if(it != roc.at(0).pixels.end())
    return it->enable;
  return false;
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
