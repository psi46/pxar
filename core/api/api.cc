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
			std::vector<std::pair<uint16_t,uint8_t> > pg_setup) {

  // Collect and check the testboard configuration settings

  // Read the power settings and make sure we got all:
  double va = 0, vd = 0, ia = 0, id = 0;
  for(std::vector<std::pair<std::string,double> >::iterator it = power_settings.begin(); it != power_settings.end(); ++it) {
    std::transform((*it).first.begin(), (*it).first.end(), (*it).first.begin(), ::tolower);

    if((*it).second < 0) {
      LOG(logERROR) << "Negative value for power setting " << (*it).first << "! Skipping.";
      continue;
    }

    // FIXME Range Checks!
    if((*it).first.compare("va") == 0) { 
      va = (*it).second;
      _dut->va = va;
    }
    else if((*it).first.compare("vd") == 0) {
      vd = (*it).second;
      _dut->vd = vd;
    }
    else if((*it).first.compare("ia") == 0) {
      ia = (*it).second;
      _dut->ia = ia;
    }
    else if((*it).first.compare("id") == 0) {
      id = (*it).second;
      _dut->id = id;
    }
    else { LOG(logERROR) << "Unknown power setting " << (*it).first << "! Skipping.";}
  }

  if(va == 0 || vd == 0 || ia == 0 || id == 0) {
    LOG(logCRITICAL) << "Power settings are not suffient. Please check and re-configure!";
    return false;
  }


  // Take care of the signal delay settings:
  std::map<uint8_t,uint8_t> delays;
  for(std::vector<std::pair<std::string,uint8_t> >::iterator sigIt = sig_delays.begin(); sigIt != sig_delays.end(); ++sigIt) {

    // Fill the signal timing pairs with the register from the dictionary:
    uint8_t sigRegister, sigValue = sigIt->second;
    if(!verifyRegister(sigIt->first,sigRegister,sigValue,DTB_REG)) continue;

    std::pair<std::map<uint8_t,uint8_t>::iterator,bool> ret;
    ret = delays.insert( std::make_pair(sigRegister,sigValue) );
    if(ret.second == false) {
      LOG(logWARNING) << "Overwriting existing DTB delay setting \"" << sigIt->first 
		      << "\" value " << (int)ret.first->second
		      << " with " << (int)sigValue;
      delays[sigRegister] = sigValue;
    }
  }
  // Store these validated parameters in the DUT
  _dut->sig_delays = delays;
  
  // Prepare Pattern Generator:
  if(!verifyPatternGenerator(pg_setup)) return false;
  // Store the Pattern Generator commands in the DUT:
  _dut->pg_setup = pg_setup;

  // Call the HAL to do the job:
  _hal->initTestboard(delays,pg_setup,va,vd,ia,id);
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

  // Check size of rocDACs and rocPixels against each other
  if(rocDACs.size() != rocPixels.size()) {
    LOG(logCRITICAL) << "Hm, we have " << rocDACs.size() << " DAC configs but " << rocPixels.size() << " pixel configs.";
    LOG(logCRITICAL) << "This cannot end well...";
    return false;
  }

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
      uint8_t dacRegister, dacValue = dacIt->second;
      if(!verifyRegister(dacIt->first, dacRegister, dacValue, TBM_REG)) continue;

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
      uint8_t dacRegister, dacValue = dacIt->second;
      if(!verifyRegister(dacIt->first, dacRegister, dacValue, ROC_REG)) continue;

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

  // FIXME Device types not transmitted yet!

  std::vector<tbmConfig> enabledTbms = _dut->getEnabledTbms();
  if(!enabledTbms.empty()) {LOG(logDEBUGAPI) << "Programming TBMs...";}
  for (std::vector<tbmConfig>::iterator tbmit = enabledTbms.begin(); tbmit != enabledTbms.end(); ++tbmit){
    _hal->initTBM((uint8_t)(tbmit - enabledTbms.begin()),(*tbmit).dacs);
  }

  std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();
  if(!enabledRocs.empty()) {LOG(logDEBUGAPI) << "Programming ROCs...";}
  for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
    _hal->initROC((uint8_t)(rocit - enabledRocs.begin()),(*rocit).dacs);
  }

  // As last step, write all the mask and trim information to the devices:
  MaskAndTrim();

  // The DUT is programmed, everything all right:
  _dut->_programmed = true;

  return true;
}

// API status function, checks HAL and DUT statuses
bool api::status() {
  if(_hal->status() && _dut->status()) return true;
  return false;
}

// Check if the given value lies within the valid range of the DAC. If value lies above/below valid range
// return the upper/lower bondary. If value lies wqithin the range, return the value
bool api::verifyRegister(std::string name, uint8_t &id, uint8_t &value, uint8_t type) {

  // Convert the name to lower case for comparison:
  std::transform(name.begin(), name.end(), name.begin(), ::tolower);

  // Get singleton DAC dictionary object:
  RegisterDictionary * _dict = RegisterDictionary::getInstance();

  // And get the register value from the dictionary object:
  id = _dict->getRegister(name,type);

  // Check if it was found:
  if(id == type) {
    LOG(logERROR) << "Invalid register name \"" << name << "\".";
    return false;
  }

  // Read register value limit:
  uint8_t regLimit = _dict->getSize(id, type);
  if(value > regLimit) {
    LOG(logWARNING) << "Register range overflow, set register \"" 
		    << name << "\" (" << (int)id << ") to " 
		    << (int)regLimit << " (was: " << (int)value << ")";
    value = (uint8_t)regLimit;
  }

  LOG(logDEBUGAPI) << "Verified register \"" << name << "\" (" << (int)id << "): " << (int)value << " (max " << (int)regLimit << ")"; 
  return true;
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
  if(!_hal->status()) {return 0;}
  return _hal->getTBia();
}

double api::getTBva() {
  if(!_hal->status()) {return 0;}
  return _hal->getTBva();
}

double api::getTBid() {
  if(!_hal->status()) {return 0;}
  return _hal->getTBid();
}

double api::getTBvd() {
  if(!_hal->status()) {return 0;}
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

  if(!_hal->status()) {return false;}

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
  
  if(!status()) {return false;}

  // Get the register number and check the range from dictionary:
  uint8_t dacRegister;
  if(!verifyRegister(dacName, dacRegister, dacValue, ROC_REG)) return false;

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
  else if(_dut->roc.size() > (unsigned)rocid) {
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

bool api::setTbmReg(std::string regName, uint8_t regValue, int8_t tbmid) {

  if(!status()) {return 0;}
  
  // Get the register number and check the range from dictionary:
  uint8_t _register;
  if(!verifyRegister(regName, _register, regValue, TBM_REG)) return false;

  std::pair<std::map<uint8_t,uint8_t>::iterator,bool> ret;

  if(tbmid < 0) {
    // Set the register for all active TBMs:
    // FIXME maybe go over expandLoop here?
    std::vector<tbmConfig> enabledTbms = _dut->getEnabledTbms();
    for (std::vector<tbmConfig>::iterator tbmit = enabledTbms.begin(); tbmit != enabledTbms.end(); ++tbmit) {

      // Update the DUT DAC Value:
      ret = _dut->tbm.at((uint8_t)(tbmit - enabledTbms.begin())).dacs.insert( std::make_pair(_register,regValue) );
      if(ret.second == true) {
	LOG(logWARNING) << "DAC \"" << regName << "\" was not initialized. Created with value " << (int)regValue;
      }
      else {
	_dut->tbm.at((uint8_t)(tbmit - enabledTbms.begin())).dacs[_register] = regValue;
	LOG(logDEBUGAPI) << "DAC \"" << regName << "\" updated with value " << (int)regValue;
      }

      _hal->tbmSetReg((uint8_t) (tbmit - enabledTbms.begin()),_register,regValue);
    }
  }
  else if(_dut->tbm.size() > (unsigned)tbmid) {
    // Set the register only in the given TBM (even if that is disabled!)

    // Update the DUT register Value:
    ret = _dut->tbm.at(tbmid).dacs.insert( std::make_pair(_register,regValue) );
    if(ret.second == true) {
      LOG(logWARNING) << "Register \"" << regName << "\" was not initialized. Created with value " << (int)regValue;
    }
    else {
      _dut->tbm.at(tbmid).dacs[_register] = regValue;
	LOG(logDEBUGAPI) << "Register \"" << regName << "\" updated with value " << (int)regValue;
    }

    _hal->tbmSetReg((uint8_t)tbmid,_register,regValue);
  }
  else {
    LOG(logERROR) << "ROC " << tbmid << " is not existing in the DUT!";
    return false;
  }
  return true;
}

std::vector< std::pair<uint8_t, std::vector<pixel> > > api::getPulseheightVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
										uint16_t flags, uint32_t nTriggers) {

  if(!status()) {return std::vector< std::pair<uint8_t, std::vector<pixel> > >();}

  // Check DAC range
  if(dacMin > dacMax) {
    // Swapping the range:
    LOG(logWARNING) << "Swapping upper and lower bound.";
    uint8_t temp = dacMin;
    dacMin = dacMax;
    dacMax = temp;
  }

  // Get the register number and check the range from dictionary:
  uint8_t dacRegister;
  if(!verifyRegister(dacName, dacRegister, dacMax, ROC_REG)) {
    return std::vector< std::pair<uint8_t, std::vector<pixel> > >();
  }

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

  if(!status()) {return std::vector< std::pair<uint8_t, std::vector<pixel> > >();}
  
  // Check DAC range
  if(dacMin > dacMax) {
    // Swapping the range:
    LOG(logWARNING) << "Swapping upper and lower bound.";
    uint8_t temp = dacMin;
    dacMin = dacMax;
    dacMax = temp;
  }

  // Get the register number and check the range from dictionary:
  uint8_t dacRegister;
  if(!verifyRegister(dacName, dacRegister, dacMax, ROC_REG)) {
    return std::vector< std::pair<uint8_t, std::vector<pixel> > >();
  }
  
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

  if(!status()) {return std::vector< std::pair<uint8_t, std::vector<pixel> > >();}

  // Check DAC range
  if(dacMin > dacMax) {
    // Swapping the range:
    LOG(logWARNING) << "Swapping upper and lower bound.";
    uint8_t temp = dacMin;
    dacMin = dacMax;
    dacMax = temp;
  }

  // Get the register number and check the range from dictionary:
  uint8_t dacRegister;
  if(!verifyRegister(dacName, dacRegister, dacMax, ROC_REG)) {
    return std::vector< std::pair<uint8_t, std::vector<pixel> > >();
  }

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
									      uint16_t flags, uint32_t nTriggers) {

  if(!status()) {return std::vector< std::pair<uint8_t, std::vector<pixel> > >();}

  // Check DAC range
  if(dacMin > dacMax) {
    // Swapping the range:
    LOG(logWARNING) << "Swapping upper and lower bound.";
    uint8_t temp = dacMin;
    dacMin = dacMax;
    dacMax = temp;
  }

  // Get the register number and check the range from dictionary:
  uint8_t dacRegister;
  if(!verifyRegister(dacName, dacRegister, dacMax, ROC_REG)) {
    return std::vector< std::pair<uint8_t, std::vector<pixel> > >();
  }

  // Setup the correct _hal calls for this test
  HalMemFnPixel pixelfn = NULL;
  HalMemFnRoc rocfn = NULL;
  HalMemFnModule modulefn = NULL;

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


std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > api::getPulseheightVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
													std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
													uint16_t flags, uint32_t nTriggers){

  if(!status()) {return std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > >();}

  // Check DAC ranges
  if(dac1min > dac1max) {
    // Swapping the range:
    LOG(logWARNING) << "Swapping upper and lower bound.";
    uint8_t temp = dac1min;
    dac1min = dac1max;
    dac1max = temp;
  }
  if(dac2min > dac2max) {
    // Swapping the range:
    LOG(logWARNING) << "Swapping upper and lower bound.";
    uint8_t temp = dac2min;
    dac2min = dac2max;
    dac2max = temp;
  }

  // Get the register number and check the range from dictionary:
  uint8_t dac1register, dac2register;
  if(!verifyRegister(dac1name, dac1register, dac1max, ROC_REG)) {
    return std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > >();
  }
  if(!verifyRegister(dac2name, dac2register, dac2max, ROC_REG)) {
    return std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > >();
  }

  // Setup the correct _hal calls for this test
  HalMemFnPixel pixelfn = &hal::PixelCalibrateDacDacScan;
  HalMemFnRoc rocfn = NULL;
  HalMemFnModule modulefn = NULL;

 // We want the pulse height back from the DacDac function, so no internal flags needed.

  // Load the test parameters into vector
  std::vector<int32_t> param;
  param.push_back(static_cast<int32_t>(dac1register));  
  param.push_back(static_cast<int32_t>(dac1min));
  param.push_back(static_cast<int32_t>(dac1max));
  param.push_back(static_cast<int32_t>(dac2register));  
  param.push_back(static_cast<int32_t>(dac2min));
  param.push_back(static_cast<int32_t>(dac2max));
  param.push_back(static_cast<int32_t>(flags));
  param.push_back(static_cast<int32_t>(nTriggers));

  // check if the flags indicate that the user explicitly asks for serial execution of test:
  // FIXME: FLAGS NOT YET CHECKED!
  bool forceSerial = flags & FLAG_FORCE_SERIAL;
  std::vector< std::vector<pixel> >* data = expandLoop(pixelfn, rocfn, modulefn, param, forceSerial);
  // repack data into the expected return format
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > >* result = repackDacDacScanData(data,dac1min,dac1max,dac2min,dac2max);

  // Reset the original value for the scanned DAC:
  // FIXME maybe go over expandLoop here?
  std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();
  for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
    uint8_t oldDac1Value = _dut->getDAC((size_t)(rocit - enabledRocs.begin()),dac1name);
    uint8_t oldDac2Value = _dut->getDAC((size_t)(rocit - enabledRocs.begin()),dac2name);
    LOG(logDEBUGAPI) << "Reset DAC \"" << dac1name << "\" to original value " << (int)oldDac1Value;
    LOG(logDEBUGAPI) << "Reset DAC \"" << dac2name << "\" to original value " << (int)oldDac2Value;
    _hal->rocSetDAC((uint8_t) (rocit - enabledRocs.begin()),dac1register,oldDac1Value);
    _hal->rocSetDAC((uint8_t) (rocit - enabledRocs.begin()),dac2register,oldDac2Value);
  }

  delete data;
  return *result;
}

std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > api::getEfficiencyVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
												       std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
												       uint16_t flags, uint32_t nTriggers) {

  if(!status()) {return std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > >();}

  // Check DAC ranges
  if(dac1min > dac1max) {
    // Swapping the range:
    LOG(logWARNING) << "Swapping upper and lower bound.";
    uint8_t temp = dac1min;
    dac1min = dac1max;
    dac1max = temp;
  }
  if(dac2min > dac2max) {
    // Swapping the range:
    LOG(logWARNING) << "Swapping upper and lower bound.";
    uint8_t temp = dac2min;
    dac2min = dac2max;
    dac2max = temp;
  }

  // Get the register number and check the range from dictionary:
  uint8_t dac1register, dac2register;
  if(!verifyRegister(dac1name, dac1register, dac1max, ROC_REG)) {
    return std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > >();
  }
  if(!verifyRegister(dac2name, dac2register, dac2max, ROC_REG)) {
    return std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > >();
  }

  // Setup the correct _hal calls for this test
  HalMemFnPixel pixelfn = &hal::PixelCalibrateDacDacScan;
  HalMemFnRoc rocfn = NULL;
  HalMemFnModule modulefn = NULL;

 // We want the efficiency back from the Map function, so let's set the internal flag:
  int32_t internal_flags = 0;
  internal_flags |= flags;
  internal_flags |= FLAG_INTERNAL_GET_EFFICIENCY;
  LOG(logDEBUGAPI) << "Efficiency flag set, flags now at " << internal_flags;

  // Load the test parameters into vector
  std::vector<int32_t> param;
  param.push_back(static_cast<int32_t>(dac1register));  
  param.push_back(static_cast<int32_t>(dac1min));
  param.push_back(static_cast<int32_t>(dac1max));
  param.push_back(static_cast<int32_t>(dac2register));  
  param.push_back(static_cast<int32_t>(dac2min));
  param.push_back(static_cast<int32_t>(dac2max));
  param.push_back(static_cast<int32_t>(internal_flags));
  param.push_back(static_cast<int32_t>(nTriggers));

  // check if the flags indicate that the user explicitly asks for serial execution of test:
  // FIXME: FLAGS NOT YET CHECKED!
  bool forceSerial = internal_flags & FLAG_FORCE_SERIAL;
  std::vector< std::vector<pixel> >* data = expandLoop(pixelfn, rocfn, modulefn, param, forceSerial);
  // repack data into the expected return format
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > >* result = repackDacDacScanData(data,dac1min,dac1max,dac2min,dac2max);

  // Reset the original value for the scanned DAC:
  // FIXME maybe go over expandLoop here?
  std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();
  for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
    uint8_t oldDac1Value = _dut->getDAC((size_t)(rocit - enabledRocs.begin()),dac1name);
    uint8_t oldDac2Value = _dut->getDAC((size_t)(rocit - enabledRocs.begin()),dac2name);
    LOG(logDEBUGAPI) << "Reset DAC \"" << dac1name << "\" to original value " << (int)oldDac1Value;
    LOG(logDEBUGAPI) << "Reset DAC \"" << dac2name << "\" to original value " << (int)oldDac2Value;
    _hal->rocSetDAC((uint8_t) (rocit - enabledRocs.begin()),dac1register,oldDac1Value);
    _hal->rocSetDAC((uint8_t) (rocit - enabledRocs.begin()),dac2register,oldDac2Value);
  }

  delete data;
  return *result;
}

std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > api::getThresholdVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
												      std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
												      uint16_t flags, uint32_t nTriggers) {

  if(!status()) {return std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > >();}

  // Check DAC ranges
  if(dac1min > dac1max) {
    // Swapping the range:
    LOG(logWARNING) << "Swapping upper and lower bound.";
    uint8_t temp = dac1min;
    dac1min = dac1max;
    dac1max = temp;
  }
  if(dac2min > dac2max) {
    // Swapping the range:
    LOG(logWARNING) << "Swapping upper and lower bound.";
    uint8_t temp = dac2min;
    dac2min = dac2max;
    dac2max = temp;
  }

  // Get the register number and check the range from dictionary:
  uint8_t dac1register, dac2register;
  if(!verifyRegister(dac1name, dac1register, dac1max, ROC_REG)) {
    return std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > >();
  }
  if(!verifyRegister(dac2name, dac2register, dac2max, ROC_REG)) {
    return std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > >();
  }

  // Setup the correct _hal calls for this test
  HalMemFnPixel pixelfn = NULL;
  HalMemFnRoc rocfn = NULL;
  HalMemFnModule modulefn = NULL;

  // Load the test parameters into vector
  std::vector<int32_t> param;
  param.push_back(static_cast<int32_t>(dac1register));  
  param.push_back(static_cast<int32_t>(dac1min));
  param.push_back(static_cast<int32_t>(dac1max));
  param.push_back(static_cast<int32_t>(dac2register));  
  param.push_back(static_cast<int32_t>(dac2min));
  param.push_back(static_cast<int32_t>(dac2max));
  param.push_back(static_cast<int32_t>(flags));
  param.push_back(static_cast<int32_t>(nTriggers));

  // check if the flags indicate that the user explicitly asks for serial execution of test:
  // FIXME: FLAGS NOT YET CHECKED!
  bool forceSerial = flags & FLAG_FORCE_SERIAL;
  std::vector< std::vector<pixel> >* data = expandLoop(pixelfn, rocfn, modulefn, param, forceSerial);
  // repack data into the expected return format
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > >* result = repackDacDacScanData(data,dac1min,dac1max,dac2min,dac2max);

  // Reset the original value for the scanned DAC:
  // FIXME maybe go over expandLoop here?
  std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();
  for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
    uint8_t oldDac1Value = _dut->getDAC((size_t)(rocit - enabledRocs.begin()),dac1name);
    uint8_t oldDac2Value = _dut->getDAC((size_t)(rocit - enabledRocs.begin()),dac2name);
    LOG(logDEBUGAPI) << "Reset DAC \"" << dac1name << "\" to original value " << (int)oldDac1Value;
    LOG(logDEBUGAPI) << "Reset DAC \"" << dac2name << "\" to original value " << (int)oldDac2Value;
    _hal->rocSetDAC((uint8_t) (rocit - enabledRocs.begin()),dac1register,oldDac1Value);
    _hal->rocSetDAC((uint8_t) (rocit - enabledRocs.begin()),dac2register,oldDac2Value);
  }

  delete data;
  return *result;
}

std::vector<pixel> api::getPulseheightMap(uint16_t flags, uint32_t nTriggers) {

  if(!status()) {return std::vector<pixel>();}

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

  // Repacking of all data segments into one long map vector:
  std::vector<pixel>* result = repackMapData(data);
  delete data;
  return *result;
}

std::vector<pixel> api::getEfficiencyMap(uint16_t flags, uint32_t nTriggers) {

  if(!status()) {return std::vector<pixel>();}

  // Setup the correct _hal calls for this test
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

  // Repacking of all data segments into one long map vector:
  std::vector<pixel>* result = repackMapData(data);
  delete data;
  return *result;
}

std::vector<pixel> api::getThresholdMap(uint16_t flags, uint32_t nTriggers) {

  if(!status()) {return std::vector<pixel>();}

  // Setup the correct _hal calls for this test
  HalMemFnPixel pixelfn = NULL;
  HalMemFnRoc rocfn = NULL;
  HalMemFnModule modulefn = NULL;

  // Load the test parameters into vector
  std::vector<int32_t> param;
  param.push_back(static_cast<int32_t>(flags));
  param.push_back(static_cast<int32_t>(nTriggers));

  // check if the flags indicate that the user explicitly asks for serial execution of test:
  // FIXME: FLAGS NOT YET CHECKED!
  bool forceSerial = flags & FLAG_FORCE_SERIAL;
  std::vector< std::vector<pixel> >* data = expandLoop(pixelfn, rocfn, modulefn, param, forceSerial);

  // Repacking of all data segments into one long map vector:
  std::vector<pixel>* result = repackMapData(data);
  delete data;
  return *result;
}
  
int32_t api::getReadbackValue(std::string parameterName) {

  if(!status()) {return -1;}
}


/** DAQ functions **/
bool api::daqStart(std::vector<std::pair<uint16_t, uint8_t> > pg_setup) {

  if(!status()) {return false;}

  // FIXME maybe add check for already running DAQ? Some _daq flag in DUT?

  LOG(logDEBUGAPI) << "Starting new DAQ session...";
  if(!pg_setup.empty()) {
    // Prepare new Pattern Generator:
    if(!verifyPatternGenerator(pg_setup)) return false;
    _hal->SetupPatternGenerator(pg_setup);
  }
  
  // Setup the configured mask and trim state of the DUT:
  MaskAndTrim();

  // Set Calibrate bits in the PUCs (we use the testrange for that):
  SetCalibrateBits(true);

  // Check the DUT if we have TBMs enabled or not and choose the right
  // deserializer:
  _hal->daqStart(_dut->sig_delays[SIG_DESER160PHASE],_dut->getNEnabledTbms());

  return true;
}

void api::daqTrigger(uint32_t nTrig) {
  // Just passing the call to the HAL, not doing anything else here:
  _hal->daqTrigger(nTrig);
}

std::vector<uint16_t> api::daqGetBuffer() {

  // Reading out all data from the DTB and returning the raw blob.
  // Select the right readout channels depending on the number of TBMs
  std::vector<uint16_t> data = _hal->daqRead(_dut->getNEnabledTbms());
  
  // We read out everything, reset the buffer:
  // Reset all active channels:
  _hal->daqReset(_dut->getNEnabledTbms());
  return data;
}

std::vector<pixel> api::daqGetEvent() {}

bool api::daqStop() {

  if(!status()) {return false;}

  // Stop all active DAQ channels (depending on number of TBMs)
  _hal->daqStop(_dut->getNEnabledTbms());

  // FIXME We should probably mask the full DUT again here

  // Reset all the Calibrate bits and signals:
  SetCalibrateBits(false);

  // Re-program the old Pattern Generator setup which is stored in the DUT.
  // Since these patterns are verified already, just write them:
  LOG(logDEBUGAPI) << "Resetting Pattern Generator to previous state.";
  _hal->SetupPatternGenerator(_dut->pg_setup);
  
  return false;
}


std::vector< std::vector<pixel> >* api::expandLoop(HalMemFnPixel pixelfn, HalMemFnRoc rocfn, HalMemFnModule modulefn, std::vector<int32_t> param,  bool forceSerial){
  
  // pointer to vector to hold our data
  std::vector< std::vector<pixel> >* data = NULL;


  // Do the masking/unmasking&trimming for all ROCs first
  MaskAndTrim();

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



std::vector<pixel>* api::repackMapData (std::vector< std::vector<pixel> >* data) {

  std::vector<pixel>* result = new std::vector<pixel>();
  LOG(logDEBUGAPI) << "Simple Map Repack of " << data->size() << " data blocks.";

  for(std::vector<std::vector<pixel> >::iterator it = data->begin(); it!= data->end(); ++it) {
    for(std::vector<pixel>::iterator px = (*it).begin(); px != (*it).end(); ++px) {
      result->push_back(*px);
    }
  }

  LOG(logDEBUGAPI) << "Correctly repacked Map data for delivery.";
  return result;
}

std::vector< std::pair<uint8_t, std::vector<pixel> > >* api::repackDacScanData (std::vector< std::vector<pixel> >* data, uint8_t dacMin, uint8_t dacMax){
  std::vector< std::pair<uint8_t, std::vector<pixel> > >* result = new std::vector< std::pair<uint8_t, std::vector<pixel> > >();
  uint8_t currentDAC = dacMin;

  LOG(logDEBUGAPI) << "Packing range " << (int)dacMin << "-" << (int)dacMax << ", data has " << data->size() << " entries.";

  for (std::vector<std::vector<pixel> >::iterator vecit = data->begin(); vecit!=data->end();++vecit){
    result->push_back(std::make_pair(currentDAC, *vecit));
    currentDAC++;
  }

  LOG(logDEBUGAPI) << "Repack end: current " << (int)currentDAC << " (max " << (int)dacMax << ")";
  if (currentDAC!=dacMax){
    // FIXME: THIS SHOULD THROW A CUSTOM EXCEPTION
    LOG(logCRITICAL) << "data structure size not as expected! " << data->size() << " data blocks do not fit to " << dacMax-dacMin << " DAC values!";
    delete result;
    return NULL;
  }

  LOG(logDEBUGAPI) << "Correctly repacked DacScan data for delivery.";
  return result;
}

std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > >* api::repackDacDacScanData (std::vector< std::vector<pixel> >* data, uint8_t dac1min, uint8_t dac1max, uint8_t dac2min, uint8_t dac2max) {

  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > >* result = new std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > >();

  uint8_t current1dac = dac1min;
  uint8_t current2dac = dac2min;

  for (std::vector<std::vector<pixel> >::iterator vecit = data->begin(); vecit!=data->end();++vecit){

    result->push_back(std::make_pair(current1dac, std::make_pair(current2dac, *vecit)));

    if(current2dac == dac2max-1) {
      current2dac = dac2min;
      current1dac++;
    }
    else current2dac++;
  }

  LOG(logDEBUGAPI) << "Repack end: current1 " << (int)current1dac << " (max " << (int)dac1max << "), current2 " 
		   << (int)current2dac << " (max" << (int)dac2max << ")";

  if (current1dac != dac1max){
    // FIXME: THIS SHOULD THROW A CUSTOM EXCEPTION
    LOG(logCRITICAL) << "data structure size not as expected! " << data->size() << " data blocks do not fit to " << (dac1max-dac1min)*(dac2max-dac2min) << " DAC values!";
    delete result;
    return NULL;
  }
  
  LOG(logDEBUGAPI) << "Correctly repacked DacDacScan data for delivery.";
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
  LOG(logDEBUGAPI) << "Segment size: " << segmentsize;

  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();
  // Loop over each data segment:
  for (int segment = 0; segment < segmentsize; segment++) {
    std::vector<pixel> pixjoined;
    // Loop over all ROCs to merge their data segments into one
    for (uint8_t rocid = 0; rocid < nRocs;rocid++) {
      // Copy all pixels over:
      for(std::vector<pixel>::iterator it = data->at(segment+segmentsize*rocid).begin(); it != data->at(segment+segmentsize*rocid).end(); ++it) {
	pixjoined.push_back((*it));
      }
    }
    result->push_back(pixjoined);
  }

  LOG(logDEBUGAPI) << "Joined data has " << result->size() << " segments with each " << result->at(0).size() << " entries.";
  return result;
}


// Function to program the device with all the needed trimming and masking stuff
// It sets both the needed PUC mask&trim bits and the DCOL enable bits.
void api::MaskAndTrim() {

  // Run over all existing ROCs:
  for (std::vector<rocConfig>::iterator rocit = _dut->roc.begin(); rocit != _dut->roc.end(); ++rocit) {

    // Check if we can run on full ROCs:
    uint16_t masked = _dut->getNMaskedPixels((uint8_t)(rocit-_dut->roc.begin()));
    LOG(logDEBUGAPI) << "ROC " << (int)(rocit-_dut->roc.begin()) << " features " << masked << " masked pixels.";

    // This ROC is completely unmasked, let's trim it:
    if(masked == 0) {
      LOG(logDEBUGAPI) << "Unmasking and trimming ROC " << (int)(rocit-_dut->roc.begin()) << " in one go.";
      _hal->RocSetMask((int)(rocit-_dut->roc.begin()),false,rocit->pixels);
      continue;
    }
    else if(masked == ROC_NUMROWS*ROC_NUMCOLS) {
      LOG(logDEBUGAPI) << "Masking ROC " << (int)(rocit-_dut->roc.begin()) << " in one go.";
      _hal->RocSetMask((int)(rocit-_dut->roc.begin()),true);
      continue;
    }
    // Choose the version with less calls (less than half the pixels to change):
    else if(masked <= ROC_NUMROWS*ROC_NUMCOLS/2) {
      // We have more unmasked than masked pixels:
      LOG(logDEBUGAPI) << "Unmasking and trimming ROC " << (int)(rocit-_dut->roc.begin()) << " before masking single pixels.";
      _hal->RocSetMask((int)(rocit-_dut->roc.begin()),false,rocit->pixels);

      // Disable all unneeded columns:
      std::vector<bool> enabledColumns = _dut->getEnabledColumns((int)(rocit-_dut->roc.begin()));
      for(std::vector<bool>::iterator it = enabledColumns.begin(); it != enabledColumns.end(); ++it) {
	if(!(*it)) _hal->ColumnSetEnable((int)(rocit - _dut->roc.begin()),(int)(it - enabledColumns.begin()),(*it));
      }

      // And then mask the required pixels:
      for(std::vector<pixelConfig>::iterator pxit = rocit->pixels.begin(); pxit != rocit->pixels.end(); ++pxit) {
	if(pxit->mask == true) {_hal->PixelSetMask((int)(rocit-_dut->roc.begin()),pxit->column,pxit->row,true);}
      }
    }
    else {
      // Some are unmasked, but not too many. First mask that ROC:
      LOG(logDEBUGAPI) << "Masking ROC " << (int)(rocit-_dut->roc.begin()) << " before unmasking single pixels.";
      _hal->RocSetMask((int)(rocit-_dut->roc.begin()),true);

      // Enable all needed columns:
      std::vector<bool> enabledColumns = _dut->getEnabledColumns((int)(rocit-_dut->roc.begin()));
      for(std::vector<bool>::iterator it = enabledColumns.begin(); it != enabledColumns.end(); ++it) {
	if((*it)) _hal->ColumnSetEnable((int)(rocit - _dut->roc.begin()),(int)(it - enabledColumns.begin()),(*it));
      }

      // And then unmask the required pixels with their trim values:
      for(std::vector<pixelConfig>::iterator pxit = rocit->pixels.begin(); pxit != rocit->pixels.end(); ++pxit) {
	if(pxit->mask == false) {_hal->PixelSetMask((int)(rocit-_dut->roc.begin()),pxit->column,pxit->row,false,pxit->trim);}
      }
    }
  }

}

// Function to suppy all enabled pixels in the test range ("enable") with 
// a roc_Pix_Cal bit:
void api::SetCalibrateBits(bool enable) {

  // Run over all existing ROCs:
  for (std::vector<rocConfig>::iterator rocit = _dut->roc.begin(); rocit != _dut->roc.end(); ++rocit) {

    // Check if the signal has to be turned on or off:
    if(enable) {
      // Loop over all pixels in this ROC and set the Cal bit:
      for(std::vector<pixelConfig>::iterator pxit = rocit->pixels.begin(); pxit != rocit->pixels.end(); ++pxit) {
      
	if(pxit->enable == true) {
	  _hal->PixelSetCalibrate((int)(rocit-_dut->roc.begin()),pxit->column,pxit->row,0);
	}
      }

    }
    // Clear the signal for the full ROC:
    else {_hal->RocClearCalibrate((int)(rocit-_dut->roc.begin()));}
  }
}


bool api::verifyPatternGenerator(std::vector<std::pair<uint16_t,uint8_t> > &pg_setup) {
  
  for(std::vector<std::pair<uint16_t,uint8_t> >::iterator it = pg_setup.begin(); it != pg_setup.end(); ++it) {
    if((*it).second == 0 && it != pg_setup.end() -1 ) {
      LOG(logCRITICAL) << "Found delay = 0 on early entry! This stops the pattern generator at position " 
		       << (int)(it - pg_setup.begin())  << ".";
      return false;
    }
    // Check last entry for PG stop signal (delay = 0):
    if(it == pg_setup.end() - 1 && (*it).second != 0) {
      LOG(logWARNING) << "No delay = 0 found on last entry. Setting last delay to 0 to stop the pattern generator.";
      (*it).second = 0;
    }
  }

  return true;
}
