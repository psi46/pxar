#include <iostream>
#include "PixSetup.hh"

using namespace std;

ClassImp(PixSetup)

// ----------------------------------------------------------------------
PixSetup::PixSetup(TBInterface *tb, PixTestParameters *tp, PixModule *m) {
  fTB     = tb; 
  fTP     = tp; 
  fModule = m; 
  init(); 
  cout << "PixSetup ctor(TBInterface *, string)" << endl;
}

// ----------------------------------------------------------------------
PixSetup::~PixSetup() {

}

// ----------------------------------------------------------------------
void PixSetup::init() {
  cout << "PixSetup init" << endl;
}

