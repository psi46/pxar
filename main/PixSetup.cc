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
  fDoAnalysisOnly    = false; 
  init(); 
  LOG(logDEBUG) << "PixSetup ctor(pxar::api *a, PixTestParameters *tp, ConfigParameters *cp, SysCommand *sc), api = " << fApi;
}

// ----------------------------------------------------------------------
PixSetup::PixSetup() {
  fApi               = 0; 
  fPixTestParameters = 0; 
  fConfigParameters  = 0; 
  fSysCommand        = 0; 
  init(); 
  LOG(logDEBUG) << "PixSetup ctor()";
}

// ----------------------------------------------------------------------
PixSetup::~PixSetup() {

}

// ----------------------------------------------------------------------
void PixSetup::init() {
  LOG(logDEBUG) << "PixSetup init";
}

