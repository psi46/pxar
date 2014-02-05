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
PixSetup::PixSetup(string verbosity, PixTestParameters *tp, ConfigParameters *cp, SysCommand *sc) {
  fPixTestParameters = tp; 
  fConfigParameters  = cp; 
  fSysCommand        = sc; 
  fDoAnalysisOnly    = false; 
  init(); 
  LOG(logDEBUG) << "PixSetup ctor(pxar::api *a, PixTestParameters *tp, ConfigParameters *cp, SysCommand *sc), api = " << fApi;

  vector<vector<pair<string,uint8_t> > >       rocDACs = fConfigParameters->getRocDacs(); 
  vector<vector<pair<string,uint8_t> > >       tbmDACs = fConfigParameters->getTbmDacs(); 
  vector<vector<pixelConfig> >                 rocPixels = fConfigParameters->getRocPixelConfig();
  vector<pair<string,uint8_t> >                sig_delays = fConfigParameters->getTbSigDelays(); 
  vector<pair<string, double> >                power_settings = fConfigParameters->getTbPowerSettings();
  vector<pair<uint16_t, uint8_t> >             pg_setup = fConfigParameters->getTbPgSettings();

  fApi = new pxar::api("*", verbosity);
  fApi->initTestboard(sig_delays, power_settings, pg_setup);
  fApi->initDUT(fConfigParameters->getTbmType(), tbmDACs, 
		fConfigParameters->getRocType(), rocDACs, 
		rocPixels);
  LOG(logINFO) << "DUT info: ";
  fApi->_dut->info(); 


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
void PixSetup::killApi() {
  if (fApi) delete fApi;
}

// ----------------------------------------------------------------------
void PixSetup::init() {
  LOG(logDEBUG) << "PixSetup init";
}

