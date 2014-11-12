/**
 * pxar API class implementation
 */

#include "api.h"
#include "hal.h"
#include "log.h"
#include "timer.h"
#include "helper.h"
#include "dictionaries.h"
#include <algorithm>
#include <fstream>
#include <cmath>
#include "constants.h"
#include "config.h"

using namespace pxar;

pxarCore::pxarCore(std::string usbId, std::string logLevel) : 
  _daq_running(false), 
  _daq_buffersize(DTB_SOURCE_BUFFER_SIZE),
  _ndecode_errors_lastdaq(0),
  _daq_startstop_warning(false)
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

pxarCore::~pxarCore() {
  delete _dut;
  delete _hal;
}

std::string pxarCore::getVersion() { return PACKAGE_STRING; }

bool pxarCore::initTestboard(std::vector<std::pair<std::string,uint8_t> > sig_delays,
			std::vector<std::pair<std::string,double> > power_settings,
			std::vector<std::pair<std::string,uint8_t> > pg_setup) {

  // Check the HAL status before doing anything else:
  if(!_hal->compatible()) return false;

  // Collect and check the testboard configuration settings

  // Power settings:
  checkTestboardPower(power_settings);

  // Signal Delays:
  checkTestboardDelays(sig_delays);
  
  // Prepare Pattern Generator:
  verifyPatternGenerator(pg_setup);

  // Call the HAL to do the job:
  _hal->initTestboard(_dut->sig_delays,_dut->pg_setup,_dut->pg_sum,_dut->va,_dut->vd,_dut->ia,_dut->id);
  return true;
}

void pxarCore::setTestboardDelays(std::vector<std::pair<std::string,uint8_t> > sig_delays) {
  if(!_hal->status()) {
    LOG(logERROR) << "Signal delays not updated!";
    return;
  }
  checkTestboardDelays(sig_delays);
  _hal->setTestboardDelays(_dut->sig_delays);
  LOG(logDEBUGAPI) << "Testboard signal delays updated.";
}

void pxarCore::setPatternGenerator(std::vector<std::pair<std::string,uint8_t> > pg_setup) {
  if(!_hal->status()) {
    LOG(logERROR) << "Pattern generator not updated!";
    return;
  }
  verifyPatternGenerator(pg_setup);
  _hal->SetupPatternGenerator(_dut->pg_setup,_dut->pg_sum);
  LOG(logDEBUGAPI) << "Pattern generator verified and updated.";
}

void pxarCore::setTestboardPower(std::vector<std::pair<std::string,double> > power_settings) {
  if(!_hal->status()) {
    LOG(logERROR) << "Voltages/current limits not upated!";
    return;
  }
  checkTestboardPower(power_settings);
  _hal->setTestboardPower(_dut->va,_dut->vd,_dut->ia,_dut->id);
  LOG(logDEBUGAPI) << "Voltages/current limits updated.";
}

bool pxarCore::initDUT(uint8_t hubid,
		       std::string tbmtype, 
		       std::vector<std::vector<std::pair<std::string,uint8_t> > > tbmDACs,
		       std::string roctype,
		       std::vector<std::vector<std::pair<std::string,uint8_t> > > rocDACs,
		       std::vector<std::vector<pixelConfig> > rocPixels) {
  std::vector<uint8_t> rocI2Cs;
  return initDUT(hubid, tbmtype, tbmDACs, roctype, rocDACs, rocPixels, rocI2Cs);
}

bool pxarCore::initDUT(uint8_t hubid,
		       std::string tbmtype, 
		       std::vector<std::vector<std::pair<std::string,uint8_t> > > tbmDACs,
		       std::string roctype,
		       std::vector<std::vector<std::pair<std::string,uint8_t> > > rocDACs,
		       std::vector<std::vector<pixelConfig> > rocPixels,
		       std::vector<uint8_t> rocI2Cs) {

  // Check if the HAL is ready:
  if(!_hal->status()) return false;

  // Verification/sanitry checks of supplied DUT configuration values

  // Check if I2C addresses were supplied - if so, check size agains sets of DACs:
  if(!rocI2Cs.empty()) {
    if(rocI2Cs.size() != rocDACs.size()) {
      LOG(logCRITICAL) << "Hm, we have " << rocI2Cs.size() << " I2C addresses but " << rocDACs.size() << " DAC configs.";
      LOG(logCRITICAL) << "This cannot end well...";
      throw InvalidConfig("Mismatch between number of I2C addresses and DAC configurations");
    }
    LOG(logDEBUGAPI) << "I2C addresses for all ROCs are provided as user input.";
  }
  else { LOG(logDEBUGAPI) << "I2C addresses will be automatically generated."; }

  // Check size of rocDACs and rocPixels against each other
  if(rocDACs.size() != rocPixels.size()) {
    LOG(logCRITICAL) << "Hm, we have " << rocDACs.size() << " DAC configs but " << rocPixels.size() << " pixel configs.";
    LOG(logCRITICAL) << "This cannot end well...";
    throw InvalidConfig("Mismatch between number of DAC and pixel configurations");
  }
  // check for presence of DAC/pixel configurations
  if (rocDACs.size() == 0 || rocPixels.size() == 0){
    LOG(logCRITICAL) << "No DAC/pixel configurations for any ROC supplied!";
    throw InvalidConfig("No DAC/pixel configurations for any ROC supplied");
  }
  // check individual pixel configs
  for(std::vector<std::vector<pixelConfig> >::iterator rocit = rocPixels.begin();rocit != rocPixels.end(); rocit++){
    // check pixel configuration sizes
    if ((*rocit).size() == 0){
      LOG(logWARNING) << "No pixel configured for ROC "<< static_cast<int>(rocit - rocPixels.begin()) << "!";
    }
    if ((*rocit).size() > 4160){
      LOG(logCRITICAL) << "Too many pixels (N_pixel="<< rocit->size() <<" > 4160) configured for ROC "<< static_cast<int>(rocit - rocPixels.begin()) << "!";
      throw InvalidConfig("Too many pixels (>4160) configured");
    }
    // check individual pixel configurations
    int nduplicates = 0;
    for(std::vector<pixelConfig>::iterator pixit = rocit->begin(); pixit != rocit->end(); pixit++){
      if (std::count_if(rocit->begin(),rocit->end(),findPixelXY(pixit->column(),pixit->row())) > 1){
	LOG(logCRITICAL) << "Config for pixel in column " << static_cast<int>(pixit->column()) << " and row "<< static_cast<int>(pixit->row()) << " present multiple times in ROC " << static_cast<int>(rocit-rocPixels.begin()) << "!";
	nduplicates++;
      }
    }
    if (nduplicates>0){
      throw InvalidConfig("Duplicate pixel configurations present");
    }

    // check for pixels out of range
    if (std::count_if((*rocit).begin(),(*rocit).end(),findPixelBeyondXY(51,79)) > 0) {
      LOG(logCRITICAL) << "Found pixels with values for column and row outside of valid address range on ROC "<< static_cast<int>(rocit - rocPixels.begin()) << "!";
      throw InvalidConfig("Found pixels with values for column and row outside of valid address range");
    }
  }

  LOG(logDEBUGAPI) << "We have " << rocDACs.size() << " DAC configs and " << rocPixels.size() << " pixel configs, with " << rocDACs.at(0).size() << " and " << rocPixels.at(0).size() << " entries for the first ROC, respectively.";

  // First initialized the API's DUT instance with the information supplied.

  // Store the hubId:
  _dut->hubId = hubid;

  // Initialize TBMs:
  LOG(logDEBUGAPI) << "Received settings for " << tbmDACs.size() << " TBM cores.";

  for(std::vector<std::vector<std::pair<std::string,uint8_t> > >::iterator tbmIt = tbmDACs.begin(); tbmIt != tbmDACs.end(); ++tbmIt) {

    LOG(logDEBUGAPI) << "Processing TBM Core " << static_cast<int>(tbmIt - tbmDACs.begin());
    // Prepare a new TBM configuration
    tbmConfig newtbm;

    // Set the TBM type (get value from dictionary)
    newtbm.type = stringToDeviceCode(tbmtype);
    if(newtbm.type == 0x0) return false;
    
    // Loop over all the DAC settings supplied and fill them into the TBM dacs
    for(std::vector<std::pair<std::string,uint8_t> >::iterator dacIt = (*tbmIt).begin(); dacIt != (*tbmIt).end(); ++dacIt) {

      // Fill the register pairs with the register id from the dictionary:
      uint8_t tbmregister, value = dacIt->second;
      if(!verifyRegister(dacIt->first, tbmregister, value, TBM_REG)) continue;

      // Check if this is fore core alpha or beta:
      if((tbmIt - tbmDACs.begin())%2 == 0) { tbmregister = 0xE0 | tbmregister; } // alpha core
      else { tbmregister = 0xF0 | tbmregister; } // beta core
      
      std::pair<std::map<uint8_t,uint8_t>::iterator,bool> ret;
      ret = newtbm.dacs.insert( std::make_pair(tbmregister,value) );
      if(ret.second == false) {
	LOG(logWARNING) << "Overwriting existing DAC \"" << dacIt->first 
			<< "\" value " << static_cast<int>(ret.first->second)
			<< " with " << static_cast<int>(value);
	newtbm.dacs[tbmregister] = value;
      }
    }

    // Done. Enable bit is already set by tbmConfig constructor.
    _dut->tbm.push_back(newtbm);
  }

  // Check number of configured TBM cores. If we only got one register vector, we re-use it for the second TBM core:
  if(_dut->tbm.size() == 1) {
    LOG(logDEBUGAPI) << "Only register settings for one TBM core supplied. Duplicating to second core.";
    // Prepare a new TBM configuration and copy over all settings:
    tbmConfig newtbm;
    newtbm.type = _dut->tbm.at(0).type;
    
    for(std::map<uint8_t,uint8_t>::iterator reg = _dut->tbm.at(0).dacs.begin(); reg != _dut->tbm.at(0).dacs.end(); ++reg) {
      uint8_t tbmregister = reg->first;
      // Flip the last bit of the TBM core identifier:
      tbmregister ^= (1u << 4);
      newtbm.dacs.insert(std::make_pair(tbmregister,reg->second));
    }
    _dut->tbm.push_back(newtbm);
  }

  // Check if we have any TBM present to select termination for the DTB RDA/Tout input:
  if(!_dut->tbm.empty()) {
    // We have RDA input from a TBM, this needs LCDS termination:
    _hal->SigSetLCDS();
    LOG(logDEBUGAPI) << "RDA/Tout DTB input termination set to LCDS.";
  }
  else {
    // We expect the direct TokenOut signal from a ROC which needs LVDS termination:
    _hal->SigSetLVDS();
    LOG(logDEBUGAPI) << "RDA/Tout DTB input termination set to LVDS.";
  }

  // Initialize ROCs:
  for(std::vector<std::vector<std::pair<std::string,uint8_t> > >::iterator rocIt = rocDACs.begin(); rocIt != rocDACs.end(); ++rocIt){

    // Prepare a new ROC configuration
    rocConfig newroc;
    // Set the ROC type (get value from dictionary)
    newroc.type = stringToDeviceCode(roctype);
    if(newroc.type == 0x0) return false;

    // If no I2C addresses have been supplied, we just assume they are consecutively numbered:
    if(rocI2Cs.empty()) { newroc.i2c_address = static_cast<uint8_t>(rocIt - rocDACs.begin()); }
    // if we have adresses, let's pick the right one and assign it:
    else { newroc.i2c_address = static_cast<uint8_t>(rocI2Cs.at(rocIt - rocDACs.begin())); }
    LOG(logDEBUGAPI) << "I2C address for the next ROC is: " << static_cast<int>(newroc.i2c_address);
    
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
    for(std::vector<pixelConfig>::iterator pixIt = rocPixels.at(rocIt - rocDACs.begin()).begin(); pixIt != rocPixels.at(rocIt - rocDACs.begin()).end(); ++pixIt) {
      // Check the trim value to be within boundaries:
      if((*pixIt).trim() > 15) {
	LOG(logWARNING) << "Pixel " 
			<< static_cast<int>((*pixIt).column()) << ", " 
			<< static_cast<int>((*pixIt).row())<< " trim value " 
			<< static_cast<int>((*pixIt).trim()) << " exceeds limit. Set to 15.";
	(*pixIt).setTrim(15);
      }
      // Push the pixelConfigs into the rocConfig:
      newroc.pixels.push_back(*pixIt);
    }

    // Done. Enable bit is already set by rocConfig constructor.
    _dut->roc.push_back(newroc);
  }

  // All data is stored in the DUT struct, now programming it.
  _dut->_initialized = true;
  return programDUT();
}

bool pxarCore::programDUT() {

  if(!_dut->_initialized) {
    LOG(logERROR) << "DUT not initialized, unable to program it.";
    return false;
  }

  // First thing to do: startup DUT power if not yet done
  _hal->Pon();

  // Start programming the devices here!
  _hal->setHubId(_dut->hubId); 

  std::vector<tbmConfig> enabledTbms = _dut->getEnabledTbms();
  if(!enabledTbms.empty()) {LOG(logDEBUGAPI) << "Programming TBMs...";}
  for (std::vector<tbmConfig>::iterator tbmit = enabledTbms.begin(); tbmit != enabledTbms.end(); ++tbmit){
    _hal->initTBMCore((*tbmit).type,(*tbmit).dacs);
  }

  std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();
  if(!enabledRocs.empty()) {LOG(logDEBUGAPI) << "Programming ROCs...";}
  for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
    _hal->initROC(rocit->i2c_address,(*rocit).type, (*rocit).dacs);
  }

  // As last step, mask all pixels in the device and detach all double column readouts::
  MaskAndTrim(false);
  for (std::vector<rocConfig>::iterator rocit = _dut->roc.begin(); rocit != _dut->roc.end(); ++rocit) {
    _hal->AllColumnsSetEnable(rocit->i2c_address,true);
  }

  // The DUT is programmed, everything all right:
  _dut->_programmed = true;

  return true;
}

// API status function, checks HAL and DUT statuses
bool pxarCore::status() {
  if(_hal->status() && _dut->status()) return true;
  return false;
}

// Lookup register and check value range
bool pxarCore::verifyRegister(std::string name, uint8_t &id, uint8_t &value, uint8_t type) {

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
uint8_t pxarCore::stringToDeviceCode(std::string name) {

  // Convert the name to lower case for comparison:
  std::transform(name.begin(), name.end(), name.begin(), ::tolower);
  LOG(logDEBUGAPI) << "Looking up device type for \"" << name << "\"";

  // Get singleton device dictionary object:
  DeviceDictionary * _devices = DeviceDictionary::getInstance();

  // And get the device code from the dictionary object:
  uint8_t _code = _devices->getDevCode(name);
  LOG(logDEBUGAPI) << "Device type return: " << static_cast<int>(_code);

  if(_code == 0x0) {LOG(logERROR) << "Unknown device \"" << static_cast<int>(_code) << "\"!";}
  return _code;
}


// DTB functions

bool pxarCore::flashTB(std::string filename) {

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

double pxarCore::getTBia() {
  if(!_hal->status()) {return 0;}
  return _hal->getTBia();
}

double pxarCore::getTBva() {
  if(!_hal->status()) {return 0;}
  return _hal->getTBva();
}

double pxarCore::getTBid() {
  if(!_hal->status()) {return 0;}
  return _hal->getTBid();
}

double pxarCore::getTBvd() {
  if(!_hal->status()) {return 0;}
  return _hal->getTBvd();
}


void pxarCore::HVoff() {
  _hal->HVoff();
}

void pxarCore::HVon() {
  _hal->HVon();
}

void pxarCore::Poff() {
  _hal->Poff();
  // Reset the programmed state of the DUT (lost by turning off power)
  _dut->_programmed = false;
}

void pxarCore::Pon() {
  // Power is turned on when programming the DUT.
  // Re-program the DUT after power has been switched on:
  programDUT();
}

bool pxarCore::SignalProbe(std::string probe, std::string name) {

  if(!_hal->status()) {return false;}

  // Get singleton Probe dictionary object:
  ProbeDictionary * _dict = ProbeDictionary::getInstance();

  // Convert the probe name to lower case for comparison:
  std::transform(probe.begin(), probe.end(), probe.begin(), ::tolower);
  
  // Convert the name to lower case for comparison:
  std::transform(name.begin(), name.end(), name.begin(), ::tolower);
  
  // Digital signal probes:
  if(probe.compare(0,1,"d") == 0) {
    
    // And get the register value from the dictionary object:
    uint8_t signal = _dict->getSignal(name,PROBE_DIGITAL);
    LOG(logDEBUGAPI) << "Digital probe signal lookup for \"" << name 
		     << "\" returned signal: " << static_cast<int>(signal);

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

    // And get the register value from the dictionary object:
    uint8_t signal = _dict->getSignal(name, PROBE_ANALOG);
    LOG(logDEBUGAPI) << "Analog probe signal lookup for \"" << name 
		     << "\" returned signal: " << static_cast<int>(signal);

    // Select the correct probe for the output:
    if(probe.compare("a1") == 0) {
      _hal->SignalProbeA1(signal);
      return true;
    }
    else if(probe.compare("a2") == 0) {
      _hal->SignalProbeA2(signal);
      return true;
    }
    else if (probe.compare("adc") == 0) {
       _hal->SignalProbeADC(signal, 0);
      return true;
    }
  }
    
  LOG(logERROR) << "Invalid probe name \"" << probe << "\" selected!";
  return false;
}



std::vector<uint16_t> pxarCore::daqADC(std::string signalName, uint8_t gain, uint16_t nSample, uint8_t source, uint8_t start){
    
    vector<uint16_t> data;
    if(!_hal->status()) {return data;}
    
    ProbeDictionary * _dict = ProbeDictionary::getInstance();
    std::transform(signalName.begin(), signalName.end(), signalName.begin(), ::tolower);
    uint8_t signal = _dict->getSignal(signalName, PROBE_ANALOG);
 
    data = _hal->daqADC(signal, gain, nSample, source, start);
    return data;
}


  
// TEST functions

bool pxarCore::setDAC(std::string dacName, uint8_t dacValue, uint8_t rocid) {
  
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

    _hal->rocSetDAC(_dut->roc.at(rocid).i2c_address,dacRegister,dacValue);
  }
  else {
    LOG(logERROR) << "ROC " << static_cast<int>(rocid) << " does not exist in the DUT!";
    return false;
  }
  return true;
}

bool pxarCore::setDAC(std::string dacName, uint8_t dacValue) {
  
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

uint8_t pxarCore::getDACRange(std::string dacName) {
  
  // Get the register number and check the range from dictionary:
  uint8_t dacRegister;
  uint8_t val = 0;
  if(!verifyRegister(dacName, dacRegister, val, ROC_REG)) return 0;
  
  // Get singleton DAC dictionary object:
  RegisterDictionary * _dict = RegisterDictionary::getInstance();

  // Read register value limit:
  return _dict->getSize(dacRegister, ROC_REG);
}

bool pxarCore::setTbmReg(std::string regName, uint8_t regValue, uint8_t tbmid) {

  if(!status()) {return 0;}
  
  // Get the register number and check the range from dictionary:
  uint8_t _register;
  if(!verifyRegister(regName, _register, regValue, TBM_REG)) return false;

  std::pair<std::map<uint8_t,uint8_t>::iterator,bool> ret;
  if(_dut->tbm.size() > static_cast<size_t>(tbmid)) {
    // Set the register only in the given TBM (even if that is disabled!)
    
    // Get the core (alpha/beta) from one of the registers:
    _register |= _dut->tbm.at(tbmid).dacs.begin()->first&0xF0;
    
    // Update the DUT register Value:
    ret = _dut->tbm.at(tbmid).dacs.insert(std::make_pair(_register,regValue));
    if(ret.second == true) {
      LOG(logWARNING) << "Register \"" << regName << "\" (" << std::hex << static_cast<int>(_register) << std::dec << ") was not initialized. Created with value " << static_cast<int>(regValue);
    }
    else {
      _dut->tbm.at(tbmid).dacs[_register] = regValue;
      LOG(logDEBUGAPI) << "Register \"" << regName << "\" (" << std::hex << static_cast<int>(_register) << std::dec << ") updated with value " << static_cast<int>(regValue);
    }
    
    _hal->tbmSetReg(_register,regValue);
  }
  else {
    LOG(logERROR) << "TBM " << tbmid << " is not existing in the DUT!";
    return false;
  }
  return true;
}

bool pxarCore::setTbmReg(std::string regName, uint8_t regValue) {

  for(size_t tbms = 0; tbms < _dut->tbm.size(); ++tbms) {
    if(!setTbmReg(regName, regValue, tbms)) return false;
  }
  return true;
}

std::vector< std::pair<uint8_t, std::vector<pixel> > > pxarCore::getPulseheightVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, uint16_t flags, uint16_t nTriggers) {

  // No step size provided - scanning all DACs with step size 1:
  return getPulseheightVsDAC(dacName, 1, dacMin, dacMax, flags, nTriggers);
}

std::vector< std::pair<uint8_t, std::vector<pixel> > > pxarCore::getPulseheightVsDAC(std::string dacName, uint8_t dacStep, uint8_t dacMin, uint8_t dacMax, uint16_t flags, uint16_t nTriggers) {

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
  HalMemFnPixelSerial   pixelfn      = &hal::SingleRocOnePixelDacScan;
  HalMemFnPixelParallel multipixelfn = &hal::MultiRocOnePixelDacScan;
  HalMemFnRocSerial     rocfn        = &hal::SingleRocAllPixelsDacScan;
  HalMemFnRocParallel   multirocfn   = &hal::MultiRocAllPixelsDacScan;

  // We want the pulse height back from the Map function, no internal flag needed.

  // Load the test parameters into vector
  std::vector<int32_t> param;
  param.push_back(static_cast<int32_t>(dacRegister));
  param.push_back(static_cast<int32_t>(dacMin));
  param.push_back(static_cast<int32_t>(dacMax));
  param.push_back(static_cast<int32_t>(flags));
  param.push_back(static_cast<int32_t>(nTriggers));
  param.push_back(static_cast<int32_t>(dacStep));

  // check if the flags indicate that the user explicitly asks for serial execution of test:
  std::vector<Event*> data = expandLoop(pixelfn, multipixelfn, rocfn, multirocfn, param, flags);
  // repack data into the expected return format
  std::vector< std::pair<uint8_t, std::vector<pixel> > > result = repackDacScanData(data,dacStep,dacMin,dacMax,nTriggers,flags,false);

  // Reset the original value for the scanned DAC:
  std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();
  for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
    uint8_t oldDacValue = _dut->getDAC(static_cast<size_t>(rocit - enabledRocs.begin()),dacName);
    LOG(logDEBUGAPI) << "Reset DAC \"" << dacName << "\" to original value " << static_cast<int>(oldDacValue);
    _hal->rocSetDAC(static_cast<uint8_t>(rocit - enabledRocs.begin()),dacRegister,oldDacValue);
  }

  return result;
}

std::vector< std::pair<uint8_t, std::vector<pixel> > > pxarCore::getEfficiencyVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, uint16_t flags, uint16_t nTriggers) {

  // No step size provided - scanning all DACs with step size 1:
  return getEfficiencyVsDAC(dacName, 1, dacMin, dacMax, flags, nTriggers);
}

std::vector< std::pair<uint8_t, std::vector<pixel> > > pxarCore::getEfficiencyVsDAC(std::string dacName, uint8_t dacStep, uint8_t dacMin, uint8_t dacMax, uint16_t flags, uint16_t nTriggers) {

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
  HalMemFnPixelSerial   pixelfn      = &hal::SingleRocOnePixelDacScan;
  HalMemFnPixelParallel multipixelfn = &hal::MultiRocOnePixelDacScan;
  HalMemFnRocSerial     rocfn        = &hal::SingleRocAllPixelsDacScan;
  HalMemFnRocParallel   multirocfn   = &hal::MultiRocAllPixelsDacScan;

  // Load the test parameters into vector
  std::vector<int32_t> param;
  param.push_back(static_cast<int32_t>(dacRegister));  
  param.push_back(static_cast<int32_t>(dacMin));
  param.push_back(static_cast<int32_t>(dacMax));
  param.push_back(static_cast<int32_t>(flags));
  param.push_back(static_cast<int32_t>(nTriggers));
  param.push_back(static_cast<int32_t>(dacStep));

  // check if the flags indicate that the user explicitly asks for serial execution of test:
  std::vector<Event*> data = expandLoop(pixelfn, multipixelfn, rocfn, multirocfn, param, flags);
  // repack data into the expected return format
  std::vector< std::pair<uint8_t, std::vector<pixel> > > result = repackDacScanData(data,dacStep,dacMin,dacMax,nTriggers,flags,true);

  // Reset the original value for the scanned DAC:
  std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();
  for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
    uint8_t oldDacValue = _dut->getDAC(static_cast<size_t>(rocit - enabledRocs.begin()),dacName);
    LOG(logDEBUGAPI) << "Reset DAC \"" << dacName << "\" to original value " << static_cast<int>(oldDacValue);
    _hal->rocSetDAC(static_cast<uint8_t>(rocit - enabledRocs.begin()),dacRegister,oldDacValue);
  }

  return result;
}

std::vector< std::pair<uint8_t, std::vector<pixel> > > pxarCore::getThresholdVsDAC(std::string dacName, std::string dac2name, uint8_t dac2min, uint8_t dac2max, uint16_t flags, uint16_t nTriggers) {
  // Get the full DAC range for scanning:
  uint8_t dac1min = 0;
  uint8_t dac1max = getDACRange(dacName);
  uint8_t dacStep = 1;
  return getThresholdVsDAC(dacName, dacStep, dac1min, dac1max, dac2name, dacStep, dac2min, dac2max, flags, nTriggers);
}

std::vector< std::pair<uint8_t, std::vector<pixel> > > pxarCore::getThresholdVsDAC(std::string dac1name, uint8_t dac1step, uint8_t dac1min, uint8_t dac1max, std::string dac2name, uint8_t dac2step, uint8_t dac2min, uint8_t dac2max, uint16_t flags, uint16_t nTriggers) {
  // No threshold level provided - set threshold to 50%:
  uint8_t threshold = 50;
  return getThresholdVsDAC(dac1name, dac1step, dac1min, dac1max, dac2name, dac2step, dac2min, dac2max, threshold, flags, nTriggers);
}

std::vector< std::pair<uint8_t, std::vector<pixel> > > pxarCore::getThresholdVsDAC(std::string dac1name, uint8_t dac1step, uint8_t dac1min, uint8_t dac1max, std::string dac2name, uint8_t dac2step, uint8_t dac2min, uint8_t dac2max, uint8_t threshold, uint16_t flags, uint16_t nTriggers) {

  if(!status()) {return std::vector< std::pair<uint8_t, std::vector<pixel> > >();}

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
    return std::vector< std::pair<uint8_t, std::vector<pixel> > >();
  }
  if(!verifyRegister(dac2name, dac2register, dac2max, ROC_REG)) {
    return std::vector< std::pair<uint8_t, std::vector<pixel> > >();
  }

  // Check the threshold percentage level provided:
  if(threshold == 0 || threshold > 100) {
    LOG(logCRITICAL) << "Threshold level of " << static_cast<int>(threshold) << "% is not possible!";
    return std::vector< std::pair<uint8_t, std::vector<pixel> > >();
  }

  // Setup the correct _hal calls for this test
  HalMemFnPixelSerial   pixelfn      = &hal::SingleRocOnePixelDacDacScan;
  HalMemFnPixelParallel multipixelfn = &hal::MultiRocOnePixelDacDacScan;
  // In Principle these functions exist, but they would take years to run and fill up the buffer
  HalMemFnRocSerial     rocfn        = NULL; // &hal::SingleRocAllPixelsDacDacScan;
  HalMemFnRocParallel   multirocfn   = NULL; // &hal::MultiRocAllPixelsDacDacScan;

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
  param.push_back(static_cast<int32_t>(dac1step));
  param.push_back(static_cast<int32_t>(dac2step));

  // check if the flags indicate that the user explicitly asks for serial execution of test:
  std::vector<Event*> data = expandLoop(pixelfn, multipixelfn, rocfn, multirocfn, param, flags);
  // repack data into the expected return format
  std::vector< std::pair<uint8_t, std::vector<pixel> > > result = repackThresholdDacScanData(data,dac1step,dac1min,dac1max,dac2step,dac2min,dac2max,threshold,nTriggers,flags);

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

  return result;
}


std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > pxarCore::getPulseheightVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, std::string dac2name, uint8_t dac2min, uint8_t dac2max, uint16_t flags, uint16_t nTriggers) {

  // No step size provided - scanning all DACs with step size 1:
  return getPulseheightVsDACDAC(dac1name, 1, dac1min, dac1max, dac2name, 1, dac2min, dac2max, flags, nTriggers);
}

std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > pxarCore::getPulseheightVsDACDAC(std::string dac1name, uint8_t dac1step, uint8_t dac1min, uint8_t dac1max, std::string dac2name, uint8_t dac2step, uint8_t dac2min, uint8_t dac2max, uint16_t flags, uint16_t nTriggers) {

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
  HalMemFnPixelSerial   pixelfn      = &hal::SingleRocOnePixelDacDacScan;
  HalMemFnPixelParallel multipixelfn = &hal::MultiRocOnePixelDacDacScan;
  HalMemFnRocSerial     rocfn        = &hal::SingleRocAllPixelsDacDacScan;
  HalMemFnRocParallel   multirocfn   = &hal::MultiRocAllPixelsDacDacScan;

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
  param.push_back(static_cast<int32_t>(dac1step));
  param.push_back(static_cast<int32_t>(dac2step));

  // check if the flags indicate that the user explicitly asks for serial execution of test:
  std::vector<Event*> data = expandLoop(pixelfn, multipixelfn, rocfn, multirocfn, param, flags);
  // repack data into the expected return format
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > result = repackDacDacScanData(data,dac1step,dac1min,dac1max,dac2step,dac2min,dac2max,nTriggers,flags,false);

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

  return result;
}

std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > pxarCore::getEfficiencyVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, std::string dac2name, uint8_t dac2min, uint8_t dac2max, uint16_t flags, uint16_t nTriggers) {

  // No step size provided - scanning all DACs with step size 1:
  return getEfficiencyVsDACDAC(dac1name, 1, dac1min, dac1max, dac2name, 1, dac2min, dac2max, flags, nTriggers);
}

std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > pxarCore::getEfficiencyVsDACDAC(std::string dac1name, uint8_t dac1step, uint8_t dac1min, uint8_t dac1max, std::string dac2name, uint8_t dac2step, uint8_t dac2min, uint8_t dac2max, uint16_t flags, uint16_t nTriggers) {

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
  HalMemFnPixelSerial   pixelfn      = &hal::SingleRocOnePixelDacDacScan;
  HalMemFnPixelParallel multipixelfn = &hal::MultiRocOnePixelDacDacScan;
  HalMemFnRocSerial     rocfn        = &hal::SingleRocAllPixelsDacDacScan;
  HalMemFnRocParallel   multirocfn   = &hal::MultiRocAllPixelsDacDacScan;

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
  param.push_back(static_cast<int32_t>(dac1step));
  param.push_back(static_cast<int32_t>(dac2step));

  // check if the flags indicate that the user explicitly asks for serial execution of test:
  std::vector<Event*> data = expandLoop(pixelfn, multipixelfn, rocfn, multirocfn, param, flags);
  // repack data into the expected return format
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > result = repackDacDacScanData(data,dac1step,dac1min,dac1max,dac2step,dac2min,dac2max,nTriggers,flags,true);

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

  return result;
}

std::vector<pixel> pxarCore::getPulseheightMap(uint16_t flags, uint16_t nTriggers) {

  if(!status()) {return std::vector<pixel>();}

  // Setup the correct _hal calls for this test
  HalMemFnPixelSerial   pixelfn      = &hal::SingleRocOnePixelCalibrate;
  HalMemFnPixelParallel multipixelfn = &hal::MultiRocOnePixelCalibrate;
  HalMemFnRocSerial     rocfn        = &hal::SingleRocAllPixelsCalibrate;
  HalMemFnRocParallel   multirocfn   = &hal::MultiRocAllPixelsCalibrate;

  // Load the test parameters into vector
  std::vector<int32_t> param;
  param.push_back(static_cast<int32_t>(flags));
  param.push_back(static_cast<int32_t>(nTriggers));

  // check if the flags indicate that the user explicitly asks for serial execution of test:
  std::vector<Event*> data = expandLoop(pixelfn, multipixelfn, rocfn, multirocfn, param, flags);

  // Repacking of all data segments into one long map vector:
  std::vector<pixel> result = repackMapData(data, nTriggers, flags,false);

  return result;
}

std::vector<pixel> pxarCore::getEfficiencyMap(uint16_t flags, uint16_t nTriggers) {

  if(!status()) {return std::vector<pixel>();}

  // Setup the correct _hal calls for this test
  HalMemFnPixelSerial   pixelfn      = &hal::SingleRocOnePixelCalibrate;
  HalMemFnPixelParallel multipixelfn = &hal::MultiRocOnePixelCalibrate;
  HalMemFnRocSerial     rocfn        = &hal::SingleRocAllPixelsCalibrate;
  HalMemFnRocParallel   multirocfn   = &hal::MultiRocAllPixelsCalibrate;

  // Load the test parameters into vector
  std::vector<int32_t> param;
  param.push_back(static_cast<int32_t>(flags));
  param.push_back(static_cast<int32_t>(nTriggers));

  // check if the flags indicate that the user explicitly asks for serial execution of test:
  std::vector<Event*> data = expandLoop(pixelfn, multipixelfn, rocfn, multirocfn, param, flags);

  // Repacking of all data segments into one long map vector:
  std::vector<pixel> result = repackMapData(data, nTriggers, flags, true);

  return result;
}

std::vector<pixel> pxarCore::getThresholdMap(std::string dacName, uint16_t flags, uint16_t nTriggers) {
  // Get the full DAC range for scanning:
  uint8_t dacMin = 0;
  uint8_t dacMax = getDACRange(dacName);
  uint8_t dacStep = 1;
  return getThresholdMap(dacName, dacStep, dacMin, dacMax, flags, nTriggers);
}

std::vector<pixel> pxarCore::getThresholdMap(std::string dacName, uint8_t dacStep, uint8_t dacMin, uint8_t dacMax, uint16_t flags, uint16_t nTriggers) {
  // No threshold level provided - set threshold to 50%:
  uint8_t threshold = 50;
  return getThresholdMap(dacName, dacStep, dacMin, dacMax, threshold, flags, nTriggers);
}

std::vector<pixel> pxarCore::getThresholdMap(std::string dacName, uint8_t dacStep, uint8_t dacMin, uint8_t dacMax, uint8_t threshold, uint16_t flags, uint16_t nTriggers) {

  if(!status()) {return std::vector<pixel>();}

  // Scan the maximum DAC range for threshold:
  uint8_t dacRegister;
  if(!verifyRegister(dacName, dacRegister, dacMax, ROC_REG)) {
    return std::vector<pixel>();
  }

  // Check the threshold percentage level provided:
  if(threshold == 0 || threshold > 100) {
    LOG(logCRITICAL) << "Threshold level of " << static_cast<int>(threshold) << "% is not possible!";
    return std::vector<pixel>();
  }

  // Setup the correct _hal calls for this test, a threshold map is a 1D dac scan:
  HalMemFnPixelSerial   pixelfn      = &hal::SingleRocOnePixelDacScan;
  HalMemFnPixelParallel multipixelfn = &hal::MultiRocOnePixelDacScan;
  HalMemFnRocSerial     rocfn        = &hal::SingleRocAllPixelsDacScan;
  HalMemFnRocParallel   multirocfn   = &hal::MultiRocAllPixelsDacScan;

  // Load the test parameters into vector
  std::vector<int32_t> param;
  param.push_back(static_cast<int32_t>(dacRegister));  
  param.push_back(static_cast<int32_t>(dacMin));
  param.push_back(static_cast<int32_t>(dacMax));
  param.push_back(static_cast<int32_t>(flags));
  param.push_back(static_cast<int32_t>(nTriggers));
  param.push_back(static_cast<int32_t>(dacStep));

  // check if the flags indicate that the user explicitly asks for serial execution of test:
  std::vector<Event*> data = expandLoop(pixelfn, multipixelfn, rocfn, multirocfn, param, flags);

  // Repacking of all data segments into one long map vector:
  std::vector<pixel> result = repackThresholdMapData(data, dacStep, dacMin, dacMax, threshold, nTriggers, flags);

  return result;
}

std::vector<std::vector<uint16_t> > pxarCore::daqGetReadback() {

  std::vector<std::vector<uint16_t> > values;
  if(!status()) { return values; }

  values = _hal->daqReadback();
  LOG(logDEBUGAPI) << "Decoders provided readback values for " << values.size() << " ROCs.";
  return values;
}


// DAQ functions

bool pxarCore::daqStart() {
  return daqStart(_daq_buffersize,true);
}

bool pxarCore::daqStart(const int buffersize, const bool init) {

  if(!status()) {return false;}
  if(daqStatus()) {return false;}

  // Clearing previously initialized DAQ sessions:
  _hal->daqClear();

  // Check requested buffer size:
  if(buffersize > DTB_SOURCE_BUFFER_SIZE) {
    LOG(logWARNING) << "Requested buffer size too large, setting to max. " \
		    << DTB_SOURCE_BUFFER_SIZE;
    _daq_buffersize = DTB_SOURCE_BUFFER_SIZE;
  }
  else { _daq_buffersize = buffersize; }


  LOG(logDEBUGAPI) << "Starting new DAQ session...";
  
  // Check if we want to program the DUT or just leave it:
  if(init) {
    // Setup the configured mask and trim state of the DUT:
    MaskAndTrim(true);

    // Set Calibrate bits in the PUCs (we use the testrange for that):
    SetCalibrateBits(true);

    // Attaching all columns to the readout:
    for (std::vector<rocConfig>::iterator rocit = _dut->roc.begin(); rocit != _dut->roc.end(); ++rocit) {
      _hal->AllColumnsSetEnable(rocit->i2c_address,true);
    }
  }
  else if(!_daq_startstop_warning){
    LOG(logWARNING) << "Not unmasking DUT, not setting Calibrate bits!"; 
    _daq_startstop_warning = true;
  }

  // Check the DUT if we have TBMs enabled or not and choose the right deserializer:
  uint8_t type = 0x0;
  if(!_dut->tbm.empty()) { type = _dut->tbm.at(0).type; }

  // And start the DAQ session:
  _hal->daqStart(_dut->sig_delays[SIG_DESER160PHASE],type,buffersize);

  _daq_running = true;
  return true;
}

bool pxarCore::daqStatus()
{

  uint8_t perFull;

  return daqStatus(perFull);

}

bool pxarCore::daqStatus(uint8_t & perFull) {

  // Check if a DAQ session is running:
  if(!_daq_running) {
    LOG(logDEBUGAPI) << "DAQ not running!";
    return false;
  }

  // Check if we still have enough buffer memory left (with some safety margin).
  // Only filling buffer up to 90% in order not to lose data.
  uint32_t filled_buffer = _hal->daqBufferStatus();
  perFull = static_cast<uint8_t>(static_cast<float>(filled_buffer)/_daq_buffersize*100.0);
  if(filled_buffer > 0.9*_daq_buffersize) {
    LOG(logWARNING) << "DAQ buffer about to overflow!";
    return false;
  }

  LOG(logDEBUGAPI) << "Everything alright, buffer size " << filled_buffer
		   << "/" << _daq_buffersize;
  return true;
}

uint16_t pxarCore::daqTrigger(uint32_t nTrig, uint16_t period) {

  if(!daqStatus()) { return 0; }
  // Pattern Generator loop doesn't work for delay periods smaller than
  // the pattern generator duration, so limit it to that:
  if(period < _dut->pg_sum) {
    period = _dut->pg_sum;
    LOG(logWARNING) << "Loop period setting too small for configured "
		    << "Pattern generator. "
		    << "Forcing loop delay to " << period << " clk";
    LOG(logWARNING) << "To suppress this warning supply a larger delay setting";
  }
  // Just passing the call to the HAL, not doing anything else here:
  _hal->daqTrigger(nTrig,period);
  return period;
}

uint16_t pxarCore::daqTriggerLoop(uint16_t period) {

  if(!daqStatus()) { return 0; }

  // Pattern Generator loop doesn't work for delay periods smaller than
  // the pattern generator duration, so limit it to that:
  if(period < _dut->pg_sum) {
    period = _dut->pg_sum;
    LOG(logWARNING) << "Loop period setting too small for configured "
		    << "Pattern generator. "
		    << "Forcing loop delay to " << period << " clk";
    LOG(logWARNING) << "To suppress this warning supply a larger delay setting";
  }
  _hal->daqTriggerLoop(period);
  LOG(logDEBUGAPI) << "Loop period set to " << period << " clk";
  return period;
}

void pxarCore::daqTriggerLoopHalt() {

  // Just halt the pattern generator loop:
  _hal->daqTriggerLoopHalt();
}

std::vector<uint16_t> pxarCore::daqGetBuffer() {

  // Reading out all data from the DTB and returning the raw blob.
  std::vector<uint16_t> buffer = _hal->daqBuffer();
  return buffer;
}

std::vector<rawEvent> pxarCore::daqGetRawEventBuffer() {

  // Reading out all data from the DTB and returning the raw blob.
  // Select the right readout channels depending on the number of TBMs
  std::vector<rawEvent> data = std::vector<rawEvent>();
  std::vector<rawEvent*> buffer = _hal->daqAllRawEvents();

  // Dereference all vector entries and give data back:
  for(std::vector<rawEvent*>::iterator it = buffer.begin(); it != buffer.end(); ++it) {
    data.push_back(**it);
  }
  return data;
}

std::vector<Event> pxarCore::daqGetEventBuffer() {

  // Reading out all data from the DTB and returning the decoded Event buffer.
  // Select the right readout channels depending on the number of TBMs
  std::vector<Event> data = std::vector<Event>();
  std::vector<Event*> buffer = _hal->daqAllEvents();

  // check the data for decoder errors and update our internal counter
  getDecoderErrorCount();

  // Dereference all vector entries and give data back:
  for(std::vector<Event*>::iterator it = buffer.begin(); it != buffer.end(); ++it) {
    data.push_back(**it);
  }
  return data;
}

Event pxarCore::daqGetEvent() {

  // Check DAQ status:
  if(!daqStatus()) { return Event(); }

  // Return the next decoded Event from the FIFO buffer:
  return (*_hal->daqEvent());
}

rawEvent pxarCore::daqGetRawEvent() {

  // Check DAQ status:
  if(!daqStatus()) { return rawEvent(); }

  // Return the next raw data record from the FIFO buffer:
  return (*_hal->daqRawEvent());
}

uint32_t pxarCore::daqGetNDecoderErrors() {

  // Return the accumulated number of decoding errors:
  return _ndecode_errors_lastdaq;
}


bool pxarCore::daqStop() {
  return daqStop(true);
}

bool pxarCore::daqStop(const bool init) {

  if(!status()) {return false;}
  if(!_daq_running) {
    LOG(logINFO) << "No DAQ running, not executing daqStop command.";
    return false;
  }

  _daq_running = false;
  
  // Stop all active DAQ channels:
  _hal->daqStop();

  // If the init flag is set, mask and clear the DUT again:
  if(init) {
    // Mask all pixels in the device again:
    MaskAndTrim(false);

    // Reset all the Calibrate bits and signals:
    SetCalibrateBits(false);

    // Detaching all columns to the readout:
    for (std::vector<rocConfig>::iterator rocit = _dut->roc.begin(); rocit != _dut->roc.end(); ++rocit) {
      _hal->AllColumnsSetEnable(rocit->i2c_address,false);
    }
  }
  else if(!_daq_startstop_warning){
    LOG(logWARNING) << "Not unmasking DUT, not setting Calibrate bits!"; 
    _daq_startstop_warning = true;
  }

  return true;
}


std::vector<Event*> pxarCore::expandLoop(HalMemFnPixelSerial pixelfn, HalMemFnPixelParallel multipixelfn, HalMemFnRocSerial rocfn, HalMemFnRocParallel multirocfn, std::vector<int32_t> param, uint16_t flags) {
  
  // pointer to vector to hold our data
  std::vector<Event*> data = std::vector<Event*>();

  // Start test timer:
  timer t;

  // Check if all pixels are configured the same way on all ROCs. If this is not the case, we need to run this in FLAG_FORCE_SERIAL mode:
  std::vector<uint8_t> enabledRocs = _dut->getEnabledRocIDs();
  for(std::vector<uint8_t>::iterator rc = enabledRocs.begin(); rc != enabledRocs.end(); ++rc) {
    // Compare the configuration of the first ROC with all others:
    if(!comparePixelConfiguration(_dut->getEnabledPixels(enabledRocs.at(0)),_dut->getEnabledPixels(*rc))) {
      flags |= FLAG_FORCE_SERIAL;
      LOG(logDEBUGAPI) << "Not all ROCs have their pixels configured the same way. "
		       << "Running in FLAG_FORCE_SERIAL mode.";
      break;
    }
  }

  // Do the masking/unmasking&trimming for all ROCs first.
  // Unless we are running in FLAG_FORCE_UNMASKED mode, we need to transmit the new trim values to the NIOS core and mask the whole DUT:
  if((flags & FLAG_FORCE_UNMASKED) == 0) {
    MaskAndTrimNIOS();
    MaskAndTrim(false);
  }
  // If we run in FLAG_FORCE_SERIAL mode, mask the whole DUT:
  else if((flags & FLAG_FORCE_SERIAL) != 0) { MaskAndTrim(false); }
  // Else just trim all the pixels:
  else { MaskAndTrim(true); }

  // Check if we might use parallel routine on whole module: more than one ROC
  // must be enabled and parallel execution not disabled by user
  if ((_dut->getNEnabledRocs() > 1) && ((flags & FLAG_FORCE_SERIAL) == 0)) {

    // Get the I2C addresses for all enabled ROCs from the config:
    std::vector<uint8_t> rocs_i2c = _dut->getEnabledRocI2Caddr();

    // Check if all pixels are enabled:
    if (_dut->getAllPixelEnable() && multirocfn != NULL) {
      LOG(logDEBUGAPI) << "\"The Loop\" contains one call to \'multirocfn\'";
      
      // execute call to HAL layer routine
      data = CALL_MEMBER_FN(*_hal,multirocfn)(rocs_i2c, param);
    } // ROCs parallel
    // Otherwise call the Pixel Parallel function several times:
    else if (multipixelfn != NULL) {
      
      // Get one of the enabled ROCs:
      std::vector<uint8_t> enabledRocs = _dut->getEnabledRocIDs();
      std::vector<Event*> rocdata = std::vector<Event*>();
      std::vector<pixelConfig> enabledPixels = _dut->getEnabledPixels(enabledRocs.front());

      LOG(logDEBUGAPI) << "\"The Loop\" contains "
		       << enabledPixels.size() << " calls to \'multipixelfn\'";

      for (std::vector<pixelConfig>::iterator px = enabledPixels.begin(); px != enabledPixels.end(); ++px) {
	// execute call to HAL layer routine and store data in buffer
	std::vector<Event*> buffer = CALL_MEMBER_FN(*_hal,multipixelfn)(rocs_i2c, px->column(), px->row(), param);

	// merge pixel data into roc data storage vector
	if (rocdata.empty()){
	  rocdata = buffer; // for first time call
	} else {
	  // Add buffer vector to the end of existing Event data:
	  rocdata.reserve(rocdata.size() + buffer.size());
	  rocdata.insert(rocdata.end(), buffer.begin(), buffer.end());
	}
      } // pixel loop
	// append rocdata to main data storage vector
      if (data.empty()) data = rocdata;
      else {
	data.reserve(data.size() + rocdata.size());
	data.insert(data.end(), rocdata.begin(), rocdata.end());
      }
    } // Pixels parallel
  } // Parallel functions

  // Either we only have one ROC enabled or we force serial test execution:
  else {

    // -> single ROC / ROC-by-ROC operation
    // check if all pixels are enabled
    // if so, use routine that accesses whole ROC
    if (_dut->getAllPixelEnable() && rocfn != NULL){

      // loop over all enabled ROCs
      std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();

      LOG(logDEBUGAPI) << "\"The Loop\" contains " << enabledRocs.size() << " calls to \'rocfn\'";

      for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit) {

	// If we have serial execution make sure to trim the ROC if we requested forceUnmasked:
	if(((flags & FLAG_FORCE_SERIAL) != 0) && ((flags & FLAG_FORCE_UNMASKED) != 0)) { MaskAndTrim(true,rocit); }

	// execute call to HAL layer routine and save returned data in buffer
	std::vector<Event*> rocdata = CALL_MEMBER_FN(*_hal,rocfn)(rocit->i2c_address, param);
	// append rocdata to main data storage vector
        if (data.empty()) data = rocdata;
	else {
	  data.reserve(data.size() + rocdata.size());
	  data.insert(data.end(), rocdata.begin(), rocdata.end());
	}
      } // roc loop
    }
    else if (pixelfn != NULL) {

      // -> we operate on single pixels
      // loop over all enabled ROCs
      std::vector<rocConfig> enabledRocs = _dut->getEnabledRocs();

      LOG(logDEBUGAPI) << "\"The Loop\" contains " << enabledRocs.size() << " enabled ROCs.";

      for (std::vector<rocConfig>::iterator rocit = enabledRocs.begin(); rocit != enabledRocs.end(); ++rocit){
	std::vector<Event*> rocdata = std::vector<Event*>();
	std::vector<pixelConfig> enabledPixels = _dut->getEnabledPixels(static_cast<uint8_t>(rocit - enabledRocs.begin()));


	LOG(logDEBUGAPI) << "\"The Loop\" for the current ROC contains " \
			 << enabledPixels.size() << " calls to \'pixelfn\'";

	for (std::vector<pixelConfig>::iterator pixit = enabledPixels.begin(); pixit != enabledPixels.end(); ++pixit) {
	  // execute call to HAL layer routine and store data in buffer
	  std::vector<Event*> buffer = CALL_MEMBER_FN(*_hal,pixelfn)(rocit->i2c_address, pixit->column(), pixit->row(), param);
	  // merge pixel data into roc data storage vector
	  if (rocdata.empty()){
	    rocdata = buffer; // for first time call
	  } else {
	    // Add buffer vector to the end of existing Event data:
	    rocdata.reserve(rocdata.size() + buffer.size());
	    rocdata.insert(rocdata.end(), buffer.begin(), buffer.end());
	  }
	} // pixel loop
	// append rocdata to main data storage vector
        if (data.empty()) data = rocdata;
	else {
	  data.reserve(data.size() + rocdata.size());
	  data.insert(data.end(), rocdata.begin(), rocdata.end());
	}
      } // roc loop
    }// single pixel fnc
    else {
      LOG(logCRITICAL) << "LOOP EXPANSION FAILED -- NO MATCHING FUNCTION TO CALL?!";
      // do NOT throw an exception here: this is not a runtime problem
      // but can only be a bug in the code -> this could not be handled by unwinding the stack
      return data;
    }
  } // single roc fnc

  // check that we ended up with data
  if (data.empty()){
    LOG(logCRITICAL) << "NO DATA FROM TEST FUNCTION -- are any TBMs/ROCs/PIXs enabled?!";
    return data;
  }
  
  // update the internal decoder error count for this data sample
  getDecoderErrorCount();

  // Test is over, mask the whole device again and clear leftover calibrate signals:
  MaskAndTrim(false);
  SetCalibrateBits(false);

  // Print timer value:
  LOG(logINFO) << "Test took " << t << "ms.";

  return data;
} // expandLoop()


std::vector<Event*> pxarCore::condenseTriggers(std::vector<Event*> data, uint16_t nTriggers, bool efficiency) {

  std::vector<Event*> packed;

  if(data.size()%nTriggers != 0) {
    LOG(logCRITICAL) << "Data size does not correspond to " << nTriggers << " triggers! Aborting data processing!";
    return packed;
  }

  for(std::vector<Event*>::iterator Eventit = data.begin(); Eventit!= data.end(); Eventit += nTriggers) {

    Event * evt = new Event();
    std::map<pixel,uint16_t> pxcount = std::map<pixel,uint16_t>();
    std::map<pixel,double> pxmean = std::map<pixel,double>();
    std::map<pixel,double> pxm2 = std::map<pixel,double>();

    for(std::vector<Event*>::iterator it = Eventit; it != Eventit+nTriggers; ++it) {

      // Loop over all contained pixels:
      for(std::vector<pixel>::iterator pixit = (*it)->pixels.begin(); pixit != (*it)->pixels.end(); ++pixit) {

	// Check if we have that particular pixel already in:
	std::vector<pixel>::iterator px = std::find_if(evt->pixels.begin(),
						       evt->pixels.end(),
						       findPixelXY(pixit->column(), pixit->row(), pixit->roc()));
	// Pixel is known:
	if(px != evt->pixels.end()) {
	  if(efficiency) { px->setValue(px->value()+1); }
	  else {
	    // Calculate the variance incrementally:
	    double delta = pixit->value() - pxmean[*px];
	    pxmean[*px] += delta/pxcount[*px];
	    pxm2[*px] += delta*(pixit->value() - pxmean[*px]);
	    pxcount[*px]++;
	  }
	}
	// Pixel is new:
	else {
	  if(efficiency) { pixit->setValue(1); }
	  else { 
	    // Initialize counters and temporary variables:
	    pxcount.insert(std::make_pair(*pixit,1));
	    pxmean.insert(std::make_pair(*pixit,pixit->value()));
	    pxm2.insert(std::make_pair(*pixit,0));
	  }
	  evt->pixels.push_back(*pixit);
	}
      }

      // Delete the original data, not needed anymore:
      delete *it;
    }

    // Calculate mean and variance for the pulse height depending on the
    // number of triggers received:
    if(!efficiency) {
      for(std::vector<pixel>::iterator px = evt->pixels.begin(); px != evt->pixels.end(); ++px) {
	px->setValue(pxmean[*px]); // The mean
	px->setVariance(pxm2[*px]/(pxcount[*px] - 1)); // The variance
      }
    }
    packed.push_back(evt);
  }

  return packed;
}

std::vector<pixel> pxarCore::repackMapData (std::vector<Event*> data, uint16_t nTriggers, uint16_t flags, bool efficiency) {

  // Keep track of the pixel to be expected:
  uint8_t expected_column = 0, expected_row = 0;

  std::vector<pixel> result;
  LOG(logDEBUGAPI) << "Simple Map Repack of " << data.size() << " data blocks, returning " << (efficiency ? "efficiency" : "averaged pulse height") << ".";

  // Measure time:
  timer t;

  // First reduce triggers, we have #nTriggers Events which belong together:
  std::vector<Event*> packed = condenseTriggers(data, nTriggers, efficiency);

  // Loop over all Events we have:
  for(std::vector<Event*>::iterator Eventit = packed.begin(); Eventit!= packed.end(); ++Eventit) {

    // For every Event, loop over all contained pixels:
    for(std::vector<pixel>::iterator pixit = (*Eventit)->pixels.begin(); pixit != (*Eventit)->pixels.end(); ++pixit) {
      // Check for pulsed pixels being present:
      if((flags&FLAG_CHECK_ORDER) != 0) {
	if(pixit->column() != expected_column || pixit->row() != expected_row) {

	  // With the full chip unmasked we want to know if the pixel in question was amongst the ones recorded:
	  if((flags&FLAG_FORCE_UNMASKED) != 0) { LOG(logDEBUGPIPES) << "This is a background hit: " << (*pixit); }
	  else {
	    // With only the pixel in question unmasked we want to warn about other appeareances:
	    LOG(logERROR) << "This pixel doesn't belong here: " << (*pixit) << ". Expected [" << static_cast<int>(expected_column) << "," << static_cast<int>(expected_row) << ",x]";
	  }

	  // Convention: set a negative pixel value for out-of-order pixel hits:
	  pixit->setValue(-1*pixit->value());
	}
      }
      result.push_back(*pixit);
    } // loop over pixels

    if((flags&FLAG_CHECK_ORDER) != 0) {
      expected_row++;
      if(expected_row >= ROC_NUMROWS) { expected_row = 0; expected_column++; }
      if(expected_column >= ROC_NUMCOLS) { expected_row = 0; expected_column = 0; }
    }
  } // loop over Events

  // Sort the output map by ROC->col->row - just because we are so nice:
  if((flags&FLAG_NOSORT) == 0) { std::sort(result.begin(),result.end()); }

  // Cleanup temporary data:
  for(std::vector<Event*>::iterator it = packed.begin(); it != packed.end(); ++it) { delete *it; }

  LOG(logDEBUGAPI) << "Correctly repacked Map data for delivery.";
  LOG(logDEBUGAPI) << "Repacking took " << t << "ms.";
  return result;
}

std::vector< std::pair<uint8_t, std::vector<pixel> > > pxarCore::repackDacScanData (std::vector<Event*> data, uint8_t dacStep, uint8_t dacMin, uint8_t dacMax, uint16_t nTriggers, uint16_t /*flags*/, bool efficiency){

  std::vector< std::pair<uint8_t, std::vector<pixel> > > result;

  // Measure time:
  timer t;

  // First reduce triggers, we have #nTriggers Events which belong together:
  std::vector<Event*> packed = condenseTriggers(data, nTriggers, efficiency);

  if(packed.size() % static_cast<size_t>((dacMax-dacMin)/dacStep+1) != 0) {
    LOG(logCRITICAL) << "Data size not as expected! " << packed.size() << " data blocks do not fit to " << static_cast<int>((dacMax-dacMin)/dacStep+1) << " DAC values!";
    return result;
  }

  LOG(logDEBUGAPI) << "Packing DAC range " << static_cast<int>(dacMin) << " - " << static_cast<int>(dacMax) << " (step size " << static_cast<int>(dacStep) << "), data has " << packed.size() << " entries.";

  // Prepare the result vector
  for(size_t dac = dacMin; dac <= dacMax; dac += dacStep) { result.push_back(std::make_pair(dac,std::vector<pixel>())); }

  size_t currentDAC = dacMin;
  // Loop over the packed data and separate into DAC ranges, potentially several rounds:
  for(std::vector<Event*>::iterator Eventit = packed.begin(); Eventit!= packed.end(); ++Eventit) {
    if(currentDAC > dacMax) { currentDAC = dacMin; }
    result.at((currentDAC-dacMin)/dacStep).second.insert(result.at((currentDAC-dacMin)/dacStep).second.end(),
					       (*Eventit)->pixels.begin(),
					       (*Eventit)->pixels.end());
    currentDAC += dacStep;
  }
  
  // Cleanup temporary data:
  for(std::vector<Event*>::iterator it = packed.begin(); it != packed.end(); ++it) { delete *it; }

  LOG(logDEBUGAPI) << "Correctly repacked DacScan data for delivery.";
  LOG(logDEBUGAPI) << "Repacking took " << t << "ms.";
  return result;
}

std::vector<pixel> pxarCore::repackThresholdMapData (std::vector<Event*> data, uint8_t dacStep, uint8_t dacMin, uint8_t dacMax, uint8_t thresholdlevel, uint16_t nTriggers, uint16_t flags) {

  std::vector<pixel> result;
  // Vector of pixels for which a threshold has already been found
  std::vector<pixel> found;

  // Threshold is the the given efficiency level "thresholdlevel"
  // Using ceiling function to take higher threshold when in doubt.
  uint16_t threshold = static_cast<uint16_t>(ceil(static_cast<float>(nTriggers)*thresholdlevel/100));
  LOG(logDEBUGAPI) << "Scanning for threshold level " << threshold << ", " 
		   << ((flags&FLAG_RISING_EDGE) == 0 ? "falling":"rising") << " edge";

  // Measure time:
  timer t;

  // First, pack the data as it would be a regular Dac Scan:
  std::vector<std::pair<uint8_t,std::vector<pixel> > > packed_dac = repackDacScanData(data, dacStep, dacMin, dacMax, nTriggers, flags, true);

  // Efficiency map:
  std::map<pixel,uint8_t> oldvalue;

  // Then loop over all pixels and DAC settings, start from the back if we are looking for falling edge.
  // This ensures that we end up having the correct edge, even if the efficiency suddenly changes from 0 to max.
  std::vector<std::pair<uint8_t,std::vector<pixel> > >::iterator it_start;
  std::vector<std::pair<uint8_t,std::vector<pixel> > >::iterator it_end;
  int increase_op;
  if((flags&FLAG_RISING_EDGE) != 0) { it_start = packed_dac.begin(); it_end = packed_dac.end(); increase_op = 1; }
  else { it_start = packed_dac.end()-1; it_end = packed_dac.begin()-1; increase_op = -1;  }

  for(std::vector<std::pair<uint8_t,std::vector<pixel> > >::iterator it = it_start; it != it_end; it += increase_op) {
    // For every DAC value, loop over all pixels:
    for(std::vector<pixel>::iterator pixit = it->second.begin(); pixit != it->second.end(); ++pixit) {
      // Check if for this pixel a threshold has been found already and we can skip the rest:
      std::vector<pixel>::iterator px_found = std::find_if(found.begin(),
							   found.end(),
							   findPixelXY(pixit->column(), pixit->row(), pixit->roc()));
      if(px_found != found.end()) continue;

      // Check if we have that particular pixel already in the result vector:
      std::vector<pixel>::iterator px = std::find_if(result.begin(),
						     result.end(),
						     findPixelXY(pixit->column(), pixit->row(), pixit->roc()));
  
      // Pixel is known:
      if(px != result.end()) {
	// Calculate efficiency deltas and slope:
	uint8_t delta_old = abs(oldvalue[*px] - threshold);
	uint8_t delta_new = abs(pixit->value() - threshold);
	bool positive_slope = (pixit->value()-oldvalue[*px] > 0 ? true : false);

	// Check which value is closer to the threshold. Only if the slope is positive AND
	// the new delta between value and threshold is *larger* then the old delta, we 
	// found the threshold. If slope is negative, we just have a ripple in the DAC's 
	// distribution:
	if(positive_slope && !(delta_new < delta_old)) {        
	  found.push_back(*pixit);    
	  continue; 
	}

	// No threshold found yet, update the DAC threshold value for the pixel:
	px->setValue(it->first);
	// Update the oldvalue map:
	oldvalue[*px] = pixit->value();
      }
      // Pixel is new, just adding it:
      else {
        // If the pixel is above threshold at first appearance, the respective
	// DAC value is set as its threshold:
	if(pixit->value() >= threshold) { found.push_back(*pixit); }

	// Store the pixel with original efficiency
	oldvalue.insert(std::make_pair(*pixit,pixit->value()));

	// Push pixel to result vector with current DAC as value field:
	pixit->setValue(it->first);
	result.push_back(*pixit);
      }
    }
  }

  // Check for pixels that have not reached the threshold at all:
  for(std::vector<pixel>::iterator px = result.begin(); px != result.end(); ++px) {
    std::vector<pixel>::iterator px_found = std::find_if(found.begin(),
							 found.end(),
							 findPixelXY(px->column(), px->row(), px->roc()));
    // The pixel is in the "found" vector, which means it crossed threshold at some point:
    if(px_found != found.end()) continue;

    // The pixel is not in and never reached the threshold. We set the return value to
    // "dacMax" (rising edge) or "dacMin" (falling edge):
    if((flags&FLAG_RISING_EDGE) != 0) { px->setValue(dacMax); }
    else { px->setValue(dacMin); }
    LOG(logWARNING) << "No threshold found for " << (*px);
  }

  // Sort the output map by ROC->col->row - just because we are so nice:
  if((flags&FLAG_NOSORT) == 0) { std::sort(result.begin(),result.end()); }

  LOG(logDEBUGAPI) << "Correctly repacked&analyzed ThresholdMap data for delivery.";
  LOG(logDEBUGAPI) << "Repacking took " << t << "ms.";
  return result;
}

std::vector<std::pair<uint8_t,std::vector<pixel> > > pxarCore::repackThresholdDacScanData (std::vector<Event*> data, uint8_t dac1step, uint8_t dac1min, uint8_t dac1max, uint8_t dac2step, uint8_t dac2min, uint8_t dac2max, uint8_t thresholdlevel, uint16_t nTriggers, uint16_t flags) {

  std::vector<std::pair<uint8_t,std::vector<pixel> > > result;
  // Map of pixels with already assigned threshold (key is the dac2 value):
  std::map<uint8_t,std::vector<pixel> > found;

  // Threshold is the the given efficiency level "thresholdlevel":
  // Using ceiling function to take higher threshold when in doubt.
  uint16_t threshold = static_cast<uint16_t>(ceil(static_cast<float>(nTriggers)*thresholdlevel/100));
  LOG(logDEBUGAPI) << "Scanning for threshold level " << threshold << ", " 
		   << ((flags&FLAG_RISING_EDGE) == 0 ? "falling":"rising") << " edge";

  // Measure time:
  timer t;

  // First, pack the data as it would be a regular DacDac Scan:
  std::vector<std::pair<uint8_t,std::pair<uint8_t,std::vector<pixel> > > > packed_dacdac = repackDacDacScanData(data,dac1step,dac1min,dac1max,dac2step,dac2min,dac2max,nTriggers,flags,true);

  // Efficiency map:
  std::map<uint8_t,std::map<pixel,uint8_t> > oldvalue;  

  // Then loop over all pixels and DAC settings, start from the back if we are looking for falling edge.
  // This ensures that we end up having the correct edge, even if the efficiency suddenly changes from 0 to max.
  std::vector<std::pair<uint8_t,std::pair<uint8_t,std::vector<pixel> > > >::iterator it_start;
  std::vector<std::pair<uint8_t,std::pair<uint8_t,std::vector<pixel> > > >::iterator it_end;
  int increase_op;
  if((flags&FLAG_RISING_EDGE) != 0) { it_start = packed_dacdac.begin(); it_end = packed_dacdac.end(); increase_op = 1; }
  else { it_start = packed_dacdac.end()-1; it_end = packed_dacdac.begin()-1; increase_op = -1;  }

  for(std::vector<std::pair<uint8_t,std::pair<uint8_t,std::vector<pixel> > > >::iterator it = it_start; it != it_end; it += increase_op) {

    // For every DAC/DAC entry, loop over all pixels:
    for(std::vector<pixel>::iterator pixit = it->second.second.begin(); pixit != it->second.second.end(); ++pixit) {
      
      // Find the current DAC2 value in the result vector (simple replace for find_if):
      std::vector<std::pair<uint8_t, std::vector<pixel> > >::iterator dac;
      for(dac = result.begin(); dac != result.end(); ++dac) { if(it->second.first == dac->first) break; }

      // Didn't find the DAC2 value:
      if(dac == result.end()) {
	result.push_back(std::make_pair(it->second.first,std::vector<pixel>()));
	dac = result.end() - 1;
	// Also add an entry for bookkeeping:
	found.insert(std::make_pair(it->second.first,std::vector<pixel>()));
	oldvalue.insert(std::make_pair(it->second.first,std::map<pixel,uint8_t>()));
      }
      
      // Check if for this pixel a threshold has been found already and we can skip the rest:
      std::vector<pixel>::iterator px_found = std::find_if(found[dac->first].begin(),
							   found[dac->first].end(),
							   findPixelXY(pixit->column(), pixit->row(), pixit->roc()));
      if(px_found != found[dac->first].end()) continue;

      // Check if we have that particular pixel already in:
      std::vector<pixel>::iterator px = std::find_if(dac->second.begin(),
						     dac->second.end(),
						     findPixelXY(pixit->column(), pixit->row(), pixit->roc()));

      // Pixel is known:
      if(px != dac->second.end()) {
	// Calculate efficiency deltas and slope:
	uint8_t delta_old = abs(oldvalue[dac->first][*px] - threshold);
	uint8_t delta_new = abs(pixit->value() - threshold);
	bool positive_slope = (pixit->value() - oldvalue[dac->first][*px] > 0 ? true : false);

        // Check which value is closer to the threshold. Only if the slope is positive AND
	// the new delta between value and threshold is *larger* then the old delta, we 
	// found the threshold. If slope is negative, we just have a ripple in the DAC's 
	// distribution:
	if(positive_slope && !(delta_new < delta_old)) {
	  found[dac->first].push_back(*pixit);
	  continue;
	}

        // No threshold found yet, update the DAC threshold value for the pixel:
	px->setValue(it->first);
	// Update the oldvalue map:
	oldvalue[dac->first][*px] = pixit->value();
      }
      // Pixel is new, just adding it:
      else {
        // If the pixel is above threshold at first appearance, the respective
	// DAC value is set as its threshold:
	if(pixit->value() >= threshold) { found[dac->first].push_back(*pixit); }

	// Store the pixel with original efficiency
	oldvalue[dac->first].insert(std::make_pair(*pixit,pixit->value()));
	// Push pixel to result vector with current DAC as value field:
	pixit->setValue(it->first);
	dac->second.push_back(*pixit);
      }
    }
  }

  // Check for pixels that have not reached the threshold at all:
  for(std::vector<std::pair<uint8_t,std::vector<pixel> > >::iterator dac = result.begin(); dac != result.end(); ++dac) {
    
    for(std::vector<pixel>::iterator px = dac->second.begin(); px != dac->second.end(); px++) {
      std::vector<pixel>::iterator px_found = std::find_if(found[dac->first].begin(),
							   found[dac->first].end(),
							   findPixelXY(px->column(), px->row(), px->roc()));
      // The pixel is in the "found" vector, which means it crossed threshold at some point:
      if(px_found != found[dac->first].end()) continue;

      // The pixel is not in and never reached the threshold. We set the return value to
      // "dacMax" (rising edge) or "dacMin" (falling edge):
      if((flags&FLAG_RISING_EDGE) != 0) { px->setValue(dac2max); }
      else { px->setValue(dac2min); }
      LOG(logWARNING) << "No threshold found for " << (*px) << " at DAC value " << static_cast<int>(dac->first);
    }
  }

  // Sort the output map by DAC values and ROC->col->row - just because we are so nice:
  if((flags&FLAG_NOSORT) == 0) { std::sort(result.begin(),result.end()); }

  LOG(logDEBUGAPI) << "Correctly repacked&analyzed ThresholdDacScan data for delivery.";
  LOG(logDEBUGAPI) << "Repacking took " << t << "ms.";
  return result;
}

std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > pxarCore::repackDacDacScanData (std::vector<Event*> data, uint8_t dac1step, uint8_t dac1min, uint8_t dac1max, uint8_t dac2step, uint8_t dac2min, uint8_t dac2max, uint16_t nTriggers, uint16_t /*flags*/, bool efficiency) {
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > result;

  // Measure time:
  timer t;

  // First reduce triggers, we have #nTriggers Events which belong together:
  std::vector<Event*> packed = condenseTriggers(data, nTriggers, efficiency);

  if(packed.size() % static_cast<size_t>(((dac1max-dac1min)/dac1step+1)*((dac2max-dac2min)/dac2step+1)) != 0) {
    LOG(logCRITICAL) << "Data size not as expected! " << packed.size() << " data blocks do not fit to " << static_cast<int>(((dac1max-dac1min)/dac1step+1)*((dac2max-dac2min)/dac2step+1)) << " DAC values!";
    return result;
  }

  LOG(logDEBUGAPI) << "Packing DAC range [" << static_cast<int>(dac1min) << " - " << static_cast<int>(dac1max) 
		   << ", step size " << static_cast<int>(dac1step) << "]x[" 
		   << static_cast<int>(dac2min) << " - " << static_cast<int>(dac2max)
		   << ", step size " << static_cast<int>(dac2step)
		   << "], data has " << packed.size() << " entries.";

  // Prepare the result vector
  for(size_t dac1 = dac1min; dac1 <= dac1max; dac1 += dac1step) {
    std::pair<uint8_t,std::vector<pixel> > dacpair;
    for(size_t dac2 = dac2min; dac2 <= dac2max; dac2 += dac2step) {
      dacpair = std::make_pair(dac2,std::vector<pixel>());
      result.push_back(std::make_pair(dac1,dacpair));
    }
  }

  size_t current1dac = dac1min;
  size_t current2dac = dac2min;

  // Loop over the packed data and separeate into DAC ranges, potentially several rounds:
  int i = 0;
  for(std::vector<Event*>::iterator Eventit = packed.begin(); Eventit!= packed.end(); ++Eventit) {
    if(current2dac > dac2max) {
      current2dac = dac2min;
      current1dac += dac1step;
    }
    if(current1dac > dac1max) { current1dac = dac1min; }

    result.at((current1dac-dac1min)/dac1step*((dac2max-dac2min)/dac2step+1) + (current2dac-dac2min)/dac2step).second.second.insert(result.at((current1dac-dac1min)/dac1step*((dac2max-dac2min)/dac2step+1) + (current2dac-dac2min)/dac2step).second.second.end(),
												       (*Eventit)->pixels.begin(),
												       (*Eventit)->pixels.end());
    i++;
    current2dac += dac2step;
  }
  
  // Cleanup temporary data:
  for(std::vector<Event*>::iterator it = packed.begin(); it != packed.end(); ++it) { delete *it; }

  LOG(logDEBUGAPI) << "Correctly repacked DacDacScan data for delivery.";
  LOG(logDEBUGAPI) << "Repacking took " << t << "ms.";
  return result;
}

// Update mask and trim bits for the full DUT in NIOS structs:
void pxarCore::MaskAndTrimNIOS() {

  // First transmit all configured I2C addresses:
  _hal->SetupI2CValues(_dut->getRocI2Caddr());
  
  // Now run over all existing ROCs and transmit the pixel trim/mask data:
  for (std::vector<rocConfig>::iterator rocit = _dut->roc.begin(); rocit != _dut->roc.end(); ++rocit) {
    _hal->SetupTrimValues(rocit->i2c_address,rocit->pixels);
  }
}

// Mask/Unmask and trim all ROCs:
void pxarCore::MaskAndTrim(bool trim) {
  // Run over all existing ROCs:
  for (std::vector<rocConfig>::iterator rocit = _dut->roc.begin(); rocit != _dut->roc.end(); ++rocit) {
    MaskAndTrim(trim,rocit);
  }
}

// Mask/Unmask and trim one ROC:
void pxarCore::MaskAndTrim(bool trim, std::vector<rocConfig>::iterator rocit) {

  // This ROC is supposed to be trimmed as configured, so let's trim it:
  if(trim) {
    LOG(logDEBUGAPI) << "ROC@I2C " << static_cast<int>(rocit->i2c_address) << " features "
		     << static_cast<int>(std::count_if(rocit->pixels.begin(),rocit->pixels.end(),configMaskSet(true)))
		     << " masked pixels.";
    LOG(logDEBUGAPI) << "Unmasking and trimming ROC@I2C " << static_cast<int>(rocit->i2c_address) << " in one go.";
    _hal->RocSetMask(rocit->i2c_address,false,rocit->pixels);
    return;
  }
  else {
    LOG(logDEBUGAPI) << "Masking ROC@I2C " << static_cast<int>(rocit->i2c_address) << " in one go.";
    _hal->RocSetMask(rocit->i2c_address,true);
    return;
  }
}

// Program the calibrate bits in ROC PUCs:
void pxarCore::SetCalibrateBits(bool enable) {

  // Run over all existing ROCs:
  for (std::vector<rocConfig>::iterator rocit = _dut->roc.begin(); rocit != _dut->roc.end(); ++rocit) {

    LOG(logDEBUGAPI) << "Configuring calibrate bits in all enabled PUCs of ROC@I2C " << static_cast<int>(rocit->i2c_address);
    // Check if the signal has to be turned on or off:
    if(enable) {
      // Loop over all pixels in this ROC and set the Cal bit:
      for(std::vector<pixelConfig>::iterator pxit = rocit->pixels.begin(); pxit != rocit->pixels.end(); ++pxit) {
	if(pxit->enable() == true) { _hal->PixelSetCalibrate(rocit->i2c_address,pxit->column(),pxit->row(),0); }
      }

    }
    // Clear the signal for the full ROC:
    else {_hal->RocClearCalibrate(rocit->i2c_address);}
  }
}

void pxarCore::checkTestboardDelays(std::vector<std::pair<std::string,uint8_t> > sig_delays) {

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
}

void pxarCore::checkTestboardPower(std::vector<std::pair<std::string,double> > power_settings) {

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

  if(va < 0.01 || vd < 0.01 || ia < 0.01 || id < 0.01) {
    LOG(logCRITICAL) << "Power settings are not sufficient. Please check and re-configure!";
    throw InvalidConfig("Power settings are not sufficient. Please check and re-configure.");
  }
}

void pxarCore::verifyPatternGenerator(std::vector<std::pair<std::string,uint8_t> > &pg_setup) {
  
  std::vector<std::pair<uint16_t,uint8_t> > patterns;

  // Get the Pattern Generator dictionary for lookup:
  PatternGeneratorDictionary * _dict = PatternGeneratorDictionary::getInstance();

  // Check total length of the pattern generator:
  if(pg_setup.size() > 256) {
    LOG(logCRITICAL) << "Pattern too long (" << pg_setup.size() << " entries) for pattern generator. "
		     << "Only 256 entries allowed!";
    throw InvalidConfig("Pattern too long for pattern generator. Please check and re-configure.");
  }
  else { LOG(logDEBUGAPI) << "Pattern generator setup with " << pg_setup.size() << " entries provided."; }

  // Loop over all entries provided:
  for(std::vector<std::pair<std::string,uint8_t> >::iterator it = pg_setup.begin(); it != pg_setup.end(); ++it) {

    // Check for current element if delay is zero:
    if(it->second == 0 && it != pg_setup.end() -1 ) {
      LOG(logCRITICAL) << "Found delay = 0 on early entry! This stops the pattern generator at position " 
		       << static_cast<int>(it - pg_setup.begin())  << ".";
      throw InvalidConfig("Found delay = 0 on early entry! This stops the pattern generator.");
    }

    // Check last entry for PG stop signal (delay = 0):
    if(it == pg_setup.end() - 1 && it->second != 0) {
      LOG(logWARNING) << "No delay = 0 found on last entry. Setting last delay to 0 to stop the pattern generator.";
      it->second = 0;
    }

    // Convert the name to lower case for comparison:
    std::transform(it->first.begin(), it->first.end(), it->first.begin(), ::tolower);

    std::istringstream signals(it->first);
    std::string s;
    uint16_t signal = 0;
    // Tokenize the signal string into single PG signals, separated by ";":
    while (std::getline(signals, s, ';')) {
      // Get the signal from the dictionary object:
      uint16_t sig = _dict->getSignal(s);
      if(sig != PG_ERR) signal += sig;
      else {
	LOG(logCRITICAL) << "Could not find pattern generator signal \"" << s << "\" in the dictionary!";
	throw InvalidConfig("Wrong pattern generator signal provided.");
      }
      LOG(logDEBUGAPI) << "Found PG signal " << s << " (" << std::hex << sig << std::dec << ")";
    }
    patterns.push_back(std::make_pair(signal,it->second));
  }

  // Store the Pattern Generator commands in the DUT:
  _dut->pg_setup = patterns;
  // Calculate the sum of all delays and store it:
  _dut->pg_sum = getPatternGeneratorDelaySum(_dut->pg_setup);
}

uint32_t pxarCore::getPatternGeneratorDelaySum(std::vector<std::pair<uint16_t,uint8_t> > &pg_setup) {

  uint32_t delay_sum = 0;
  // Total cycle time is sum of delays plus once clock cycle for the actual command:
  for(std::vector<std::pair<uint16_t,uint8_t> >::iterator it = pg_setup.begin(); it != pg_setup.end(); ++it) { delay_sum += (*it).second + 1; }
  // Add one more clock cycle:
  delay_sum++;
  LOG(logDEBUGAPI) << "Sum of Pattern generator delays: " << delay_sum << " clk";
  return delay_sum;
}

void pxarCore::getDecoderErrorCount(){
  // check the data for any decoding errors (stored in the events as counters)
  _ndecode_errors_lastdaq = _hal->daqErrorCount();
  if (_ndecode_errors_lastdaq){
    LOG(logCRITICAL) << "A total of " << _ndecode_errors_lastdaq << " pixels could not be decoded in this DAQ readout.";
  }
}

bool pxarCore::setExternalClock(bool enable) {

  LOG(logDEBUGAPI) << "Setting clock to " << (enable ? "external" : "internal") << " source.";
  if(enable) {
    // Try to set the clock to external source:
    if(_hal->IsClockPresent()) { _hal->SetClockSource(CLK_SRC_EXT); return true; }
    else LOG(logCRITICAL) << "DTB reports that no external clock is present!";
    return false;
  }
  else {
    // Set the clock to internal source:
    _hal->SetClockSource(CLK_SRC_INT);
    return true;
  }
}

void pxarCore::setSignalMode(std::string signal, uint8_t mode) {
 
  uint8_t sigRegister, value = 0;
  if(!verifyRegister(signal, sigRegister, value, DTB_REG)) return;
  
  LOG(logDEBUGAPI) << "Setting signal " << signal << " (" 
		   << static_cast<int>(sigRegister) << ")  to mode "
		   << static_cast<int>(mode) << ".";
  _hal->SigSetMode(sigRegister, mode);
}

void pxarCore::setClockStretch(uint8_t src, uint16_t delay, uint16_t width)
{
  LOG(logDEBUGAPI) << "Set Clock Stretch " << static_cast<int>(src) << " " << static_cast<int>(delay) << " " << static_cast<int>(width); 
  _hal->SetClockStretch(src,width,delay);
  
}

uint16_t pxarCore::GetADC( uint8_t rpc_par1 ){
  
  if( ! status() ) { return 0; } 

  return _hal->GetADC( rpc_par1 );

}
