#include "hal.h"
#include "log.h"
#include "rpc_impl.h"
#include <fstream>

using namespace pxar;

hal::hal(std::string name) {

  // Reset the state of the HAL instance:
  _initialized = false;

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
      CheckCompatibility();

      // ...and do the obligatory welcome LED blink:
      _testboard->Welcome();
      _testboard->Flush();
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
  
  // Finally, initialize the testboard:
  _testboard->Init();
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

  if(!_initialized) {
    LOG(logERROR) << "Testboard not initialized yet!";
  }

  return _initialized;
}

void hal::initTestboard(std::vector<std::pair<uint8_t,uint8_t> > sig_delays) {

  //FIXME get from board configuration:

  // Write testboard delay settings to the repsective registers:
  for(std::vector<std::pair<uint8_t,uint8_t> >::iterator sigIt = sig_delays.begin(); sigIt != sig_delays.end(); ++sigIt) {
    LOG(logDEBUGHAL) << "Set DTB delay " << (int)sigIt->first << " to value " << (int)sigIt->second;
    _testboard->Sig_SetDelay(sigIt->first, sigIt->second);
  }
  _testboard->Flush();
  LOG(logDEBUG) << "Testboard delays set.";

  // Set voltages:
  setTBva(1.8);
  setTBvd(2.5);
  
  // Set current limits:
  setTBia(1.199);
  setTBid(1.000);
  _testboard->Flush();
  LOG(logDEBUG) << "Voltages/current limits set.";

  // We are ready for operations now, mark the HAL as initialized:
  _initialized = true;
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
    while (true) {
      // FIXME not sure LOG works wit flush:
      LOG(logINFO) << "\rDownload running... " 
		   << ((int)(100 * recordCount / file_lines)) << " % " << flush;
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

void hal::initTBM() {
  //FIXME
  /*  SetTBMChannel(configParameters->tbmChannel);
      Tbmenable(configParameters->tbmEnable);*/
}

void hal::initROC(uint8_t rocId, std::vector<std::pair<uint8_t,uint8_t> > dacVector) {

  // Turn on the output power of the testboard if not already done:
  LOG(logDEBUGHAL) << "Turn testboard ouput power on.";
  _testboard->Pon();
  mDelay(300);

  // Set the I2C address of the ROC we are configuring right now:
  _testboard->roc_I2cAddr(rocId);
  mDelay(300);

  // Programm all DAC registers according to the configuration data:
  LOG(logDEBUGHAL) << "Setting DAC vector for ROC " << (int)rocId << ".";
  rocSetDACs(rocId,dacVector);
  mDelay(300);

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

void hal::CheckCompatibility(){
  
  // Get the number of RPC calls available on both ends:
  int32_t dtb_callcount = _testboard->GetRpcCallCount();
  int32_t host_callcount = _testboard->GetHostRpcCallCount();

  // If they don't match check RPC calls one by one and print offenders:
  if(dtb_callcount != host_callcount) {
    LOG(logERROR) << "RPC Call count of DTB and host do not match:";
    LOG(logERROR) << "   " << dtb_callcount << " DTB RPC calls vs. ";
    LOG(logERROR) << "   " << host_callcount << " host RPC calls defined!";

    for(int id = 0; id < max(dtb_callcount,host_callcount); id++) {

      std::string dtb_callname;
      std::string host_callname;

      if(id < dtb_callcount) {
	if(!_testboard->GetRpcCallName(id,dtb_callname)) {
	  LOG(logERROR) << "Error in fetching DTB RPC call name.";
	}
      }
      if(id < host_callcount) {
	if(!_testboard->GetHostRpcCallName(id,host_callname)) {
	  LOG(logERROR) << "Error in fetching host RPC call name.";
	}
      }

      if(dtb_callname.compare(host_callname) != 0) {
	LOG(logERROR) << "ID " << id 
		  << ": (DTB) \"" << dtb_callname 
		  << "\" != (Host) \"" << host_callname << "\"";
      }
    }

    // For now, just print a message and don't to anything else:
    LOG(logERROR) << "Please update your DTB with the correct flash file!";
  }
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
  _testboard->_SetIA(uint16_t(IA*10000));
}

void hal::setTBva(double VA){
  // Set the VA analog voltage in V:
  _testboard->_SetVA(uint16_t(VA*1000));
}

void hal::setTBid(double ID) {
 // Set the VD digital current limit in A:
  _testboard->_SetID(uint16_t(ID*10000));
}

void hal::setTBvd(double VD) {
  // Set the VD digital voltage in V:
  _testboard->_SetVD(uint16_t(VD*1000));
}


bool hal::rocSetDACs(uint8_t rocId, std::vector< std::pair< uint8_t, uint8_t> > dacPairs) {

  // Iterate over all DAC id/value pairs and set the DAC
  for(std::vector< std::pair<uint8_t,uint8_t> >::iterator it = dacPairs.begin(); it != dacPairs.end(); ++it) {
    // One of the DAC settings had an issue, abort:
    if(!rocSetDAC(rocId, it->first, it->second)) return false;
  }

  // Send all queued commands to the testboard:
  _testboard->Flush();
  // Everything went all right:
  return true;
}

bool hal::rocSetDAC(uint8_t rocId, uint8_t dacId, uint8_t dacValue) {

  // Make sure we are writing to the correct ROC by setting the I2C address:
  _testboard->roc_I2cAddr(rocId);

  LOG(logDEBUGHAL) << "Set DAC" << (int)dacId << " to " << (int)dacValue;
  _testboard->roc_SetDAC(dacId,dacValue);
  return true;
}

void hal::rocMask(uint8_t rocid, std::vector<int32_t> parameter) {
  
  LOG(logDEBUGHAL) << "Masking full ROC ID " << rocid;
  _testboard->roc_I2cAddr(rocid);
  _testboard->roc_Chip_Mask();
}

void hal::pixelMask(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  LOG(logDEBUGHAL) << "Masking pixel " << column << "," << row 
		   << "on ROC ID " << rocid;
  _testboard->roc_I2cAddr(rocid);
  _testboard->roc_Pix_Mask(column, row);

}

/* ---------------- TEST FUNCTIONS ---------------------- */

std::vector< std::vector<pixel> >* hal::RocCalibrateMap(uint8_t rocid, std::vector<int32_t> parameter) {}

std::vector< std::vector<pixel> >* hal::DummyPixelTestSkeleton(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter){
  LOG(logDEBUGHAL) << " called DummyPixelTestSkeleton routine";
  // pack some random data
  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();
  // we 'scan' dac values
  int32_t dacmin = parameter.at(0);
  int32_t dacmax = parameter.at(1);
  for (int i=dacmin;i<dacmax;i++){
    std::vector<pixel> dacscan;
    pixel newpixel;
    newpixel.column = column;
    newpixel.row = row;
    newpixel.roc_id = rocid;
    newpixel.value = rocid*column+row*i;
    dacscan.push_back(newpixel);
    result->push_back(dacscan);
  }
  return result;
}

std::vector< std::vector<pixel> >* hal::DummyRocTestSkeleton(uint8_t rocid, std::vector<int32_t> parameter){
  LOG(logDEBUGHAL) << " called DummyRocTestSkeleton routine";
  // pack some random data
  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();
  // we 'scan' dac values
  int32_t dacmin = parameter.at(0);
  int32_t dacmax = parameter.at(1);
  for (int i=dacmin;i<dacmax;i++){
    std::vector<pixel> dacscan;
    // over the full roc
    for (int column=0;column<52;column++){
      for (int row=0;row<80;row++){
	pixel newpixel;
	newpixel.column = column;
	newpixel.row = row;
	newpixel.roc_id = rocid;
	newpixel.value = rocid*column+row*i;
	dacscan.push_back(newpixel);
      }
    }
    result->push_back(dacscan);
  }
  return result;
}

std::vector< std::vector<pixel> >* hal::DummyModuleTestSkeleton(std::vector<int32_t> parameter){
  LOG(logDEBUGHAL) << " called DummyModuleTestSkeleton routine";
  // pack some random data
  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();
  for (int rocid=0;rocid<16;rocid++){
    // we 'scan' dac values
    int32_t dacmin = parameter.at(0);
    int32_t dacmax = parameter.at(1);
    for (int i=dacmin;i<dacmax;i++){
      std::vector<pixel> dacscan;
      // over the full roc
      for (int column=0;column<52;column++){
	for (int row=0;row<80;row++){
	  pixel newpixel;
	  newpixel.column = column;
	  newpixel.row = row;
	  newpixel.roc_id = rocid;
	  newpixel.value = rocid*column+row*i;
	  dacscan.push_back(newpixel);
	}
      }
      result->push_back(dacscan);
    }
  }
  return result;
}




//FIXME DEBUG
int32_t hal::PH(int32_t col, int32_t row, int32_t trim, int16_t nTriggers)
{
  _testboard->Daq_Open(50000);
  _testboard->Daq_Select_Deser160(4);
  _testboard->uDelay(100);
  _testboard->Daq_Start();
  _testboard->uDelay(100);

  _testboard->roc_Col_Enable(col, true);
  _testboard->roc_Pix_Trim(col, row, trim);
  _testboard->roc_Pix_Cal (col, row, false);

  vector<uint16_t> data;

  _testboard->uDelay(100);
  for (int16_t k=0; k<nTriggers; k++)
    {
      _testboard->Pg_Single();
      _testboard->uDelay(20);
    }

  _testboard->roc_Pix_Mask(col, row);
  _testboard->roc_Col_Enable(col, false);
  _testboard->roc_ClrCal();

  _testboard->Daq_Stop();
  _testboard->Daq_Read(data, 4000);
  _testboard->Daq_Close();

  for(std::vector<uint16_t>::iterator it = data.begin(); it != data.end(); ++it) {
    LOG(logDEBUGHAL) << std::hex << (*it) << std::dec;
  }

  return -9999;
}


void hal::HVoff() {
  _testboard->HVoff();
}

void hal::HVon() {
  _testboard->HVon();
}
 
void hal::Pon() {
  _testboard->Pon();
}

void hal::Poff() {
  _testboard->Poff();
}
