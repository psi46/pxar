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

  LOG(logQUIET) << "Instanciating API for " << PACKAGE_STRING;

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

  // FIXME Missing configuration:
  //  * power settings
  //  * pattern generator setup

  // Take care of the signal delay settings:
  std::vector<std::pair<uint8_t,uint8_t> > delays;
  for(std::vector<std::pair<std::string,uint8_t> >::iterator sigIt = sig_delays.begin(); sigIt != sig_delays.end(); ++sigIt) {
    // Fill the DAC pairs with the register from the dictionary:
    uint8_t sigRegister = stringToRegister(sigIt->first);
    uint8_t sigValue = registerRangeCheck(sigRegister, sigIt->second);

    delays.push_back(std::make_pair(sigRegister,sigValue));
  }
    
  _hal->initTestboard(delays);
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

  // FIXME check size of rocDACs and rocPixels against each other

  // FIXME masking not done yet.

  // Initialize TBMs:
  for(std::vector<std::vector<std::pair<std::string,uint8_t> > >::iterator tbmIt = tbmDACs.begin(); tbmIt != tbmDACs.end(); ++tbmIt){
    // Prepare a new TBM configuration
    tbmConfig newtbm;
    // Set the TBM type (get value from dictionary)
    newtbm.type = stringToDeviceCode(tbmtype);
    if(newtbm.type == 0x0) return false;
    
    // Loop over all the DAC settings supplied and fill them into the TBM dacs
    for(std::vector<std::pair<std::string,uint8_t> >::iterator dacIt = (*tbmIt).begin(); dacIt != (*tbmIt).end(); ++dacIt) {

      // Fill the DAC pairs with the register from the dictionary:
      uint8_t dacRegister = stringToRegister(dacIt->first);
      // FIXME 0x0 is valid TBM register!
      /*      if(dacRegister == 0x0) {
	continue;
	LOG(logERROR) << "Invalid register name \"" << dacIt->first << "\"!";
	}*/

      uint8_t dacValue = registerRangeCheck(dacRegister, dacIt->second);
      std::pair<std::map<uint8_t,uint8_t>::iterator,bool> ret;
      ret = newtbm.dacs.insert( std::make_pair(dacRegister,dacValue) );
      if(ret.second == false) {
	LOG(logWARNING) << "Overwriting existing DAC \"" << dacIt->first 
			<< "\" value " << (int)ret.first->second
			<< " with " << (int)dacValue;
	newtbm.dacs[dacRegister] = dacValue;
      }
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
    newroc.type = stringToDeviceCode(roctype);
    if(newroc.type == 0x0) return false;
    
    // Loop over all the DAC settings supplied and fill them into the ROC dacs
    for(std::vector<std::pair<std::string,uint8_t> >::iterator dacIt = (*rocIt).begin(); dacIt != (*rocIt).end(); ++dacIt){
      // Fill the DAC pairs with the register from the dictionary:
      uint8_t dacRegister = stringToRegister(dacIt->first);
      if(dacRegister == 0x0) {
	continue;
	LOG(logERROR) << "Invalid register name \"" << dacIt->first << "\"!";
      }

      uint8_t dacValue = registerRangeCheck(dacRegister, dacIt->second);
      std::pair<std::map<uint8_t,uint8_t>::iterator,bool> ret;
      ret = newroc.dacs.insert( std::make_pair(dacRegister,dacValue) );
      if(ret.second == false) {
	LOG(logWARNING) << "Overwriting existing DAC \"" << dacIt->first 
			<< "\" value " << (int)ret.first->second
			<< " with " << (int)dacValue;
	newroc.dacs[dacRegister] = dacValue;
      }
    }

    // Loop over all pixelConfigs supplied:
    for(std::vector<pixelConfig>::iterator pixIt = rocPixels.at(nROCs).begin(); pixIt != rocPixels.at(nROCs).end(); ++pixIt) {
      // Check the trim value to be within boundaries:
      if((*pixIt).trim > 15) {
	LOG(logWARNING) << "Pixel " << (int)(*pixIt).column << ", " << (int)(*pixIt).row << " trim value " << (int)(*pixIt).trim << " exceeds limit. Set to 15.";
	(*pixIt).trim = 15;
      }
      // Push the pixelConfigs into the rocConfig:
      newroc.pixels.push_back(*pixIt);
    }

    // Done. Enable bit is already set by rocConfig constructor.
    _dut->roc.push_back(newroc);
    nROCs++;
  }

  // All data is stored in the DUT struct, now programming it.
  _dut->_initialized = true;
  return programDUT();
}

bool api::programDUT() {

  if(!_dut->_initialized) {
    LOG(logERROR) << "DUT not initialized, unable to program it.";
    return false;
  }

  // Start programming the devices here!

  // FIXME TBMs not programmed at all right now!
  // FIXME Device types not transmitted yet!

  // FIXME maybe go over expandLoop here?
  LOG(logDEBUGAPI) << "Programming ROCS...";
  std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();
  for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
    _hal->initROC((uint8_t)(rocit - enabledRocs.begin()),(*rocit).dacs, (*rocit).pixels);
  }

  // The DUT is programmed, everything all right:
  _dut->_programmed = true;

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
  LOG(logDEBUGAPI) << "Looking up register for: \"" << name << "\"";

  // Get singleton DAC dictionary object:
  RegisterDictionary * _dict = RegisterDictionary::getInstance();

  // And get the register value from the dictionary object:
  uint8_t _register = _dict->getRegister(name);
  LOG(logDEBUGAPI) << "Register return: " << (int)_register;

  return _register;
}

// Check if the given value lies within the valid range of the DAC. If value lies above/below valid range
// return the upper/lower bondary. If value lies wqithin the range, return the value
uint8_t api::registerRangeCheck(uint8_t regId, uint8_t value) {

  // Get singleton DAC dictionary object:
  RegisterDictionary * _dict = RegisterDictionary::getInstance();

  // Read register value limit:
  uint8_t regLimit = _dict->getSize(regId);
  LOG(logDEBUGAPI) << "Max. value of register " << (int)regId << " is " << (int)regLimit;
  
  if(value > regLimit) {
    LOG(logWARNING) << "Register range overflow, set register " << (int)regId 
		    << " to " << (int)regLimit << " (was: " << (int)value << ")";
    value = (uint8_t)regLimit;
  }

  return value;
}

// Return the device code for the given name, return 0x0 if invalid:
uint8_t api::stringToDeviceCode(std::string name) {

  // Convert the name to lower case for comparison:
  std::transform(name.begin(), name.end(), name.begin(), ::tolower);
  LOG(logDEBUGAPI) << "Looking up device type for \"" << name << "\"";

  // Get singleton device dictionary object:
  DeviceDictionary * _devices = DeviceDictionary::getInstance();

  // And get the device code from the dictionary object:
  uint8_t _code = _devices->getDevCode(name);
  LOG(logDEBUGAPI) << "Device type return: " << (int)_code;

  if(_code == 0x0) {LOG(logCRITICAL) << "Unknown device \"" << (int)_code << "\"!";}
  return _code;
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
  // Reset the programmed state of the DUT (lost by turning off power)
  _dut->_programmed = false;
}

void api::Pon() {
  _hal->Pon();
  // Re-program the DUT after power has been switched on:
  programDUT();
}

bool api::SignalProbe(std::string probe, std::string name) {
  
  // Convert the probe name to lower case for comparison:
  std::transform(probe.begin(), probe.end(), probe.begin(), ::tolower);
  
  // Convert the name to lower case for comparison:
  std::transform(name.begin(), name.end(), name.begin(), ::tolower);
  
  // Digital signal probes:
  if(probe.compare(0,1,"d") == 0) {
    
    LOG(logDEBUGAPI) << "Looking up digital probe signal for: \"" << name << "\"";
  
    // Get singleton Probe dictionary object:
    ProbeDictionary * _dict = ProbeDictionary::getInstance();
  
    // And get the register value from the dictionary object:
    uint8_t signal = _dict->getSignal(name);
    LOG(logDEBUGAPI) << "Probe signal return: " << (int)signal;

    // Select the correct probe for the output:
    if(probe.compare("d1") == 0) {
      _hal->SignalProbeD1(signal);
      return true;
    }
    else if(probe.compare("d2") == 0) {
      _hal->SignalProbeD2(signal);
      return true;
    }
  }
  // Analog signal probes:
  else if(probe.compare(0,1,"a") == 0) {

    LOG(logDEBUGAPI) << "Looking up analog probe signal for: \"" << name << "\"";

    // Get singleton Probe dictionary object:
    ProbeADictionary * _dict = ProbeADictionary::getInstance();
  
    // And get the register value from the dictionary object:
    uint8_t signal = _dict->getSignal(name);
    LOG(logDEBUGAPI) << "Probe signal return: " << (int)signal;

    // Select the correct probe for the output:
    if(probe.compare("a1") == 0) {
      _hal->SignalProbeA1(signal);
      return true;
    }
    else if(probe.compare("a2") == 0) {
      _hal->SignalProbeA2(signal);
      return true;
    }
  }
    
  LOG(logERROR) << "Invalid probe name \"" << probe << "\" selected!";
  return false;
}


  
/** TEST functions **/

bool api::setDAC(std::string dacName, uint8_t dacValue, int8_t rocid) {
  
  // Get the register number and check the range from dictionary:
  uint8_t dacRegister = stringToRegister(dacName);
  if(dacRegister == 0x0) {
   LOG(logERROR) << "Invalid register name \"" << dacName << "\"!";
    return false;
  }
  dacValue = registerRangeCheck(dacRegister, dacValue);

  std::pair<std::map<uint8_t,uint8_t>::iterator,bool> ret;

  if(rocid < 0) {
    // Set the DAC for all active ROCs:
    // FIXME maybe go over expandLoop here?
    std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();
    for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit) {

      // Update the DUT DAC Value:
      ret = _dut->roc.at((uint8_t)(rocit - enabledRocs.begin())).dacs.insert( std::make_pair(dacRegister,dacValue) );
      if(ret.second == true) {
	LOG(logWARNING) << "DAC \"" << dacName << "\" was not initialized. Created with value " << (int)dacValue;
      }
      else {
	_dut->roc.at((uint8_t)(rocit - enabledRocs.begin())).dacs[dacRegister] = dacValue;
	LOG(logDEBUGAPI) << "DAC \"" << dacName << "\" updated with value " << (int)dacValue;
      }

      _hal->rocSetDAC((uint8_t) (rocit - enabledRocs.begin()),dacRegister,dacValue);
    }
  }
  else if(_dut->roc.size() > rocid) {
    // Set the DAC only in the given ROC (even if that is disabled!)

    // Update the DUT DAC Value:
    ret = _dut->roc.at(rocid).dacs.insert( std::make_pair(dacRegister,dacValue) );
    if(ret.second == true) {
      LOG(logWARNING) << "DAC \"" << dacName << "\" was not initialized. Created with value " << (int)dacValue;
    }
    else {
      _dut->roc.at(rocid).dacs[dacRegister] = dacValue;
	LOG(logDEBUGAPI) << "DAC \"" << dacName << "\" updated with value " << (int)dacValue;
    }

    _hal->rocSetDAC((uint8_t)rocid,dacRegister,dacValue);
  }
  else {
    LOG(logERROR) << "ROC " << rocid << " is not existing in the DUT!";
    return false;
  }
  return true;
}

std::vector< std::pair<uint8_t, std::vector<pixel> > > api::getPulseheightVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
										uint16_t flags, uint32_t nTriggers) {

  // Get the register number and check the range from dictionary:
  uint8_t dacRegister = stringToRegister(dacName);
  if(dacRegister == 0x0) {
    LOG(logERROR) << "Invalid register name \"" << dacName << "\"!";
    return std::vector< std::pair<uint8_t, std::vector<pixel> > >();
  }

  // Check DAC range
  if(dacMin > dacMax) {
    // FIXME: THIS SHOULD THROW A CUSTOM EXCEPTION
    LOG(logERROR) << "DacMin > DacMax! ";
    return std::vector< std::pair<uint8_t, std::vector<pixel> > >();
  }
  dacMax = registerRangeCheck(dacRegister, dacMax);

  // Setup the correct _hal calls for this test
  HalMemFnPixel pixelfn = &hal::PixelCalibrateDacScan;
  HalMemFnRoc rocfn = NULL;
  HalMemFnModule modulefn = NULL;

  // We want the pulse height back from the Map function, no internal flag needed.

  // Load the test parameters into vector
  std::vector<int32_t> param;
  param.push_back(static_cast<int32_t>(dacRegister));  
  param.push_back(static_cast<int32_t>(dacMin));
  param.push_back(static_cast<int32_t>(dacMax));
  param.push_back(static_cast<int32_t>(flags));
  param.push_back(static_cast<int32_t>(nTriggers));

  // check if the flags indicate that the user explicitly asks for serial execution of test:
  // FIXME: FLAGS NOT YET CHECKED!
  bool forceSerial = flags & FLAG_FORCE_SERIAL;
  std::vector< std::vector<pixel> >* data = expandLoop(pixelfn, rocfn, modulefn, param, forceSerial);
  // repack data into the expected return format
  std::vector< std::pair<uint8_t, std::vector<pixel> > >* result = repackDacScanData(data,dacMin,dacMax);

  // Reset the original value for the scanned DAC:
  // FIXME maybe go over expandLoop here?
  std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();
  for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
    uint8_t oldDacValue = _dut->getDAC((size_t)(rocit - enabledRocs.begin()),dacName);
    LOG(logDEBUGAPI) << "Reset DAC \"" << dacName << "\" to original value " << (int)oldDacValue;
    _hal->rocSetDAC((uint8_t) (rocit - enabledRocs.begin()),dacRegister,oldDacValue);
  }

  delete data;
  return *result;
}

std::vector< std::pair<uint8_t, std::vector<pixel> > > api::getDebugVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
										uint16_t flags, uint32_t nTriggers) {
  
  // Get the register number and check the range from dictionary:
  uint8_t dacRegister = stringToRegister(dacName);
  if(dacRegister == 0x0) {
    LOG(logERROR) << "Invalid register name \"" << dacName << "\"!";
    return std::vector< std::pair<uint8_t, std::vector<pixel> > >();
  }

  // Check DAC range
  if(dacMin > dacMax) {
    // FIXME: THIS SHOULD THROW A CUSTOM EXCEPTION
    LOG(logERROR) << "DacMin > DacMax! ";
    return std::vector< std::pair<uint8_t, std::vector<pixel> > >();
  }
  dacMax = registerRangeCheck(dacRegister, dacMax);

  // Setup the correct _hal calls for this test (FIXME:DUMMYONLY)
  HalMemFnPixel pixelfn = &hal::DummyPixelTestSkeleton;
  HalMemFnRoc rocfn = &hal::DummyRocTestSkeleton;
  HalMemFnModule modulefn = &hal::DummyModuleTestSkeleton;

  // Load the test parameters into vector
  std::vector<int32_t> param;
  param.push_back(static_cast<int32_t>(dacRegister));  
  param.push_back(static_cast<int32_t>(dacMin));
  param.push_back(static_cast<int32_t>(dacMax));
  param.push_back(static_cast<int32_t>(flags));
  param.push_back(static_cast<int32_t>(nTriggers));

  // check if the flags indicate that the user explicitly asks for serial execution of test:
  // FIXME: FLAGS NOT YET CHECKED!
  bool forceSerial = flags & FLAG_FORCE_SERIAL;
  std::vector< std::vector<pixel> >* data = expandLoop(pixelfn, rocfn, modulefn, param, forceSerial);
  // repack data into the expected return format
  std::vector< std::pair<uint8_t, std::vector<pixel> > >* result = repackDacScanData(data,dacMin,dacMax);
  delete data;
  return *result;

} // getPulseheightVsDAC

std::vector< std::pair<uint8_t, std::vector<pixel> > > api::getEfficiencyVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
									       uint16_t flags, uint32_t nTriggers) {

  // Get the register number and check the range from dictionary:
  uint8_t dacRegister = stringToRegister(dacName);
  if(dacRegister == 0x0) {
    LOG(logERROR) << "Invalid register name \"" << dacName << "\"!";
    return std::vector< std::pair<uint8_t, std::vector<pixel> > >();
  }

  // Check DAC range
  if(dacMin > dacMax) {
    // FIXME: THIS SHOULD THROW A CUSTOM EXCEPTION
    LOG(logERROR) << "DacMin > DacMax! ";
    return std::vector< std::pair<uint8_t, std::vector<pixel> > >();
  }
  dacMax = registerRangeCheck(dacRegister, dacMax);

  // Setup the correct _hal calls for this test
  HalMemFnPixel pixelfn = &hal::PixelCalibrateDacScan;
  HalMemFnRoc rocfn = NULL;
  HalMemFnModule modulefn = NULL;

 // We want the efficiency back from the Map function, so let's set the internal flag:
  int32_t internal_flags = 0;
  internal_flags |= flags;
  internal_flags |= FLAG_INTERNAL_GET_EFFICIENCY;
  LOG(logDEBUGAPI) << "Efficiency flag set, flags now at " << internal_flags;

  // Load the test parameters into vector
  std::vector<int32_t> param;
  param.push_back(static_cast<int32_t>(dacRegister));  
  param.push_back(static_cast<int32_t>(dacMin));
  param.push_back(static_cast<int32_t>(dacMax));
  param.push_back(static_cast<int32_t>(internal_flags));
  param.push_back(static_cast<int32_t>(nTriggers));

  // check if the flags indicate that the user explicitly asks for serial execution of test:
  // FIXME: FLAGS NOT YET CHECKED!
  bool forceSerial = internal_flags & FLAG_FORCE_SERIAL;
  std::vector< std::vector<pixel> >* data = expandLoop(pixelfn, rocfn, modulefn, param, forceSerial);
  // repack data into the expected return format
  std::vector< std::pair<uint8_t, std::vector<pixel> > >* result = repackDacScanData(data,dacMin,dacMax);

  // Reset the original value for the scanned DAC:
  // FIXME maybe go over expandLoop here?
  std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();
  for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
    uint8_t oldDacValue = _dut->getDAC((size_t)(rocit - enabledRocs.begin()),dacName);
    LOG(logDEBUGAPI) << "Reset DAC \"" << dacName << "\" to original value " << (int)oldDacValue;
    _hal->rocSetDAC((uint8_t) (rocit - enabledRocs.begin()),dacRegister,oldDacValue);
  }

  delete data;
  return *result;

}

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

std::vector<pixel> api::getPulseheightMap(uint16_t flags, uint32_t nTriggers) {

  // Setup the correct _hal calls for this test (ROC wide only)
  HalMemFnPixel pixelfn = &hal::PixelCalibrateMap;
  HalMemFnRoc rocfn = &hal::RocCalibrateMap;
  HalMemFnModule modulefn = NULL; //&hal::DummyModuleTestSkeleton; FIXME parallel later?

  // We want the pulse height back from the Map function, no flag needed.

  // Load the test parameters into vector
  std::vector<int32_t> param;
  param.push_back(static_cast<int32_t>(flags));
  param.push_back(static_cast<int32_t>(nTriggers));

  // check if the flags indicate that the user explicitly asks for serial execution of test:
  // FIXME: FLAGS NOT YET CHECKED!
  bool forceSerial = flags & FLAG_FORCE_SERIAL;
  std::vector< std::vector<pixel> >* data = expandLoop(pixelfn, rocfn, modulefn, param, forceSerial);

  // FIXME do proper repacking instead of just returning the first vector entry!
  return data->at(0);
}

std::vector<pixel> api::getEfficiencyMap(uint16_t flags, uint32_t nTriggers) {

  // Setup the correct _hal calls for this test (ROC wide only)
  HalMemFnPixel pixelfn = &hal::PixelCalibrateMap;
  HalMemFnRoc rocfn = &hal::RocCalibrateMap;
  HalMemFnModule modulefn = NULL; //&hal::DummyModuleTestSkeleton; FIXME parallel later?

  // We want the efficiency back from the Map function, so let's set the internal flag:
  int32_t internal_flags = 0;
  internal_flags |= flags;
  internal_flags |= FLAG_INTERNAL_GET_EFFICIENCY;
  LOG(logDEBUGAPI) << "Efficiency flag set, flags now at " << internal_flags;

  // Load the test parameters into vector
  std::vector<int32_t> param;
  param.push_back(static_cast<int32_t>(internal_flags));
  param.push_back(static_cast<int32_t>(nTriggers));

  // check if the flags indicate that the user explicitly asks for serial execution of test:
  // FIXME: FLAGS NOT YET CHECKED!
  bool forceSerial = internal_flags & FLAG_FORCE_SERIAL;
  std::vector< std::vector<pixel> >* data = expandLoop(pixelfn, rocfn, modulefn, param, forceSerial);

  // FIXME do proper repacking instead of just returning the first vector entry!
  return data->at(0);
}

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
