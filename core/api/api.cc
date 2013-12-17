/**
 * pxar API class implementation
 */

#include "api.h"
#include "hal.h"

using namespace pxar;

api::api() {
  //FIXME: once we have config file parsing, we need the DTB name here:
  _hal = new hal("*");
}

api::~api() {
  //delete _dut;
  delete _hal;
}

bool api::initTB() {
  return false;
}
  
bool api::initDUT(std::vector<std::pair<uint8_t,uint8_t> > dacVector) {

  // Get the DUT up and running:
  _dut = new dut();

  // FIXME read these information from the DUT object:
  // this is highly incomplete and is only a demonstration for single ROCs
  //for(nrocs)

  rocConfig newroc;
  newroc.dacs = dacVector;
  newroc.type = 0x0; //ROC_PSI46DIGV2;
  newroc.enable = true;
  _dut->roc.push_back(newroc);

  for(size_t n = 0; n < _dut->roc.size(); n++) {
    if(_dut->roc[n].enable) _hal->initROC(n,_dut->roc[n].dacs);
  }

  //if(TBM _hal->initTBM();
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


/** DAQ functions **/
bool api::daqStart() {
  return false;
}
    
std::vector<pixel> api::daqGetEvent() {}

bool api::daqStop() {
  return false;
}




/** DUT class functions **/

int32_t dut::getEnabledPixels(size_t rocId) {}

bool dut::getPixelEnabled(uint8_t column, uint8_t row, size_t rocId) {
  return false;
}

uint8_t dut::getDAC(std::string dacName, size_t rocId) {}

std::vector<std::pair<uint8_t,uint8_t> > dut::getDACs(size_t rocId) {}

void dut::setROCEnable(size_t rocId, bool enable) {}

void dut::setTBMEnable(size_t tbmId, bool enable) {}

void dut::setPixelEnable(uint8_t column, uint8_t row, bool enable) {}

void dut::setAllPixelEnable(bool enable) {}
