#include <iostream>
#include "PixSetup.hh"

using namespace std;
using namespace pxar;

ClassImp(PixSetup)

// ----------------------------------------------------------------------
PixSetup::PixSetup(api *a, PixTestParameters *tp, ConfigParameters *cp, SysCommand *sc) {
  fApi               = a; 
  fPixTestParameters = tp; 
  fConfigParameters  = cp; 
  fSysCommand        = sc; 
  fModule = 0; 
  init(); 
  cout << "PixSetup ctor(pxar::api *a, PixTestParameters *tp, ConfigParameters *cp, SysCommand *sc)" << endl;
}

// ----------------------------------------------------------------------
PixSetup::PixSetup() {
  fApi               = 0; 
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

