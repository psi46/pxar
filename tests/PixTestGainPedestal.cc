#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include <fstream>

#include <TH1.h>
#include <TRandom.h>
#include <TMath.h>

#include "PixTestGainPedestal.hh"
#include "PixUtil.hh"
#include "log.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestGainPedestal)

// ----------------------------------------------------------------------
PixTestGainPedestal::PixTestGainPedestal(PixSetup *a, std::string name) : PixTest(a, name), fParNtrig(-1), fParNpointsLo(-1), fParNpointsHi(-1) {
  PixTest::init();
  init(); 
}


//----------------------------------------------------------
PixTestGainPedestal::PixTestGainPedestal() : PixTest() {
  //  LOG(logDEBUG) << "PixTestGainPedestal ctor()";
}

// ----------------------------------------------------------------------
bool PixTestGainPedestal::setParameter(string parName, string sval) {
  bool found(false);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());
      if (!parName.compare("Ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParNtrig  ->" << fParNtrig << "<- from sval = " << sval;
      }
      if (!parName.compare("NpointsLo")) {
	fParNpointsLo = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParNpointsLo  ->" << fParNpointsLo << "<- from sval = " << sval;
      }

      if (!parName.compare("NpointsHi")) {
	fParNpointsHi = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParNpointsHi  ->" << fParNpointsHi << "<- from sval = " << sval;
      }

      setToolTips();
      break;
    }
  }
  
  return found; 
}


// ----------------------------------------------------------------------
void PixTestGainPedestal::setToolTips() {
  fTestTip    = string(Form("measure and fit pulseheight vs VCAL (combining low- and high-range)\n")); 
  fSummaryTip = string("all ROCs are displayed side-by-side. Note the orientation:")
    + string("\nthe canvas bottom corresponds to the narrow module side with the cable")
    ;
}

// ----------------------------------------------------------------------
void PixTestGainPedestal::init() {

  setToolTips(); 

  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}

// ----------------------------------------------------------------------
void PixTestGainPedestal::bookHist(string /*name*/) {
  fDirectory->cd(); 
  //  fHistList.clear();

}


//----------------------------------------------------------
PixTestGainPedestal::~PixTestGainPedestal() {
  LOG(logDEBUG) << "PixTestGainPedestal dtor";
}


// ----------------------------------------------------------------------
void PixTestGainPedestal::doTest() {
  if (fPixSetup->isDummy()) {
    dummyAnalysis(); 
    return;
  }

  fDirectory->cd();
  PixTest::update(); 
  bigBanner(Form("PixTestGainPedestal::doTest() ntrig = %d, npointsLo = %d, npointHi = %d", 
		 fParNtrig, fParNpointsLo, fParNpointsHi));

  measure();
  fit();
}


// ----------------------------------------------------------------------
void PixTestGainPedestal::runCommand(string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;
  if (!command.compare("measure")) {
    measure(); 
    return;
  }
  if (!command.compare("fit")) {
    fit(); 
    return;
  }
  return;
}



// ----------------------------------------------------------------------
void PixTestGainPedestal::measure() {
  cacheDacs();
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  // -- make sure that low range is selected
  fApi->setDAC("ctrlreg", 0);

  int RFLAG(7); 
  vector<TH1*> thr0 = scurveMaps("vcal", "gainpedestal", fParNtrig, 0, 255, RFLAG, 2); 
  TH1 *h1 = (*fDisplayedHist); 
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update(); 
  restoreDacs();
  LOG(logINFO) << "PixTestGainPedestal::gainPedestal() done ";
}




// ----------------------------------------------------------------------
void PixTestGainPedestal::fit() {
  PixTest::update(); 
  fDirectory->cd();
}



// ----------------------------------------------------------------------
void PixTestGainPedestal::output4moreweb() {
  // -- nothing required here
}
