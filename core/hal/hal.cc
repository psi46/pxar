#include "hal.h"
#include "log.h"
#include "timer.h"
#include "helper.h"
#include "config.h"
#include "constants.h"
#include <fstream>
#include <algorithm>

using namespace pxar;



hal::hal(std::string name) :
  _initialized(false),
  _compatible(false),
  m_tbmtype(TBM_NONE),
  m_adctimeout(300),
  m_tindelay(13),
  m_toutdelay(8),
  deser160phase(4),
  m_roctype(0),
  m_roccount(0),
  m_tokenchains(),
  m_daqstatus(),
  _currentTrgSrc(TRG_SEL_PG_DIR),
  m_src(),
  m_splitter(),
  m_decoder()
{

  // Get a new CTestboard class instance:
  _testboard = new CTestboard();

  // Check if any boards are connected:
  FindDTB(name);

  // Open the testboard connection:
  if(_testboard->Open(name)) {
    LOG(logQUIET) << "Connection to board " << name << " opened.";
    try {
      // Print the useful SW/FW versioning info:
      PrintInfo();

      // Check if all RPC calls are matched:
      if(CheckCompatibility()) {
	// Set compatibility flag
	_compatible = true;

	// ...and do the obligatory welcome LED blink:
	_testboard->Welcome();
	_testboard->Flush();

	// Finally, initialize the testboard:
	_testboard->Init();

      }
    }
    catch(CRpcError &e) {
      // Something went wrong:
      e.What();
      LOG(logCRITICAL) << "DTB software version could not be identified, please update!";
      _testboard->Close();
      LOG(logCRITICAL) << "Connection to board " << name << " has been cancelled.";
      throw FirmwareVersionMismatch("DTB software version could not be identified, please update!");
    }
  }
  else {
    // USB port cannot be accessed correctly, most likely access right issue:
    LOG(logCRITICAL) << "USB error: " << _testboard->ConnectionError();
    LOG(logCRITICAL) << "DTB: could not open port to device.";
    LOG(logCRITICAL) << "Make sure you have permission to access USB devices.";
    LOG(logCRITICAL) << "(see documentation for UDEV configuration examples)";
    throw UsbConnectionError("USB connection problem to DTB: could not open port to device.");
  }

  // We connected to a DTB, now let's initialize the pipe works:
  for(size_t channel = 0; channel < DTB_DAQ_CHANNELS; channel++) {
    m_src.push_back(dtbSource());
    m_splitter.push_back(dtbEventSplitter());
    m_decoder.push_back(dtbEventDecoder());
  }
  
}

hal::~hal() {
  // Shut down and close the testboard connection on destruction of HAL object:
  
  // Turn High Voltage off:
  _testboard->HVoff();

  // Turn DUT power off:
  _testboard->Poff();

  // Turn off triggers (default to Pattern Generator, direct):
  _testboard->Trigger_Select(TRG_SEL_PG_DIR);
  
  // Close the RPC/USB Connection:
  LOG(logQUIET) << "Connection to board " << _testboard->GetBoardId() << " closed.";
  _testboard->Close();
  delete _testboard;
}

bool hal::status() {

  if(!_initialized || !_compatible) {
    LOG(logERROR) << "Testboard not initialized yet!";
  }
  return _initialized;
}

uint32_t hal::GetHashForString(const char * s)
{
  // Using some primes
  uint32_t h = 31;
  while (*s) { h = (h * 54059) ^ (s[0] * 76963); s++; }
  return h%86969;
}

uint32_t hal::GetHashForStringVector(const std::vector<std::string> & v)
{
  uint32_t ret = 0;
  for (size_t i=0; i < v.size(); i++) {ret += ((i+1)*(GetHashForString(v[i].c_str())));}
  return ret;
}

void hal::initTestboard(std::map<uint8_t,uint8_t> sig_delays, std::vector<std::pair<uint16_t,uint8_t> > pg_setup, uint16_t delaysum, double va, double vd, double ia, double id) {

  // Set the power limits:
  setTestboardPower(va,vd,ia,id);

  // Set the delays:
  setTestboardDelays(sig_delays);

  // Set up Pattern Generator:
  SetupPatternGenerator(pg_setup,delaysum);

  // We are ready for operations now, mark the HAL as initialized:
  _initialized = true;
}

void hal::setTestboardPower(double va, double vd, double ia, double id) {

  // Set voltages and current limits:
  setTBva(va);
  setTBvd(vd);
  setTBia(ia);
  setTBid(id);
  _testboard->Flush();
}

void hal::setTestboardDelays(std::map<uint8_t,uint8_t> sig_delays) {

  // Default signal level: 15 (highest)
  uint8_t signal_level = 15;
  // Find the signal level entry (if one):
  std::map<uint8_t,uint8_t>::iterator lvl = sig_delays.find(SIG_LEVEL);
  if(lvl != sig_delays.end()) {
    signal_level = lvl->second;
    sig_delays.erase(lvl);
  }
  LOG(logDEBUGHAL) << "Setting all DTB signal levels to " << static_cast<int>(signal_level);

  _testboard->Deser400_SetPhaseAutoAll();  
  LOG(logDEBUGHAL) << "Defaulting all DESER400 modules to automatic phase selection.";
  
  // Write testboard delay settings and deserializer phases to the respective registers:
  for(std::map<uint8_t,uint8_t>::iterator sigIt = sig_delays.begin(); sigIt != sig_delays.end(); ++sigIt) {

    if(sigIt->first == SIG_DESER160PHASE) {
      LOG(logDEBUGHAL) << "Set DTB deser160 phase to value " << static_cast<int>(sigIt->second);
      _testboard->Daq_Select_Deser160(sigIt->second);
      // FIXME
      deser160phase = sigIt->second;
    }
    else if(sigIt->first == SIG_DESER400RATE) {
      LOG(logDEBUGHAL) << "Set DTB deser400 phase sampling rate to value " << static_cast<int>(sigIt->second);
      // This function is a DTB-internal call to Deser400_PdPhase(), I don't know why Beat decided to encapsulate it...
      _testboard->Deser400_GateRun(0,sigIt->second);
    }
    else if(sigIt->first == SIG_DESER400PHASE0) {
      LOG(logDEBUGHAL) << "Set DTB deser400 module 0 phase to value " << static_cast<int>(sigIt->second);
      _testboard->Deser400_SetPhase(0,sigIt->second);
    }
    else if(sigIt->first == SIG_DESER400PHASE1) {
      LOG(logDEBUGHAL) << "Set DTB deser400 module 1 phase to value " << static_cast<int>(sigIt->second);
      _testboard->Deser400_SetPhase(1,sigIt->second);
    }
    else if(sigIt->first == SIG_DESER400PHASE2) {
      LOG(logDEBUGHAL) << "Set DTB deser400 module 2 phase to value " << static_cast<int>(sigIt->second);
      _testboard->Deser400_SetPhase(2,sigIt->second);
    }
    else if(sigIt->first == SIG_DESER400PHASE3) {
      LOG(logDEBUGHAL) << "Set DTB deser400 module 3 phase to value " << static_cast<int>(sigIt->second);
      _testboard->Deser400_SetPhase(3,sigIt->second);
    }
    else if(sigIt->first == SIG_LOOP_TRIGGER_DELAY) {
      LOG(logDEBUGHAL) << "Set DTB loop delay between triggers to " << static_cast<int>(sigIt->second)*10 <<" clk";
      _testboard->SetLoopTriggerDelay(sigIt->second*10);
    }
    else if(sigIt->first == SIG_LOOP_TRIM_DELAY) {
      LOG(logDEBUGHAL) << "Set DTB loop delay after trimming to " << static_cast<int>(sigIt->second)*10 <<" clk";
      _testboard->SetLoopTrimDelay(sigIt->second*10);
    }
    else if(sigIt->first == SIG_TRIGGER_LATENCY) {
      LOG(logDEBUGHAL) << "Set latency for external triggers to " << static_cast<int>(sigIt->second) <<" clk";
      _testboard->Trigger_Delay(sigIt->second);
    }
    else if(sigIt->first == SIG_TRIGGER_TIMEOUT) {
      LOG(logDEBUGHAL) << "Set token out timeout for external triggers to " << static_cast<int>(sigIt->second) <<" clk";
      _testboard->Trigger_Timeout(sigIt->second);
    }
    else if(sigIt->first == SIG_RDA_TOUT) {
      LOG(logDEBUGHAL) << "set TOUT / RDA delay to value " << static_cast<int>(sigIt->second);
      _testboard->Sig_SetRdaToutDelay(sigIt->second);
    }
    else if(sigIt->first == SIG_ADC_TINDELAY) {
      LOG(logDEBUGHAL) << "caching ADC Token In delay as " << static_cast<int>(sigIt->second);
      m_tindelay = sigIt->second;
    }
    else if(sigIt->first == SIG_ADC_TOUTDELAY) {
      LOG(logDEBUGHAL) << "caching ADC Token Out delay as " << static_cast<int>(sigIt->second);
      m_toutdelay = sigIt->second;
    }
    else if(sigIt->first == SIG_ADC_TIMEOUT) {
      LOG(logDEBUGHAL) << "caching ADC timeout as " << static_cast<int>(sigIt->second)*10 << " clk";
      m_adctimeout = sigIt->second*10;
    }
    else {
      LOG(logDEBUGHAL) << "Set DTB delay " << static_cast<int>(sigIt->first) << " to value " << static_cast<int>(sigIt->second);
      _testboard->Sig_SetDelay(sigIt->first, sigIt->second);
      // Also set the signal level:
      _testboard->Sig_SetLevel(sigIt->first, signal_level);
    }
  }
  _testboard->Flush();
}

void hal::SetupPatternGenerator(std::vector<std::pair<uint16_t,uint8_t> > pg_setup, uint16_t delaysum) {

  // Write the (sorted!) PG patterns into adjacent register addresses:
  std::vector<uint16_t> cmd;

  LOG(logDEBUGHAL) << "Setting Pattern Generator:";
  for(std::vector<std::pair<uint16_t,uint8_t> >::iterator it = pg_setup.begin(); it != pg_setup.end(); ++it) {
    cmd.push_back((*it).first | (*it).second);
    LOG(logDEBUGHAL) << " cmd " << std::hex << cmd.back() << std::dec
		     << " (addr " << static_cast<int>(cmd.size()-1)
		     << " pat " << std::hex << static_cast<int>((*it).first) << std::dec
		     << " del " << static_cast<int>((*it).second) << ")";
  }

  _testboard->Pg_SetCmdAll(cmd);
  _testboard->Pg_SetSum(delaysum);

  // Since the last delay is known to be zero we don't have to overwrite the rest of the address range - 
  // the Pattern generator will stop automatically at that point.
}

bool hal::flashTestboard(std::ifstream& flashFile) {

  if (_testboard->UpgradeGetVersion() == 0x0100) {
    LOG(logINFO) << "Starting DTB firmware upgrade...";

    // Reading lines of file:
    std::string line;
    size_t file_lines;
    for (file_lines = 0; getline(flashFile, line); ++file_lines)
      ;
    flashFile.clear(flashFile.goodbit);
    flashFile.seekg(std::ios::beg);

    // Check if upgrade is possible
    if (_testboard->UpgradeStart(0x0100) != 0) {
      std::string msg;
      _testboard->UpgradeErrorMsg(msg);
      LOG(logCRITICAL) << "UPGRADE: " << msg.data();
      return false;
    }

    // Download the flash data
    std::string rec;
    uint16_t recordCount = 0;
    LOG(logINFO) << "Download running... ";

    while (true) {
      // LOG doesn't works with flush, so we can't display the percentage:
      //             << static_cast<int>(100 * recordCount / file_lines) << " % " << flush;
      getline(flashFile, rec);
      if (flashFile.good()) {
	if (rec.size() == 0) continue;
	recordCount++;
	if (_testboard->UpgradeData(rec) != 0) {
	  std::string msg;
	  _testboard->UpgradeErrorMsg(msg);
	  LOG(logCRITICAL) << "UPGRADE: " << msg.data();
	  return false;
	}
      }
      else if (flashFile.eof()) break;
      else {
	LOG(logCRITICAL) << "UPGRADE: Error reading file.";
	return false;
      }
    }
      
    if (_testboard->UpgradeError() != 0) {
      std::string msg;
      _testboard->UpgradeErrorMsg(msg);
      LOG(logCRITICAL) << "UPGRADE: " << msg.data();
      return false;
    }

    // Write EPCS FLASH
    LOG(logINFO) << "DTB download complete.";
    mDelay(200);
    LOG(logINFO) << "FLASH write start (LED 1..4 on)";
    LOG(logWARNING) << "DO NOT INTERUPT DTB POWER! - Wait till LEDs goes off and connection is closed.";
    _testboard->UpgradeExec(recordCount);
    _testboard->Flush();
    return true;
  }

  LOG(logCRITICAL) << "ERROR UPGRADE: Could not upgrade this DTB version!";
  return false;
}

void hal::initTBMCore(tbmConfig tbm) {

  // Turn the TBM on:
  _testboard->tbm_Enable(true);

  // Store TBM type:
  m_tbmtype = tbm.type;

  // Store the token chain lengths:
  for(std::vector<uint8_t>::iterator i = tbm.tokenchains.begin(); i != tbm.tokenchains.end(); i++) {
    // One tokenchain setting per TBM channel:
    m_tokenchains.push_back(*i);
  }

  // Program all registers according to the configuration data:
  LOG(logDEBUGHAL) << "Setting register vector for TBM Core " << tbm.corename() << ".";
  if(tbm.NoTokenPass()) { LOG(logDEBUGHAL) << "This TBM has the NoTokenPass register set!"; }
  tbmSetRegs(tbm.hubid,tbm.core,tbm.dacs);
}

void hal::setTBMType(uint8_t type) {
  LOG(logDEBUGHAL) << "Updating TBM type to 0x" << std::hex << static_cast<int>(type) << std::dec << ".";
  m_tbmtype = type;
}

void hal::initROC(uint8_t roci2c, uint8_t type, std::map< uint8_t,uint8_t > dacVector) {

  // Set the pixel address inverted flag if we have the PSI46digV1 chip
  if(type == ROC_PSI46DIG || type == ROC_PSI46DIG_TRIG) {
    LOG(logDEBUGHAL) << "Pixel address is inverted in this ROC type.";
  }
  // Store ROC type for later HAL usage:
  m_roctype = type;
  m_roccount++;
  LOG(logDEBUGHAL) << "Currently have " << static_cast<int>(m_roccount) << " ROCs in HAL";

  // Programm all DAC registers according to the configuration data:
  LOG(logDEBUGHAL) << "Setting DAC vector for ROC@I2C " << static_cast<int>(roci2c) << ".";
  rocSetDACs(roci2c,dacVector);
}

void hal::PrintInfo() {
  std::string info;
  _testboard->GetInfo(info);
  LOG(logINFO) << "DTB startup information" << std::endl 
	       << "--- DTB info------------------------------------------" << std::endl
	       << info
	       << "------------------------------------------------------";
}

bool hal::CheckCompatibility() {

  std::string dtb_hashcmd;
  uint32_t hostCmdHash = 0, dtbCmdHash = 0;

  // This is a legacy check for boards with an old firmware not featuring the hash function:
  _testboard->GetRpcCallName(5,dtb_hashcmd);
  if(dtb_hashcmd.compare("GetRpcCallHash$I") != 0) {
    LOG(logCRITICAL) << "Your DTB flash file is outdated, it does not provide a RPC hash value for compatibility checks.";
    return false;
    // FIXME rework upgrade/flashing mechanism - does not work with exceptions yet!
    //throw FirmwareVersionMismatch("Your DTB flash file is outdated, it does not provide a RPC hash value for compatibility checks.");
  }
  else {
    // Get hash for the Host RPC command list:
    LOG(logDEBUGHAL) << "Hashing Host RPC command list.";
    hostCmdHash = GetHashForStringVector(_testboard->GetHostRpcCallNames());
    LOG(logDEBUGHAL) << "Host Hash: " << hostCmdHash;

    // Get hash for the DTB RPC command list:
    LOG(logDEBUGHAL) << "Fetching DTB RPC command hash.";
    dtbCmdHash = _testboard->GetRpcCallHash();
    LOG(logDEBUGHAL) << "DTB Hash: " << dtbCmdHash;
  }

  // If they don't match check RPC calls one by one and print offenders:
  if(dtb_hashcmd.compare("GetRpcCallHash$I") != 0 || dtbCmdHash != hostCmdHash) {
    LOG(logWARNING) << "RPC Call hashes of DTB and Host do not match!";

    if(!_testboard->RpcLink()) {
      LOG(logCRITICAL) << "Please update your DTB with the correct flash file.";
      LOG(logCRITICAL) << "Get Firmware " << PACKAGE_FIRMWARE << " from " << PACKAGE_FIRMWARE_URL;
      // FIXME rework upgrade/flashing mechanism - does not work with exceptions yet!
      //throw FirmwareVersionMismatch("RPC Call hashes of DTB and Host do not match!");
      return false;
    }
    else { 
      // hashes do not match but all functions we need for pxar are present
      return true; 
    }
  }
  else { LOG(logINFO) << "RPC call hashes of host and DTB match: " << hostCmdHash; }

  // We are through all checks, testboard is successfully connected:
  return true;
}

bool hal::FindDTB(std::string &rpcId) {

  // Try to access interfaces:
  uint32_t interfaceList = _testboard->GetInterfaceListSize();
  if(interfaceList == 0) {
    LOG(logCRITICAL) << "Could not find any interface.";
    throw UsbConnectionError("Could not find any interface.");
  }
  else { LOG(logDEBUGHAL) << "Found " << interfaceList << " interfaces."; }

  // Find attached USB and/or ETH devices that match the DTB naming scheme:
  std::vector<std::pair<std::string, std::string> > deviceList = _testboard->GetDeviceList();

  // We have no DTBs at all:
  if(deviceList.empty()) {
    LOG(logCRITICAL) << "Could not find any connected DTB.";
    throw UsbConnectionError("Could not find any connected DTB.");
  }
  else { LOG(logDEBUGHAL) << "Found " << deviceList.size() << " connected DTBs."; }

  // We have one DTB, check if we can use it:
  if (deviceList.size() == 1) {
    if(rpcId == "*") { rpcId = deviceList.front().second; }
    else if(rpcId != deviceList.front().second) {
      LOG(logCRITICAL) << "Could not find DTB \"" << rpcId << "\".";
      throw UsbConnectionError("Could not find DTB " + rpcId);
    }
    _testboard->SelectInterface(deviceList.front().first);
    return true;
  }

  // We have more than one DTB - check if selected DTB is among connected:
  if(rpcId != "*") {
    for(std::vector<std::pair<std::string,std::string> >::iterator dev = deviceList.begin(); dev != deviceList.end(); dev++) {
      if(rpcId == dev->second) {
	LOG(logINFO) << "Found DTB " << rpcId;
	_testboard->SelectInterface(dev->first);
	return true;
      }
    }
  }

  // If more than 1 connected device list them
  LOG(logINFO) << "Connected DTBs:";
  for(std::vector<std::pair<std::string,std::string> >::iterator dev = deviceList.begin(); dev != deviceList.end(); dev++) {
    LOG(logINFO) << (dev-deviceList.begin()) << ": " << dev->second << " on " << dev->first;
    _testboard->SelectInterface(dev->first);
    if(_testboard->Open(dev->second, false)) {
      try { LOG(logINFO) << "BID = " << _testboard->GetBoardId(); }
      catch (CRpcError &e) {
	LOG(logERROR) << "Problem: ";
	e.What();
      }
      catch (...) {
	LOG(logERROR) << "Not identifiable";
      }
      _testboard->Close();
    }
    else LOG(logWARNING) << "-- in use";
    _testboard->ClearInterface();
  }

  LOG(logINFO) << "Please choose DTB (0-" << (deviceList.size()-1) << "): ";
  uint32_t nr = 0;
  char choice[8];
  if(fgets(choice, 8, stdin) == NULL) return false;
  sscanf (choice, "%ud", &nr);
  if (nr >= deviceList.size()) {
    nr = 0;
    LOG(logCRITICAL) << "No DTB opened";
    throw UsbConnectionError("No DTB opened.");
  }

  // Return the selected DTB's USB id as reference string:
  rpcId = deviceList.at(nr).second;
  _testboard->SelectInterface(deviceList.at(nr).first);
  return true;
}

double hal::getTBia() {
  // Return the VA analog current in A:
  return (_testboard->_GetIA()/10000.0);
}

double hal::getTBva(){
  // Return the VA analog voltage in V:
  return (_testboard->_GetVA()/1000.0);
}

double hal::getTBid() {
  // Return the VD digital current in A:
  return (_testboard->_GetID()/10000.0);
}

double hal::getTBvd() {
  // Return the VD digital voltage in V:
  return (_testboard->_GetVD()/1000.0);
}


void hal::setTBia(double IA) {
  // Set the VA analog current limit in A:
  LOG(logDEBUGHAL) << "Set DTB analog current limit to IA = " << IA << "A";
  _testboard->_SetIA(uint16_t(IA*10000));
}

void hal::setTBva(double VA){
  // Set the VA analog voltage in V:
  LOG(logDEBUGHAL) << "Set DTB analog output voltage to VA = " << VA << "V";
  _testboard->_SetVA(uint16_t(VA*1000));
}

void hal::setTBid(double ID) {
 // Set the VD digital current limit in A:
  LOG(logDEBUGHAL) << "Set DTB digital current limit to ID = " << ID << "A";
  _testboard->_SetID(uint16_t(ID*10000));
}

void hal::setTBvd(double VD) {
  // Set the VD digital voltage in V:
  LOG(logDEBUGHAL) << "Set DTB digital output voltage to VD = " << VD << "V";
  _testboard->_SetVD(uint16_t(VD*1000));
}

void hal::setHubId(uint8_t hubid) {
  LOG(logDEBUGHAL) << "Setting Hub ID: " << static_cast<int>(hubid);
  _testboard->mod_Addr(hubid);
}

void hal::setHubId(uint8_t hub0, uint8_t hub1) {
  LOG(logDEBUGHAL) << "Setting both Layer 1 Hub IDs: " << static_cast<int>(hub0) << ", " << static_cast<int>(hub1);
  _testboard->mod_Addr(hub0, hub1);
}

bool hal::rocSetDACs(uint8_t roci2c, std::map< uint8_t, uint8_t > dacPairs) {

  // Make sure we are writing to the correct ROC by setting the I2C address:
  _testboard->roc_I2cAddr(roci2c);

  // Check if WBC has been updated:
  bool is_wbc = false;

  // Check if one of the DACs to be set is RangeTemp and shift it to the end:
  std::map<uint8_t,uint8_t>::iterator rangetemp = dacPairs.end();

  // Iterate over all DAC id/value pairs and set the DAC
  for(std::map< uint8_t,uint8_t >::iterator it = dacPairs.begin(); it != dacPairs.end(); ++it) {
    if(it->first == ROC_DAC_RangeTemp) { rangetemp = it; continue; }

    LOG(logDEBUGHAL) << "Set DAC" << static_cast<int>(it->first) << " to " << static_cast<int>(it->second);
    _testboard->roc_SetDAC(it->first,it->second);
    if(it->first == ROC_DAC_WBC) { is_wbc = true; }
  }

  // Check if RangeTemp has been omitted and set it now - this allows to read its value via lastDAC:
  if(rangetemp != dacPairs.end()) {
    LOG(logDEBUGHAL) << "Set DAC" << static_cast<int>(rangetemp->first) << " to " << static_cast<int>(rangetemp->second);
    _testboard->roc_SetDAC(rangetemp->first,rangetemp->second);
  }

  // Make sure to issue a ROC Reset after WBC has been programmed:
  if(is_wbc) {
    LOG(logDEBUGHAL) << "WBC has been programmed - sending a ROC Reset command.";
    daqTriggerSingleSignal(TRG_SEND_RSR);
  }

  // Send all queued commands to the testboard:
  _testboard->Flush();
  // Everything went all right:
  return true;
}

bool hal::rocSetDAC(uint8_t roci2c, uint8_t dacId, uint8_t dacValue) {

  // Make sure we are writing to the correct ROC by setting the I2C address:
  _testboard->roc_I2cAddr(roci2c);

  LOG(logDEBUGHAL) << "ROC@I2C " << static_cast<size_t>(roci2c) 
		   << ": Set DAC" << static_cast<int>(dacId) << " to " << static_cast<int>(dacValue);
  _testboard->roc_SetDAC(dacId,dacValue);
  _testboard->Flush();

  // Make sure to issue a ROC Reset after the DAc WBC has been programmed:
  if(dacId == ROC_DAC_WBC) {
    LOG(logDEBUGHAL) << "WBC has been programmed - sending a ROC Reset command.";
    daqTriggerSingleSignal(TRG_SEND_RSR);
  }
  return true;
}

bool hal::tbmSetRegs(uint8_t hubid, uint8_t core, std::map< uint8_t, uint8_t > regPairs) {

  // Iterate over all register id/value pairs and set them
  for(std::map< uint8_t,uint8_t >::iterator it = regPairs.begin(); it != regPairs.end(); ++it) {
    // One of the register settings had an issue, abort:
    if(!tbmSetReg(hubid, core | it->first, it->second),false) return false;
  }

  // Send all queued commands to the testboard:
  _testboard->Flush();
  // Everything went all right:
  return true;
}

bool hal::tbmSetReg(uint8_t hubid, uint8_t regId, uint8_t regValue, bool flush) {

  LOG(logDEBUGHAL) << "TBM@HUB " << static_cast<int>(hubid)
		   << ": set register \"0x" << std::hex << static_cast<int>(regId) 
		   << "\" to 0x" << static_cast<int>(regValue) << std::dec;

  // Make sure we are writing to the correct TBM by setting the module's hub id:
  _testboard->mod_Addr(hubid);

  // Set this register:
  _testboard->tbm_Set(regId,regValue);

  // If requested, flush immediately:
  if(flush) _testboard->Flush();
  return true;
}

void hal::tbmSelectRDA(uint8_t rda_id) {
  _testboard->tbm_SelectRDA(rda_id);
}

void hal::SetupI2CValues(std::vector<uint8_t> roci2cs) {

  LOG(logDEBUGHAL) << "Writing the following available I2C devices into NIOS storage:";
  LOG(logDEBUGHAL) << listVector(roci2cs);

  // Write all ROC I2C addresses to the NIOS storage:
  _testboard->SetI2CAddresses(roci2cs);

}

void hal::SetupTrimValues(uint8_t roci2c, std::vector<pixelConfig> pixels) {

  // Prepare the trim vector containing both mask bit and trim bits:
  std::vector<uint8_t> trim;

  // Set the default to "masked" (everything >15 is interpreted as such):
  for(size_t i = 0; i < ROC_NUMCOLS*ROC_NUMROWS; i++) { trim.push_back(20); }

  // Write the information from the pixel configs:
  for(std::vector<pixelConfig>::iterator pxIt = pixels.begin(); pxIt != pixels.end(); ++pxIt) {
    size_t position = pxIt->column()*ROC_NUMROWS + pxIt->row();
    // trim values larger than 15 are interpreted as masked:
    if(pxIt->mask()) trim[position] = 20;
    else trim[position] = pxIt->trim();
  }

  LOG(logDEBUGHAL) << "Updating NIOS trimming & masking configuration for ROC with I2C address " 
		   << static_cast<int>(roci2c) << ".";

  _testboard->SetTrimValues(roci2c,trim);
}

void hal::RocSetMask(uint8_t roci2c, bool mask, std::vector<pixelConfig> pixels) {

  _testboard->roc_I2cAddr(roci2c);

  // Check if we want to mask or unmask&trim:
  if(mask) {
    // This is quite easy:
    LOG(logDEBUGHAL) << "Masking full ROC@I2C " << static_cast<int>(roci2c);

    // Mask the PUC and detach all DC from their readout (both done on NIOS):
    _testboard->roc_Chip_Mask();
  }
  else {
    // Prepare configuration of the pixels, linearize vector:
    std::vector<int16_t> trim;

    // Set the default to "masked":
    for(size_t i = 0; i < ROC_NUMCOLS*ROC_NUMROWS; i++) { trim.push_back(-1); }
    
    // Write the information from the pixel configs:
    for(std::vector<pixelConfig>::iterator pxIt = pixels.begin(); pxIt != pixels.end(); ++pxIt) {
      size_t position = pxIt->column()*ROC_NUMROWS + pxIt->row();
      if(pxIt->mask()) trim[position] = -1;
      else trim[position] = pxIt->trim();
    }

    // We really want to program that full thing with correct mask/trim bits:
    LOG(logDEBUGHAL) << "Updating mask bits & trim values of ROC@I2C " << static_cast<int>(roci2c);

    // Trim the whole ROC:
    _testboard->TrimChip(trim);
  }
}

void hal::AllColumnsSetEnable(uint8_t roci2c, bool enable) {

  // Set the correct ROC I2C address:
  _testboard->roc_I2cAddr(roci2c);

  // Attach/detach all columns:
  LOG(logDEBUGAPI) << (enable ? "Attaching" : "Detaching")
		   << " all columns "
		   << (enable ? "to" : "from")
		   << " periphery for ROC@I2C " << static_cast<int>(roci2c);

  _testboard->roc_AllCol_Enable(enable);
  _testboard->Flush();
}

void hal::PixelSetCalibrate(uint8_t roci2c, uint8_t column, uint8_t row, uint16_t flags) {

  // Set the correct ROC I2C address:
  _testboard->roc_I2cAddr(roci2c);

  // Set the calibrate bit and the CALS setting:
  bool useSensorPadForCalibration  = (flags & FLAG_CALS) != 0;
  _testboard->roc_Pix_Cal(column,row,useSensorPadForCalibration);
}

void hal::RocSetCalibrate(uint8_t roci2c, std::vector<pixelConfig> pixels, uint16_t flags) {

  // Set the correct ROC I2C address:
  _testboard->roc_I2cAddr(roci2c);

  // Set the calibrate bit and the CALS setting:
  bool useSensorPadForCalibration  = (flags & FLAG_CALS) != 0;

  // Write the information from the pixel configs:
  for(std::vector<pixelConfig>::iterator pxIt = pixels.begin(); pxIt != pixels.end(); ++pxIt) {
    _testboard->roc_Pix_Cal(pxIt->column(),pxIt->row(),useSensorPadForCalibration);
  }
}

void hal::RocClearCalibrate(uint8_t roci2c) {

  // Set the correct ROC I2C address:
  _testboard->roc_I2cAddr(roci2c);

  LOG(logDEBUGHAL) << "Clearing calibrate signal for ROC " << static_cast<int>(roci2c);
 _testboard->roc_ClrCal();
}

void hal::estimateDataVolume(uint32_t events, uint8_t nROCs) {

  uint32_t nSamples = 0;
  // DESER400: header 3 words, pixel 6 words
  if(m_tbmtype != TBM_NONE && m_tbmtype != TBM_EMU) { nSamples = events*nROCs*(3+6); }
  // DESER160: header 1 word, pixel 2 words
  else { nSamples = events*nROCs*(1+2); }

  LOG(logINFO) << "Expecting " << events << " events.";
  LOG(logDEBUGHAL) << "Estimated data volume: "
		   << (nSamples/1000) << "k/" << (DTB_SOURCE_BUFFER_SIZE/1000) 
		   << "k (~" << (100*static_cast<double>(nSamples)/DTB_SOURCE_BUFFER_SIZE) << "% allocated DTB RAM)";

}

// ---------------- TEST FUNCTIONS ----------------------

std::vector<Event> hal::MultiRocAllPixelsCalibrate(std::vector<uint8_t> roci2cs, bool efficiency, std::vector<int32_t> parameter) {

  uint16_t flags = static_cast<uint16_t>(parameter.at(0));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(1));

  // We expect one Event per pixel per trigger, all ROCs are triggered in parallel:
  int expected = nTriggers*ROC_NUMROWS*ROC_NUMCOLS;

  LOG(logDEBUGHAL) << "Called MultiRocAllPixelsCalibrate with flags " << listFlags(flags) << ", running " << nTriggers << " triggers.";
  LOG(logDEBUGHAL) << "Function will take care of all pixels on " << roci2cs.size() << " ROCs with the I2C addresses:";
  LOG(logDEBUGHAL) << listVector(roci2cs);
  estimateDataVolume(expected, roci2cs.size());

  // Prepare for data acquisition:
  daqStart(flags,deser160phase);
  timer t;

  // Call the RPC command containing the trigger loop:
  bool done = false;
  std::vector<Event> data = std::vector<Event>();
  while(!done) {
    done = _testboard->LoopMultiRocAllPixelsCalibrate(roci2cs, nTriggers, flags);
    LOG(logDEBUGHAL) << "Loop " << (done ? "finished" : "interrupted") << " (" << t << "ms), reading " << daqBufferStatus() << " words...";
    addCondensedData(data,nTriggers,efficiency,t);
  }
  LOG(logDEBUGHAL) << "Loop done after " << t << "ms. Readout size: " << data.size() << " events.";

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();

  // check for missing events
  int missing = expected/nTriggers - data.size();
  if(missing != 0) {
    LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events.";
    // serious runtime issue as data is invalid and cannot be recovered at this point:
    data.clear();
    throw DataMissingEvent("Incomplete DAQ data readout in function "+std::string(__func__),missing);
  }

  return data;
}

std::vector<Event> hal::MultiRocOnePixelCalibrate(std::vector<uint8_t> roci2cs, uint8_t column, uint8_t row, bool efficiency, std::vector<int32_t> parameter) {

  uint16_t flags = static_cast<uint16_t>(parameter.at(0));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(1));

  LOG(logDEBUGHAL) << "Called MultiRocOnePixelCalibrate with flags " << listFlags(flags) << ", running " << nTriggers << " triggers.";
  LOG(logDEBUGHAL) << "Function will take care of pixel " << static_cast<int>(column) << "," 
		   << static_cast<int>(row) << " on "
		   << roci2cs.size() << " ROCs with the I2C addresses:";
  LOG(logDEBUGHAL) << listVector(roci2cs);
  estimateDataVolume(nTriggers, roci2cs.size());

  // Prepare for data acquisition:
  daqStart(flags,deser160phase);
  timer t;

  // Call the RPC command containing the trigger loop:
  bool done = false;
  std::vector<Event> data = std::vector<Event>();
  while(!done) {
    done = _testboard->LoopMultiRocOnePixelCalibrate(roci2cs, column, row, nTriggers, flags);
    LOG(logDEBUGHAL) << "Loop " << (done ? "finished" : "interrupted") << " (" << t << "ms), reading " << daqBufferStatus() << " words...";
    addCondensedData(data,nTriggers,efficiency,t);
  }
  LOG(logDEBUGHAL) << "Loop done after " << t << "ms. Readout size: " << data.size() << " events.";

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();

  // We expect one Event per trigger, all ROCs are triggered in parallel:
  int missing = 1 - data.size();
  if(missing != 0) { 
    LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events."; 
    // serious runtime issue as data is invalid and cannot be recovered at this point:
    data.clear();
    throw DataMissingEvent("Incomplete DAQ data readout in function "+std::string(__func__),missing);
  }

  return data;
}

std::vector<Event> hal::SingleRocAllPixelsCalibrate(uint8_t roci2c, bool efficiency, std::vector<int32_t> parameter) {

  uint16_t flags = static_cast<uint16_t>(parameter.at(0));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(1));

  // We are expecting one Event per pixel per trigger, only one ROC is triggered:
  int expected = nTriggers*ROC_NUMROWS*ROC_NUMCOLS;

  LOG(logDEBUGHAL) << "Called SingleRocAllPixelsCalibrate with flags " << listFlags(flags) << ", running " << nTriggers << " triggers on I2C " << static_cast<int>(roci2c) << ".";
  estimateDataVolume(expected, 1);

  // Prepare for data acquisition:
  daqStart(flags,deser160phase);
  timer t;

  // Call the RPC command containing the trigger loop:
  bool done = false;
  std::vector<Event> data = std::vector<Event>();
  while(!done) {
    done = _testboard->LoopSingleRocAllPixelsCalibrate(roci2c, nTriggers, flags);
    LOG(logDEBUGHAL) << "Loop " << (done ? "finished" : "interrupted") << " (" << t << "ms), reading " << daqBufferStatus() << " words...";
    addCondensedData(data,nTriggers,efficiency,t);
  }
  LOG(logDEBUGHAL) << "Loop done after " << t << "ms. Readout size: " << data.size() << " events.";

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();

  // check for missing events
  int missing = expected/nTriggers - data.size();
  if(missing != 0) { 
    LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events.";
    // serious runtime issue as data is invalid and cannot be recovered at this point:
    data.clear();
    throw DataMissingEvent("Incomplete DAQ data readout in function "+std::string(__func__),missing);
  }

  return data;
}

std::vector<Event> hal::SingleRocOnePixelCalibrate(uint8_t roci2c, uint8_t column, uint8_t row, bool efficiency, std::vector<int32_t> parameter) {

  uint16_t flags = static_cast<uint16_t>(parameter.at(0));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(1));

  LOG(logDEBUGHAL) << "Called SingleRocOnePixelCalibrate for pixel " << static_cast<int>(column) << ","
		   << static_cast<int>(row) << " with flags " << listFlags(flags) << ", running "
		   << nTriggers << " triggers.";
  estimateDataVolume(nTriggers, 1);

 // Prepare for data acquisition:
  daqStart(flags,deser160phase);
  timer t;

  // Call the RPC command containing the trigger loop:
  bool done = false;
  std::vector<Event> data = std::vector<Event>();
  while(!done) {
    done = _testboard->LoopSingleRocOnePixelCalibrate(roci2c, column, row, nTriggers, flags);
    LOG(logDEBUGHAL) << "Loop " << (done ? "finished" : "interrupted") << " (" << t << "ms), reading " << daqBufferStatus() << " words...";
    addCondensedData(data,nTriggers,efficiency,t);
  }
  LOG(logDEBUGHAL) << "Loop done after " << t << "ms. Readout size: " << data.size() << " events.";

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();

  // We are expecting one Event per trigger:
  int missing = 1 - data.size();
  if(missing != 0) { 
    LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events.";
    // serious runtime issue as data is invalid and cannot be recovered at this point:
    data.clear();
    throw DataMissingEvent("Incomplete DAQ data readout in function "+std::string(__func__),missing);
  }

  return data;
}


std::vector<Event> hal::MultiRocAllPixelsDacScan(std::vector<uint8_t> roci2cs, bool efficiency, std::vector<int32_t> parameter) {

  uint8_t dacreg = static_cast<uint8_t>(parameter.at(0));
  uint8_t dacmin = static_cast<uint8_t>(parameter.at(1));
  uint8_t dacmax = static_cast<uint8_t>(parameter.at(2));
  uint16_t flags = static_cast<uint16_t>(parameter.at(3));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(4));
  uint8_t dacstep = static_cast<uint8_t>(parameter.at(5));

  // We are expecting one Event per DAC setting per trigger per pixel:
  int expected = static_cast<size_t>((dacmax-dacmin)/dacstep+1)*nTriggers*ROC_NUMCOLS*ROC_NUMROWS;

  LOG(logDEBUGHAL) << "Called MultiRocAllPixelsDacScan with flags " << listFlags(flags) << ", running " << nTriggers << " triggers.";
  LOG(logDEBUGHAL) << "Function will take care of all pixels on " << roci2cs.size() << " ROCs with the I2C addresses:";
  LOG(logDEBUGHAL) << listVector(roci2cs);
  LOG(logDEBUGHAL) << "Scanning DAC " << static_cast<int>(dacreg) 
		   << " from " << static_cast<int>(dacmin) 
		   << " to " << static_cast<int>(dacmax)
		   << " (step size " << static_cast<int>(dacstep) << ")";
  estimateDataVolume(expected, roci2cs.size());

 // Prepare for data acquisition:
  daqStart(flags,deser160phase);
  timer t;

  // Call the RPC command containing the trigger loop:
  bool done = false;
  std::vector<Event> data = std::vector<Event>();
  while(!done) {
    done = _testboard->LoopMultiRocAllPixelsDacScan(roci2cs, nTriggers, flags, dacreg, dacstep, dacmin, dacmax);
    LOG(logDEBUGHAL) << "Loop " << (done ? "finished" : "interrupted") << " (" << t << "ms), reading " << daqBufferStatus() << " words...";
    addCondensedData(data,nTriggers,efficiency,t);
  }
  LOG(logDEBUGHAL) << "Loop done after " << t << "ms. Readout size: " << data.size() << " events.";

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();

  // check for errors in readout (i.e. missing events)
  int missing = expected/nTriggers - data.size();
  if(missing != 0) {
    LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events.";
    // serious runtime issue as data is invalid and cannot be recovered at this point:
    data.clear();
    throw DataMissingEvent("Incomplete DAQ data readout in function "+std::string(__func__),missing);
  }

  return data;
}

std::vector<Event> hal::MultiRocOnePixelDacScan(std::vector<uint8_t> roci2cs, uint8_t column, uint8_t row, bool efficiency, std::vector<int32_t> parameter) {

  uint8_t dacreg = static_cast<uint8_t>(parameter.at(0));
  uint8_t dacmin = static_cast<uint8_t>(parameter.at(1));
  uint8_t dacmax = static_cast<uint8_t>(parameter.at(2));
  uint16_t flags = static_cast<uint16_t>(parameter.at(3));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(4));
  uint8_t dacstep = static_cast<uint8_t>(parameter.at(5));

  // We expect one Event per DAC value per trigger:
  int expected = static_cast<size_t>((dacmax-dacmin)/dacstep+1)*nTriggers;

  LOG(logDEBUGHAL) << "Called MultiRocOnePixelDacScan with flags " << listFlags(flags) << ", running " << nTriggers << " triggers.";
  LOG(logDEBUGHAL) << "Function will take care of pixel " << static_cast<int>(column) << "," 
		   << static_cast<int>(row) << " on "
		   << roci2cs.size() << " ROCs with the I2C addresses:";
  LOG(logDEBUGHAL) << listVector(roci2cs);
  LOG(logDEBUGHAL) << "Scanning DAC " << static_cast<int>(dacreg) 
		   << " from " << static_cast<int>(dacmin) 
		   << " to " << static_cast<int>(dacmax)
		   << " (step size " << static_cast<int>(dacstep) << ")";
  estimateDataVolume(expected, roci2cs.size());

 // Prepare for data acquisition:
  daqStart(flags,deser160phase);
  timer t;

  // Call the RPC command containing the trigger loop:
  bool done = false;
  std::vector<Event> data = std::vector<Event>();
  while(!done) {
    done = _testboard->LoopMultiRocOnePixelDacScan(roci2cs, column, row, nTriggers, flags, dacreg, dacstep, dacmin, dacmax);
    LOG(logDEBUGHAL) << "Loop " << (done ? "finished" : "interrupted") << " (" << t << "ms), reading " << daqBufferStatus() << " words...";
    addCondensedData(data,nTriggers,efficiency,t);
  }
  LOG(logDEBUGHAL) << "Loop done after " << t << "ms. Readout size: " << data.size() << " events.";

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();

  // check for errors in readout (i.e. missing events)
  int missing = expected/nTriggers - data.size();
  if(missing != 0) { 
    LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events.";
    // serious runtime issue as data is invalid and cannot be recovered at this point:
    data.clear();
    throw DataMissingEvent("Incomplete DAQ data readout in function "+std::string(__func__),missing);
  }

  return data;
}

std::vector<Event> hal::SingleRocAllPixelsDacScan(uint8_t roci2c, bool efficiency, std::vector<int32_t> parameter) {

  uint8_t dacreg = static_cast<uint8_t>(parameter.at(0));
  uint8_t dacmin = static_cast<uint8_t>(parameter.at(1));
  uint8_t dacmax = static_cast<uint8_t>(parameter.at(2));
  uint16_t flags = static_cast<uint16_t>(parameter.at(3));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(4));
  uint8_t dacstep = static_cast<uint8_t>(parameter.at(5));

  // We expect one Event per DAC value per trigger per pixel:
  int expected = static_cast<size_t>((dacmax-dacmin)/dacstep+1)*nTriggers*ROC_NUMCOLS*ROC_NUMROWS;

  LOG(logDEBUGHAL) << "Called SingleRocAllPixelsDacScan with flags " << listFlags(flags) << ", running " << nTriggers << " triggers.";
  LOG(logDEBUGHAL) << "Scanning DAC " << static_cast<int>(dacreg) 
		   << " from " << static_cast<int>(dacmin) 
		   << " to " << static_cast<int>(dacmax)
		   << " (step size " << static_cast<int>(dacstep) << ")";
  estimateDataVolume(expected, 1);

 // Prepare for data acquisition:
  daqStart(flags,deser160phase);
  timer t;

  // Call the RPC command containing the trigger loop:
  bool done = false;
  std::vector<Event> data = std::vector<Event>();
  while(!done) {
    done = _testboard->LoopSingleRocAllPixelsDacScan(roci2c, nTriggers, flags, dacreg, dacstep, dacmin, dacmax);
    LOG(logDEBUGHAL) << "Loop " << (done ? "finished" : "interrupted") << " (" << t << "ms), reading " << daqBufferStatus() << " words...";
    addCondensedData(data,nTriggers,efficiency,t);
  }
  LOG(logDEBUGHAL) << "Loop done after " << t << "ms. Readout size: " << data.size() << " events.";

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();

  // check for errors in readout (i.e. missing events)
  int missing = expected/nTriggers - data.size();
  if(missing != 0) { 
    LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events.";
    // serious runtime issue as data is invalid and cannot be recovered at this point:
    data.clear();
    throw DataMissingEvent("Incomplete DAQ data readout in function "+std::string(__func__),missing);
  }

  return data;
}

std::vector<Event> hal::SingleRocOnePixelDacScan(uint8_t roci2c, uint8_t column, uint8_t row, bool efficiency, std::vector<int32_t> parameter) {

  uint8_t dacreg = static_cast<uint8_t>(parameter.at(0));
  uint8_t dacmin = static_cast<uint8_t>(parameter.at(1));
  uint8_t dacmax = static_cast<uint8_t>(parameter.at(2));
  uint16_t flags = static_cast<uint16_t>(parameter.at(3));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(4));
  uint8_t dacstep = static_cast<uint8_t>(parameter.at(5));

  // We expect one Event per DAC value per trigger:
  int expected = static_cast<size_t>((dacmax-dacmin)/dacstep+1)*nTriggers;

  LOG(logDEBUGHAL) << "Called SingleRocOnePixelDacScan with flags " << listFlags(flags) << ", running " << nTriggers << " triggers.";
  LOG(logDEBUGHAL) << "Scanning DAC " << static_cast<int>(dacreg) 
		   << " from " << static_cast<int>(dacmin) 
		   << " to " << static_cast<int>(dacmax)
		   << " (step size " << static_cast<int>(dacstep) << ")";
  estimateDataVolume(expected, 1);

  // Prepare for data acquisition:
  daqStart(flags,deser160phase);
  timer t;

  // Call the RPC command containing the trigger loop:
  bool done = false;
  std::vector<Event> data = std::vector<Event>();
  while(!done) {
    done = _testboard->LoopSingleRocOnePixelDacScan(roci2c, column, row, nTriggers, flags, dacreg, dacstep, dacmin, dacmax);
    LOG(logDEBUGHAL) << "Loop " << (done ? "finished" : "interrupted") << " (" << t << "ms), reading " << daqBufferStatus() << " words...";
    addCondensedData(data,nTriggers,efficiency,t);
  }
  LOG(logDEBUGHAL) << "Loop done after " << t << "ms. Readout size: " << data.size() << " events.";

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();

  // check for errors in readout (i.e. missing events)
  int missing = expected/nTriggers - data.size();
  if(missing != 0) { 
    LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events.";
    // serious runtime issue as data is invalid and cannot be recovered at this point:
    data.clear();
    throw DataMissingEvent("Incomplete DAQ data readout in function "+std::string(__func__),missing);
  }

  return data;
}

std::vector<Event> hal::MultiRocAllPixelsDacDacScan(std::vector<uint8_t> roci2cs, bool efficiency, std::vector<int32_t> parameter) {

  uint8_t dac1reg = static_cast<uint8_t>(parameter.at(0));
  uint8_t dac1min = static_cast<uint8_t>(parameter.at(1));
  uint8_t dac1max = static_cast<uint8_t>(parameter.at(2));
  uint8_t dac2reg = static_cast<uint8_t>(parameter.at(3));
  uint8_t dac2min = static_cast<uint8_t>(parameter.at(4));
  uint8_t dac2max = static_cast<uint8_t>(parameter.at(5));
  uint16_t flags = static_cast<uint16_t>(parameter.at(6));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(7));
  uint8_t dac1step = static_cast<uint8_t>(parameter.at(8));
  uint8_t dac2step = static_cast<uint8_t>(parameter.at(9));

  // We expect one Event per DAC1 value per DAC2 value per trigger per pixel:
  int expected = static_cast<size_t>((dac1max-dac1min)/dac1step+1)*static_cast<size_t>((dac2max-dac2min)/dac2step+1)*nTriggers*ROC_NUMROWS*ROC_NUMCOLS;

  LOG(logDEBUGHAL) << "Called MultiRocAllPixelsDacDacScan with flags " << listFlags(flags) << ", running " << nTriggers << " triggers.";
  LOG(logDEBUGHAL) << "Function will take care of all pixels on " << roci2cs.size() << " ROCs with the I2C addresses:";
  LOG(logDEBUGHAL) << listVector(roci2cs);
  LOG(logDEBUGHAL) << "Scanning DAC " << static_cast<int>(dac1reg) 
		   << " from " << static_cast<int>(dac1min) 
		   << " to " << static_cast<int>(dac1max)
		   << " (step size " << static_cast<int>(dac1step) << ")"
		   << " vs. DAC " << static_cast<int>(dac2reg) 
		   << " from " << static_cast<int>(dac2min) 
		   << " to " << static_cast<int>(dac2max)
		   << " (step size " << static_cast<int>(dac2step) << ")";
  estimateDataVolume(expected, roci2cs.size());

  // Prepare for data acquisition:
  daqStart(flags,deser160phase);
  timer t;

  // Call the RPC command containing the trigger loop:
  bool done = false;
  std::vector<Event> data = std::vector<Event>();
  while(!done) {
    done = _testboard->LoopMultiRocAllPixelsDacDacScan(roci2cs, nTriggers, flags, dac1reg, dac1step, dac1min, dac1max, dac2reg, dac2step, dac2min, dac2max);
    LOG(logDEBUGHAL) << "Loop " << (done ? "finished" : "interrupted") << " (" << t << "ms), reading " << daqBufferStatus() << " words...";
    addCondensedData(data,nTriggers,efficiency,t);
  }
  LOG(logDEBUGHAL) << "Loop done after " << t << "ms. Readout size: " << data.size() << " events.";

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();

  // check for errors in readout (i.e. missing events)
  int missing = expected/nTriggers - data.size();
  if(missing != 0) { 
    LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events.";
    // serious runtime issue as data is invalid and cannot be recovered at this point:
    data.clear();
    throw DataMissingEvent("Incomplete DAQ data readout in function "+std::string(__func__),missing);
  }

  return data;
}

std::vector<Event> hal::MultiRocOnePixelDacDacScan(std::vector<uint8_t> roci2cs, uint8_t column, uint8_t row, bool efficiency, std::vector<int32_t> parameter) {

  uint8_t dac1reg = static_cast<uint8_t>(parameter.at(0));
  uint8_t dac1min = static_cast<uint8_t>(parameter.at(1));
  uint8_t dac1max = static_cast<uint8_t>(parameter.at(2));
  uint8_t dac2reg = static_cast<uint8_t>(parameter.at(3));
  uint8_t dac2min = static_cast<uint8_t>(parameter.at(4));
  uint8_t dac2max = static_cast<uint8_t>(parameter.at(5));
  uint16_t flags = static_cast<uint16_t>(parameter.at(6));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(7));
  uint8_t dac1step = static_cast<uint8_t>(parameter.at(8));
  uint8_t dac2step = static_cast<uint8_t>(parameter.at(9));

  // We expect one Event per DAC1 value per DAC2 value per trigger:
  int expected = static_cast<size_t>((dac1max-dac1min)/dac1step+1)*static_cast<size_t>((dac2max-dac2min)/dac2step+1)*nTriggers;

  LOG(logDEBUGHAL) << "Called MultiRocOnePixelDacDacScan with flags " << listFlags(flags) << ", running " << nTriggers << " triggers.";

  LOG(logDEBUGHAL) << "Function will take care of pixel " << static_cast<int>(column) << "," 
		   << static_cast<int>(row) << " on "
		   << roci2cs.size() << " ROCs with the I2C addresses:";
  LOG(logDEBUGHAL) << listVector(roci2cs);
  LOG(logDEBUGHAL) << "Scanning DAC " << static_cast<int>(dac1reg) 
		   << " from " << static_cast<int>(dac1min) 
		   << " to " << static_cast<int>(dac1max)
		   << " (step size " << static_cast<int>(dac1step) << ")"
		   << " vs. DAC " << static_cast<int>(dac2reg) 
		   << " from " << static_cast<int>(dac2min) 
		   << " to " << static_cast<int>(dac2max)
		   << " (step size " << static_cast<int>(dac2step) << ")";
  estimateDataVolume(expected, roci2cs.size());

  // Prepare for data acquisition:
  daqStart(flags,deser160phase);
  timer t;

  // Call the RPC command containing the trigger loop:
  bool done = false;
  std::vector<Event> data = std::vector<Event>();
  while(!done) {
    done = _testboard->LoopMultiRocOnePixelDacDacScan(roci2cs, column, row, nTriggers, flags, dac1reg, dac1step, dac1min, dac1max, dac2reg, dac2step, dac2min, dac2max);
    LOG(logDEBUGHAL) << "Loop " << (done ? "finished" : "interrupted") << " (" << t << "ms), reading " << daqBufferStatus() << " words...";
    addCondensedData(data,nTriggers,efficiency,t);
  }
  LOG(logDEBUGHAL) << "Loop done after " << t << "ms. Readout size: " << data.size() << " events.";

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();

  // check for errors in readout (i.e. missing events)
  int missing = expected/nTriggers - data.size();
  if(missing != 0) { 
    LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events.";
    // serious runtime issue as data is invalid and cannot be recovered at this point:
    data.clear();
    throw DataMissingEvent("Incomplete DAQ data readout in function "+std::string(__func__),missing);
  }

  return data;
}

std::vector<Event> hal::SingleRocAllPixelsDacDacScan(uint8_t roci2c, bool efficiency, std::vector<int32_t> parameter) {

  uint8_t dac1reg = static_cast<uint8_t>(parameter.at(0));
  uint8_t dac1min = static_cast<uint8_t>(parameter.at(1));
  uint8_t dac1max = static_cast<uint8_t>(parameter.at(2));
  uint8_t dac2reg = static_cast<uint8_t>(parameter.at(3));
  uint8_t dac2min = static_cast<uint8_t>(parameter.at(4));
  uint8_t dac2max = static_cast<uint8_t>(parameter.at(5));
  uint16_t flags = static_cast<uint16_t>(parameter.at(6));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(7));
  uint8_t dac1step = static_cast<uint8_t>(parameter.at(8));
  uint8_t dac2step = static_cast<uint8_t>(parameter.at(9));

  // We expect one Event per DAC1 value per DAC2 value per trigger per pixel:
  int expected = static_cast<size_t>((dac1max-dac1min)/dac1step+1)*static_cast<size_t>((dac2max-dac2min)/dac2step+1)*nTriggers*ROC_NUMCOLS*ROC_NUMROWS;

  LOG(logDEBUGHAL) << "Called SingleRocAllPixelsDacDacScan with flags " << listFlags(flags) << ", running " << nTriggers << " triggers.";

  LOG(logDEBUGHAL) << "Scanning DAC " << static_cast<int>(dac1reg) 
		   << " from " << static_cast<int>(dac1min) 
		   << " to " << static_cast<int>(dac1max)
		   << " (step size " << static_cast<int>(dac1step) << ")"
		   << " vs. DAC " << static_cast<int>(dac2reg) 
		   << " from " << static_cast<int>(dac2min) 
		   << " to " << static_cast<int>(dac2max)
		   << " (step size " << static_cast<int>(dac2step) << ")";
  estimateDataVolume(expected, 1);

  // Prepare for data acquisition:
  daqStart(flags,deser160phase);
  timer t;

  // Call the RPC command containing the trigger loop:
  bool done = false;
  std::vector<Event> data = std::vector<Event>();
  while(!done) {
    done = _testboard->LoopSingleRocAllPixelsDacDacScan(roci2c, nTriggers, flags, dac1reg, dac1step, dac1min, dac1max, dac2reg, dac2step, dac2min, dac2max);
    LOG(logDEBUGHAL) << "Loop " << (done ? "finished" : "interrupted") << " (" << t << "ms), reading " << daqBufferStatus() << " words...";
    addCondensedData(data,nTriggers,efficiency,t);
  }
  LOG(logDEBUGHAL) << "Loop done after " << t << "ms. Readout size: " << data.size() << " events.";

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();

  // check for errors in readout (i.e. missing events)
  int missing = expected/nTriggers - data.size();
  if(missing != 0) { 
    LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events.";
    // serious runtime issue as data is invalid and cannot be recovered at this point:
    data.clear();
    throw DataMissingEvent("Incomplete DAQ data readout in function "+std::string(__func__),missing);
  }

  return data;
}

std::vector<Event> hal::SingleRocOnePixelDacDacScan(uint8_t roci2c, uint8_t column, uint8_t row, bool efficiency, std::vector<int32_t> parameter) {

  uint8_t dac1reg = static_cast<uint8_t>(parameter.at(0));
  uint8_t dac1min = static_cast<uint8_t>(parameter.at(1));
  uint8_t dac1max = static_cast<uint8_t>(parameter.at(2));
  uint8_t dac2reg = static_cast<uint8_t>(parameter.at(3));
  uint8_t dac2min = static_cast<uint8_t>(parameter.at(4));
  uint8_t dac2max = static_cast<uint8_t>(parameter.at(5));
  uint16_t flags = static_cast<uint16_t>(parameter.at(6));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(7));
  uint8_t dac1step = static_cast<uint8_t>(parameter.at(8));
  uint8_t dac2step = static_cast<uint8_t>(parameter.at(9));

  // We expect one Event per DAC1 value per DAC2 value per trigger:
  int expected = static_cast<size_t>((dac1max-dac1min)/dac1step+1)*static_cast<size_t>((dac2max-dac2min)/dac2step+1)*nTriggers;

  LOG(logDEBUGHAL) << "Called SingleRocOnePixelDacDacScan with flags " << listFlags(flags) << ", running " << nTriggers << " triggers.";

  LOG(logDEBUGHAL) << "Scanning DAC " << static_cast<int>(dac1reg) 
		   << " from " << static_cast<int>(dac1min) 
		   << " to " << static_cast<int>(dac1max)
		   << " (step size " << static_cast<int>(dac1step) << ")"
		   << " vs. DAC " << static_cast<int>(dac2reg) 
		   << " from " << static_cast<int>(dac2min) 
		   << " to " << static_cast<int>(dac2max)
		   << " (step size " << static_cast<int>(dac2step) << ")";
  estimateDataVolume(expected, 1);

  // Prepare for data acquisition:
  daqStart(flags,deser160phase);
  timer t;

  // Call the RPC command containing the trigger loop:
  bool done = false;
  std::vector<Event> data = std::vector<Event>();
  while(!done) {
    done = _testboard->LoopSingleRocOnePixelDacDacScan(roci2c, column, row, nTriggers, flags, dac1reg, dac1step, dac1min, dac1max, dac2reg, dac2step, dac2min, dac2max);
    LOG(logDEBUGHAL) << "Loop " << (done ? "finished" : "interrupted") << " (" << t << "ms), reading " << daqBufferStatus() << " words...";
    addCondensedData(data,nTriggers,efficiency,t);
  }
  LOG(logDEBUGHAL) << "Loop done after " << t << "ms. Readout size: " << data.size() << " events.";

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();

  // check for errors in readout (i.e. missing events)
  int missing = expected/nTriggers - data.size();
  if(missing != 0) { 
    LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events.";
    // serious runtime issue as data is invalid and cannot be recovered at this point:
    data.clear();
    throw DataMissingEvent("Incomplete DAQ data readout in function "+std::string(__func__),missing);
  }

  return data;
}

// Testboard power switches:

void hal::HVon() {
  // Turn on HV and execute (flush):
  LOG(logDEBUGHAL) << "Turning on High Voltage for sensor bias...";
  _testboard->HVon();
  _testboard->Flush();

  // Wait a little and let the HV relais do its job:
  mDelay(400);
}

void hal::HVoff() {
  // Turn off HV and execute (flush):
  _testboard->HVoff();
  _testboard->Flush();
}
 
void hal::Pon() {
  // Turn on DUT power and execute (flush):
  LOG(logDEBUGHAL) << "Powering up testboard DUT connection...";
  _testboard->Pon();
  _testboard->Flush();

  // Clear HAL internal counters:
  m_tbmtype = TBM_NONE;
  m_roctype = ROC_NONE;
  m_roccount = 0;
  m_tokenchains.clear();

  // Wait a little and let the power switch do its job:
  mDelay(300);
}

void hal::Poff() {
  // Turn off DUT power and execute (flush):
  _testboard->Poff();
  _testboard->Flush();
}


// Testboard probe channel selectors:

void hal::SignalProbeD1(uint8_t signal) {
  _testboard->SignalProbeD1(signal);
  _testboard->uDelay(100);
  _testboard->Flush();
}

void hal::SignalProbeD2(uint8_t signal) {
  _testboard->SignalProbeD2(signal);
  _testboard->uDelay(100);
  _testboard->Flush();
}

void hal::SignalProbeDeserD1(uint8_t deser, uint8_t signal) {
  LOG(logDEBUGHAL) << "Setting Deser " << static_cast<int>(deser) << " to signal " << static_cast<int>(signal) << " (D1)";
  _testboard->SignalProbeDeserD1(deser, signal);
  _testboard->uDelay(100);
  _testboard->Flush();
}

void hal::SignalProbeDeserD2(uint8_t deser, uint8_t signal) {
  LOG(logDEBUGHAL) << "Setting Deser " << static_cast<int>(deser) << " to signal " << static_cast<int>(signal) << " (D2)";
  _testboard->SignalProbeDeserD2(deser, signal);
  _testboard->uDelay(100);
  _testboard->Flush();
}

void hal::SignalProbeA1(uint8_t signal) {
  _testboard->SignalProbeA1(signal);
  _testboard->uDelay(100);
  _testboard->Flush();
}

void hal::SignalProbeA2(uint8_t signal) {
  _testboard->SignalProbeA2(signal);
  _testboard->uDelay(100);
  _testboard->Flush();
}

void hal::SignalProbeADC(uint8_t signal, uint8_t gain) {
  _testboard->SignalProbeADC(signal, gain);
  _testboard->uDelay(100);
  _testboard->Flush();
}

void hal::SetClockSource(uint8_t src) {
	_testboard->SetClockSource(src);
	_testboard->uDelay(100);
	_testboard->Flush();
}

bool hal::IsClockPresent() {
  return _testboard->IsClockPresent();
}

void hal::SetClockStretch(uint8_t src, uint16_t delay, uint16_t width) {

  _testboard->SetClockStretch(src, delay, width);
  _testboard->uDelay(100);
  _testboard->Flush();
}

void hal::SigSetMode(uint8_t signal, uint8_t mode) {
    _testboard->Sig_SetMode(signal, mode);
    _testboard->uDelay(100);
    _testboard->Flush();
}

void hal::SigSetPRBS(uint8_t signal, uint8_t speed) {
  _testboard->Sig_SetPRBS(signal, speed);
  _testboard->uDelay(100);
  _testboard->Flush();
}

void hal::SigSetLCDS(){
    _testboard->Sig_SetLCDS();
    _testboard->uDelay(100);
    _testboard->Flush();
}

void hal::SigSetLVDS(){
    _testboard->Sig_SetLVDS();
    _testboard->uDelay(100);
    _testboard->Flush();
}



void hal::daqStart(uint16_t flags, uint8_t deser160phase, uint32_t buffersize) {

  LOG(logDEBUGHAL) << "Starting new DAQ session.";
  for(uint8_t channel = 0; channel < DTB_DAQ_CHANNELS; channel++) { m_daqstatus.push_back(false); }
  
  // Clear all decoder instances:
  for(size_t ch = 0; ch < m_decoder.size(); ch++) { m_decoder.at(ch).Clear(); }

  // Figure out the number of DAQ channels we need:
  if(m_tokenchains.empty()) { m_tokenchains.push_back(m_roccount); }
  
  LOG(logDEBUGHAL) << "Number of token chains: " << m_tokenchains.size();
  if(m_tokenchains.size() > DTB_DAQ_CHANNELS) {
    LOG(logCRITICAL) << "Cannot serve " << m_tokenchains.size() << " DAQ channels, only " << DTB_DAQ_CHANNELS << " available.";
    throw InvalidConfig("Number of requested DAQ channels too high");
  }

    // Split the total buffer size when having more than one channel
  buffersize /= m_tokenchains.size();

  // Open all DAQ channels we need:
  uint8_t rocid_offset = 0;
  for(size_t i = 0; i < m_tokenchains.size(); i++) {
    // Open DAQ in channel i:
    uint32_t allocated_buffer = _testboard->Daq_Open(buffersize, i);
    LOG(logDEBUGHAL) << "Channel " << i << ": token chain: "
				<< static_cast<int>(m_tokenchains.at(i))
				<< " offset " << static_cast<int>(rocid_offset) << " buffer " << allocated_buffer;
    // Initialize the data source, set tokenchain length to zero if no token pass is expected:
    m_src.at(i) = dtbSource(_testboard,( (m_tbmtype == TBM_10C || m_tbmtype == TBM_10D) && m_roccount == 16 ) ? ((i + 6) % 8) : i,m_tokenchains.at(i),rocid_offset,m_tbmtype,m_roctype,true,flags);
    m_src.at(i) >> m_splitter.at(i);
    _testboard->uDelay(100);
    // Increment the ROC id offset by the amount of ROCs expected:
    rocid_offset += m_tokenchains.at(i);
  }

  // Data acquisition with real TBM:
  if(m_tbmtype != TBM_NONE && m_tbmtype != TBM_EMU) {
    // Check if we have all information needed concerning the token chains:
    if(m_tokenchains.size() < 2 || (m_tbmtype >= TBM_09 && m_tokenchains.size() < 4)) {
      LOG(logCRITICAL) << "Invalid number of token chains for TBM type " << std::hex << m_tbmtype << std::dec << ": " << m_tokenchains.size();
      throw InvalidConfig("Invalid token chain setting for selected TBM type");
    }

    // Select the Deser400 as DAQ source:
    LOG(logDEBUGHAL) << "Enabling Deserializer400 for data acquisition.";
    _testboard->Daq_Select_Deser400();
    
    // Reset the Deserializer 400, re-synchronize:
    _testboard->Daq_Deser400_Reset(3);

    // Daq_Select_Deser400() resets the phase selection, allow 150 ms to find a new phase
    _testboard->Flush();
    mDelay(150);

    // If we have an old TBM version set up the DESER400 to read old data format:
    // "old" is everything before TBM08B (so: TBM08, TBM08A)
    if(m_tbmtype < TBM_08A) { 
      LOG(logDEBUGHAL) << "Pre-series TBM with outdated trailer format. Configuring DESER400 accordingly.";
      _testboard->Daq_Deser400_OldFormat(true);
    }
    else { _testboard->Daq_Deser400_OldFormat(false); }
  }
  // Data acquisition without real TBM:
  else {
    // Token chain length: N ROCs for ADC or DESER160 readout.
    LOG(logDEBUGHAL) << "Determined total Token Chain Length: " << static_cast<int>(m_roccount) << " ROCs.";

    if(m_roctype < ROC_PSI46DIG) {
      LOG(logDEBUGHAL) << "Enabling ADC for analog ROC data acquisition."
		       << " Timout: " << static_cast<int>(m_adctimeout)
		       << " Delay Tin/Tout: " << static_cast<int>(m_tindelay)
		       << "/" << static_cast<int>(m_toutdelay);
      _testboard->Daq_Select_ADC(m_adctimeout, 0, m_tindelay, m_toutdelay);
      _testboard->SignalProbeADC(PROBEA_SDATA1, GAIN_4);
      _testboard->uDelay(800); // to stabilize ADC input signal
    }
    else {
      LOG(logDEBUGHAL) << "Enabling Deserializer160 for data acquisition."
		       << " Phase: " << static_cast<int>(deser160phase);
      _testboard->Daq_Select_Deser160(deser160phase);
    }
  }

  // Start all open DAQ channels:
  for(size_t i = 0; i < m_tokenchains.size(); i++) {
    // Start DAQ in channel i:
    _testboard->uDelay(100);
    _testboard->Daq_Start(i);
    m_daqstatus.at(i) = true;
  }
  
  _testboard->uDelay(100);
  _testboard->Flush();
}

Event hal::daqEvent() {

  Event current_Event;
  uint16_t flags = 0;
  
  // Read the next Event from each of the pipes, copy the data:
  for(size_t ch = 0; ch < m_src.size(); ch++) {
    if(m_src.at(ch).isConnected()) {
      dataSink<Event*> Eventpump;
      m_splitter.at(ch) >> m_decoder.at(ch) >> Eventpump;
      // Read the supplied DAQ flags:
      if(ch == 0) { flags = Eventpump.GetFlags(); }

      try { current_Event += *Eventpump.Get(); }
      catch (dsBufferEmpty &) {
	// If nothing has been read yet, just throw DataNoevent:
	if(ch == 0) throw DataNoEvent("No event available");
	
	// Else the previous channels already got data, so we have to retry:
	try { current_Event += *Eventpump.Get(); }
	catch (dsBufferEmpty &) {
	  LOG(logCRITICAL) << "Found data in channel" << (ch > 1 ? std::string("s 0-" + (ch-1)) : std::string(" 0")) << " but not in channel " << ch << "!";
	  throw DataChannelMismatch("No event available in channel " + ch);
	}
      }
      catch (dataPipeException &e) { LOG(logERROR) << e.what(); return current_Event; }
    }
  }

  // Check for the channels all reporting the same event number:
  if((flags & FLAG_DISABLE_EVENTID_CHECK) == 0 && !equalElements(current_Event.triggerCounts())) {
    LOG(logERROR) << "Channels report mismatching event numbers: " << listVector(current_Event.triggerCounts());
    throw DataEventNumberMismatch("Channels report mismatching event numbers: " + listVector(current_Event.triggerCounts()));
  }

  return current_Event;
}

std::vector<Event> hal::daqAllEvents() {

  std::vector<Event> evt;
  uint16_t flags = 0;
  
  // Prepare channel flags:
  std::vector<bool> done_ch;
  for(size_t i = 0; i < m_src.size(); i++) { done_ch.push_back(false); }

  while(1) {
    // Read the next Event from each of the pipes:
    Event current_Event;
    for(size_t ch = 0; ch < m_src.size(); ch++) {
      if(m_src.at(ch).isConnected() && (!done_ch.at(ch))) {
	dataSink<Event*> Eventpump;
	m_splitter.at(ch) >> m_decoder.at(ch) >> Eventpump;

	// Read the supplied DAQ flags:
      if(flags == 0 && ch == 0) { flags = Eventpump.GetFlags(); }

	// Add all event data from this channel:
	try { current_Event += *Eventpump.Get(); }
	catch (dsBufferEmpty &) {
	  LOG(logDEBUGHAL) << "Finished readout Channel " << ch << ".";
	  // Reset the DTB memory to work around buffer issue:
	  _testboard->Daq_MemReset(ch);
	  done_ch.at(ch) = true;
	}
	catch (dataPipeException &e) { LOG(logERROR) << e.what(); return evt; }
      }
      else { done_ch.at(ch) = true; }
    }

    _testboard->Flush();

    // If all readout is finished, return:
    std::vector<bool>::iterator fin = std::find(done_ch.begin(), done_ch.end(), false);
    if(fin == done_ch.end()) {
      LOG(logDEBUGHAL) << "Drained all DAQ channels.";
      break;
    }
    else {
      // Check for the channels all reporting the same event number:
      if((flags & FLAG_DISABLE_EVENTID_CHECK) == 0 && !equalElements(current_Event.triggerCounts())) {
	LOG(logERROR) << "Channels report mismatching event numbers: " << listVector(current_Event.triggerCounts());
	throw DataEventNumberMismatch("Channels report mismatching event numbers: " + listVector(current_Event.triggerCounts()));
      }
      // Store the event
      evt.push_back(current_Event);
    }
  }
  
  if(evt.empty()) throw DataNoEvent("No event available");
  return evt;
}

rawEvent hal::daqRawEvent() {

  rawEvent current_Event;
  
  // Read the next Event from each of the pipes, copy the data:
  for(size_t ch = 0; ch < m_src.size(); ch++) {
    if(m_src.at(ch).isConnected()) {
      dataSink<rawEvent*> rawpump;
      m_splitter.at(ch) >> rawpump;

      try { current_Event += *rawpump.Get(); }
      // One of the channels did not return anything!
      catch (dsBufferEmpty &) {
	// If nothing has been read yet, just throw DataNoevent:
	if(ch == 0) throw DataNoEvent("No event available");
	
	// Else the previous channels already got data, so we have to retry:
	try { current_Event += *rawpump.Get(); }
	catch (dsBufferEmpty &) {
	  LOG(logCRITICAL) << "Found data in channel" << (ch > 1 ? std::string("s 0-" + (ch-1)) : std::string(" 0")) << " but not in channel " << ch << "!";
	  throw DataChannelMismatch("No event available in channel " + ch);
	}
      }
      catch (dataPipeException &e) { LOG(logERROR) << e.what(); return current_Event; }
    }
  }

  return current_Event;
}

std::vector<rawEvent> hal::daqAllRawEvents() {

  std::vector<rawEvent> raw;

  // Prepare channel flags:
  std::vector<bool> done_ch;
  for(size_t i = 0; i < m_src.size(); i++) { done_ch.push_back(false); }

  while(1) {
    // Read the next Event from each of the pipes:
    rawEvent current_Event;
    
    for(size_t ch = 0; ch < m_src.size(); ch++) {
      if(m_src.at(ch).isConnected() && (!done_ch.at(ch))) {
	dataSink<rawEvent*> rawpump;
	m_splitter.at(ch) >> rawpump;
	
	try { current_Event += *rawpump.Get(); }
	catch (dsBufferEmpty &) {
	  LOG(logDEBUGHAL) << "Finished readout Channel " << ch << ".";
	  // Reset the DTB memory to work around buffer issue:
	  _testboard->Daq_MemReset(ch);
	  done_ch.at(ch) = true;
	}
	catch (dataPipeException &e) { LOG(logERROR) << e.what(); return raw; }
      }
      else { done_ch.at(ch) = true; }
    }

    _testboard->Flush();

    // If all readout is finished, return:
    std::vector<bool>::iterator fin = std::find(done_ch.begin(), done_ch.end(), false);
    if(fin == done_ch.end()) {
      LOG(logDEBUGHAL) << "Drained all DAQ channels.";
      break;
    }
    else { raw.push_back(current_Event); }
  }

  if(raw.empty()) throw DataNoEvent("No event available");
  return raw;
}

std::vector<uint16_t> hal::daqBuffer() {

  std::vector<uint16_t> raw;
  
  // Read the full data blob from each of the pipes:
  for(size_t ch = 0; ch < m_src.size(); ch++) {
    if(m_src.at(ch).isConnected()) {
      dataSink<uint16_t> rawpump;
      m_src.at(ch) >> rawpump;
      try { while(1) { raw.push_back(rawpump.Get()); } }
      catch (dsBufferEmpty &) {
	LOG(logDEBUGHAL) << "Finished readout Channel " << ch << ".";
	// Reset the DTB memory to work around buffer issue:
	_testboard->Daq_MemReset(ch);
      }
      catch (dataPipeException &e) { LOG(logERROR) << e.what(); return raw; }
    }
  }

  _testboard->Flush();
  if(raw.empty()) throw DataNoEvent("No data available");
  return raw;
}

void hal::daqTriggerSource(uint16_t source) {

  // Update the locally cached setting for trigger source:
  _currentTrgSrc = source;

  LOG(logDEBUGHAL) << "Configuring trigger source " << std::hex << source << std::dec;

  // Write new trigger source to DTB:
  _testboard->Trigger_Select(source);
  _testboard->Flush();
}

void hal::daqTriggerGenRandom(uint32_t rate) {

  LOG(logDEBUGHAL) << "Configuring trigger generator with rate " << rate;

  // Activate random generator:
  _testboard->Trigger_SetGenRandom(rate);
}

void hal::daqTriggerGenPeriodic(uint32_t period) {

  LOG(logDEBUGHAL) << "Configuring trigger generator with period " << period;

  // Activate periodic generator:
  _testboard->Trigger_SetGenPeriodic(period);
}

void hal::daqTriggerPgExtern() {

    // Connect the DTB TRG input to the PG trigger input

    LOG(logDEBUGHAL) << "Configuring externally triggered pattern generator";

    _testboard->Pg_Trigger();
    _testboard->Flush();

}

void hal::daqTriggerSingleSignal(uint8_t signal) {

  // Attach the single signal direct source for triggers
  // in addition to the currently active source:
  _testboard->Trigger_Select(TRG_SEL_SINGLE_DIR | _currentTrgSrc);
  _testboard->Flush();

  // Send the requested signal:
  _testboard->Trigger_Send(signal);
  _testboard->Flush();
  
  // Reset the trigger source to cached setting:
  _testboard->Trigger_Select(_currentTrgSrc);
  _testboard->Flush();
}

void hal::daqTrigger(uint32_t nTrig, uint16_t period) {

  LOG(logDEBUGHAL) << "Triggering " << nTrig << "x";
  _testboard->Pg_Triggers(nTrig, period);
  // Push to testboard:
  _testboard->Flush();
}

void hal::daqTriggerLoop(uint16_t period) {
  
  LOG(logDEBUGHAL) << "Trigger loop every " << period << " clock cycles started.";
  _testboard->Pg_Loop(period);
  _testboard->uDelay(20);
  // Push to testboard:
  _testboard->Flush();
}

void hal::daqTriggerLoopHalt() {
  
  LOG(logDEBUGHAL) << "Trigger loop halted.";
  _testboard->Pg_Stop();
  // Push to testboard:
  _testboard->Flush();
}

uint32_t hal::daqBufferStatus() {

  uint32_t buffered_data = 0;
  // Summing up data words in all active DAQ channels:
  for(uint8_t channel = 0; channel < DTB_DAQ_CHANNELS; channel++) {
    if(m_daqstatus.size() > channel && m_daqstatus.at(channel)) {
      buffered_data += _testboard->Daq_GetSize(channel);
      LOG(logDEBUGHAL) << "daqbufferstatus" << static_cast<int>(channel) << ": " << _testboard->Daq_GetSize(channel);
    }
  }
  return buffered_data;
}

statistics hal::daqStatistics() {
  // Read statistics from the active channels:
  statistics errors;
  for(size_t ch = 0; ch < m_decoder.size(); ch++) {
    errors += m_decoder.at(ch).getStatistics();
  }
  return errors;
}

std::vector<std::vector<uint16_t> > hal::daqReadback() {

  // Collect readback values from all decoder instances:
  std::vector<std::vector<uint16_t> > rb;

  for(size_t ch = 0; ch < m_decoder.size(); ch++) {
    std::vector<std::vector<uint16_t> > tmp_rb = m_decoder.at(ch).getReadback();
    rb.insert(rb.end(),tmp_rb.begin(),tmp_rb.end());
  }
  return rb;
}

std::vector<uint8_t> hal::daqXORsum(uint8_t channel) {

  // Collect the XOR sum values from the selected DAQ channel:
  if(channel < m_decoder.size()) return m_decoder.at(channel).getXORsum();
  else return std::vector<uint8_t>();
}

void hal::daqStop() {

  // Stop the Pattern Generator, just in case (also stops Pg_Loop())
  _testboard->Pg_Stop();

  // Calling Daq_Stop here - calling Daq_Close would also trigger
  // a RAM reset (deleting the recorded data)

  // Stopping DAQ for all 8 possible channels
  // FIXME provide daq_stop_all NIOS funktion?
  for(uint8_t channel = 0; channel < DTB_DAQ_CHANNELS; channel++) { _testboard->Daq_Stop(channel); }
  _testboard->uDelay(100);
  _testboard->Flush();

  LOG(logDEBUGHAL) << "Stopped DAQ session.";
}

void hal::daqClear() {

  // Disconnect the data pipes from the DTB:
  for(size_t ch = 0; ch < m_src.size(); ch++) { m_src.at(ch) = dtbSource(); }

  // Running Daq_Close() to delete all data and free allocated RAM:
  LOG(logDEBUGHAL) << "Closing DAQ session, deleting data buffers.";
  for(uint8_t channel = 0; channel < DTB_DAQ_CHANNELS; channel++) { _testboard->Daq_Close(channel); }
  m_daqstatus.clear();
}

std::vector<uint16_t> hal::daqADC(uint8_t analog_probe, uint8_t gain, uint16_t nSample, uint8_t source, uint8_t start, uint8_t stop){
    
  std::vector<uint16_t> data;
  _testboard->SignalProbeADC(analog_probe, gain);
  _testboard->uDelay(100);
  _testboard->Flush();
  if(source==1){
	_testboard->Daq_Select_ADC(nSample, 1, start, stop);
   }else{
	_testboard->Daq_Select_ADC(nSample, 2, start, stop);
  }	   
  _testboard->uDelay(1000);
  _testboard->Flush();
  _testboard->Daq_Open(nSample, 0);
  _testboard->uDelay(10);
  if( source == 1){
    _testboard->Daq_Start(0);
    _testboard->Pg_Single();
  }else if (source==2){
    _testboard->roc_SetDAC(250, 195);
    _testboard->roc_SetDAC(250,  61);
    _testboard->Daq_Start(0);
    _testboard->roc_SetDAC(250, 195);
    _testboard->roc_SetDAC(250,  61);
  }else if ( ((source & 0xf0)== 0xe0) || ((source & 0xf0)==0xf0) ){
 	// tbm readback, notes:
    // readable registers have odd numbers: reg -> reg | 1
    // "When READING data from the TBM, a data byte of all 255 must be sent out. "
    _testboard->Daq_Start(0);
	_testboard->tbm_Set(source | 1, 0xff); 
  }
  _testboard->uDelay(1000);
  _testboard->Daq_Stop(0);
  _testboard->Daq_Read(data, nSample);
  _testboard->Daq_Close(0);
  _testboard->Flush();
  return data;
}

uint16_t hal::GetADC(uint8_t rpc_par1){
  return _testboard->GetADC(rpc_par1);
}

std::vector<Event> hal::condenseTriggers(std::vector<Event> &data, uint16_t nTriggers, bool efficiency) {

  std::vector<Event> packed;

  if(data.size()%nTriggers != 0) {
    LOG(logCRITICAL) << "Data size does not correspond to " << nTriggers << " triggers! Aborting data processing!";
    return packed;
  }

  for(std::vector<Event>::iterator Eventit = data.begin(); Eventit!= data.end(); Eventit += nTriggers) {

    Event evt;
    std::map<pixel,uint16_t> pxcount = std::map<pixel,uint16_t>();
    std::map<pixel,double> pxmean = std::map<pixel,double>();
    std::map<pixel,double> pxm2 = std::map<pixel,double>();

    for(std::vector<Event>::iterator it = Eventit; it != Eventit+nTriggers; ++it) {

      // Loop over all contained pixels:
      for(std::vector<pixel>::iterator pixit = (it)->pixels.begin(); pixit != (it)->pixels.end(); ++pixit) {

	// Check if we have that particular pixel already in:
	std::vector<pixel>::iterator px = std::find_if(evt.pixels.begin(),
						       evt.pixels.end(),
						       findPixelXY(pixit->column(), pixit->row(), pixit->roc()));
	// Pixel is known:
	if(px != evt.pixels.end()) {
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
	  evt.pixels.push_back(*pixit);
	}
      }
    }

    // Calculate mean and variance for the pulse height depending on the
    // number of triggers received:
    if(!efficiency) {
      for(std::vector<pixel>::iterator px = evt.pixels.begin(); px != evt.pixels.end(); ++px) {
	px->setValue(pxmean[*px]); // The mean
	px->setVariance(pxm2[*px]/(pxcount[*px] - 1)); // The variance
      }
    }
    packed.push_back(evt);
  }

  // Clean up the dangling pointers in the vector:
  data.clear();
  return packed;
}

void hal::addCondensedData(std::vector<Event> &data, uint16_t nTriggers, bool efficiency, timer t) {

  std::vector<Event> tmpdata = std::vector<Event>();
  try {
    tmpdata = daqAllEvents();
    tmpdata = condenseTriggers(tmpdata, nTriggers, efficiency);
    data.insert(data.end(),tmpdata.begin(),tmpdata.end());
    LOG(logDEBUGHAL) << (tmpdata.size()*nTriggers) << " events read and condensed (" << t << "ms), "
		     << data.size() << " events buffered.";
    LOG(logINFO) << (data.size()*nTriggers) << " events read in total (" << t << "ms).";
  }
  catch(DataNoEvent) {}
  catch(DataException &e) {
    LOG(logCRITICAL) << "Error in DAQ: " << e.what() << " Aborting test.";
    throw e;
  }
}
