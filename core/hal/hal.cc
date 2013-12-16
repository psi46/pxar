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

void hal::Configure() {
  // FIXME nothing done yet.
}

void hal::initTBM() {
  /*  SetTBMChannel(configParameters->tbmChannel);
      Tbmenable(configParameters->tbmEnable);*/
}

void hal::initROC() {
  //FIXME get configuration! E.g. I2C address

  // Turn output power of the tesboard on:
  _testboard->Pon();

  //  if(configParameters->hvOn)
  _testboard->HVon();

  // Set the I2C address of the ROC we are configuring right now:
  _testboard->SetRocAddress(0);
  //or:
  //_testboard->roc_I2cAddr(0);

  /*
  // Send hard reset to connected modules / TBMs
  _testboard->ResetOn(); 
  _testboard->Flush();
  gDelay->Mdelay(100);
  _testboard->ResetOff();
  _testboard->Flush();
  */
}

void hal::PrintInfo() {
  std::string info;
  _testboard->GetInfo(info);
  std::cout << "--- DTB info-------------------------------------" << std::endl
	    << info
	    <<"-------------------------------------------------" << std::endl;
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

int32_t hal::getTBia() {
  return _testboard->GetIA();
}

int32_t hal::getTBva(){
  return _testboard->GetVA();
}

int32_t hal::getTBid() {
  return _testboard->GetID();
}

int32_t hal::getTBvd() {
  return _testboard->GetVD();
}

bool hal::rocSetDAC(uint8_t dacId, uint8_t dacValue) {
  
  
  _testboard->roc_SetDAC(dacId,dacValue);
  return true;
}
