#include "hal.h"
#include "log.h"
#include "rpc_impl.h"
#include "constants.h"
#include <fstream>

using namespace pxar;

hal::hal(std::string name) {

  // Reset the states of the HAL instance:
  _initialized = false;
  _compatible = false;
  _fallback_mode = false;

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
    LOG(logINFO) << "Staring DTB firmware upgrade...";

    // Reading lines of file:
    string line;
    size_t file_lines;
    for (file_lines = 0; getline(flashFile, line); ++file_lines)
      ;
    flashFile.clear(flashFile.goodbit);
    flashFile.seekg(ios::beg);

    // Check if upgrade is possible
    if (_testboard->UpgradeStart(0x0100) != 0) {
      string msg;
      _testboard->UpgradeErrorMsg(msg);
      LOG(logCRITICAL) << "UPGRADE: " << msg.data();
      return false;
    }

    // Download the flash data
    string rec;
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
	  string msg;
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
      string msg;
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

  // We have TBMs - this means we currently need the software fallback mode:
  _fallback_mode = true;
  LOG(logWARNING) << "You are now running in NIOS II softcore fallback mode. "
		  << "This might increase the test run duration dramatically!";

  // Turn the TBM on:
  _testboard->tbm_Enable(true);
  // FIXME Beat: 31 is default hub address for the new modules:
  _testboard->mod_Addr(31);
  _testboard->Flush();

  // Programm all registers according to the configuration data:
  LOG(logDEBUGHAL) << "Setting register vector for TBM " << static_cast<int>(tbmId) << ".";
  tbmSetRegs(tbmId,regVector);
}

void hal::initROC(uint8_t rocId, uint8_t roctype, std::map< uint8_t,uint8_t > dacVector) {

  // Set the pixel address inverted flag if we have the PSI46digV1 chip
  if(roctype == ROC_PSI46DIG || roctype == ROC_PSI46DIG_TRIG) {
    LOG(logDEBUGHAL) << "Pixel address is inverted in this ROC type.";
    _testboard->SetPixelAddressInverted(true);
  }
  else { _testboard->SetPixelAddressInverted(false); }

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
  usleep(ms*1000);
}

bool hal::CheckCompatibility(){

  std::string dtb_hashcmd;
  uint32_t hostCmdHash;
  uint32_t dtbCmdHash;

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

    for(int id = 0; id < max(dtb_callcount,host_callcount); id++) {
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
    LOG(logCRITICAL) << "Please update your DTB with the correct flash file!";
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
  fgets(choice, 8, stdin);
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

  // Make sure we are writing to the correct TBM by setting its sddress:
  // FIXME Magic from Beat, need to understand this and be able to program also the second TBM:
  _testboard->mod_Addr(31);

  LOG(logDEBUGHAL) << "Set Reg" << std::hex << static_cast<int>(regId) << std::dec << " to " << std::hex << static_cast<int>(regValue) << std::dec << " for both TBM cores.";
  // Set this register for both TBM cores:
  uint8_t regCore1 = 0xE0 | regId;
  uint8_t regCore2 = 0xF0 | regId;
  LOG(logDEBUGHAL) << "Core 1: register " << std::hex << static_cast<int>(regCore1) << " = " << static_cast<int>(regValue) << std::dec;
  LOG(logDEBUGHAL) << "Core 2: register " << std::hex << static_cast<int>(regCore2) << " = " << static_cast<int>(regValue) << std::dec;

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

void hal::PixelSetCalibrate(uint8_t rocid, uint8_t column, uint8_t row, int32_t flags) {

  // Set the correct ROC I2C address:
  _testboard->roc_I2cAddr(rocid);

  // Set the calibrate bit and the CALS setting:
  _testboard->roc_Pix_Cal(column,row,flags&FLAG_USE_CALS);
}

void hal::RocClearCalibrate(uint8_t rocid) {

  // Set the correct ROC I2C address:
  _testboard->roc_I2cAddr(rocid);

  LOG(logDEBUGHAL) << "Clearing calibrate signal for ROC " << static_cast<int>(rocid);
 _testboard->roc_ClrCal();
}


// ---------------- TEST FUNCTIONS ----------------------

std::vector< std::vector<pixel> >* hal::RocCalibrateMap(uint8_t rocid, std::vector<int32_t> parameter) {

  int32_t flags = parameter.at(0);
  int32_t nTriggers = parameter.at(1);

  LOG(logDEBUGHAL) << "Called RocCalibrateMap with flags " << static_cast<int>(flags) << ", running " << nTriggers << " triggers.";
  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();
  std::vector<int16_t> nReadouts;
  std::vector<int32_t> PHsum;
  std::vector<uint32_t> address;

  // Set the correct ROC I2C address:
  _testboard->roc_I2cAddr(rocid);

  // Call the RPC command:
  int status;
  if(_fallback_mode) { status = _testboard->fallback_CalibrateMap(nTriggers, nReadouts, PHsum, address); }
  else { status = _testboard->CalibrateMap(nTriggers, nReadouts, PHsum, address); }
  LOG(logDEBUGHAL) << "Function returns: " << status;

  size_t n = nReadouts.size();
  size_t p = PHsum.size();
  size_t a = address.size();
  LOG(logDEBUGHAL) << "Data size: nReadouts " << static_cast<int>(n)
		   << ", PHsum " << static_cast<int>(p)
		   << ", address " << static_cast<int>(a);

  // Check if all information has been transmitted:
  if(a != n || a != p || n != p) {
    // FIXME custom exception?
    LOG(logCRITICAL) << "Data size not as expected!";
    return result;
  }

  // Log what we get back in the value field:
  if(flags & FLAG_INTERNAL_GET_EFFICIENCY) {LOG(logDEBUGHAL) << "Returning nReadouts for efficiency measurement.";}
  else {LOG(logDEBUGHAL) << "Returning PHsum for pulse height averaging.";}

  // Fill the return data vector:
  std::vector<pixel> data;
  for(std::vector<uint32_t>::iterator it = address.begin(); it != address.end(); ++it) {
    if(flags & FLAG_INTERNAL_GET_EFFICIENCY) { 
      data.push_back(pixel((*it),nReadouts.at(static_cast<size_t>(it - address.begin()))));
    }
    else { 
      data.push_back(pixel((*it),static_cast<int32_t>(PHsum.at(static_cast<size_t>(it-address.begin()))/nTriggers)));
    }
    
  }
  result->push_back(data);

  return result;
}

std::vector< std::vector<pixel> >* hal::RocThresholdMap(uint8_t rocid, std::vector<int32_t> parameter) {

  int32_t flags = parameter.at(0);
  int32_t nTriggers = parameter.at(1);
  int32_t dacReg = parameter.at(2);

  LOG(logDEBUGHAL) << "Called RocThresholdMap with flags " << static_cast<int>(flags) << ", running " << nTriggers << " triggers.";
  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();
  std::vector<int16_t> threshold;
  std::vector<uint32_t> address;

  // Set the correct ROC I2C address:
  _testboard->roc_I2cAddr(rocid);

  // Call the RPC command:
  int status;
  // FIXME the non-fallback ThresholdMap is not yet exported as RPC command!
  //if(_fallback_mode) { 
  status = _testboard->fallback_ThresholdMap(nTriggers, dacReg, flags & FLAG_THRSCAN_RISING, flags & FLAG_XTALK, flags & FLAG_USE_CALS, threshold, address);
    //}
  //else { status = _testboard->ThresholdMap(nTriggers, dacReg, flags & FLAG_THRSCAN_RISING, flags & FLAG_XTALK, flags & FLAG_USE_CALS, threshold, address); }
  LOG(logDEBUGHAL) << "Function returns: " << status;

  size_t t = threshold.size();
  size_t a = address.size();
  LOG(logDEBUGHAL) << "Data size: threshold " << static_cast<int>(t)
		   << ", address " << static_cast<int>(a);

  // Check if all information has been transmitted:
  // FIXME have no address data yet!
  /*  if(a != t) {
    // FIXME custom exception?
    LOG(logCRITICAL) << "Data size not as expected!";
    return result;
  }*/

  // Fill the return data vector:
  std::vector<pixel> data;
  uint8_t column = 0, row = 0;
  for(std::vector<int16_t>::iterator it = threshold.begin(); it != threshold.end(); ++it) {

    pixel newpixel;
    newpixel.column = column;
    newpixel.row = row;
    newpixel.roc_id = rocid;
    newpixel.value =  static_cast<int32_t>(*it);
    data.push_back(newpixel);

    row++;
    if(row >= ROC_NUMROWS) {
      column++;
      row = 0;
    }
  }

  result->push_back(data);
  return result;
}

std::vector< std::vector<pixel> >* hal::PixelCalibrateMap(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  int32_t flags = parameter.at(0);
  int32_t nTriggers = parameter.at(1);

  LOG(logDEBUGHAL) << "Called PixelCalibrateMap with flags " << static_cast<int>(flags) << ", running " << nTriggers << " triggers.";
  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();
  int16_t nReadouts;
  int32_t PHsum;
  std::vector<pixel> data;

  // Set the correct ROC I2C address:
  _testboard->roc_I2cAddr(rocid);

  // Call the RPC command:
  int status = _testboard->CalibratePixel(nTriggers, column, row, nReadouts, PHsum);
  LOG(logDEBUGHAL) << "Function returns: " << status;

  pixel newpixel;
  newpixel.column = column;
  newpixel.row = row;
  newpixel.roc_id = rocid;

  // Decide over what we get back in the value field:
  if(flags & FLAG_INTERNAL_GET_EFFICIENCY) {
    newpixel.value =  static_cast<int32_t>(nReadouts);
    LOG(logDEBUGHAL) << "Returning nReadouts for efficiency measurement.";
  }
  else {
    newpixel.value =  static_cast<int32_t>(PHsum/nTriggers);
    LOG(logDEBUGHAL) << "Returning PHsum for pulse height averaging.";
  }

  data.push_back(newpixel);
  result->push_back(data);

  return result;
}

std::vector< std::vector<pixel> >* hal::PixelThresholdMap(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  int32_t flags = parameter.at(0);
  int32_t nTriggers = parameter.at(1);
  int32_t dacReg = parameter.at(2);

  // Fixed variables for now:
  int32_t start = (flags & FLAG_THRSCAN_RISING ? 0 : 255);
  int32_t step = (flags & FLAG_THRSCAN_RISING ? 1 : -1); // Step size: take fine grained for single pixel.
  int32_t thrLevel = nTriggers/2; // Level defined as threshold: 50% response

  LOG(logDEBUGHAL) << "Called PixelThresholdMap with flags " << static_cast<int>(flags) << ", running " << nTriggers << " triggers, start " << start << " stepsize " << step << " register " << dacReg;
  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();
  std::vector<pixel> data;

  // Set the correct ROC I2C address:
  _testboard->roc_I2cAddr(rocid);

  // Call the RPC command:
  //FIXME trimming just set to 15?! Remove from NIOS function, should be set by HAL!
  int32_t value = _testboard->PixelThreshold(column, row, start, step, thrLevel, nTriggers, dacReg, 
					     flags & FLAG_XTALK, flags & FLAG_USE_CALS);
  LOG(logDEBUGHAL) << "Function returns: " << value;

  pixel newpixel;
  newpixel.column = column;
  newpixel.row = row;
  newpixel.roc_id = rocid;
  newpixel.value =  value;

  data.push_back(newpixel);
  result->push_back(data);

  return result;
}

std::vector< std::vector<pixel> >* hal::PixelCalibrateDacScan(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  int32_t dacreg = parameter.at(0);
  int32_t dacmin = parameter.at(1);
  int32_t dacmax = parameter.at(2);
  int32_t flags = parameter.at(3);
  int32_t nTriggers = parameter.at(4);

  LOG(logDEBUGHAL) << "Called PixelCalibrateDacScan with flags " << static_cast<int>(flags) << ", running " << nTriggers << " triggers.";
  LOG(logDEBUGHAL) << "Scanning DAC " << dacreg << " from " << dacmin << " to " << dacmax;

  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();
  std::vector<int16_t> nReadouts;
  std::vector<int32_t> PHsum;

  // Set the correct ROC I2C address:
  _testboard->roc_I2cAddr(rocid);

  // Call the RPC command:
  int status;
  if(_fallback_mode) { status = _testboard->fallback_CalibrateDacScan(nTriggers, column, row, dacreg, dacmin, dacmax, nReadouts, PHsum); }
  else { status = _testboard->CalibrateDacScan(nTriggers, column, row, dacreg, dacmin, dacmax, nReadouts, PHsum); }
  LOG(logDEBUGHAL) << "Function returns: " << status;

  size_t n = nReadouts.size();
  size_t p = PHsum.size();
  LOG(logDEBUGHAL) << "Data size: nReadouts " << (int)n
		   << ", PHsum " << (int)p;

  // Check if all information has been transmitted:
  if(n != p || n != static_cast<size_t>(dacmax-dacmin)) {
    // FIXME custom exception?
    LOG(logCRITICAL) << "Data size not as expected!";
    return result;
  }

  // Read the vectors from the beginning, not from dacmin:
  size_t it = 0;
  for (int i = dacmin; i < dacmax; i++) {
    std::vector<pixel> data;
    pixel newpixel;
    newpixel.column = column;
    newpixel.row = row;
    newpixel.roc_id = rocid;

    // Decide over what we get back in the value field:
    if(flags & FLAG_INTERNAL_GET_EFFICIENCY) { newpixel.value =  static_cast<int32_t>(nReadouts.at(it)); }
    else { newpixel.value =  static_cast<int32_t>(PHsum.at(it)/nTriggers); }
    data.push_back(newpixel);
    result->push_back(data);
    it++;
  }

  LOG(logDEBUGHAL) << "Result has size " << result->size();
  return result;
}

std::vector< std::vector<pixel> >* hal::PixelCalibrateDacDacScan(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  int32_t dac1reg = parameter.at(0);
  int32_t dac1min = parameter.at(1);
  int32_t dac1max = parameter.at(2);
  int32_t dac2reg = parameter.at(3);
  int32_t dac2min = parameter.at(4);
  int32_t dac2max = parameter.at(5);
  int32_t flags = parameter.at(6);
  int32_t nTriggers = parameter.at(7);

  LOG(logDEBUGHAL) << "Called PixelCalibrateDacDacScan with flags " << static_cast<int>(flags) << ", running " << nTriggers << " triggers.";
  LOG(logDEBUGHAL) << "Scanning field DAC " << dac1reg << " " << dac1min << "-" << dac1max 
		   << ", DAC " << dac2reg << " " << dac2min << "-" << dac2max;

  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();
  std::vector<int16_t> nReadouts;
  std::vector<int32_t> PHsum;

  // Set the correct ROC I2C address:
  _testboard->roc_I2cAddr(rocid);

  // Call the RPC command:
  int status;
  if(_fallback_mode) { status = _testboard->fallback_CalibrateDacDacScan(nTriggers, column, row, dac1reg, dac1min, dac1max, dac2reg, dac2min, dac2max, nReadouts, PHsum); }
  else { status = _testboard->CalibrateDacDacScan(nTriggers, column, row, dac1reg, dac1min, dac1max, dac2reg, dac2min, dac2max, nReadouts, PHsum); }
  LOG(logDEBUGHAL) << "Function returns: " << status;

  size_t n = nReadouts.size();
  size_t p = PHsum.size();
  LOG(logDEBUGHAL) << "Data size: nReadouts " << (int)n
		   << ", PHsum " << (int)p;

  // Check if all information has been transmitted:
  if(n != p || n != static_cast<size_t>((dac1max-dac1min)*(dac2max-dac2min))) {
    // FIXME custom exception?
    LOG(logCRITICAL) << "Data size not as expected!";
    return result;
  }

  // Read the vectors from the beginning, not from dacmin:
  size_t it = 0;
  for (int i = dac1min; i < dac1max; i++) {
    for(int j = dac2min; j < dac2max; j++) {
      std::vector<pixel> data;
      pixel newpixel;
      newpixel.column = column;
      newpixel.row = row;
      newpixel.roc_id = rocid;

      // Decide over what we get back in the value field:
      if(flags & FLAG_INTERNAL_GET_EFFICIENCY) { newpixel.value =  static_cast<int32_t>(nReadouts.at(it)); }
      else { newpixel.value =  static_cast<int32_t>(PHsum.at(it)/nTriggers); }
      data.push_back(newpixel);
      result->push_back(data);
      it++;
    }
  }
  return result;
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

bool hal::daqStart(uint8_t deser160phase, uint8_t nTBMs) {

  LOG(logDEBUGHAL) << "Starting new DAQ session.";
  uint32_t buffer = 500000;

  uint32_t allocated_buffer_ch0 = _testboard->Daq_Open(buffer,0);
  LOG(logDEBUGHAL) << "Allocated buffer size, Channel 0: " << allocated_buffer_ch0;

  _testboard->uDelay(100);

  if(nTBMs > 0) {
    LOG(logDEBUGHAL) << "Enabling Deserializer400 for data acquisition.";
    uint32_t allocated_buffer_ch1 = _testboard->Daq_Open(buffer,1);
    LOG(logDEBUGHAL) << "Allocated buffer size, Channel 1: " << allocated_buffer_ch1;

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

bool hal::daqStop(uint8_t nTBMs) {

  LOG(logDEBUGHAL) << "Stopped DAQ session. Data still in buffers.";

  // Stop the Pattern Generator, just in case (also stops Pg_Loop())
  _testboard->Pg_Stop();

  // Calling Daq_Stop here - calling Daq_Diable would also trigger
  // a FIFO reset (deleting the recorded data)
  if(nTBMs > 0) { _testboard->Daq_Stop(1); }
  _testboard->Daq_Stop(0);
  _testboard->uDelay(100);

  return true;
}

std::vector<uint16_t> hal::daqRead(uint8_t nTBMs) {

  std::vector<uint16_t> data;
  uint32_t buffersize_ch0, buffersize_ch1;
  uint32_t remaining_buffer_ch0, remaining_buffer_ch1;

  buffersize_ch0 = _testboard->Daq_GetSize(0);
  LOG(logDEBUGHAL) << "Available data in channel 0: " << buffersize_ch0;
  if(nTBMs > 0) {
    buffersize_ch1 = _testboard->Daq_GetSize(1);
    LOG(logDEBUGHAL) << "Available data in channel 1: " << buffersize_ch1;
  }

  // FIXME check if buffersize exceeds maximum transfer size and split if so:
  int status = _testboard->Daq_Read(data,buffersize_ch0,remaining_buffer_ch0,0);
  LOG(logDEBUGHAL) << "Function returns: " << status;
  LOG(logDEBUGHAL) << "Read " << data.size() << " data words in channel 0, " 
		   << remaining_buffer_ch0 << " words remaining in buffer.";

  if(nTBMs > 0) {
    std::vector<uint16_t> data1;

    status = _testboard->Daq_Read(data1, buffersize_ch1, remaining_buffer_ch1, 1);
    LOG(logDEBUGHAL) << "Function returns: " << status;
    LOG(logDEBUGHAL) << "Read " << data1.size() << " data words, " 
		     << remaining_buffer_ch1 << " words remaining in buffer.";

    data.insert( data.end(), data1.begin(), data1.end() );
  }

  return data;
}

bool hal::daqReset(uint8_t nTBMs) {

  LOG(logDEBUGHAL) << "Closing DAQ session, deleting data buffers.";
  if(nTBMs > 0) {_testboard->Daq_Close(1);}
  _testboard->Daq_Close(0);
  return true;
}
