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
 
  vector<pair<string, double> > power_settings = fPixSetup->getConfigParameters()->getTbPowerSettings();
  vector<pair<uint16_t, uint8_t> > pg_setup = fPixSetup->getConfigParameters()->getTbPgSettings();;


  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  fApi->_dut->testPixel(12, 34, true);
  fApi->_dut->maskPixel(12, 34, false);
  
  fApi->_dut->testPixel(34, 12, true);
  fApi->_dut->maskPixel(34, 12, false);
  
  fApi->_dut->testPixel(48, 67, true);
  fApi->_dut->maskPixel(48, 67, false);


  TH2D *h2 = new TH2D("setupDaq", "setup DAQ", 5, -2., 3., 25, 0., 25.); 
  fHistList.push_back(h2); 
  TH1D *h1 = new TH1D("setupDaq1d", "setup DAQ 1D", 5*25, 0., 5*25.); 
  fHistList.push_back(h1); 

  TH2D *h3 = new TH2D("setupEff", "setup Eff", 5, -2., 3., 25, 0., 25.); 
  fHistList.push_back(h3); 
  TH1D *h4 = new TH1D("setupEff1d", "setup Eff1D", 5*25, 0., 5*25.); 
  fHistList.push_back(h4); 

  for (int iclk = 0; iclk < 25; ++iclk) {
    for (int ioffset = -2; ioffset < 2; ++ioffset) {
      fPixSetup->getConfigParameters()->setTbParameter("clk", iclk); 
      fPixSetup->getConfigParameters()->setTbParameter("ctr", iclk); 
      fPixSetup->getConfigParameters()->setTbParameter("sda", iclk+15+ioffset); 
      fPixSetup->getConfigParameters()->setTbParameter("tin", iclk+5+ioffset); 


      vector<pair<string, uint8_t> > sig_delays = fPixSetup->getConfigParameters()->getTbSigDelays();
      fPixSetup->getConfigParameters()->dumpParameters(sig_delays); 
      
      fApi->initTestboard(sig_delays, power_settings, pg_setup);
      
      fApi->daqStart(fPixSetup->getConfigParameters()->getTbPgSettings());
      fApi->daqTrigger(fParNtrig);
      fApi->daqStop();
      vector<pxar::Event> daqdat = fApi->daqGetEventBuffer();
      vector<pixel> results = fApi->getEfficiencyMap(0, fParNtrig);

      cout << "clk = " << iclk << " number of DAQ events read from board: " << daqdat.size() 
	   << " eff map size: " << results.size()
	   << endl;

      h2->Fill(ioffset, iclk, daqdat.size()); 
      h1->Fill(iclk*5 + ioffset, daqdat.size()); 
      h3->Fill(ioffset, iclk, results.size()); 
      h4->Fill(iclk*5 + ioffset, results.size()); 
    }
  }

  h2->Draw("colz");
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);
  PixTest::update(); 
  LOG(logINFO) << "PixTestSetup::doTest() done";
}
