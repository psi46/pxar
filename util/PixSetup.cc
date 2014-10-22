#include <iostream>
#include "PixSetup.hh"
#include "log.h"
#include <cstdlib>

#include "rsstools.hh"
#include "shist256.hh"

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
  LOG(logDEBUG) << "PixSetup free fPxarMemory";
  free(fPxarMemory);
}


// ----------------------------------------------------------------------
void PixSetup::killApi() {
  if (fApi) delete fApi;
}

// ----------------------------------------------------------------------
void PixSetup::init() {
  rsstools rss;
  LOG(logDEBUG) << "PixSetup init start; getCurrentRSS() = " << rss.getCurrentRSS();
  int N(100000);
  //  fPxarMemory = std::malloc(300000000);
  fPxarMemory = std::calloc(N, sizeof(shist256));
  fPxarMemHi  = ((shist256*)fPxarMemory) + N;

  LOG(logDEBUG) << "fPixTestParameters = " << fPixTestParameters;
  LOG(logDEBUG) << " fConfigParameters = " << fConfigParameters;
  LOG(logDEBUG) << "       fPxarMemory = " << fPxarMemory;
  LOG(logDEBUG)	<< "        fPxarMemHi = " << fPxarMemHi;

  if (0 == fPxarMemory) {
    LOG(logERROR) << "not enough memory; go invest money into a larger computer";
    exit(1);
  } else {
    //     shist256 *p = (shist256*)fPxarMemory; 
    //     int cnt(0); 
    //     while (p < fPxarMemHi) {
    //       if (cnt%100 == 0) cout << p << ": " << p->get(0) << ", " << (p - (shist256*)fPxarMemory) << endl;
    //       p += 1;
    //       ++cnt;
    //     }
    //     p -= 1; 
    //     cout << p << ": " << p->get(0) << ", " << (p - (shist256*)fPxarMemory) << endl;
  }
  LOG(logDEBUG) << "PixSetup init done;  getCurrentRSS() = " << rss.getCurrentRSS() << " fPxarMemory = " << fPxarMemory;
}

