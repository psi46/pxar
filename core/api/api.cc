/**
 * pxar API class implementation
 */

#include "api.h"
#include "hal.h"
#include "log.h"
#include "dictionaries.h"
#include <algorithm>
#include <fstream>

/** Define a macro for calls to member functions through pointers 
 *  to member functions (used in the loop expansion routines).
 *  Follows advice of http://www.parashift.com/c++-faq/macro-for-ptr-to-memfn.html
 */
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))


using namespace pxar;

api::api(std::string usbId, std::string logLevel) {

  // Set up the libpxar API/HAL logging mechanism:
  Log::ReportingLevel() = Log::FromString(logLevel);
  LOG(logINFO) << "Log level: " << logLevel;

  // Get a new HAL instance with the DTB USB ID passed to the API constructor:
  _hal = new hal(usbId);

  // Get the DUT up and running:
  _dut = new dut();
  _dut->_initialized = false;
}

api::~api() {
  delete _dut;
  delete _hal;
}

bool api::initTestboard(std::vector<std::pair<std::string,uint8_t> > sig_delays,
                       std::vector<std::pair<std::string,double> > power_settings,
                       std::vector<std::pair<std::string,uint8_t> > pg_setup) {
  //FIXME Anything we need to do here? Probably depends on how we get the config...
  _hal->initTestboard();
  return true;
}
  
bool api::initDUT(std::string tbmtype, 
		  std::vector<std::vector<std::pair<std::string,uint8_t> > > tbmDACs,
		  std::string roctype,
		  std::vector<std::vector<std::pair<std::string,uint8_t> > > rocDACs,
		  std::vector<std::vector<pixelConfig> > rocPixels) {

  // Check if the HAL is ready:
  if(!_hal->status()) return false;

  // First initialized the API's DUT instance with the information supplied.
  // FIXME TODO: currently values are not checked for sanity! (Num pixels etc.)

  // FIXME check size of rocDACs and rocPixels agains each other

  // Initialize TBMs:
  for(std::vector<std::vector<std::pair<std::string,uint8_t> > >::iterator tbmIt = tbmDACs.begin(); tbmIt != tbmDACs.end(); ++tbmIt){
    // Prepare a new TBM configuration
    tbmConfig newtbm;
    // Set the TBM type (get value from dictionary)
    newtbm.type = 0x0; //FIXME stringToType(tbmtype);
    
    // Loop over all the DAC settings supplied and fill them into the TBM dacs
    for(std::vector<std::pair<std::string,uint8_t> >::iterator dacIt = (*tbmIt).begin(); dacIt != (*tbmIt).end(); ++dacIt) {

      // Fill the DAC pairs with the register from the dictionary:
      uint8_t dacRegister = stringToRegister(dacIt->first);
      if(dacRegister == 0x0) continue;

      uint8_t dacValue = registerRangeCheck(dacRegister, dacIt->second);
      newtbm.dacs.push_back(std::make_pair(dacRegister,dacValue));
    }

    // Done. Enable bit is already set by tbmConfig constructor.
    _dut->tbm.push_back(newtbm);
  }

  // Initialize ROCs:
  size_t nROCs = 0;

  for(std::vector<std::vector<std::pair<std::string,uint8_t> > >::iterator rocIt = rocDACs.begin(); rocIt != rocDACs.end(); ++rocIt){

    // Prepare a new ROC configuration
    rocConfig newroc;
    // Set the ROC type (get value from dictionary)
    newroc.type = 0x0; //FIXME stringToType(roctype);
    
    // Loop over all the DAC settings supplied and fill them into the ROC dacs
    for(std::vector<std::pair<std::string,uint8_t> >::iterator dacIt = (*rocIt).begin(); dacIt != (*rocIt).end(); ++dacIt){
      // Fill the DAC pairs with the register from the dictionary:
      uint8_t dacRegister = stringToRegister(dacIt->first);
      if(dacRegister == 0x0) continue;

      uint8_t dacValue = registerRangeCheck(dacRegister, dacIt->second);
      newroc.dacs.push_back(std::make_pair(dacRegister,dacValue));
    }

    // Loop over all pixelConfigs supplied:
    for(std::vector<pixelConfig>::iterator pixIt = rocPixels.at(nROCs).begin(); pixIt != rocPixels.at(nROCs).end(); ++pixIt){
      // Push the pixelConfigs into the rocConfig:
      newroc.pixels.push_back(*pixIt);
    }

    // Done. Enable bit is already set by rocConfig constructor.
    _dut->roc.push_back(newroc);
    nROCs++;
  }

  // FIXME start programming the devices here! WATCH OUT, DICTIONARIES REQUIRED BEFORE!
  /*
  for(size_t n = 0; n < _dut->roc.size(); n++) {
    if(_dut->roc[n].enable) _hal->initROC(n,_dut->roc[n].dacs);
  }
  */

  _dut->_initialized = true;
  return true;
}

// API status function, checks HAL and DUT statuses
bool api::status() {
  if(_hal->status() && _dut->status()) return true;
  return false;
}

// Return the register id of the DAC specified by "name", return 0x0 if invalid:
uint8_t api::stringToRegister(std::string name) {

  // Convert the name to lower case for comparison:
  std::transform(name.begin(), name.end(), name.begin(), ::tolower);
  LOG(logDEBUGAPI) << "Name to fetch register for: " << name;

  // Get singleton DAC dictionary object:
  RegisterDictionary * _dict = RegisterDictionary::getInstance();

  // And get the register value from the dictionary object:
  uint8_t _register = _dict->getRegister(name);
  LOG(logDEBUGAPI) << "Register return: " << (int)_register;

  if(_register == 0x0) {LOG(logERROR) << "Invalid register name " << name << "!";}
  return _register;
}

// Check if the given value lies within the valid range of the DAC. If value lies above/below valid range
// return the upper/lower bondary. If value lies wqithin the range, return the value
uint8_t api::registerRangeCheck(uint8_t regId, uint8_t value) {

  // Get singleton DAC dictionary object:
  RegisterDictionary * _dict = RegisterDictionary::getInstance();
  uint16_t regLimit = 1;

  for(int i = 0; i < _dict->getSize(regId); i++) {regLimit *= 2;}
  LOG(logDEBUGAPI) << "Upper limit of register " << (int)regId << " is " << regLimit;
  
  if(value <= 0) {
    LOG(logWARNING) << "Register range underflow, set register " << (int)regId << " to 0.";
    value = 0;
  }
  else if(value >= regLimit) {
    uint8_t limit = (uint8_t)(regLimit - 1);
    LOG(logWARNING) << "Register range overflow, set register " << (int)regId << " to " << (int)limit << ".";
    value = limit;
  }

  return value;
}



/** DTB functions **/

bool api::flashTB(std::string filename) {

  if(_hal->status() || _dut->status()) {
    LOG(logERROR) << "The testboard should only be flashed without initialization"
		  << " and with all attached DUTs powered down.";
    LOG(logERROR) << "Please power cycle the testboard and flash directly after startup!";
    return false;
  }

  // Try to open the flash file
  std::ifstream flashFile;

  LOG(logINFO) << "Trying to open " << filename;
  flashFile.open(filename.c_str(), std::ifstream::in);
  if(!flashFile.is_open()) {
    LOG(logERROR) << "Could not open specified DTB flash file \"" << filename<< "\"!";
    return false;
  }
  
  // Call the HAL routine to do the flashing:
  bool status = false;
  status = _hal->flashTestboard(flashFile);
  flashFile.close();
  
  return status;
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


void api::HVoff() {
  _hal->HVoff();
}

void api::HVon() {
  _hal->HVon();
}

void api::Poff() {
  _hal->Poff();
}

void api::Pon() {
  _hal->Pon();
}

  
/** TEST functions **/

bool api::setDAC(std::string dacName, uint8_t dacValue) {
  return false;
}

std::vector< std::pair<uint8_t, std::vector<pixel> > > api::getPulseheightVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
										uint16_t flags, uint32_t nTriggers) {}

std::vector< std::pair<uint8_t, std::vector<pixel> > > api::getDebugVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
										uint16_t flags, uint32_t nTriggers) {
  // check range
  if (dacMin>dacMax){
    // FIXME: THIS SHOULD THROW A CUSTOM EXCEPTION
    LOG(logERROR) << "DacMin > DacMax! ";
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
  std::vector< std::pair<uint8_t, std::vector<pixel> > >* result = repackDacScanData(data,dacMin,dacMax);
  delete data;
  return *result;

} // getPulseheightVsDAC

std::vector< std::pair<uint8_t, std::vector<pixel> > > api::getEfficiencyVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
									       uint16_t flags, uint32_t nTriggers) {}

std::vector< std::pair<uint8_t, std::vector<pixel> > > api::getThresholdVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
									      uint16_t flags, uint32_t nTriggers) {}


std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > api::getPulseheightVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
													std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
													uint16_t flags, uint32_t nTriggers){

} // getPulseheightVsDACDAC

std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > api::getEfficiencyVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
												       std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
												       uint16_t flags, uint32_t nTriggers) {}

std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > api::getThresholdVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
												      std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
												      uint16_t flags, uint32_t nTriggers) {}

std::vector<pixel> api::getPulseheightMap(uint16_t flags, uint32_t nTriggers) {}

std::vector<pixel> api::getEfficiencyMap(uint16_t flags, uint32_t nTriggers) {}

std::vector<pixel> api::getThresholdMap(uint16_t flags, uint32_t nTriggers) {}
  
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

    LOG(logDEBUGAPI) << "\"The Loop\" contains one call to \'modulefn\'";
    // execute call to HAL layer routine
    data = CALL_MEMBER_FN(*_hal,modulefn)(param);
  } 
  else {

    // -> single ROC / ROC-by-ROC operation
    // check if all pixels are enabled
    // if so, use routine that accesses whole ROC
    if (_dut->getAllPixelEnable() && rocfn != NULL){

      // loop over all enabled ROCs
      std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();

      LOG(logDEBUGAPI) << "\"The Loop\" contains " << enabledRocs.size() << " calls to \'rocfn\'";

      for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
	// execute call to HAL layer routine and save returned data in buffer
	std::vector< std::vector<pixel> >* rocdata = CALL_MEMBER_FN(*_hal,rocfn)((uint8_t) (rocit - enabledRocs.begin()), param); // rocit - enabledRocs.begin() == index
	// append rocdata to main data storage vector
	if (!data) data = rocdata;
	else {
	  data->reserve( data->size() + rocdata->size());
	  data->insert(data->end(), rocdata->begin(), rocdata->end());
	  delete rocdata;
	}
      } // roc loop
    } 
    else if (pixelfn != NULL){

      // -> we operate on single pixels
      // loop over all enabled ROCs
      std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();

      LOG(logDEBUGAPI) << "\"The Loop\" contains " << enabledRocs.size() << " enabled ROCs.";

      for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
	std::vector< std::vector<pixel> >* rocdata = NULL;
	std::vector<pixelConfig> enabledPixels = _dut->getEnabledPixels((uint8_t)(rocit - enabledRocs.begin()));


	LOG(logDEBUGAPI) << "\"The Loop\" for the current ROC contains " \
			 << enabledPixels.size() << " calls to \'pixelfn\'";

	for (std::vector<pixelConfig>::iterator pixit = enabledPixels.begin(); pixit != enabledPixels.end(); ++pixit) {
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
		pixelit++;
	      }
	    }
	    delete buffer;
	  }
	} // pixel loop
	// append rocdata to main data storage vector
	if (!data) data = rocdata;
	else {
	  data->reserve( data->size() + rocdata->size());
	  data->insert(data->end(), rocdata->begin(), rocdata->end());
	  delete rocdata;
	}
      } // roc loop
    }// single pixel fnc
    else {
      // FIXME: THIS SHOULD THROW A CUSTOM EXCEPTION
      LOG(logCRITICAL) << "LOOP EXPANSION FAILED -- NO MATCHING FUNCTION TO CALL?!";
      return NULL;
    }
  } // single roc fnc
  // now repack the data to join the individual ROC segments and return
  std::vector< std::vector<pixel> >* compactdata = compactRocLoopData(data, _dut->getNEnabledRocs());
  delete data; // clean up
  return compactdata;
} // expandLoop()



std::vector< std::pair<uint8_t, std::vector<pixel> > >* api::repackDacScanData (std::vector< std::vector<pixel> >* data, uint8_t dacMin, uint8_t dacMax){
  std::vector< std::pair<uint8_t, std::vector<pixel> > >* result = new std::vector< std::pair<uint8_t, std::vector<pixel> > >();
  uint8_t currentDAC = dacMin;
  for (std::vector<std::vector<pixel> >::iterator vecit = data->begin(); vecit!=data->end();++vecit){
    result->push_back(std::make_pair(currentDAC, *vecit));
    currentDAC++;
  }
  if (currentDAC!=dacMax){
    // FIXME: THIS SHOULD THROW A CUSTOM EXCEPTION
    LOG(logCRITICAL) << "data structure size not as expected! " << data->size() << " data blocks do not fit to " << dacMax-dacMin << " DAC values!";
    delete result;
    return NULL;
  }
  return result;
}


std::vector< std::vector<pixel> >* api::compactRocLoopData (std::vector< std::vector<pixel> >* data, uint8_t nRocs){
  if (data->size() % nRocs != 0) {
    // FIXME: THIS SHOULD THROW A CUSTOM EXCEPTION
    LOG(logCRITICAL) << "data structure size not as expected! " << data->size() << " data blocks do not fit to " << nRocs << " active ROCs!";
    return NULL;
  }

  // the size of the data blocks of each ROC
  int segmentsize = data->size()/nRocs;

  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();
  // copy data segment of first ROC into the main data vector
  result->insert(result->end(), data->begin(), data->begin()+segmentsize);

  // loop over all remaining rocs to merge their data segments into one
  for (uint8_t rocid = 1; rocid<nRocs;rocid++){
    std::vector<pixel> pixjoined;
    // loop over each data segment belonging to this roc
    for (int segment = 0; segment<segmentsize;segment++){
      // copy pixel over
      pixjoined.reserve(pixjoined.size() + data->at(segment+segmentsize*rocid).size());
      pixjoined.insert(pixjoined.end(), data->at(segment+segmentsize*rocid).begin(),data->at(segment+segmentsize*rocid).end());
    }
    result->push_back(pixjoined);
  }
  return result;
}
