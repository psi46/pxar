#include <iostream>
#include "PixSetup.hh"

using namespace std;
using namespace pxar;

ClassImp(PixSetup)

// ----------------------------------------------------------------------
PixSetup::PixSetup(TBInterface *tb, api *a, PixTestParameters *tp, ConfigParameters *cp, SysCommand *sc) {
  fTBInterface       = tb; 
  fAPI               = a; 
  fPixTestParameters = tp; 
  fConfigParameters  = cp; 
  fSysCommand        = sc; 
  fModule = 0; 
  init(); 
  cout << "PixSetup ctor(TBInterface *tb, PixTestParameters *tp, ConfigParameters *cp, SysCommand *sc)" << endl;
}

// ----------------------------------------------------------------------
PixSetup::PixSetup() {
  fTBInterface       = 0; 
  fAPI               = 0; 
  fPixTestParameters = 0; 
  fConfigParameters  = 0; 
  fSysCommand        = 0; 
  fModule = 0; 
  init(); 
  cout << "PixSetup ctor()" << endl;
}

// ----------------------------------------------------------------------
PixSetup::~PixSetup() {

}

// ----------------------------------------------------------------------
void PixSetup::init() {
  cout << "PixSetup init" << endl;
}

