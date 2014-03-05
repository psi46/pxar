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
  nTBMs(0),
  deser160phase(4),
  rocType(0),
  hubId(31)
{

  // Get a new CTestboard class instance:
  _testboard = new CTestboard();

  // Check if any boards are connected:
  if(!FindDTB(name)) throw CRpcError(CRpcError::READ_ERROR);

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
    }
  }
  else {
    // USB port cannot be accessed correctly, most likely access right issue:
    LOG(logCRITICAL) << "USB error: " << _testboard->ConnectionError();
    LOG(logCRITICAL) << "DTB: could not open port to device.";
    LOG(logCRITICAL) << "Make sure you have permission to access USB devices.";
    LOG(logCRITICAL) << "(see documentation for UDEV configuration examples)";
  }
}

hal::~hal() {
  // Shut down and close the testboard connection on destruction of HAL object:
  
  // Turn High Voltage off:
  _testboard->HVoff();

  // Turn DUT power off:
  _testboard->Poff();

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

void hal::initTestboard(std::map<uint8_t,uint8_t> sig_delays, std::vector<std::pair<uint16_t,uint8_t> > pg_setup, double va, double vd, double ia, double id) {

  // Set voltages and current limits:
  setTBva(va);
  setTBvd(vd);
  setTBia(ia);
  setTBid(id);
  _testboard->Flush();
  LOG(logDEBUGHAL) << "Voltages/current limits set.";


  // Write testboard delay settings and deserializer phases to the repsective registers:
  for(std::map<uint8_t,uint8_t>::iterator sigIt = sig_delays.begin(); sigIt != sig_delays.end(); ++sigIt) {

    if(sigIt->first == SIG_DESER160PHASE) {
      LOG(logDEBUGHAL) << "Set DTB deser160 phase to value " << static_cast<int>(sigIt->second);
      _testboard->Daq_Select_Deser160(sigIt->second);
      // FIXME
      deser160phase = sigIt->second;
    }
    else {
      LOG(logDEBUGHAL) << "Set DTB delay " << static_cast<int>(sigIt->first) << " to value " << static_cast<int>(sigIt->second);
      _testboard->Sig_SetDelay(sigIt->first, sigIt->second);
      // Also set the signal level, all levels default to 15 (highest) for now:
      _testboard->Sig_SetLevel(sigIt->first, 15);
    }
  }
  _testboard->Flush();
  LOG(logDEBUGHAL) << "Testboard delays set.";


  // Set up Pattern Generator:
  SetupPatternGenerator(pg_setup);

  // We are ready for operations now, mark the HAL as initialized:
  _initialized = true;
}

void hal::SetupPatternGenerator(std::vector<std::pair<uint16_t,uint8_t> > pg_setup) {

  // Write the (sorted!) PG patterns into adjacent register addresses:
  uint8_t addr = 0;
  for(std::vector<std::pair<uint16_t,uint8_t> >::iterator it = pg_setup.begin(); it != pg_setup.end(); ++it) {
    uint16_t cmd = (*it).first | (*it).second;
    LOG(logDEBUGHAL) << "Setting PG cmd " << std::hex << cmd << std::dec 
		     << " (addr " << static_cast<int>(addr) << " pat " << std::hex << static_cast<int>((*it).first) << std::dec
		     << " del " << static_cast<int>((*it).second) << ")";
    _testboard->Pg_SetCmd(addr, cmd);
    addr++;
  }

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
    LOG(logINFO) << "DO NOT INTERUPT DTB POWER !";
    LOG(logINFO) << "Wait till LEDs goes off.";
    LOG(logINFO) << "Power-cycle the DTB.";
    _testboard->UpgradeExec(recordCount);
    _testboard->Flush();
    return true;
  }

  LOG(logCRITICAL) << "ERROR UPGRADE: Could not upgrade this DTB version!";
  return false;
}

void hal::initTBM(uint8_t tbmId, std::map< uint8_t,uint8_t > regVector) {

  // Turn the TBM on:
  _testboard->tbm_Enable(true);
  // FIXME
  nTBMs++;

  // FIXME Beat: 31 is default hub address for the new modules:
  LOG(logDEBUGHAL) << "Module addr is " << static_cast<int>(hubId) << ".";

  _testboard->mod_Addr(hubId);
  _testboard->Flush();

  // Program all registers according to the configuration data:
  LOG(logDEBUGHAL) << "Setting register vector for TBM " << static_cast<int>(tbmId) << ".";
  tbmSetRegs(tbmId,regVector);
}

void hal::initROC(uint8_t rocId, uint8_t type, std::map< uint8_t,uint8_t > dacVector) {

  // Set the pixel address inverted flag if we have the PSI46digV1 chip
  if(type == ROC_PSI46DIG || type == ROC_PSI46DIG_TRIG) {
    LOG(logDEBUGHAL) << "Pixel address is inverted in this ROC type.";
  }
  // FIXME
  rocType = type;

  // Programm all DAC registers according to the configuration data:
  LOG(logDEBUGHAL) << "Setting DAC vector for ROC " << static_cast<int>(rocId) << ".";
  rocSetDACs(rocId,dacVector);
}

void hal::PrintInfo() {
  std::string info;
  _testboard->GetInfo(info);
  LOG(logINFO) << "DTB startup information" << std::endl 
	       << "--- DTB info------------------------------------------" << std::endl
	       << info
	       << "------------------------------------------------------";
}

void hal::mDelay(uint32_t ms) {
  // Wait for the given time in milliseconds:
#ifdef WIN32
  Sleep(ms);
#else
  usleep(ms*1000);
#endif
}

bool hal::CheckCompatibility(){

  std::string dtb_hashcmd;
  uint32_t hostCmdHash = 0, dtbCmdHash = 0;

  // This is a legacy check for boards with an old firmware not featuring the hash function:
  _testboard->GetRpcCallName(5,dtb_hashcmd);
  if(dtb_hashcmd.compare("GetRpcCallHash$I") != 0) {
    LOG(logCRITICAL) << "Your DTB flash file is outdated, it does not proved a RPC hash value for compatibility checks.";
  }
  else {
    // Get hash for the Host RPC command list:
    LOG(logDEBUGHAL) << "Hashing Host RPC command list.";
    hostCmdHash = GetHashForStringVector(_testboard->GetHostRpcCallNames());
    LOG(logDEBUGHAL) << "Host Hash: " << hostCmdHash;

    // Get hash for the DTB RPC command list:
    LOG(logDEBUGHAL) << "Fetching DTB RPC command hash.";
    dtbCmdHash = _testboard->GetRpcCallHash();
    LOG(logDEBUGHAL) << "Host Hash: " << dtbCmdHash;
  }

  // If they don't match check RPC calls one by one and print offenders:
  if(dtb_hashcmd.compare("GetRpcCallHash$I") != 0 || dtbCmdHash != hostCmdHash) {
    LOG(logCRITICAL) << "RPC Call hashes of DTB and Host do not match!";

    // Get the number of RPC calls available on both ends:
    int32_t dtb_callcount = _testboard->GetRpcCallCount();
    int32_t host_callcount = _testboard->GetHostRpcCallCount();

    for(int id = 0; id < std::max(dtb_callcount,host_callcount); id++) {
      std::string dtb_callname;
      std::string host_callname;

      if(id < dtb_callcount) {_testboard->GetRpcCallName(id,dtb_callname);}
      if(id < host_callcount) {_testboard->GetHostRpcCallName(id,host_callname);}

      if(dtb_callname.compare(host_callname) != 0) {
	LOG(logDEBUGHAL) << "ID " << id 
			 << ": (DTB) \"" << dtb_callname 
			 << "\" != (Host) \"" << host_callname << "\"";
      }
    }

    // For now, just print a message and don't to anything else:
    LOG(logCRITICAL) << "Please update your DTB with the correct flash file.";
    LOG(logCRITICAL) << "Get Firmware " << PACKAGE_FIRMWARE << " from " << PACKAGE_FIRMWARE_URL;
    return false;
  }
  else { LOG(logINFO) << "RPC call hashes of host and DTB match: " << hostCmdHash; }

  // We are though all checks, testboard is successfully connected:
  return true;
}

bool hal::FindDTB(std::string &usbId) {

  // Find attached USB devices that match the DTB naming scheme:
  std::string name;
  std::vector<std::string> devList;
  unsigned int nDev;
  unsigned int nr;

  try {
    if (!_testboard->EnumFirst(nDev)) throw int(1);
    for (nr=0; nr<nDev; nr++)	{
      if (!_testboard->EnumNext(name)) continue;
      if (name.size() < 4) continue;
      if (name.compare(0, 4, "DTB_") == 0) devList.push_back(name);
    }
  }
  catch (int e) {
    switch (e) {
    case 1: LOG(logCRITICAL) << "Cannot access the USB driver\n"; return false;
    default: return false;
    }
  }

  if (devList.size() == 0) {
    LOG(logCRITICAL) << "No DTB connected.\n";
    return false;
  }

  if (devList.size() == 1) {
    usbId = devList[0];
    return true;
  }

  // If more than 1 connected device list them
  LOG(logINFO) << "\nConnected DTBs:\n";
  for (nr=0; nr<devList.size(); nr++) {
    LOG(logINFO) << nr << ":" << devList[nr];
    if (_testboard->Open(devList[nr], false)) {
      try {
	unsigned int bid = _testboard->GetBoardId();
	LOG(logINFO) << "  BID=" << bid;
      }
      catch (...) {
	LOG(logERROR) << "  Not identifiable\n";
      }
      _testboard->Close();
    }
    else LOG(logWARNING) << " - in use\n";
  }

  LOG(logINFO) << "Please choose DTB (0-" << (nDev-1) << "): ";
  char choice[8];
  if(fgets(choice, 8, stdin) == NULL) return false;
  sscanf (choice, "%ud", &nr);
  if (nr >= devList.size()) {
    nr = 0;
    LOG(logCRITICAL) << "No DTB opened\n";
    return false;
  }

  // Return the selected DTB's USB id as reference string:
  usbId = devList[nr];
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


bool hal::rocSetDACs(uint8_t rocId, std::map< uint8_t, uint8_t > dacPairs) {

  // Make sure we are writing to the correct ROC by setting the I2C address:
  _testboard->roc_I2cAddr(rocId);

  // Iterate over all DAC id/value pairs and set the DAC
  for(std::map< uint8_t,uint8_t >::iterator it = dacPairs.begin(); it != dacPairs.end(); ++it) {
    LOG(logDEBUGHAL) << "Set DAC" << static_cast<int>(it->first) << " to " << static_cast<int>(it->second);
    _testboard->roc_SetDAC(it->first,it->second);
  }

  // Send all queued commands to the testboard:
  _testboard->Flush();
  // Everything went all right:
  return true;
}

bool hal::rocSetDAC(uint8_t rocId, uint8_t dacId, uint8_t dacValue) {

  // Make sure we are writing to the correct ROC by setting the I2C address:
  _testboard->roc_I2cAddr(rocId);

  LOG(logDEBUGHAL) << "Set DAC" << static_cast<int>(dacId) << " to " << static_cast<int>(dacValue);
  _testboard->roc_SetDAC(dacId,dacValue);
  return true;
}

bool hal::tbmSetRegs(uint8_t tbmId, std::map< uint8_t, uint8_t > regPairs) {

  // Iterate over all register id/value pairs and set them
  for(std::map< uint8_t,uint8_t >::iterator it = regPairs.begin(); it != regPairs.end(); ++it) {
    // One of the register settings had an issue, abort:
    if(!tbmSetReg(tbmId, it->first, it->second)) return false;
  }

  // Send all queued commands to the testboard:
  _testboard->Flush();
  // Everything went all right:
  return true;
}

bool hal::tbmSetReg(uint8_t tbmId, uint8_t regId, uint8_t regValue) {
  // FIXME currently only one TBM supported...

  // Make sure we are writing to the correct TBM by setting its sddress:
  // FIXME Magic from Beat, need to understand this and be able to program also the second TBM:
  _testboard->mod_Addr(hubId);

  LOG(logDEBUGHAL) << "Set Reg" << std::hex << static_cast<int>(regId) << std::dec << " to " << std::hex << static_cast<int>(regValue) << std::dec << " for both TBM cores.";
  // Set this register for both TBM cores:
  uint8_t regCore1 = 0xE0 | regId;
  uint8_t regCore2 = 0xF0 | regId;
  LOG(logDEBUGHAL) << "Core " << tbmId << " : register " << std::hex << static_cast<int>(regCore1) << " = " << static_cast<int>(regValue) << std::dec;
//  LOG(logDEBUGHAL) << "Core 2: register " << std::hex << static_cast<int>(regCore2) << " = " << static_cast<int>(regValue) << std::dec;

  _testboard->tbm_Set(regCore1,regValue);
  _testboard->tbm_Set(regCore2,regValue);
  return true;
}

void hal::RocSetMask(uint8_t rocid, bool mask, std::vector<pixelConfig> pixels) {

  _testboard->roc_I2cAddr(rocid);
  
  // Check if we want to mask or unmask&trim:
  if(mask) {
    // This is quite easy:
    LOG(logDEBUGHAL) << "Masking ROC " << static_cast<int>(rocid);

    // Mask the PUC and detach all DC from their readout (both done on NIOS):
    _testboard->roc_Chip_Mask();
  }
  else {
    // We really want to enable that full thing:
    LOG(logDEBUGHAL) << "Updating mask bits & trim values of ROC " << static_cast<int>(rocid);

    // Prepare configuration of the pixels, linearize vector:
    std::vector<int16_t> trim;
    // Set default trim value to 15:
    for(size_t i = 0; i < ROC_NUMCOLS*ROC_NUMROWS; i++) { trim.push_back(15); }
    for(std::vector<pixelConfig>::iterator pxIt = pixels.begin(); pxIt != pixels.end(); ++pxIt) {
      size_t position = (*pxIt).column*ROC_NUMROWS + (*pxIt).row;
      trim[position] = (*pxIt).trim;
    }

    // FIXME we can do that in the TrimChip function on NIOS!
    // Detach all Double Columns from their readout:
    for(uint8_t col = 0; col < ROC_NUMCOLS; col++) {
      _testboard->roc_Col_Enable(col,true);
    }
    
    // Trim the whole ROC:
    _testboard->TrimChip(trim);
  }
}

void hal::PixelSetMask(uint8_t rocid, uint8_t column, uint8_t row, bool mask, uint8_t trim) {

  _testboard->roc_I2cAddr(rocid);

  // Check if we want to mask or unmask&trim:
  if(mask) {
    LOG(logDEBUGHAL) << "Masking pixel " << static_cast<int>(column) << "," << static_cast<int>(row)
		     << " on ROC " << static_cast<int>(rocid);
    _testboard->roc_Pix_Mask(column, row);
  }
  else {
    LOG(logDEBUGHAL) << "Trimming pixel " << static_cast<int>(column) << "," << static_cast<int>(row)
		     << " (" << static_cast<int>(trim) << ")";
    _testboard->roc_Pix_Trim(column,row,trim);
  }
}

void hal::ColumnSetEnable(uint8_t rocid, uint8_t column, bool enable) {

  // Set the correct ROC I2C address:
  _testboard->roc_I2cAddr(rocid);

  // Set the Col Enable bit:
  LOG(logDEBUGHAL) << "Setting Column " << static_cast<int>(column) << " enable bit to " << static_cast<int>(enable);
  _testboard->roc_Col_Enable(column,enable);
}

void hal::PixelSetCalibrate(uint8_t rocid, uint8_t column, uint8_t row, uint16_t flags) {

  // Set the correct ROC I2C address:
  _testboard->roc_I2cAddr(rocid);

  // Set the calibrate bit and the CALS setting:
  _testboard->roc_Pix_Cal(column,row,flags&FLAG_CALS);
}

void hal::RocClearCalibrate(uint8_t rocid) {

  // Set the correct ROC I2C address:
  _testboard->roc_I2cAddr(rocid);

  LOG(logDEBUGHAL) << "Clearing calibrate signal for ROC " << static_cast<int>(rocid);
 _testboard->roc_ClrCal();
}


// ---------------- TEST FUNCTIONS ----------------------

std::vector<Event*> hal::MultiRocAllPixelsCalibrate(std::vector<uint8_t> rocids, std::vector<int32_t> parameter) {

  uint16_t flags = static_cast<uint16_t>(parameter.at(0));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(1));

  LOG(logDEBUGHAL) << "Called MultiRocAllPixelsCalibrate with flags " << static_cast<int>(flags) << ", running " << nTriggers << " triggers.";
  LOG(logDEBUGHAL) << "Function will take care of all pixels on " << rocids.size() << " ROCs with the I2C addresses:";
  LOG(logDEBUGHAL) << listVector(rocids);

  // Prepare for data acquisition:
  daqStart(deser160phase,nTBMs);
  timer t;

  // Call the RPC command containing the trigger loop:
  _testboard->LoopMultiRocAllPixelsCalibrate(rocids, nTriggers, flags);
  LOG(logDEBUGHAL) << "Loop finished, " << daqBufferStatus() << " words in buffer, loop took " << t << "ms.";

  std::vector<Event*> data = daqAllEvents();
  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events, loop+readout took " << t << "ms.";
  // We expect one Event per pixel per trigger, all ROCs are triggered in parallel:
  int missing = nTriggers*ROC_NUMROWS*ROC_NUMCOLS - data.size();
  if(missing != 0) { LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events."; }

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();
  return data;
}

std::vector<Event*> hal::MultiRocOnePixelCalibrate(std::vector<uint8_t> rocids, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  uint16_t flags = static_cast<uint16_t>(parameter.at(0));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(1));

  LOG(logDEBUGHAL) << "Called MultiRocOnePixelCalibrate with flags " << static_cast<int>(flags) << ", running " << nTriggers << " triggers.";
  LOG(logDEBUGHAL) << "Function will take care of pixel " << static_cast<int>(column) << "," 
		   << static_cast<int>(row) << " on "
		   << rocids.size() << " ROCs with the I2C addresses:";
  LOG(logDEBUGHAL) << listVector(rocids);

  // Prepare for data acquisition:
  daqStart(deser160phase,nTBMs);
  timer t;

  // Call the RPC command containing the trigger loop:
  _testboard->LoopMultiRocOnePixelCalibrate(rocids, column, row, nTriggers, flags);
  LOG(logDEBUGHAL) << "Loop finished, " << daqBufferStatus() << " words in buffer, loop took " << t << "ms.";

  std::vector<Event*> data = daqAllEvents();
  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events, loop+readout took " << t << "ms.";
  // We expect one Event per trigger, all ROCs are triggered in parallel:
  int missing = nTriggers - data.size();
  if(missing != 0) { LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events."; }

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();

  return data;
}

std::vector<Event*> hal::SingleRocAllPixelsCalibrate(uint8_t rocid, std::vector<int32_t> parameter) {

  uint16_t flags = static_cast<uint16_t>(parameter.at(0));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(1));

  int expected = nTriggers*ROC_NUMROWS*ROC_NUMCOLS;

  LOG(logDEBUGHAL) << "Called SingleRocAllPixelsCalibrate with flags " << static_cast<int>(flags) << ", running " << nTriggers << " triggers on I2C " << static_cast<int>(rocid) << ".";
  LOG(logDEBUGHAL) << "Expecting " << expected << " events.";

  // Prepare for data acquisition:
  daqStart(deser160phase,nTBMs);
  timer t;

  // Call the RPC command containing the trigger loop:
  _testboard->LoopSingleRocAllPixelsCalibrate(rocid, nTriggers, flags);
  LOG(logDEBUGHAL) << "Loop finished, " << daqBufferStatus() << " words in buffer, loop took " << t << "ms.";

  std::vector<Event*> data = daqAllEvents();
  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events, loop+readout took " << t << "ms.";
  // We are expecting one Event per pixel per trigger, only one ROC is triggered:
  int missing = expected - data.size();
  if(missing != 0) { LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events."; }

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();
  return data;
}

std::vector<Event*> hal::SingleRocOnePixelCalibrate(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  uint16_t flags = static_cast<uint16_t>(parameter.at(0));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(1));

  LOG(logDEBUGHAL) << "Called SingleRocOnePixelCalibrate for pixel " << static_cast<int>(column) << ","
		   << static_cast<int>(row) << " with flags " << static_cast<int>(flags) << ", running "
		   << nTriggers << " triggers.";

 // Prepare for data acquisition:
  daqStart(deser160phase,nTBMs);
  timer t;

  // Call the RPC command containing the trigger loop:
  _testboard->LoopSingleRocOnePixelCalibrate(rocid, column, row, nTriggers, flags);
  LOG(logDEBUGHAL) << "Loop finished, " << daqBufferStatus() << " words in buffer, loop took " << t << "ms.";

  std::vector<Event*> data = daqAllEvents();
  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events, loop+readout took " << t << "ms.";
  // We are expecting one Event per trigger:
  int missing = nTriggers - data.size();
  if(missing != 0) { LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events."; }

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();

  return data;
}


std::vector<Event*> hal::MultiRocAllPixelsDacScan(std::vector<uint8_t> rocids, std::vector<int32_t> parameter) {

  uint8_t dacreg = static_cast<uint8_t>(parameter.at(0));
  uint8_t dacmin = static_cast<uint8_t>(parameter.at(1));
  uint8_t dacmax = static_cast<uint8_t>(parameter.at(2));
  uint16_t flags = static_cast<uint16_t>(parameter.at(3));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(4));

 LOG(logDEBUGHAL) << "Called MultiRocAllPixelsDacScan with flags " << static_cast<int>(flags) << ", running " << nTriggers << " triggers.";
  LOG(logDEBUGHAL) << "Function will take care of all pixels on " << rocids.size() << " ROCs with the I2C addresses:";
  LOG(logDEBUGHAL) << listVector(rocids);
  LOG(logDEBUGHAL) << "Scanning DAC " << static_cast<int>(dacreg) 
		   << " from " << static_cast<int>(dacmin) 
		   << " to " << static_cast<int>(dacmax);

 // Prepare for data acquisition:
  daqStart(deser160phase,nTBMs);
  timer t;

  // Call the RPC command containing the trigger loop:
  _testboard->LoopMultiRocAllPixelsDacScan(rocids, nTriggers, flags, dacreg, dacmin, dacmax);
  LOG(logDEBUGHAL) << "Loop finished, " << daqBufferStatus() << " words in buffer, loop took " << t << "ms.";

  std::vector<Event*> data = daqAllEvents();
  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events, loop+readout took " << t << "ms.";
  // We are expecting one Event per DAC setting per trigger per pixel:
 // int missing = static_cast<size_t>(dacmax-dacmin+1)*nTriggers*ROC_NUMCOLS*ROC_NUMROWS - data.size();
  int missing = static_cast<size_t>(dacmax-dacmin+1)*nTriggers*ROC_NUMCOLS - data.size();
  if(missing != 0) { LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events."; }

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();
  return data;
}

std::vector<Event*> hal::MultiRocOnePixelDacScan(std::vector<uint8_t> rocids, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  uint8_t dacreg = static_cast<uint8_t>(parameter.at(0));
  uint8_t dacmin = static_cast<uint8_t>(parameter.at(1));
  uint8_t dacmax = static_cast<uint8_t>(parameter.at(2));
  uint16_t flags = static_cast<uint16_t>(parameter.at(3));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(4));

 LOG(logDEBUGHAL) << "Called MultiRocOnePixelDacScan with flags " << static_cast<int>(flags) << ", running " << nTriggers << " triggers.";
  LOG(logDEBUGHAL) << "Function will take care of pixel " << static_cast<int>(column) << "," 
		   << static_cast<int>(row) << " on "
		   << rocids.size() << " ROCs with the I2C addresses:";
  LOG(logDEBUGHAL) << listVector(rocids);
  LOG(logDEBUGHAL) << "Scanning DAC " << static_cast<int>(dacreg) 
		   << " from " << static_cast<int>(dacmin) 
		   << " to " << static_cast<int>(dacmax);

 // Prepare for data acquisition:
  daqStart(deser160phase,nTBMs);
  timer t;

  // Call the RPC command containing the trigger loop:
  _testboard->LoopMultiRocOnePixelDacScan(rocids, column, row, nTriggers, flags, dacreg, dacmin, dacmax);
  LOG(logDEBUGHAL) << "Loop finished, " << daqBufferStatus() << " words in buffer, loop took " << t << "ms.";

  std::vector<Event*> data = daqAllEvents();
  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events, loop+readout took " << t << "ms.";
  // We expect one Event per DAC value per trigger:
  int missing = static_cast<size_t>(dacmax-dacmin+1)*nTriggers - data.size();
  if(missing != 0) { LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events."; }

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();
  return data;
}

std::vector<Event*> hal::SingleRocAllPixelsDacScan(uint8_t rocid, std::vector<int32_t> parameter) {

  uint8_t dacreg = static_cast<uint8_t>(parameter.at(0));
  uint8_t dacmin = static_cast<uint8_t>(parameter.at(1));
  uint8_t dacmax = static_cast<uint8_t>(parameter.at(2));
  uint16_t flags = static_cast<uint16_t>(parameter.at(3));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(4));

  int expected = static_cast<size_t>(dacmax-dacmin+1)*nTriggers*ROC_NUMCOLS*ROC_NUMROWS;

  LOG(logDEBUGHAL) << "Called SingleRocAllPixelsDacScan with flags " << static_cast<int>(flags) << ", running " << nTriggers << " triggers.";
  LOG(logDEBUGHAL) << "Scanning DAC " << static_cast<int>(dacreg) 
		   << " from " << static_cast<int>(dacmin) 
		   << " to " << static_cast<int>(dacmax);
  LOG(logDEBUGHAL) << "Expecting " << expected << " events.";

 // Prepare for data acquisition:
  daqStart(deser160phase,nTBMs);
  timer t;

  // Call the RPC command containing the trigger loop:
  _testboard->LoopSingleRocAllPixelsDacScan(rocid, nTriggers, flags, dacreg, dacmin, dacmax);
  LOG(logDEBUGHAL) << "Loop finished, " << daqBufferStatus() << " words in buffer, loop took " << t << "ms.";

  std::vector<Event*> data = daqAllEvents();
  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events, loop+readout took " << t << "ms.";
  // We expect one Event per DAC value per trigger per pixel:
  int missing = expected - data.size();
  if(missing != 0) { LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events."; }

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();
  return data;
}

std::vector<Event*> hal::SingleRocOnePixelDacScan(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  uint8_t dacreg = static_cast<uint8_t>(parameter.at(0));
  uint8_t dacmin = static_cast<uint8_t>(parameter.at(1));
  uint8_t dacmax = static_cast<uint8_t>(parameter.at(2));
  uint16_t flags = static_cast<uint16_t>(parameter.at(3));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(4));

  LOG(logDEBUGHAL) << "Called SingleRocOnePixelDacScan with flags " << static_cast<int>(flags) << ", running " << nTriggers << " triggers.";
  LOG(logDEBUGHAL) << "Scanning DAC " << static_cast<int>(dacreg) 
		   << " from " << static_cast<int>(dacmin) 
		   << " to " << static_cast<int>(dacmax);

 // Prepare for data acquisition:
  daqStart(deser160phase,nTBMs);
  timer t;

  // Call the RPC command containing the trigger loop:
  _testboard->LoopSingleRocOnePixelDacScan(rocid, column, row, nTriggers, flags, dacreg, dacmin, dacmax);
  LOG(logDEBUGHAL) << "Loop finished, " << daqBufferStatus() << " words in buffer, loop took " << t << "ms.";

  std::vector<Event*> data = daqAllEvents();
  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events, loop+readout took " << t << "ms.";
  // We expect one Event per DAC value per trigger:
  int missing = static_cast<size_t>(dacmax-dacmin+1)*nTriggers - data.size();
  if(missing != 0) { LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events."; }

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();
  return data;
}

std::vector<Event*> hal::MultiRocAllPixelsDacDacScan(std::vector<uint8_t> rocids, std::vector<int32_t> parameter) {

  uint8_t dac1reg = static_cast<uint8_t>(parameter.at(0));
  uint8_t dac1min = static_cast<uint8_t>(parameter.at(1));
  uint8_t dac1max = static_cast<uint8_t>(parameter.at(2));
  uint8_t dac2reg = static_cast<uint8_t>(parameter.at(3));
  uint8_t dac2min = static_cast<uint8_t>(parameter.at(4));
  uint8_t dac2max = static_cast<uint8_t>(parameter.at(5));
  uint16_t flags = static_cast<uint16_t>(parameter.at(6));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(7));

 LOG(logDEBUGHAL) << "Called MultiRocAllPixelsDacDacScan with flags " << static_cast<int>(flags) << ", running " << nTriggers << " triggers.";
  LOG(logDEBUGHAL) << "Function will take care of all pixels on " << rocids.size() << " ROCs with the I2C addresses:";
  LOG(logDEBUGHAL) << listVector(rocids);
  LOG(logDEBUGHAL) << "Scanning DAC " << static_cast<int>(dac1reg) 
		   << " from " << static_cast<int>(dac1min) 
		   << " to " << static_cast<int>(dac1max)
		   << " vs. DAC " << static_cast<int>(dac2reg) 
		   << " from " << static_cast<int>(dac2min) 
		   << " to " << static_cast<int>(dac2max);

  // Prepare for data acquisition:
  daqStart(deser160phase,nTBMs);
  timer t;

  // Call the RPC command containing the trigger loop:
  _testboard->LoopMultiRocAllPixelsDacDacScan(rocids, nTriggers, flags, dac1reg, dac1min, dac1max, dac2reg, dac2min, dac2max);
  LOG(logDEBUGHAL) << "Loop finished, " << daqBufferStatus() << " words in buffer, loop took " << t << "ms.";

  std::vector<Event*> data = daqAllEvents();
  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events, loop+readout took " << t << "ms.";
  // We expect one Event per DAC1 value per DAC2 value per trigger per pixel:
  int missing = static_cast<size_t>(dac1max-dac1min+1)*static_cast<size_t>(dac2max-dac2min+1)*nTriggers*ROC_NUMROWS*ROC_NUMCOLS - data.size();
  if(missing != 0) { LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events."; }

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();
  return data;
}

std::vector<Event*> hal::MultiRocOnePixelDacDacScan(std::vector<uint8_t> rocids, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  uint8_t dac1reg = static_cast<uint8_t>(parameter.at(0));
  uint8_t dac1min = static_cast<uint8_t>(parameter.at(1));
  uint8_t dac1max = static_cast<uint8_t>(parameter.at(2));
  uint8_t dac2reg = static_cast<uint8_t>(parameter.at(3));
  uint8_t dac2min = static_cast<uint8_t>(parameter.at(4));
  uint8_t dac2max = static_cast<uint8_t>(parameter.at(5));
  uint16_t flags = static_cast<uint16_t>(parameter.at(6));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(7));

  LOG(logDEBUGHAL) << "Called MultiRocOnePixelDacDacScan with flags " << static_cast<int>(flags) << ", running " << nTriggers << " triggers.";
  LOG(logDEBUGHAL) << "Function will take care of pixel " << static_cast<int>(column) << "," 
		   << static_cast<int>(row) << " on "
		   << rocids.size() << " ROCs with the I2C addresses:";
  LOG(logDEBUGHAL) << listVector(rocids);
  LOG(logDEBUGHAL) << "Scanning DAC " << static_cast<int>(dac1reg) 
		   << " from " << static_cast<int>(dac1min) 
		   << " to " << static_cast<int>(dac1max)
		   << " vs. DAC " << static_cast<int>(dac2reg) 
		   << " from " << static_cast<int>(dac2min) 
		   << " to " << static_cast<int>(dac2max);

  // Prepare for data acquisition:
  daqStart(deser160phase,nTBMs);
  timer t;

  // Call the RPC command containing the trigger loop:
  _testboard->LoopMultiRocOnePixelDacDacScan(rocids, column, row, nTriggers, flags, dac1reg, dac1min, dac1max, dac2reg, dac2min, dac2max);
  LOG(logDEBUGHAL) << "Loop finished, " << daqBufferStatus() << " words in buffer, loop took " << t << "ms.";

  std::vector<Event*> data = daqAllEvents();
  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events, loop+readout took " << t << "ms.";
  // We expect one Event per DAC1 value per DAC2 value per trigger:
  int missing = static_cast<size_t>(dac1max-dac1min+1)*static_cast<size_t>(dac2max-dac2min+1)*nTriggers - data.size();
  if(missing != 0) { LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events."; }

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();
  return data;
}

std::vector<Event*> hal::SingleRocAllPixelsDacDacScan(uint8_t rocid, std::vector<int32_t> parameter) {

  uint8_t dac1reg = static_cast<uint8_t>(parameter.at(0));
  uint8_t dac1min = static_cast<uint8_t>(parameter.at(1));
  uint8_t dac1max = static_cast<uint8_t>(parameter.at(2));
  uint8_t dac2reg = static_cast<uint8_t>(parameter.at(3));
  uint8_t dac2min = static_cast<uint8_t>(parameter.at(4));
  uint8_t dac2max = static_cast<uint8_t>(parameter.at(5));
  uint16_t flags = static_cast<uint16_t>(parameter.at(6));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(7));

  int expected = static_cast<size_t>(dac1max-dac1min+1)*static_cast<size_t>(dac2max-dac2min+1)*nTriggers*ROC_NUMCOLS*ROC_NUMROWS;

  LOG(logDEBUGHAL) << "Called SingleRocAllPixelsDacDacScan with flags " << static_cast<int>(flags) << ", running " << nTriggers << " triggers.";

  LOG(logDEBUGHAL) << "Scanning DAC " << static_cast<int>(dac1reg) 
		   << " from " << static_cast<int>(dac1min) 
		   << " to " << static_cast<int>(dac1max)
		   << " vs. DAC " << static_cast<int>(dac2reg) 
		   << " from " << static_cast<int>(dac2min) 
		   << " to " << static_cast<int>(dac2max);
  LOG(logDEBUGHAL) << "Expecting " << expected << " events.";

  // Prepare for data acquisition:
  daqStart(deser160phase,nTBMs);
  timer t;

  // Call the RPC command containing the trigger loop:
  _testboard->LoopSingleRocAllPixelsDacDacScan(rocid, nTriggers, flags, dac1reg, dac1min, dac1max, dac2reg, dac2min, dac2max);
  LOG(logDEBUGHAL) << "Loop finished, " << daqBufferStatus() << " words in buffer, loop took " << t << "ms.";

  std::vector<Event*> data = daqAllEvents();
  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events, loop+readout took " << t << "ms.";
  // We expect one Event per DAC1 value per DAC2 value per trigger per pixel:
  int missing = expected - data.size();
  if(missing != 0) { LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events."; }

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();
  return data;
}

std::vector<Event*> hal::SingleRocOnePixelDacDacScan(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  uint8_t dac1reg = static_cast<uint8_t>(parameter.at(0));
  uint8_t dac1min = static_cast<uint8_t>(parameter.at(1));
  uint8_t dac1max = static_cast<uint8_t>(parameter.at(2));
  uint8_t dac2reg = static_cast<uint8_t>(parameter.at(3));
  uint8_t dac2min = static_cast<uint8_t>(parameter.at(4));
  uint8_t dac2max = static_cast<uint8_t>(parameter.at(5));
  uint16_t flags = static_cast<uint16_t>(parameter.at(6));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(7));

  LOG(logDEBUGHAL) << "Called SingleRocOnePixelDacDacScan with flags " << static_cast<int>(flags) << ", running " << nTriggers << " triggers.";

  LOG(logDEBUGHAL) << "Scanning DAC " << static_cast<int>(dac1reg) 
		   << " from " << static_cast<int>(dac1min) 
		   << " to " << static_cast<int>(dac1max)
		   << " vs. DAC " << static_cast<int>(dac2reg) 
		   << " from " << static_cast<int>(dac2min) 
		   << " to " << static_cast<int>(dac2max);

  // Prepare for data acquisition:
  daqStart(deser160phase,nTBMs);
  timer t;

  // Call the RPC command containing the trigger loop:
  _testboard->LoopSingleRocOnePixelDacDacScan(rocid, column, row, nTriggers, flags, dac1reg, dac1min, dac1max, dac2reg, dac2min, dac2max);
  LOG(logDEBUGHAL) << "Loop finished, " << daqBufferStatus() << " words in buffer, loop took " << t << "ms.";

  std::vector<Event*> data = daqAllEvents();
  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events, loop+readout took " << t << "ms.";
  // We expect one Event per DAC1 value per DAC2 value per trigger:
  int missing = static_cast<size_t>(dac1max-dac1min+1)*static_cast<size_t>(dac2max-dac2min+1)*nTriggers - data.size();
  if(missing != 0) { LOG(logCRITICAL) << "Incomplete DAQ data readout! Missing " << missing << " Events."; }

  // Clear & reset the DAQ buffer on the testboard.
  daqStop();
  daqClear();
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

bool hal::daqStart(uint8_t deser160phase, uint8_t nTBMs, uint32_t buffersize) {

  LOG(logDEBUGHAL) << "Starting new DAQ session.";

  // Split the total buffer size when having more than one channel
  if(nTBMs > 0) buffersize /= 2*nTBMs;

  uint32_t allocated_buffer_ch0 = _testboard->Daq_Open(buffersize,0);
  LOG(logDEBUGHAL) << "Allocated buffer size, Channel 0: " << allocated_buffer_ch0;
  src0 = dtbSource(_testboard,0,(nTBMs > 0),rocType,true);
  src0 >> splitter0;

  _testboard->uDelay(100);

  if(nTBMs > 0) {
    LOG(logDEBUGHAL) << "Enabling Deserializer400 for data acquisition.";

    uint32_t allocated_buffer_ch1 = _testboard->Daq_Open(buffersize,1);
    LOG(logDEBUGHAL) << "Allocated buffer size, Channel 1: " << allocated_buffer_ch1;
    src1 = dtbSource(_testboard,1,(nTBMs > 0),rocType,true);
    src1 >> splitter1;


    if(nTBMs > 1) {
      LOG(logDEBUGHAL) << "Two TBMs detected, enabling more DAQ channels.";

      uint32_t allocated_buffer_ch2 = _testboard->Daq_Open(buffersize,2);
      LOG(logDEBUGHAL) << "Allocated buffer size, Channel 2: " << allocated_buffer_ch2;
      src2 = dtbSource(_testboard,2,(nTBMs > 0),rocType,true);
      src2 >> splitter2;

      uint32_t allocated_buffer_ch3 = _testboard->Daq_Open(buffersize,3);
      LOG(logDEBUGHAL) << "Allocated buffer size, Channel 3: " << allocated_buffer_ch3;
      src3 = dtbSource(_testboard,3,(nTBMs > 0),rocType,true);
      src3 >> splitter3;
    }

    // Reset the Deserializer 400, re-synchronize:
    _testboard->Daq_Deser400_Reset(3);

    // Select the Deser400 as DAQ source:
    _testboard->Daq_Select_Deser400();
    _testboard->Daq_Start(1);
  }
  else {
    LOG(logDEBUGHAL) << "Enabling Deserializer160 for data acquisition."
		     << " Phase: " << static_cast<int>(deser160phase);
    _testboard->Daq_Select_Deser160(deser160phase);
  }

  _testboard->Daq_Start(0);
  _testboard->uDelay(100);
  _testboard->Flush();

  return true;
}

Event* hal::daqEvent() {

  Event* current_Event = new Event();

  dataSink<Event*> Eventpump0, Eventpump1, Eventpump2, Eventpump3;
  splitter0 >> decoder0 >> Eventpump0;

  if(src1.isConnected()) { splitter1 >> decoder1 >> Eventpump1; }
  if(src2.isConnected()) { splitter2 >> decoder2 >> Eventpump2; }
  if(src3.isConnected()) { splitter3 >> decoder3 >> Eventpump3; }

  // FIXME check carefully: in principle we expect the same number of triggers
  // (==Events) on each pipe. Throw a critical if difference is found?
  try {
    // Read the next Event from each of the pipes, copy the data:
    *current_Event = *Eventpump0.Get();
    if(src1.isConnected()) {
      Event* tmp = Eventpump1.Get(); 
      current_Event->pixels.insert(current_Event->pixels.end(), tmp->pixels.begin(), tmp->pixels.end());
    }
    if(src2.isConnected()) {
      Event* tmp = Eventpump2.Get(); 
      current_Event->pixels.insert(current_Event->pixels.end(), tmp->pixels.begin(), tmp->pixels.end());
    }
    if(src3.isConnected()) {
      Event* tmp = Eventpump3.Get(); 
      current_Event->pixels.insert(current_Event->pixels.end(), tmp->pixels.begin(), tmp->pixels.end());
    }
  }
  catch (dsBufferEmpty &) { LOG(logDEBUGHAL) << "Finished readout."; }
  catch (dataPipeException &e) { LOG(logERROR) << e.what(); }

  return current_Event;
}

std::vector<Event*> hal::daqAllEvents() {

  std::vector<Event*> evt;

  dataSink<Event*> Eventpump0, Eventpump1, Eventpump2, Eventpump3;
  splitter0 >> decoder0 >> Eventpump0;

  if(src1.isConnected()) { splitter1 >> decoder1 >> Eventpump1; }
  if(src2.isConnected()) { splitter2 >> decoder2 >> Eventpump2; }
  if(src3.isConnected()) { splitter3 >> decoder3 >> Eventpump3; }

  // FIXME check carefully: in principle we expect the same number of triggers
  // (==Events) on each pipe. Throw a critical if difference is found?
  try {
    while(1) {
      // Read the next Event from each of the pipes:
      Event* current_Event = new Event(*Eventpump0.Get());
      if(src1.isConnected()) {
	Event* tmp = Eventpump1.Get(); 
	current_Event->pixels.insert(current_Event->pixels.end(), tmp->pixels.begin(), tmp->pixels.end());
      }
      if(src2.isConnected()) {
	Event* tmp = Eventpump2.Get(); 
	current_Event->pixels.insert(current_Event->pixels.end(), tmp->pixels.begin(), tmp->pixels.end());
      }
      if(src3.isConnected()) {
	Event* tmp = Eventpump3.Get(); 
	current_Event->pixels.insert(current_Event->pixels.end(), tmp->pixels.begin(), tmp->pixels.end());
      }
      evt.push_back(current_Event);
    }
  }
  catch (dsBufferEmpty &) { LOG(logDEBUGHAL) << "Finished readout."; }
  catch (dataPipeException &e) { LOG(logERROR) << e.what(); }

  return evt;
}

rawEvent* hal::daqRawEvent() {

  rawEvent* current_Event = new rawEvent();

  dataSink<rawEvent*> rawpump0, rawpump1, rawpump2, rawpump3;
  splitter0 >> rawpump0;

  if(src1.isConnected()) { splitter1 >> rawpump1; }
  if(src2.isConnected()) { splitter2 >> rawpump2; }
  if(src3.isConnected()) { splitter3 >> rawpump3; }

  // FIXME check carefully: in principle we expect the same number of triggers
  // (==Events) on each pipe. Throw a critical if difference is found?
  try {
    // Read the next Event from each of the pipes, copy the data:
    *current_Event = *rawpump0.Get();
    if(src1.isConnected()) {
      rawEvent tmp = *rawpump1.Get();
      for(size_t record = 0; record < tmp.GetSize(); record++) { current_Event->Add(tmp[record]); }
    }
    if(src2.isConnected()) {
      rawEvent tmp = *rawpump2.Get();
      for(size_t record = 0; record < tmp.GetSize(); record++) { current_Event->Add(tmp[record]); }
    }
    if(src3.isConnected()) {
      rawEvent tmp = *rawpump3.Get();
      for(size_t record = 0; record < tmp.GetSize(); record++) { current_Event->Add(tmp[record]); }
    }
  }
  catch (dsBufferEmpty &) { LOG(logDEBUGHAL) << "Finished readout."; }
  catch (dataPipeException &e) { LOG(logERROR) << e.what(); }

  return current_Event;
}

std::vector<rawEvent*> hal::daqAllRawEvents() {

  std::vector<rawEvent*> raw;

  dataSink<rawEvent*> rawpump0, rawpump1, rawpump2, rawpump3;
  splitter0 >> rawpump0;

  if(src1.isConnected()) { splitter1 >> rawpump1; }
  if(src2.isConnected()) { splitter2 >> rawpump2; }
  if(src3.isConnected()) { splitter3 >> rawpump3; }

  // FIXME check carefully: in principle we expect the same number of triggers
  // (==Events) on each pipe. Throw a critical if difference is found?
  try {
    while(1) {
      // Read the next Event from each of the pipes:
      rawEvent* current_Event = new rawEvent(*rawpump0.Get());
      if(src1.isConnected()) {
	rawEvent tmp = *rawpump1.Get();
	for(size_t record = 0; record < tmp.GetSize(); record++) { current_Event->Add(tmp[record]); }
      }
      if(src2.isConnected()) {
	rawEvent tmp = *rawpump2.Get();
	for(size_t record = 0; record < tmp.GetSize(); record++) { current_Event->Add(tmp[record]); }
      }
      if(src3.isConnected()) {
	rawEvent tmp = *rawpump3.Get();
	for(size_t record = 0; record < tmp.GetSize(); record++) { current_Event->Add(tmp[record]); }
      }
      raw.push_back(current_Event);
    }
  }
  catch (dsBufferEmpty &) { LOG(logDEBUGHAL) << "Finished readout."; }
  catch (dataPipeException &e) { LOG(logERROR) << e.what(); }

  return raw;
}

void hal::daqTrigger(uint32_t nTrig) {

  LOG(logDEBUGHAL) << "Triggering " << nTrig << "x";

  for (uint32_t k = 0; k < nTrig; k++) {
    _testboard->Pg_Single();
    _testboard->uDelay(20);
  }

}

void hal::daqTriggerLoop(uint16_t period) {
  
  LOG(logDEBUGHAL) << "Trigger loop every " << period << " clock cycles started.";
  _testboard->Pg_Loop(period);
  _testboard->uDelay(20);
}

uint32_t hal::daqBufferStatus() {

  uint32_t buffered_data = 0;
  // Summing up data words in all DAQ channels:
  for(uint8_t channel = 0; channel < 8; channel++) {
    buffered_data += _testboard->Daq_GetSize(channel);
  }
  return buffered_data;
}

bool hal::daqStop() {

  LOG(logDEBUGHAL) << "Stopped DAQ session.";

  // Stop the Pattern Generator, just in case (also stops Pg_Loop())
  _testboard->Pg_Stop();

  // Calling Daq_Stop here - calling Daq_Diable would also trigger
  // a FIFO reset (deleting the recorded data)
  // Stopping DAQ for all 8 possible channels
  // FIXME provide daq_stop_all NIOS funktion?
  for(uint8_t channel = 0; channel < 8; channel++) { _testboard->Daq_Stop(channel); }
  _testboard->uDelay(100);

  return true;
}

bool hal::daqClear() {

  // Disconnect the data pipe from the DTB:
  src0 = dtbSource();
  src1 = dtbSource();
  src2 = dtbSource();
  src3 = dtbSource();

  // FIXME provide daq_clear_all NIOS funktion?
  LOG(logDEBUGHAL) << "Closing DAQ session, deleting data buffers.";
  for(uint8_t channel = 0; channel < 8; channel++) { _testboard->Daq_Close(channel); }

  return true;
}
