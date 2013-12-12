#include "libpxar.h"
#include <iostream>
#include <algorithm>
#include <string>
#include <iomanip>

pXarAPI::pXarAPI(std::string name) {
  board_id = name;
  tb = new CTestboard();

  //FIXME throw error here, returns bool!
  tb->FindDTB(board_id);

  if(tb->Open(board_id)) {
    std::cout << "\nBoard " << board_id << " opened." << std::endl;
    try {
      PrintInfo();
      CheckCompatibility();
      tb->Welcome();
      tb->Flush();
    }
    catch(CRpcError &e) {
      e.What();
      std::cout << "ERROR: DTB software version could not be identified, please update it!" << std::endl;
      tb->Close();
      std::cout << "Connection to Board " << board_id << " has been cancelled" << std::endl;
    }
  }
  else {
    std::cout << "USB error: " << tb->ConnectionError() << std::endl;
    std::cout << "DTB: could not open port to device." << std::endl;
    std::cout << "Make sure you have permission to access USB devices." << std::endl;
  }
  tb->Init();
  
}

void pXarAPI::Configure() {
}

void pXarAPI::PrintInfo() {
  std::string info;
  tb->GetInfo(info);
  std::cout << "--- DTB info-------------------------------------" << std::endl
	    << info
	    <<"-------------------------------------------------" << std::endl;
}

void pXarAPI::CheckCompatibility() {

  int32_t dtb_callcount = tb->GetRpcCallCount();
  int32_t host_callcount = tb->GetHostRpcCallCount();

  std::string dtb_callname;
  std::string host_callname;

  if(dtb_callcount != host_callcount) {
    std::cout << "RPC Call count of DTB and host do not match:" << std::endl;
    std::cout << "   " << dtb_callcount << " DTB RPC calls vs. " << std::endl
	      << "   " << host_callcount << " host RPC calls defined!" << std::endl;

    for(int id = 0; id < max(dtb_callcount,host_callcount); id++) {

      dtb_callname = "";
      host_callname = "";

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
		  << ": (DTB) \"" << std::setw(26) << dtb_callname 
		  << "\" != (Host) \"" << std::setw(26) << host_callname << "\"" << std::endl;
    }

    throw CRpcError(CRpcError::UNKNOWN_CMD);

  }

  //std::cout << "RPC version: " << tb->GetRpcVersion() << std::endl;
}
