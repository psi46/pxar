#include <iostream>
#include "PixSetup.hh"
#include "log.h"

using namespace std;
using namespace pxar;

// ----------------------------------------------------------------------
PixSetup::PixSetup(pxarCore *a, PixTestParameters *tp, ConfigParameters *cp) {
  fApi               = a; 
  fPixTestParameters = tp; 
  fConfigParameters  = cp; 
  fDoAnalysisOnly    = false; 
  fMoreWebCloning    = false; 
  fDoUpdateRootFile  = false;
  init(); 
}


// ----------------------------------------------------------------------
PixSetup::PixSetup(string verbosity, PixTestParameters *tp, ConfigParameters *cp) {
  fPixTestParameters = tp; 
  fConfigParameters  = cp; 
  fDoAnalysisOnly    = false; 
  fMoreWebCloning    = false; 
  init(); 

  vector<vector<pair<string,uint8_t> > >       rocDACs = fConfigParameters->getRocDacs(); 
  vector<vector<pair<string,uint8_t> > >       tbmDACs = fConfigParameters->getTbmDacs(); 
  vector<vector<pixelConfig> >                 rocPixels = fConfigParameters->getRocPixelConfig();
  vector<pair<string,uint8_t> >                sig_delays = fConfigParameters->getTbSigDelays(); 
  vector<pair<string, double> >                power_settings = fConfigParameters->getTbPowerSettings();
  vector<pair<std::string, uint8_t> >             pg_setup = fConfigParameters->getTbPgSettings();

  fApi = new pxar::pxarCore("*", verbosity);
  fApi->initTestboard(sig_delays, power_settings, pg_setup);
  fApi->initDUT(fConfigParameters->getHubId(),
		fConfigParameters->getTbmType(), tbmDACs, 
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
  fDoAnalysisOnly    = false; 
  fMoreWebCloning    = false; 
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

