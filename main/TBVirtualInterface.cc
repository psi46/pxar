#include "TBVirtualInterface.hh"

#include <TRandom.h>

using namespace std;

//----------------------------------------------------------------------
TBVirtualInterface::TBVirtualInterface() {
  cout << "TBVirtualInterface ctor" << endl;
}


//----------------------------------------------------------------------
TBVirtualInterface::TBVirtualInterface(ConfigParameters *c) {
  cout << "TBVirtualInterface ctor" << endl;
}


//----------------------------------------------------------------------
TBVirtualInterface::~TBVirtualInterface() {
  cout << "TBVirtualInterface dtor" << endl;
}


//----------------------------------------------------------------------
bool TBVirtualInterface::IsPresent() {
  cout << "TBVirtualInterface::IsPresent()" << endl;
  return true;
}


//----------------------------------------------------------------------
void TBVirtualInterface::Cleanup() {
  cout << "TBVirtualInterface::Cleanup()" << endl;
}


// ----------------------------------------------------------------------
bool TBVirtualInterface::Execute(SysCommand *sysCommand) {
  cout << "TBVirtualInterface::Execute(): " << sysCommand->toString() << endl;
  return true; 
}

// ----------------------------------------------------------------------
vector<int> TBVirtualInterface::GetEfficiencyMap(int ntrig, int flag) {
  vector<int> bla; 
  int nreal(0); 
  for (int i = 0; i < 4160; ++i) {
    nreal = ntrig;
    // -- add random dead pixels
    if (gRandom->Rndm() > 0.999) {
      bla.push_back(0); 
    } else {
      bla.push_back(nreal); 
    }
  }
  return bla; 
}
