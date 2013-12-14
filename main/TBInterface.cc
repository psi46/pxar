#include "TBInterface.hh"

using namespace std;

TBInterface::TBInterface() {
  cout << "TBInterface ctor" << endl;
}

TBInterface::TBInterface(ConfigParameters *c) {
  cout << "TBInterface ctor with " << endl;
}



TBInterface::~TBInterface() {
  cout << "TBInterface dtor" << endl;
}


void TBInterface::HVoff() {
  std::cout << "TBInterface::HVoff()" << std::endl;
} 

void TBInterface::HVon() {
  std::cout << "TBInterface::HVon()" << std::endl;
} 

void TBInterface::Poff() {
  std::cout << "TBInterface::Poff()" << std::endl;
} 

void TBInterface::Pon() {
  std::cout << "TBInterface::Pon()" << std::endl;
} 

// ----------------------------------------------------------------------
vector<int> TBInterface::GetEfficiencyMap(int a, int b) {
  vector<int> bla; 
  bla.push_back(0); 
  return bla; 
}
