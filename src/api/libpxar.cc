#include "libpxar.h"
#include <iostream>

pXarAPI::pXarAPI(std::string name) {
  board_id = name;
  tb = new CTestboard();

  //FIXME throw error here, returns bool!
  tb->FindDTB(board_id);

  if(tb->Open(board_id)) {
    printf("\nBoard %s opened\n", board_id.c_str());
    try {
      PrintInfo();
      CheckCompatibility();
      tb->Welcome();
      tb->Flush();
      printf("RPC version: %i\n", tb->GetRpcVersion());
    }
    catch(CRpcError &e) {
      e.What();
      printf("ERROR: DTB software version could not be identified, please update it!\n");
      tb->Close();
      printf("Connection to Board %s has been cancelled\n", board_id.c_str());
    }
  }
  else {
    printf("USB error: %s\n", tb->ConnectionError());
    printf("DTB: could not open port to device.\n");
    printf("Make sure you have permission to access USB devices.\n");
  }
  tb->Init();
  
}

void pXarAPI::Configure() {
  
}

void pXarAPI::PrintInfo() {
  std::string info;
  tb->GetInfo(info);
  printf("--- DTB info-------------------------------------\n"
	 "%s"
	 "-------------------------------------------------\n",
	 info.c_str());
}

void pXarAPI::CheckCompatibility() {
  // Version checks for RPC interface
  
}
