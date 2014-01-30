/**
 * pxar API class implementation
 */

#include "api.h"
#include "hal.h"
#include "log.h"
#include "helper.h"
#include "dictionaries.h"
#include <algorithm>
#include <fstream>
#include "config.h"

using namespace pxar;

api::api(std::string usbId, std::string logLevel) : 
  _daq_running(false), 
  _daq_buffersize(0)
{

  LOG(logQUIET) << "Instanciating API for " << PACKAGE_STRING;

  // Set up the libpxar API/HAL logging mechanism:
  Log::ReportingLevel() = Log::FromString(logLevel);
  LOG(logINFO) << "Log level: " << logLevel;

  // Get a new HAL instance with the DTB USB ID passed to the API constructor:
  _hal = new hal(usbId);

  // Get the DUT up and running:
  _dut = new dut();
}

api::~api() {
  delete _dut;
  delete _hal;
}

std::string api::getVersion() { return PACKAGE_STRING; }

bool api::initTestboard(std::vector<std::pair<std::string,uint8_t> > sig_delays,
                       std::vector<std::pair<std::string,double> > power_settings,
			std::vector<std::pair<uint16_t,uint8_t> > pg_setup) {

  // Check the HAL status before doing anything else:
  if(!_hal->compatible()) return false;

  // Collect and check the testboard configuration settings

  // Read the power settings and make sure we got all, these here are the allowed limits:
  double va = 2.5, vd = 3.0, ia = 3.0, id = 3.0;
  for(std::vector<std::pair<std::string,double> >::iterator it = power_settings.begin(); it != power_settings.end(); ++it) {
    std::transform((*it).first.begin(), (*it).first.end(), (*it).first.begin(), ::tolower);

    if((*it).second < 0) {
      LOG(logERROR) << "Negative value for power setting \"" << (*it).first << "\". Using default limit.";
      continue;
    }

    if((*it).first.compare("va") == 0) { 
      if((*it).second > va) { LOG(logWARNING) << "Limiting \"" << (*it).first << "\" to " << va; }
      else { va = (*it).second; }
      _dut->va = va;
    }
    else if((*it).first.compare("vd") == 0) {
      if((*it).second > vd) { LOG(logWARNING) << "Limiting \"" << (*it).first << "\" to " << vd; }
      else {vd = (*it).second; }
      _dut->vd = vd;
    }
    else if((*it).first.compare("ia") == 0) {
      if((*it).second > ia) { LOG(logWARNING) << "Limiting \"" << (*it).first << "\" to " << ia; }
      else { ia = (*it).second; }
      _dut->ia = ia;
    }
    else if((*it).first.compare("id") == 0) {
      if((*it).second > id) { LOG(logWARNING) << "Limiting \"" << (*it).first << "\" to " << id; }
      else { id = (*it).second; }
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
		      << "\" value " << static_cast<int>(ret.first->second)
		      << " with " << static_cast<int>(sigValue);
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

    // FIXME: THESE CHECKS BELOW SHOULD THROW A CUSTOM EXCEPTION

  // Verification/sanitry checks of supplied DUT configuration values
  // Check size of rocDACs and rocPixels against each other
  if(rocDACs.size() != rocPixels.size()) {
    LOG(logCRITICAL) << "Hm, we have " << rocDACs.size() << " DAC configs but " << rocPixels.size() << " pixel configs.";
    LOG(logCRITICAL) << "This cannot end well...";
    return false;
  }
  // check for presence of DAC/pixel configurations
  if (rocDACs.size() == 0 || rocPixels.size() == 0){
    LOG(logCRITICAL) << "No DAC/pixel configurations for any ROC supplied!";
    return false;
  }
  // check individual pixel configs
  for(std::vector<std::vector<pixelConfig> >::iterator rocit = rocPixels.begin();rocit != rocPixels.end(); rocit++){
    // check pixel configuration sizes
    if ((*rocit).size() == 0){
      LOG(logWARNING) << "No pixel configured for ROC "<< (int)(rocit - rocPixels.begin()) << "!";
    }
    if ((*rocit).size() > 4160){
      LOG(logCRITICAL) << "Too many pixels (N_pixel="<< (*rocit).size() <<" > 4160) configured for ROC "<< (int)(rocit - rocPixels.begin()) << "!";
      return false;
    }
    // check individual pixel configurations
    int nduplicates = 0;
    for(std::vector<pixelConfig>::iterator pixit = (*rocit).begin();pixit != (*rocit).end(); pixit++){
      if (std::count_if((*rocit).begin(),(*rocit).end(),findPixelXY((*pixit).column,(*pixit).row)) > 1){
	LOG(logCRITICAL) << "Config for pixel in column " << (int) (*pixit).column<< " and row "<< (int) (*pixit).row << " present multiple times in ROC " << (int)(rocit-rocPixels.begin()) << "!";
	nduplicates++;
      }
    }
    if (nduplicates>0){
      return false;
    }

    // check for pixels out of range
    if (std::count_if((*rocit).begin(),(*rocit).end(),findPixelBeyondXY(51,79)) > 0){
      LOG(logCRITICAL) << "Found pixels with values for column and row outside of valid address range on ROC "<< (int)(rocit - rocPixels.begin())<< "!";
      return false;
    }
  }

  LOG(logDEBUGAPI) << "We have " << rocDACs.size() << " DAC configs and " << rocPixels.size() << " pixel configs, with " << rocDACs.at(0).size() << " and " << rocPixels.at(0).size() << " entries for the first ROC, respectively.";

  // First initialized the API's DUT instance with the information supplied.

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
			<< "\" value " << static_cast<int>(ret.first->second)
			<< " with " << static_cast<int>(dacValue);
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
			<< "\" value " << static_cast<int>(ret.first->second)
			<< " with " << static_cast<int>(dacValue);
	newroc.dacs[dacRegister] = dacValue;
      }
    }

    // Loop over all pixelConfigs supplied:
    for(std::vector<pixelConfig>::iterator pixIt = rocPixels.at(nROCs).begin(); pixIt != rocPixels.at(nROCs).end(); ++pixIt) {
      // Check the trim value to be within boundaries:
      if((*pixIt).trim > 15) {
	LOG(logWARNING) << "Pixel " 
			<< static_cast<int>((*pixIt).column) << ", " 
			<< static_cast<int>((*pixIt).row )<< " trim value " 
			<< static_cast<int>((*pixIt).trim) << " exceeds limit. Set to 15.";
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

  // First thing to do: startup DUT power if not yet done
  _hal->Pon();

  // Start programming the devices here!

  // FIXME Device types not transmitted yet!

  std::vector<tbmConfig> enabledTbms = _dut->getEnabledTbms();
  if(!enabledTbms.empty()) {LOG(logDEBUGAPI) << "Programming TBMs...";}
  for (std::vector<tbmConfig>::iterator tbmit = enabledTbms.begin(); tbmit != enabledTbms.end(); ++tbmit){
    _hal->initTBM(static_cast<uint8_t>(tbmit - enabledTbms.begin()),(*tbmit).dacs);
  }

  std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();
  if(!enabledRocs.empty()) {LOG(logDEBUGAPI) << "Programming ROCs...";}
  for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
    _hal->initROC(static_cast<uint8_t>(rocit - enabledRocs.begin()),(*rocit).type, (*rocit).dacs);
  }

  // As last step, mask all pixels in the device:
  MaskAndTrim(false);

  // The DUT is programmed, everything all right:
  _dut->_programmed = true;

  return true;
}

// API status function, checks HAL and DUT statuses
bool api::status() {
  if(_hal->status() && _dut->status()) return true;
  return false;
}

// Lookup register and check value range
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
		    << name << "\" (" << static_cast<int>(id) << ") to " 
		    << static_cast<int>(regLimit) << " (was: " << static_cast<int>(value) << ")";
    value = static_cast<uint8_t>(regLimit);
  }

  LOG(logDEBUGAPI) << "Verified register \"" << name << "\" (" << static_cast<int>(id) << "): " 
		   << static_cast<int>(value) << " (max " << static_cast<int>(regLimit) << ")"; 
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
  LOG(logDEBUGAPI) << "Device type return: " << static_cast<int>(_code);

  if(_code == 0x0) {LOG(logCRITICAL) << "Unknown device \"" << static_cast<int>(_code) << "\"!";}
  return _code;
}


// DTB functions

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
  // Power is turned on when programming the DUT.
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
    LOG(logDEBUGAPI) << "Probe signal return: " << static_cast<int>(signal);

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
    LOG(logDEBUGAPI) << "Probe signal return: " << static_cast<int>(signal);

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


  
// TEST functions

bool api::setDAC(std::string dacName, uint8_t dacValue, uint8_t rocid) {
  
  if(!status()) {return false;}

  // Get the register number and check the range from dictionary:
  uint8_t dacRegister;
  if(!verifyRegister(dacName, dacRegister, dacValue, ROC_REG)) return false;

  std::pair<std::map<uint8_t,uint8_t>::iterator,bool> ret;
  if(_dut->roc.size() > rocid) {
    // Set the DAC only in the given ROC (even if that is disabled!)

    // Update the DUT DAC Value:
    ret = _dut->roc.at(rocid).dacs.insert( std::make_pair(dacRegister,dacValue) );
    if(ret.second == true) {
      LOG(logWARNING) << "DAC \"" << dacName << "\" was not initialized. Created with value " << static_cast<int>(dacValue);
    }
    else {
      _dut->roc.at(rocid).dacs[dacRegister] = dacValue;
      LOG(logDEBUGAPI) << "DAC \"" << dacName << "\" updated with value " << static_cast<int>(dacValue);
    }

    _hal->rocSetDAC(static_cast<uint8_t>(rocid),dacRegister,dacValue);
  }
  else {
    LOG(logERROR) << "ROC " << rocid << " does not exist in the DUT!";
    return false;
  }
  return true;
}

bool api::setDAC(std::string dacName, uint8_t dacValue) {
  
  if(!status()) {return false;}

  // Get the register number and check the range from dictionary:
  uint8_t dacRegister;
  if(!verifyRegister(dacName, dacRegister, dacValue, ROC_REG)) return false;

  std::pair<std::map<uint8_t,uint8_t>::iterator,bool> ret;
  // Set the DAC for all active ROCs:
  std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();
  for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit) {

    // Update the DUT DAC Value:
    ret = _dut->roc.at(static_cast<uint8_t>(rocit - enabledRocs.begin())).dacs.insert( std::make_pair(dacRegister,dacValue) );
    if(ret.second == true) {
      LOG(logWARNING) << "DAC \"" << dacName << "\" was not initialized. Created with value " << static_cast<int>(dacValue);
    }
    else {
      _dut->roc.at(static_cast<uint8_t>(rocit - enabledRocs.begin())).dacs[dacRegister] = dacValue;
      LOG(logDEBUGAPI) << "DAC \"" << dacName << "\" updated with value " << static_cast<int>(dacValue);
    }

    _hal->rocSetDAC(static_cast<uint8_t>(rocit - enabledRocs.begin()),dacRegister,dacValue);
  }

  return true;
}

bool api::setTbmReg(std::string regName, uint8_t regValue, uint8_t tbmid) {

  if(!status()) {return 0;}
  
  // Get the register number and check the range from dictionary:
  uint8_t _register;
  if(!verifyRegister(regName, _register, regValue, TBM_REG)) return false;

  std::pair<std::map<uint8_t,uint8_t>::iterator,bool> ret;
  if(_dut->tbm.size() > static_cast<size_t>(tbmid)) {
    // Set the register only in the given TBM (even if that is disabled!)

    // Update the DUT register Value:
    ret = _dut->tbm.at(tbmid).dacs.insert( std::make_pair(_register,regValue) );
    if(ret.second == true) {
      LOG(logWARNING) << "Register \"" << regName << "\" was not initialized. Created with value " << static_cast<int>(regValue);
    }
    else {
      _dut->tbm.at(tbmid).dacs[_register] = regValue;
      LOG(logDEBUGAPI) << "Register \"" << regName << "\" updated with value " << static_cast<int>(regValue);
    }

    _hal->tbmSetReg(static_cast<uint8_t>(tbmid),_register,regValue);
  }
  else {
    LOG(logERROR) << "TBM " << tbmid << " is not existing in the DUT!";
    return false;
  }
  return true;
}

bool api::setTbmReg(std::string regName, uint8_t regValue) {

  if(!status()) {return 0;}
  
  // Get the register number and check the range from dictionary:
  uint8_t _register;
  if(!verifyRegister(regName, _register, regValue, TBM_REG)) return false;

  std::pair<std::map<uint8_t,uint8_t>::iterator,bool> ret;
  // Set the register for all active TBMs:
  std::vector<tbmConfig> enabledTbms = _dut->getEnabledTbms();
  for (std::vector<tbmConfig>::iterator tbmit = enabledTbms.begin(); tbmit != enabledTbms.end(); ++tbmit) {

    // Update the DUT register Value:
    ret = _dut->tbm.at(static_cast<uint8_t>(tbmit - enabledTbms.begin())).dacs.insert( std::make_pair(_register,regValue) );
    if(ret.second == true) {
      LOG(logWARNING) << "Register \"" << regName << "\" was not initialized. Created with value " << static_cast<int>(regValue);
    }
    else {
      _dut->tbm.at(static_cast<uint8_t>(tbmit - enabledTbms.begin())).dacs[_register] = regValue;
      LOG(logDEBUGAPI) << "Register \"" << regName << "\" updated with value " << static_cast<int>(regValue);
    }

    _hal->tbmSetReg(static_cast<uint8_t>(tbmit - enabledTbms.begin()),_register,regValue);
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
  std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();
  for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
    uint8_t oldDacValue = _dut->getDAC(static_cast<size_t>(rocit - enabledRocs.begin()),dacName);
    LOG(logDEBUGAPI) << "Reset DAC \"" << dacName << "\" to original value " << static_cast<int>(oldDacValue);
    _hal->rocSetDAC(static_cast<uint8_t>(rocit - enabledRocs.begin()),dacRegister,oldDacValue);
  }

  delete data;
  return *result;
}

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
  std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();
  for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
    uint8_t oldDacValue = _dut->getDAC(static_cast<size_t>(rocit - enabledRocs.begin()),dacName);
    LOG(logDEBUGAPI) << "Reset DAC \"" << dacName << "\" to original value " << static_cast<int>(oldDacValue);
    _hal->rocSetDAC(static_cast<uint8_t>(rocit - enabledRocs.begin()),dacRegister,oldDacValue);
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
  std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();
  for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
    uint8_t oldDacValue = _dut->getDAC(static_cast<size_t>(rocit - enabledRocs.begin()),dacName);
    LOG(logDEBUGAPI) << "Reset DAC \"" << dacName << "\" to original value " << static_cast<int>(oldDacValue);
    _hal->rocSetDAC(static_cast<uint8_t>(rocit - enabledRocs.begin()),dacRegister,oldDacValue);
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
  std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();
  for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
    uint8_t oldDac1Value = _dut->getDAC(static_cast<size_t>(rocit - enabledRocs.begin()),dac1name);
    uint8_t oldDac2Value = _dut->getDAC(static_cast<size_t>(rocit - enabledRocs.begin()),dac2name);
    LOG(logDEBUGAPI) << "Reset DAC \"" << dac1name << "\" to original value " << static_cast<int>(oldDac1Value);
    LOG(logDEBUGAPI) << "Reset DAC \"" << dac2name << "\" to original value " << static_cast<int>(oldDac2Value);
    _hal->rocSetDAC(static_cast<uint8_t>(rocit - enabledRocs.begin()),dac1register,oldDac1Value);
    _hal->rocSetDAC(static_cast<uint8_t>(rocit - enabledRocs.begin()),dac2register,oldDac2Value);
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
  std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();
  for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
    uint8_t oldDac1Value = _dut->getDAC(static_cast<size_t>(rocit - enabledRocs.begin()),dac1name);
    uint8_t oldDac2Value = _dut->getDAC(static_cast<size_t>(rocit - enabledRocs.begin()),dac2name);
    LOG(logDEBUGAPI) << "Reset DAC \"" << dac1name << "\" to original value " << static_cast<int>(oldDac1Value);
    LOG(logDEBUGAPI) << "Reset DAC \"" << dac2name << "\" to original value " << static_cast<int>(oldDac2Value);
    _hal->rocSetDAC(static_cast<uint8_t>(rocit - enabledRocs.begin()),dac1register,oldDac1Value);
    _hal->rocSetDAC(static_cast<uint8_t>(rocit - enabledRocs.begin()),dac2register,oldDac2Value);
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
  std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();
  for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
    uint8_t oldDac1Value = _dut->getDAC(static_cast<size_t>(rocit - enabledRocs.begin()),dac1name);
    uint8_t oldDac2Value = _dut->getDAC(static_cast<size_t>(rocit - enabledRocs.begin()),dac2name);
    LOG(logDEBUGAPI) << "Reset DAC \"" << dac1name << "\" to original value " << static_cast<int>(oldDac1Value);
    LOG(logDEBUGAPI) << "Reset DAC \"" << dac2name << "\" to original value " << static_cast<int>(oldDac2Value);
    _hal->rocSetDAC(static_cast<uint8_t>(rocit - enabledRocs.begin()),dac1register,oldDac1Value);
    _hal->rocSetDAC(static_cast<uint8_t>(rocit - enabledRocs.begin()),dac2register,oldDac2Value);
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

std::vector<pixel> api::getThresholdMap(std::string dacName, uint16_t flags, uint32_t nTriggers) {

  if(!status()) {return std::vector<pixel>();}

  uint8_t dacRegister, dacValue = 0;
  if(!verifyRegister(dacName, dacRegister, dacValue, ROC_REG)) {
    return std::vector<pixel>();
  }

  // Setup the correct _hal calls for this test
  HalMemFnPixel pixelfn = &hal::PixelThresholdMap;
  HalMemFnRoc rocfn = &hal::RocThresholdMap;
  HalMemFnModule modulefn = NULL;

  // Load the test parameters into vector
  std::vector<int32_t> param;
  param.push_back(static_cast<int32_t>(flags));
  param.push_back(static_cast<int32_t>(nTriggers));
  param.push_back(static_cast<int32_t>(dacRegister));

  // check if the flags indicate that the user explicitly asks for serial execution of test:
  // FIXME: FLAGS NOT YET CHECKED!
  bool forceSerial = flags & FLAG_FORCE_SERIAL;
  std::vector< std::vector<pixel> >* data = expandLoop(pixelfn, rocfn, modulefn, param, forceSerial);

  // Repacking of all data segments into one long map vector:
  std::vector<pixel>* result = repackMapData(data);
  delete data;
  return *result;
}
  
int32_t api::getReadbackValue(std::string /*parameterName*/) {

  if(!status()) {return -1;}
  LOG(logCRITICAL) << "NOT IMPLEMENTED YET! (File a bug report if you need this urgently...)";
  return -1;
}


// DAQ functions

bool api::daqStart(std::vector<std::pair<uint16_t, uint8_t> > pg_setup) {

  if(!status()) {return false;}
  if(daqStatus()) {return false;}

  // Allocate the maximum memory allowed: 50M samples
  _daq_buffersize = 50000000;

  LOG(logDEBUGAPI) << "Starting new DAQ session...";
  if(!pg_setup.empty()) {
    // Prepare new Pattern Generator:
    if(!verifyPatternGenerator(pg_setup)) return false;
    _hal->SetupPatternGenerator(pg_setup);
  }
  
  // Setup the configured mask and trim state of the DUT:
  MaskAndTrim(true);

  // Set Calibrate bits in the PUCs (we use the testrange for that):
  SetCalibrateBits(true);

  // Check the DUT if we have TBMs enabled or not and choose the right
  // deserializer:
  _hal->daqStart(_dut->sig_delays[SIG_DESER160PHASE],_dut->getNEnabledTbms(), _daq_buffersize);

  _daq_running = true;
  return true;
}

bool api::daqStatus() {

  // Check if a DAQ session is running:
  if(!_daq_running) {
    LOG(logDEBUGAPI) << "DAQ not running!";
    return false;
  }

  // Check if we still have enough buffer memory left (with some safety margin).
  // Only filling buffer up to 90% in order not to lose data.
  uint32_t filled_buffer = _hal->daqBufferStatus();
  if(filled_buffer > 0.9*_daq_buffersize) {
    LOG(logDEBUGAPI) << "DAQ buffer about to overflow!";
    return false;
  }

  LOG(logDEBUGAPI) << "Everything alright, buffer size " << filled_buffer
		   << "/" << _daq_buffersize;
  return true;
}

void api::daqTrigger(uint32_t nTrig) {

  if(daqStatus()) {
    // Just passing the call to the HAL, not doing anything else here:
    _hal->daqTrigger(nTrig);
  }
}

void api::daqTriggerLoop(uint16_t period) {

  if(daqStatus()) {
    // Pattern Generator loop doesn't work for delay periods smaller than
    // 110 clock cycles, so limit it to that:
    if(period < 110) {
      period = 110;
      LOG(logWARNING) << "Loop period setting too small for Pattern generator. "
		      << "Setting loop delay to " << period << " clk";
    }
    
    _hal->daqTriggerLoop(period);
  }
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

std::vector<pixel> api::daqGetEvent() {

  if(!daqStatus()) {return std::vector<pixel>();}

  // FIXME: needs to actually interact with the HAL and get DATA
  LOG(logCRITICAL) << "NOT IMPLEMENTED YET! (File a bug report if you need this urgently...)";
  return std::vector<pixel>();
}

bool api::daqStop() {

  if(!status()) {return false;}
  if(!daqStatus()) {return false;}

  _daq_running = false;
  
  // Stop all active DAQ channels (depending on number of TBMs)
  _hal->daqStop(_dut->getNEnabledTbms());

  // Mask all pixels in the device again:
  MaskAndTrim(false);

  // Reset all the Calibrate bits and signals:
  SetCalibrateBits(false);

  // Re-program the old Pattern Generator setup which is stored in the DUT.
  // Since these patterns are verified already, just write them:
  LOG(logDEBUGAPI) << "Resetting Pattern Generator to previous state.";
  _hal->SetupPatternGenerator(_dut->pg_setup);
  
  return false;
}


std::vector< std::vector<pixel> >* api::expandLoop(HalMemFnPixel pixelfn, HalMemFnRoc rocfn, HalMemFnModule modulefn, std::vector<int32_t> param,  bool forceSerial) {
  
  // pointer to vector to hold our data
  std::vector< std::vector<pixel> >* data = NULL;


  // Do the masking/unmasking&trimming for all ROCs first
  MaskAndTrim(true);

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
	std::vector< std::vector<pixel> >* rocdata = CALL_MEMBER_FN(*_hal,rocfn)(static_cast<uint8_t>(rocit - enabledRocs.begin()), param); // rocit - enabledRocs.begin() == index
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
	std::vector<pixelConfig> enabledPixels = _dut->getEnabledPixels(static_cast<uint8_t>(rocit - enabledRocs.begin()));


	LOG(logDEBUGAPI) << "\"The Loop\" for the current ROC contains " \
			 << enabledPixels.size() << " calls to \'pixelfn\'";

	for (std::vector<pixelConfig>::iterator pixit = enabledPixels.begin(); pixit != enabledPixels.end(); ++pixit) {
	  // execute call to HAL layer routine and store data in buffer
	  std::vector< std::vector<pixel> >* buffer = CALL_MEMBER_FN(*_hal,pixelfn)(static_cast<uint8_t>(rocit - enabledRocs.begin()), pixit->column, pixit->row, param);
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

  // Test is over, mask the whole device again:
  MaskAndTrim(false);

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

  LOG(logDEBUGAPI) << "Packing range " << static_cast<int>(dacMin) << "-" << static_cast<int>(dacMax) << ", data has " << data->size() << " entries.";

  for (std::vector<std::vector<pixel> >::iterator vecit = data->begin(); vecit!=data->end();++vecit){
    result->push_back(std::make_pair(currentDAC, *vecit));
    currentDAC++;
  }

  LOG(logDEBUGAPI) << "Repack end: current " << static_cast<int>(currentDAC) << " (max " << static_cast<int>(dacMax) << ")";
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

  LOG(logDEBUGAPI) << "Repack end: current1 " << static_cast<int>(current1dac) << " (max " << static_cast<int>(dac1max) << "), current2 " 
		   << static_cast<int>(current2dac) << " (max" << static_cast<int>(dac2max) << ")";

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


// Mask/Unmask and trim the ROCs:
void api::MaskAndTrim(bool trim) {

  // Run over all existing ROCs:
  for (std::vector<rocConfig>::iterator rocit = _dut->roc.begin(); rocit != _dut->roc.end(); ++rocit) {

    // Check if we can run on full ROCs:
    uint16_t masked = _dut->getNMaskedPixels(static_cast<uint8_t>(rocit-_dut->roc.begin()));
    LOG(logDEBUGAPI) << "ROC " << static_cast<int>(rocit-_dut->roc.begin()) << " features " << masked << " masked pixels.";

    // This ROC is completely unmasked, let's trim it:
    if(masked == 0 && trim) {
      LOG(logDEBUGAPI) << "Unmasking and trimming ROC " << static_cast<int>(rocit-_dut->roc.begin()) << " in one go.";
      _hal->RocSetMask(static_cast<int>(rocit-_dut->roc.begin()),false,rocit->pixels);
      continue;
    }
    else if(masked == ROC_NUMROWS*ROC_NUMCOLS || !trim) {
      LOG(logDEBUGAPI) << "Masking ROC " << static_cast<int>(rocit-_dut->roc.begin()) << " in one go.";
      _hal->RocSetMask(static_cast<int>(rocit-_dut->roc.begin()),true);
      continue;
    }
    // Choose the version with less calls (less than half the pixels to change):
    else if(masked <= ROC_NUMROWS*ROC_NUMCOLS/2) {
      // We have more unmasked than masked pixels:
      LOG(logDEBUGAPI) << "Unmasking and trimming ROC " << static_cast<int>(rocit-_dut->roc.begin()) << " before masking single pixels.";
      _hal->RocSetMask(static_cast<int>(rocit-_dut->roc.begin()),false,rocit->pixels);

      // Disable all unneeded columns:
      std::vector<bool> enabledColumns = _dut->getEnabledColumns(static_cast<size_t>(rocit-_dut->roc.begin()));
      for(std::vector<bool>::iterator it = enabledColumns.begin(); it != enabledColumns.end(); ++it) {
	if(!(*it)) _hal->ColumnSetEnable(static_cast<uint8_t>(rocit - _dut->roc.begin()),static_cast<uint8_t>(it - enabledColumns.begin()),(*it));
      }

      // And then mask the required pixels:
      for(std::vector<pixelConfig>::iterator pxit = rocit->pixels.begin(); pxit != rocit->pixels.end(); ++pxit) {
	if(pxit->mask == true) {_hal->PixelSetMask(static_cast<uint8_t>(rocit-_dut->roc.begin()),pxit->column,pxit->row,true);}
      }
    }
    else {
      // Some are unmasked, but not too many. First mask that ROC:
      LOG(logDEBUGAPI) << "Masking ROC " << static_cast<int>(rocit-_dut->roc.begin()) << " before unmasking single pixels.";
      _hal->RocSetMask(static_cast<uint8_t>(rocit-_dut->roc.begin()),true);

      // Enable all needed columns:
      std::vector<bool> enabledColumns = _dut->getEnabledColumns(static_cast<size_t>(rocit-_dut->roc.begin()));
      for(std::vector<bool>::iterator it = enabledColumns.begin(); it != enabledColumns.end(); ++it) {
	if((*it)) _hal->ColumnSetEnable(static_cast<uint8_t>(rocit - _dut->roc.begin()),static_cast<uint8_t>(it - enabledColumns.begin()),(*it));
      }

      // And then unmask the required pixels with their trim values:
      for(std::vector<pixelConfig>::iterator pxit = rocit->pixels.begin(); pxit != rocit->pixels.end(); ++pxit) {
	if(pxit->mask == false) {_hal->PixelSetMask(static_cast<uint8_t>(rocit-_dut->roc.begin()),pxit->column,pxit->row,false,pxit->trim);}
      }
    }
  }

}

// Program the calibrate bits in ROC PUCs:
void api::SetCalibrateBits(bool enable) {

  // Run over all existing ROCs:
  for (std::vector<rocConfig>::iterator rocit = _dut->roc.begin(); rocit != _dut->roc.end(); ++rocit) {

    LOG(logDEBUGAPI) << "Configuring calibrate bits in all enabled PUCs of ROC " << static_cast<int>(rocit-_dut->roc.begin());
    // Check if the signal has to be turned on or off:
    if(enable) {
      // Loop over all pixels in this ROC and set the Cal bit:
      for(std::vector<pixelConfig>::iterator pxit = rocit->pixels.begin(); pxit != rocit->pixels.end(); ++pxit) {
      
	if(pxit->enable == true) {
	  _hal->PixelSetCalibrate(static_cast<uint8_t>(rocit-_dut->roc.begin()),pxit->column,pxit->row,0);
	}
      }

    }
    // Clear the signal for the full ROC:
    else {_hal->RocClearCalibrate(static_cast<uint8_t>(rocit-_dut->roc.begin()));}
  }
}


bool api::verifyPatternGenerator(std::vector<std::pair<uint16_t,uint8_t> > &pg_setup) {
  
  uint32_t delay_sum = 0;

  for(std::vector<std::pair<uint16_t,uint8_t> >::iterator it = pg_setup.begin(); it != pg_setup.end(); ++it) {
    if((*it).second == 0 && it != pg_setup.end() -1 ) {
      LOG(logCRITICAL) << "Found delay = 0 on early entry! This stops the pattern generator at position " 
		       << static_cast<int>(it - pg_setup.begin())  << ".";
      return false;
    }
    // Check last entry for PG stop signal (delay = 0):
    if(it == pg_setup.end() - 1 && (*it).second != 0) {
      LOG(logWARNING) << "No delay = 0 found on last entry. Setting last delay to 0 to stop the pattern generator.";
      (*it).second = 0;
    }
    delay_sum += (*it).second;
  }

  LOG(logDEBUGAPI) << "Sum of Pattern generator delays: " << delay_sum << " clk";
  return true;
}
