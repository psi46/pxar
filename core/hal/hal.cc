#include "hal.h"
#include "rpc_impl.h"
#include <iostream>

using namespace pxar;

hal::hal(std::string name) {

  // Get a new CTestboard class instance:
  tb = new CTestboard();

  // Check if any boards are connected:
  if(!FindDTB(name)) throw CRpcError(CRpcError::READ_ERROR);

  // Open the testboard connection:
  if(tb->Open(name)) {
    std::cout << "Connection to board " << name << " opened." << std::endl;
    try {
      // Print the useful SW/FW versioning info:
      PrintInfo();

      // Check if all RPC calls are matched:
      CheckCompatibility();

      // ...and do the obligatory welcome LED blink:
      tb->Welcome();
      tb->Flush();
    }
    catch(CRpcError &e) {
      // Something went wrong:
      e.What();
      std::cout << "ERROR: DTB software version could not be identified, please update!" << std::endl;
      tb->Close();
      std::cout << "Connection to board " << name << " has been cancelled." << std::endl;
    }
  }
  else {
    // USB port cannot be accessed correctly, most likely access right issue:
    std::cout << "USB error: " << tb->ConnectionError() << std::endl;
    std::cout << "DTB: could not open port to device." << std::endl;
    std::cout << "Make sure you have permission to access USB devices." << std::endl;
    std::cout << "(see documentation for UDEV configuration examples)" << std::endl;
  }
  
  // Finally, initialize the testboard:
  tb->Init();
}

hal::~hal() {
  // Shut down and close the testboard connection on destruction of HAL object:
  
  // Turn High Voltage off:
  tb->HVoff();

  // Turn DUT power off:
  tb->Poff();

  // Close the RPC/USB Connection:
  std::cout << "Connection to board " << tb->GetBoardId() << " closed." << std::endl;
  tb->Close();
  delete tb;
}

void hal::Configure() {
  // FIXME nothing done yet.
}

void hal::PrintInfo() {
  std::string info;
  tb->GetInfo(info);
  std::cout << "--- DTB info-------------------------------------" << std::endl
	    << info
	    <<"-------------------------------------------------" << std::endl;
}

void hal::CheckCompatibility(){
  
  // Get the number of RPC calls available on both ends:
  int32_t dtb_callcount = tb->GetRpcCallCount();
  int32_t host_callcount = tb->GetHostRpcCallCount();

  // If they don't match check RPC calls one by one and print offenders:
  if(dtb_callcount != host_callcount) {
    std::cout << "RPC Call count of DTB and host do not match:" << std::endl;
    std::cout << "   " << dtb_callcount << " DTB RPC calls vs. " << std::endl
	      << "   " << host_callcount << " host RPC calls defined!" << std::endl;

    for(int id = 0; id < max(dtb_callcount,host_callcount); id++) {

      std::string dtb_callname;
      std::string host_callname;

      if(id < dtb_callcount) {
	if(!tb->GetRpcCallName(id,dtb_callname)) 
	  std::cout << "Error in fetching DTB RPC call name." << std::endl;
      }
      if(id < host_callcount) {
	if(!tb->GetHostRpcCallName(id,host_callname)) 
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
    if (!tb->EnumFirst(nDev)) throw int(1);
    for (nr=0; nr<nDev; nr++)	{
      if (!tb->EnumNext(name)) continue;
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
    if (tb->Open(devList[nr], false)) {
      try {
	unsigned int bid = tb->GetBoardId();
	std::cout << "  BID=" << bid << std::endl;
      }
      catch (...) {
	std::cout << "  Not identifiable\n" << std::endl;
      }
      tb->Close();
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

