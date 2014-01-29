#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include "PixTestTbm.hh"
#include "log.h"

#include <TH2.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestTbm)

// ----------------------------------------------------------------------
PixTestTbm::PixTestTbm(PixSetup *a, std::string name) : PixTest(a, name), fParNtrig(-1), fParVcal(-1) {
  PixTest::init(a, name);
  init(); 
  LOG(logINFO) << "PixTestTbm ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestTbm::PixTestTbm() : PixTest() {
  LOG(logINFO) << "PixTestTbm ctor()";
}

// ----------------------------------------------------------------------
bool PixTestTbm::setParameter(string parName, string sval) {
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
void PixTestTbm::init() {
  LOG(logINFO) << "PixTestTbm::init()";
  
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}


// ----------------------------------------------------------------------
void PixTestTbm::bookHist(string name) {
  fDirectory->cd(); 
  LOG(logINFO) << "nothing done with " << name; 
}


//----------------------------------------------------------
PixTestTbm::~PixTestTbm() {
  LOG(logINFO) << "PixTestTbm dtor";
}


// ----------------------------------------------------------------------
void PixTestTbm::doTest() {
  LOG(logINFO) << "PixTestTbm::doTest() ntrig = " << fParNtrig;
  PixTest::update(); 

  if (fApi) fApi->_dut->testAllPixels(true);
  vector<TH2D*> test2 = efficiencyMaps("PixelTbm", fParNtrig); 
  copy(test2.begin(), test2.end(), back_inserter(fHistList));

  TH2D *h = (TH2D*)(*fHistList.begin());

  h->Draw();
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 
  LOG(logINFO) << "PixTestTbm::doTest() done";
}
