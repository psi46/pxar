#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>

#include <TH1.h>

#include "PixTestScurves.hh"
#include "log.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestScurves)

// ----------------------------------------------------------------------
PixTestScurves::PixTestScurves(PixSetup *a, std::string name) : PixTest(a, name), 
  fParDac(""), fParNtrig(-1), fParNpix(-1), 
  fParDacLo(-1), fParDacHi(-1) {
  PixTest::init(a, name);
  init(); 
}


//----------------------------------------------------------
PixTestScurves::PixTestScurves() : PixTest() {
  //  LOG(logDEBUG) << "PixTestScurves ctor()";
}

// ----------------------------------------------------------------------
bool PixTestScurves::setParameter(string parName, string sval) {
  bool found(false);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());
      if (!parName.compare("Ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParNtrig  ->" << fParNtrig << "<- from sval = " << sval;
      }
      if (!parName.compare("Npix")) {
	fParNpix = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParNpix  ->" << fParNpix << "<- from sval = " << sval;
      }
      if (!parName.compare("DAC")) {
	fParDac = sval;
	LOG(logDEBUG) << "  setting fParDac  ->" << fParDac << "<- from sval = " << sval;
      }
      if (!parName.compare("DacLo")) {
	fParDacLo = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParDacLo  ->" << fParDacLo << "<- from sval = " << sval;
      }
      if (!parName.compare("DacHi")) {
	fParDacHi = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParDacHi  ->" << fParDacHi << "<- from sval = " << sval;
      }

      setToolTips();
      break;
    }
  }
  
  return found; 
}


// ----------------------------------------------------------------------
void PixTestScurves::setToolTips() {
  fTestTip    = string(Form("measure and fit s-curves for DAC %s\n", fParDac.c_str())); 
  fSummaryTip = string("all ROCs are displayed side-by-side. Note the orientation:")
    + string("\nthe canvas bottom corresponds to the narrow module side with the cable")
    + string("\nexplanations for all plots: ")
    + string("thr_* shows the map of the s-curve thresholds")
    + string("\nsig_* shows the map of the s-curve widths")
    + string("\ndist_* shows the distribution/projections of the threshold and width maps")
    ;
}

// ----------------------------------------------------------------------
void PixTestScurves::init() {

  setToolTips(); 

  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}

// ----------------------------------------------------------------------
void PixTestScurves::bookHist(string name) {
  fDirectory->cd(); 

  LOG(logDEBUG) << "nothing done with " << name;
  //  fHistList.clear();

}


//----------------------------------------------------------
PixTestScurves::~PixTestScurves() {
  LOG(logDEBUG) << "PixTestScurves dtor";
}


// ----------------------------------------------------------------------
void PixTestScurves::doTest() {
  fDirectory->cd();
  PixTest::update(); 
  LOG(logINFO) << "PixTestScurves::doTest() ntrig = " << fParNtrig 
	       << " using npix = " << fParNpix
	       << " scanning DAC " << fParDac 
	       << " from " << fParDacLo << " .. " << fParDacHi;

  if (fApi) fApi->_dut->testAllPixels(false);
  sparseRoc(fParNpix); 

  int RFLAG(7); 
  vector<TH1*> thr0 = scurveMaps(fParDac, "scurve"+fParDac, fParNtrig, fParDacLo, fParDacHi, RFLAG); 

  LOG(logINFO) << "PixTestScurves::doTest() done ";
  PixTest::update(); 
}

