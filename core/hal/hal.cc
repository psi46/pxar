#include "hal.h"
#include "rpc_impl.h"
#include <iostream>

using namespace pxar;

hal::hal(std::string name) {

  // Get a new CTestboard class instance:
  _testboard = new CTestboard();

  // Check if any boards are connected:
  if(!FindDTB(name)) throw CRpcError(CRpcError::READ_ERROR);

  // Open the testboard connection:
  if(_testboard->Open(name)) {
    std::cout << "Connection to board " << name << " opened." << std::endl;
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
      std::cout << "ERROR: DTB software version could not be identified, please update!" << std::endl;
      _testboard->Close();
      std::cout << "Connection to board " << name << " has been cancelled." << std::endl;
    }
  }
  else {
    // USB port cannot be accessed correctly, most likely access right issue:
    std::cout << "USB error: " << _testboard->ConnectionError() << std::endl;
    std::cout << "DTB: could not open port to device." << std::endl;
    std::cout << "Make sure you have permission to access USB devices." << std::endl;
    std::cout << "(see documentation for UDEV configuration examples)" << std::endl;
  }
  
  // Finally, initialize the testboard:
  _testboard->Init();
  initTestboard();
}

hal::~hal() {
  // Shut down and close the testboard connection on destruction of HAL object:
  
  // Turn High Voltage off:
  _testboard->HVoff();

  // Turn DUT power off:
  _testboard->Poff();

  // Close the RPC/USB Connection:
  std::cout << "Connection to board " << _testboard->GetBoardId() << " closed." << std::endl;
  _testboard->Close();
  delete _testboard;
}

void hal::initTestboard() {

  //FIXME get from board configuration:

  _testboard->Sig_SetDelay(SIG_CLK, 2);
  _testboard->Sig_SetDelay(SIG_CTR, 20);
  _testboard->Sig_SetDelay(SIG_SDA, 19);
  _testboard->Sig_SetDelay(SIG_TIN, 7);
  _testboard->Flush();
  std::cout << "Testboard delays set." << std::endl;

  // Set voltages:
  setTBva(1.8);
  setTBvd(2.5);
  
  // Set current limits:
  setTBia(1.199);
  setTBid(1.000);
  _testboard->Flush();
  std::cout << "Voltages/current limits set." << std::endl;
}

void hal::initTBM() {
  //FIXME
  /*  SetTBMChannel(configParameters->tbmChannel);
      Tbmenable(configParameters->tbmEnable);*/
}

void hal::initROC(uint8_t rocId, std::vector<std::pair<uint8_t,uint8_t> > dacVector) {

  // Turn on the output power of the testboardeady done:
  _testboard->Pon();
  mDelay(300);

  // Set the I2C address of the ROC we are configuring right now:
  _testboard->roc_I2cAddr(rocId);
  mDelay(300);

  // Programm all DAC registers according to the configuration data:
  rocSetDACs(rocId,dacVector);
  mDelay(300);

}

void hal::PrintInfo() {
  std::string info;
  _testboard->GetInfo(info);
  std::cout << "--- DTB info-------------------------------------" << std::endl
	    << info
	    <<"-------------------------------------------------" << std::endl;
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
    std::cout << "RPC Call count of DTB and host do not match:" << std::endl;
    std::cout << "   " << dtb_callcount << " DTB RPC calls vs. " << std::endl
	      << "   " << host_callcount << " host RPC calls defined!" << std::endl;

    for(int id = 0; id < max(dtb_callcount,host_callcount); id++) {

      std::string dtb_callname;
      std::string host_callname;

      if(id < dtb_callcount) {
	if(!_testboard->GetRpcCallName(id,dtb_callname)) 
	  std::cout << "Error in fetching DTB RPC call name." << std::endl;
      }
      if(id < host_callcount) {
	if(!_testboard->GetHostRpcCallName(id,host_callname)) 
	  std::cout << "Error in fetching host RPC call name." << std::endl;
      }

      if(dtb_callname.compare(host_callname) != 0) 
	std::cout << "ID " << id 
		  << ": (DTB) \"" << dtb_callname 
		  << "\" != (Host) \"" << host_callname << "\"" << std::endl;
    }

    // For now, just print a message and don't to anything else:
    std::cout << "Please update your DTB with the correct flash file!" << std::endl;
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
    case 1: std::cout << "Cannot access the USB driver\n" << std::endl; return false;
    default: return false;
    }
  }

  if (devList.size() == 0) {
    std::cout << "No DTB connected.\n" << std::endl;
    return false;
  }

  if (devList.size() == 1) {
    usbId = devList[0];
    return true;
  }

  // If more than 1 connected device list them
  std::cout << "\nConnected DTBs:\n" << std::endl;
  for (nr=0; nr<devList.size(); nr++) {
    std::cout << nr << ":" << devList[nr] << std::endl;
    if (_testboard->Open(devList[nr], false)) {
      try {
	unsigned int bid = _testboard->GetBoardId();
	std::cout << "  BID=" << bid << std::endl;
      }
      catch (...) {
	std::cout << "  Not identifiable\n" << std::endl;
      }
      _testboard->Close();
    }
    else std::cout << " - in use\n" << std::endl;
  }

  std::cout << "Please choose DTB (0-" << (nDev-1) << "): " << std::endl;
  char choice[8];
  fgets(choice, 8, stdin);
  sscanf (choice, "%ud", &nr);
  if (nr >= devList.size()) {
    nr = 0;
    std::cout << "No DTB opened\n" << std::endl;
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
    rocSetDAC(rocId, it->first, it->second);
  }
}

bool hal::rocSetDAC(uint8_t rocId, uint8_t dacId, uint8_t dacValue) {

  // Make sure we are writing to the correct ROC by setting the I2C address:
  _testboard->SetRocAddress(rocId);

  //FIXME range check missing...
  _testboard->roc_SetDAC(dacId,dacValue);
  return true;
}
