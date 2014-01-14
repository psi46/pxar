#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>

#include <TH1.h>

#include "PixTestDacScan.hh"
#include "log.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestDacScan)

// ----------------------------------------------------------------------
PixTestDacScan::PixTestDacScan(PixSetup *a, std::string name) : PixTest(a, name), fParNtrig(-1), fParDAC("nada") {
  PixTest::init(a, name);
  init(); 
  LOG(logINFO) << "PixTestDacScan ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestDacScan::PixTestDacScan() : PixTest() {
  LOG(logINFO) << "PixTestDacScan ctor()";
}

// ----------------------------------------------------------------------
bool PixTestDacScan::setParameter(string parName, string sval) {
  bool found(false);
  for (map<string,string>::iterator imap = fParameters.begin(); imap != fParameters.end(); ++imap) {
    LOG(logINFO) << "---> " << imap->first;
    if (0 == imap->first.compare(parName)) {
      found = true; 

      fParameters[parName] = sval;
      LOG(logINFO) << "  ==> parName: " << parName;
      LOG(logINFO) << "  ==> sval:    " << sval;
      if (!parName.compare("Ntrig")) fParNtrig = atoi(sval.c_str()); 
      if (!parName.compare("DAC")) fParDAC = sval; 
      break;
    }
  }
  return found; 
}


// ----------------------------------------------------------------------
void PixTestDacScan::init() {
  LOG(logINFO) << "PixTestDacScan::init()";

  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

  TH1D *h1(0);
  fHistList.clear();
  for (int i = 0; i < fPixSetup->getConfigParameters()->getNrocs(); ++i){
    h1 = new TH1D(Form("DacScan_C%d", i), Form("DacScan_C%d", i), 255, 0., 255.); 
    h1->SetMinimum(0.); 
    setTitles(h1, "DAC", "#pixels"); 
    fHistList.push_back(h1); 
  }

}


//----------------------------------------------------------
PixTestDacScan::~PixTestDacScan() {
  LOG(logINFO) << "PixTestDacScan dtor";
  std::list<TH1*>::iterator il; 
  fDirectory->cd(); 
  for (il = fHistList.begin(); il != fHistList.end(); ++il) {
    LOG(logINFO) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory); 
    (*il)->Write(); 
  }
}


// ----------------------------------------------------------------------
void PixTestDacScan::doTest() {
  LOG(logINFO) << "PixTestDacScan::doTest() ntrig = " << fParNtrig;
  clearHist();
  // -- FIXME: Should/could separate better test from display?
  uint16_t flag(0); 
  fApi->_dut->testAllPixels(true);
  vector<pair<uint8_t, vector<pixel> > > results = fApi->getEfficiencyVsDAC(fParDAC, 0, 150, 0, fParNtrig);

  LOG(logINFO) << " dacscandata.size(): " << results.size();
  for (int ichip = 0; ichip < fPixSetup->getConfigParameters()->getNrocs(); ++ichip) {
    TH1D *h = (TH1D*)fDirectory->Get(Form("DacScan_C%d", ichip));
    if (h) {
      for (int i = 0; i < results.size(); ++i) {
	h->SetBinContent(results[i].first, results[i].second.size()); 
      }
    } else {
      LOG(logINFO) << "XX did not find " << Form("DacScan_C%d", ichip);
    }
    //    cout << "done with doTest" << endl;
    h->Draw("");
    fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
    LOG(logINFO) << "fDisplayedHist = " << (*fDisplayedHist)->GetName() 
		 << " begin? " << (fDisplayedHist == fHistList.begin())
		 << " end? " << (fDisplayedHist == fHistList.end());
    PixTest::update(); 
  }

}
