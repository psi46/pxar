#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include "PixTestSetup.hh"
#include "log.h"

#include <TH2.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestSetup)

// ----------------------------------------------------------------------
PixTestSetup::PixTestSetup(PixSetup *a, std::string name) : PixTest(a, name), fParNtrig(-1), fParVcal(-1) {
  PixTest::init();
  init(); 
  LOG(logDEBUG) << "PixTestSetup ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestSetup::PixTestSetup() : PixTest() {
  LOG(logDEBUG) << "PixTestSetup ctor()";
}

// ----------------------------------------------------------------------
bool PixTestSetup::setParameter(string parName, string sval) {
  bool found(false);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 

      LOG(logDEBUG) << "  ==> parName: " << parName;
      LOG(logDEBUG) << "  ==> sval:    " << sval;
      if (!parName.compare("Ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  ==> setting fParNtrig to " << fParNtrig; 
	setToolTips();
      }
      if (!parName.compare("Vcal")) {
	fParVcal = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  ==> setting fParVcal to " << fParVcal; 
	setToolTips();
      }
      break;
    }
  }
  return found; 
}


// ----------------------------------------------------------------------
void PixTestSetup::init() {
  LOG(logDEBUG) << "PixTestSetup::init()";

  setToolTips();
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 

}


// ----------------------------------------------------------------------
void PixTestSetup::setToolTips() {
  fTestTip    = string(Form("scan testboard parameter settings and check for valid readout\n")
		       + string("TO BE IMPLEMENTED!!"))
    ;
  fSummaryTip = string("summary plot to be implemented")
    ;
}


// ----------------------------------------------------------------------
void PixTestSetup::bookHist(string name) {

  fDirectory->cd(); 

  TH2D *h2(0);
  fHistList.clear();
  for (unsigned int i = 0; i < fPixSetup->getConfigParameters()->getNrocs(); ++i){
    h2 = new TH2D(Form("Setup_%s_C%d", name.c_str(), i), Form("Setup_%s_C%d", name.c_str(), i), 52, 0., 52., 80, 0., 80.); 
    h2->SetMinimum(0.); 
    setTitles(h2, "col", "row"); 
    fHistList.push_back(h2); 
  }


}


//----------------------------------------------------------
PixTestSetup::~PixTestSetup() {
  LOG(logDEBUG) << "PixTestSetup dtor";
  std::list<TH1*>::iterator il; 
  fDirectory->cd(); 
  for (il = fHistList.begin(); il != fHistList.end(); ++il) {
    LOG(logDEBUG) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory); 
    (*il)->Write(); 
  }
}


// ----------------------------------------------------------------------
void PixTestSetup::doTest() {
  fDirectory->cd();
  LOG(logINFO) << "PixTestSetup::doTest() ntrig = " << fParNtrig;
  //FIXME clearHist(); 

  bookHist("bla");
  fApi->_dut->testAllPixels(false);
  fApi->_dut->testPixel(12, 34, true);
  fApi->_dut->testPixel(34, 12, true);
  fApi->_dut->testPixel(48, 67, true);
  

  for (int iwbc = 90; iwbc < 110; ++iwbc) {
    LOG(logQUIET)<< "WBC = " << iwbc;
    fPixSetup->getApi()->setDAC("wbc", iwbc);
    
    gSystem->ProcessEvents();
    for (int iphase = 0; iphase < 255; ++iphase) {
      fPixSetup->getConfigParameters()->setTbParameter("deser160phase", iphase); 
      
      vector<pair<string, uint8_t> > sig_delays = fPixSetup->getConfigParameters()->getTbSigDelays();
      vector<pair<string, double> > power_settings = fPixSetup->getConfigParameters()->getTbPowerSettings();
      vector<pair<uint16_t, uint8_t> > pg_setup = fPixSetup->getConfigParameters()->getTbPgSettings();;
      
      LOG(logQUIET) << "Re-programming TB: wbc = " << iwbc << " deser160phase = " << iphase; 
      
      fApi->initTestboard(sig_delays, power_settings, pg_setup);
      
      std::vector< pxar::pixel > mapdata = fApi->getEfficiencyMap(0, fParNtrig);
      
      for (vector<pixel>::iterator mapit = mapdata.begin(); mapit != mapdata.end(); ++mapit) {
	if (mapit->value > 0) {
// 	  cout << "**********************************************************************" << endl;
// 	  cout << "Px col/row: " << (int)mapit->column << "/" << (int)mapit->row << " has efficiency " 
// 	       << (int)mapit->value << "/" << fParNtrig << " = " << (mapit->value/fParNtrig) << endl;
	  break;
	}
      }
    }
  }
}
