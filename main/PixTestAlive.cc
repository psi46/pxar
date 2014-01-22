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
  
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}


// ----------------------------------------------------------------------
void PixTestAlive::bookHist(string name) {
  fDirectory->cd(); 

  fHistList.clear();

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
  PixTest::update(); 

  if (fApi) fApi->_dut->testAllPixels(true);
  vector<TH2D*> test2 = efficiencyMaps("PixelAlive", fParNtrig); 
  copy(test2.begin(), test2.end(), back_inserter(fHistList));

  TH2D *h = (TH2D*)(*fHistList.begin());

  h->Draw("colz");
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  LOG(logINFO) << "fDisplayedHist = " << (*fDisplayedHist)->GetName() 
	       << " begin? " << (fDisplayedHist == fHistList.begin())
	       << " end? " << (fDisplayedHist == fHistList.end());
  PixTest::update(); 
}
