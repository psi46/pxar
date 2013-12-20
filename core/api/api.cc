/**
 * pxar API class implementation
 */

#include "api.h"
#include "hal.h"
#include <iostream>
#include <algorithm>

/** Define a macro for calls to member functions through pointers to memeber functions (used in the loop expansion routines).
 *  Follows advice of http://www.parashift.com/c++-faq/macro-for-ptr-to-memfn.html
 */
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))


using namespace pxar;

api::api(std::string usbId, std::string logLevel) {

  // Set up the libpxar API/HAL logging mechanism:
  // FIXME
  std::cout << "Log level: " << logLevel << std::endl;

  // Get a new HAL instance with the DTB USB ID passed to the API constructor:
  _hal = new hal(usbId);

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

std::vector< std::pair<uint8_t, std::vector<pixel> > > api::getDebugVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
										uint32_t flags, uint32_t nTriggers) {
  // check range
  if (dacMin>dacMax){
    // FIXME: THIS SHOULD THROW A CUSTOM EXCEPTION
    std::cout << " ERROR: DacMin > DacMax! " << std::endl;
    return std::vector< std::pair<uint8_t, std::vector<pixel> > >();
  }
  // setup the correct _hal calls for this test (FIXME:DUMMYONLY)
  HalMemFnPixel pixelfn = &hal::DummyPixelTestSkeleton;
  HalMemFnRoc rocfn = &hal::DummyRocTestSkeleton;
  HalMemFnModule modulefn = &hal::DummyModuleTestSkeleton;
  // load the test parameters into vector
  std::vector<int32_t> param;
  // FIXME: NOT IMPLEMENTED:
  //param.push_back(static_cast<int32_t>dacNameToID(dacName));  
  param.push_back(static_cast<int32_t>(dacMin));
  param.push_back(static_cast<int32_t>(dacMax));
  param.push_back(static_cast<int32_t>(flags));
  param.push_back(static_cast<int32_t>(nTriggers));
  // check if the flags indicate that the user explicitly asks for serial execution of test:
  // FIXME: FLAGS NOT YET CHECKED!
  bool forceSerial = flags > 1;
  std::vector< std::vector<pixel> >* data = expandLoop(pixelfn, rocfn, modulefn, param, forceSerial);
  // repack data into the expected return format
  return repackDacScanData(data,dacMin,dacMax);

} // getPulseheightVsDAC

std::vector< std::pair<uint8_t, std::vector<pixel> > > api::getEfficiencyVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
									       uint32_t flags, uint32_t nTriggers) {}

std::vector< std::pair<uint8_t, std::vector<pixel> > > api::getThresholdVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
									      uint32_t flags, uint32_t nTriggers) {}


std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > api::getPulseheightVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
													std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
													uint32_t flags, uint32_t nTriggers){

} // getPulseheightVsDACDAC

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


std::vector< std::vector<pixel> >* api::expandLoop(HalMemFnPixel pixelfn, HalMemFnRoc rocfn, HalMemFnModule modulefn, std::vector<int32_t> param,  bool forceSerial){
  
  // pointer to vector to hold our data
  std::vector< std::vector<pixel> >* data = NULL;

  // check if we might use parallel routine on whole module: 16 ROCs
  // must be enabled and parallel execution not disabled by user
  if (_dut->getModuleEnable() && !forceSerial && modulefn != NULL){
    // execute call to HAL layer routine
    data = CALL_MEMBER_FN(*_hal,modulefn)(param);
  } else {
    // -> single ROC / ROC-by-ROC operation
    // check if all pixels are enabled
    // if so, use routine that accesses whole ROC
    if (_dut->getAllPixelEnable() && rocfn != NULL){
      // loop over all enabled ROCs
      std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();
      for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
	// execute call to HAL layer routine and save returned data in buffer
	std::vector< std::vector<pixel> >* rocdata = CALL_MEMBER_FN(*_hal,rocfn)((uint8_t) (rocit - enabledRocs.begin()), param); // rocit - enabledRocs.begin() == index
	// append rocdata to main data storage vector
	if (!data) data = rocdata;
	else {
	  data->reserve( data->size() + rocdata->size());
	  data->insert(data->end(), rocdata->begin(), rocdata->end());
	}
      } // roc loop
    } else if (pixelfn!= NULL){
      // -> we operate on single pixels
      // loop over all enabled ROCs
      std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();
      for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
	std::vector< std::vector<pixel> >* rocdata;
	std::vector<pixelConfig> enabledPixels = _dut->getEnabledPixels((uint8_t)(rocit - enabledRocs.begin()));
	for (std::vector<pixelConfig>::iterator pixit = enabledPixels.begin(); pixit != enabledPixels.end(); ++pixit){
	  // execute call to HAL layer routine and store data in buffer
	  std::vector< std::vector<pixel> >* buffer = CALL_MEMBER_FN(*_hal,pixelfn)((uint8_t) (rocit - enabledRocs.begin()), pixit->column, pixit->row, param);
	  // merge pixel data into roc data storage vector
	  if (!rocdata){
	    rocdata = buffer; // for first time call
	  } else {
	    // loop over all entries in outer vector (i.e. over vectors of pixel)
	    for (std::vector<std::vector<pixel> >::iterator vecit = buffer->begin(); vecit != buffer->end(); ++vecit){
	      std::vector<pixel>::iterator pixelit = vecit->begin();
	      // loop over all entries in buffer and add fired pixels to existing pixel vector
	      while (pixelit != vecit->end()){
		rocdata->at(vecit-buffer->begin()).push_back(*pixelit);
	      }
	    }
	  }
	} // pixel loop
	// append rocdata to main data storage vector
	if (!data) data = rocdata;
	else {
	  data->reserve( data->size() + rocdata->size());
	  data->insert(data->end(), rocdata->begin(), rocdata->end());
	}
      } // roc loop
    }// single pixel fnc
    else {
      // FIXME: THIS SHOULD THROW A CUSTOM EXCEPTION
      std::cout << " CRITICAL: LOOP EXPANSION FAILED -- NO MATCHING FUNCTION TO CALL?! " << std::endl;
      return NULL;
    }
  } // single roc fnc
  // now repack the data to join the individual ROC segments and return
  return compactRocLoopData(data, _dut->getNEnabledRocs());
} // expandLoop()



std::vector< std::pair<uint8_t, std::vector<pixel> > > api::repackDacScanData (std::vector< std::vector<pixel> >* data, uint8_t dacMin, uint8_t dacMax){
  std::vector< std::pair<uint8_t, std::vector<pixel> > > result;
  uint8_t currentDAC = dacMin;
  for (std::vector<std::vector<pixel> >::iterator vecit = data->begin(); vecit!=data->end();++vecit){
    result.push_back(std::make_pair(currentDAC, *vecit));
    currentDAC++;
  }
  if (currentDAC!=dacMax){
    // FIXME: THIS SHOULD THROW A CUSTOM EXCEPTION
    std::cout << " CRITICAL: data structure size not as expected! " << data->size() << " data blocks do not fit to " << dacMax-dacMin << " DAC values! " << std::endl;
    return std::vector< std::pair<uint8_t, std::vector<pixel> > > ();
  }
}


std::vector< std::vector<pixel> >* api::compactRocLoopData (std::vector< std::vector<pixel> >* data, uint8_t nRocs){
  if (data->size() % nRocs != 0) {
    // FIXME: THIS SHOULD THROW A CUSTOM EXCEPTION
    std::cout << " CRITICAL: data structure size not as expected! " << data->size() << " data blocks do not fit to " << nRocs << " active ROCs! " << std::endl;
    return NULL;
  }

  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();
  // loop over all rocs
  for (uint8_t rocid = 0; rocid<nRocs;rocid++){
    std::vector<pixel> pixjoined;
    // loop over each data segment belonging to this roc
    int segmentsize = data->size()/nRocs;
    for (int segment = 0; segment<segmentsize;segment++){
      // copy pixel over
      pixjoined.reserve(pixjoined.size() + data->at(segment+segmentsize*rocid).size());
      pixjoined.insert(pixjoined.end(), data->at(segment+segmentsize*rocid).begin(),data->at(segment+segmentsize*rocid).end());
    }
    result->push_back(pixjoined);
  }
  return result;
}
