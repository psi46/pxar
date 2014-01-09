#include <iostream>
#include "PixSetup.hh"
#include "log.h"

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
  LOG(logINFO) << "PixSetup ctor(pxar::api *a, PixTestParameters *tp, ConfigParameters *cp, SysCommand *sc)";
}

// ----------------------------------------------------------------------
PixSetup::PixSetup() {
  fApi               = 0; 
  fPixTestParameters = 0; 
  fConfigParameters  = 0; 
  fSysCommand        = 0; 
  fModule = 0; 
  init(); 
  LOG(logINFO) << "PixSetup ctor()";
}

// ----------------------------------------------------------------------
PixSetup::~PixSetup() {

}

// ----------------------------------------------------------------------
void PixSetup::init() {
  LOG(logINFO) << "PixSetup init";
}

