#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include "PixTestAlive.hh"
#include "log.h"

#include <TH2.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestAlive)

// ----------------------------------------------------------------------
PixTestAlive::PixTestAlive(PixSetup *a, std::string name) : PixTest(a, name), fParNtrig(-1), fParVcal(-1) {
  PixTest::init(a, name);
  init(); 
  LOG(logINFO) << "PixTestAlive ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestAlive::PixTestAlive() : PixTest() {
  LOG(logINFO) << "PixTestAlive ctor()";
}

// ----------------------------------------------------------------------
bool PixTestAlive::setParameter(string parName, string sval) {
  bool found(false);
  for (map<string,string>::iterator imap = fParameters.begin(); imap != fParameters.end(); ++imap) {
    LOG(logINFO) << "---> " << imap->first;
    if (0 == imap->first.compare(parName)) {
      found = true; 

      fParameters[parName] = sval;
      LOG(logINFO) << "  ==> parName: " << parName;
      LOG(logINFO) << "  ==> sval:    " << sval;
      if (!parName.compare("Ntrig")) fParNtrig = atoi(sval.c_str()); 
      if (!parName.compare("Vcal")) fParVcal = atoi(sval.c_str()); 
      break;
    }
  }
  return found; 
}


// ----------------------------------------------------------------------
void PixTestAlive::init() {
  LOG(logINFO) << "PixTestAlive::init()";

  fDirectory = gDirectory->mkdir(fName.c_str()); 
  fDirectory->cd(); 

  TH2D *h2(0);
  fHistList.clear();
  LOG(logINFO) << "fPixSetup = " << fPixSetup;
  LOG(logINFO) << "fPixSetup->getConfigParameters() = " << fPixSetup->getConfigParameters();
  LOG(logINFO) << "fPixSetup->getConfigParameters()->getNrocs() = " << fPixSetup->getConfigParameters()->getNrocs();
  for (int i = 0; i < fPixSetup->getConfigParameters()->getNrocs(); ++i){
    h2 = new TH2D(Form("PixelAlive_C%d", i), Form("PixelAlive_C%d", i), 52, 0., 52., 80, 0., 80.); 
    h2->SetMinimum(0.); 
    setTitles(h2, "col", "row"); 
    fHistList.push_back(h2); 
  }

}


//----------------------------------------------------------
PixTestAlive::~PixTestAlive() {
  LOG(logINFO) << "PixTestAlive dtor";
  std::list<TH1*>::iterator il; 
  fDirectory->cd(); 
  for (il = fHistList.begin(); il != fHistList.end(); ++il) {
    LOG(logINFO) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory); 
    (*il)->Write(); 
  }
}


// ----------------------------------------------------------------------
void PixTestAlive::doTest() {
  LOG(logINFO) << "PixTestAlive::doTest() ntrig = " << fParNtrig;
  clearHist();
  // -- FIXME: Should/could separate better test from display?
  int ichip(0); 
  uint16_t flag(0); 
  vector<pixel> results = fApi->getEfficiencyMap(0, fParNtrig);
  LOG(logINFO) << " results.size(): " << results.size();
  TH2D *h = (TH2D*)fDirectory->Get(Form("PixelAlive_C%d", ichip));
  if (h) {
    for (int i = 0; i < results.size(); ++i) {
      h->SetBinContent(i/NROW+1, i%NROW+1, static_cast<float>(results[i].value)/fParNtrig); 
    }
  } else {
    LOG(logINFO) << "XX did not find " << Form("PixelAlive_C%d", ichip);
  }
  //    cout << "done with doTest" << endl;
  h->Draw("colz");
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  LOG(logINFO) << "fDisplayedHist = " << (*fDisplayedHist)->GetName() 
	       << " begin? " << (fDisplayedHist == fHistList.begin())
	       << " end? " << (fDisplayedHist == fHistList.end());
  PixTest::update(); 
}
