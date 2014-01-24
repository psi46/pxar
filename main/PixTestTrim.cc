#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>

#include <TH1.h>

#include "PixTestTrim.hh"
#include "log.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestTrim)

// ----------------------------------------------------------------------
PixTestTrim::PixTestTrim(PixSetup *a, std::string name) : PixTest(a, name), 
  fParVcal(-1), fParNtrig(-1), 
  fParVcthrCompLo(-1), fParVcthrCompHi(-1) {
  PixTest::init(a, name);
  init(); 
  //  LOG(logINFO) << "PixTestTrim ctor(PixSetup &a, string, TGTab *)";
  for (unsigned int i = 0; i < fPIX.size(); ++i) {
    LOG(logINFO) << "  setting fPIX" << i <<  " ->" << fPIX[i].first << "/" << fPIX[i].second;
  }
}


//----------------------------------------------------------
PixTestTrim::PixTestTrim() : PixTest() {
  //  LOG(logINFO) << "PixTestTrim ctor()";
}

// ----------------------------------------------------------------------
bool PixTestTrim::setParameter(string parName, string sval) {
  bool found(false);
  string str1, str2; 
  string::size_type s1;
  int pixc, pixr; 
  for (map<string,string>::iterator imap = fParameters.begin(); imap != fParameters.end(); ++imap) {
    LOG(logINFO) << "---> " << imap->first;
    if (0 == imap->first.compare(parName)) {
      found = true; 
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());
      fParameters[parName] = sval;
      if (!parName.compare("Ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
	LOG(logINFO) << "  setting fParNtrig  ->" << fParNtrig << "<- from sval = " << sval;
      }
      if (!parName.compare("Vcal")) {
	fParVcal = atoi(sval.c_str()); 
	LOG(logINFO) << "  setting fParVcal  ->" << fParVcal << "<- from sval = " << sval;
      }
      if (!parName.compare("VcthrCompLo")) {
	fParVcthrCompLo = atoi(sval.c_str()); 
	LOG(logINFO) << "  setting fParVcthrCompLo  ->" << fParVcthrCompLo << "<- from sval = " << sval;
      }

      if (!parName.compare("VcthrCompHi")) {
	fParVcthrCompHi = atoi(sval.c_str()); 
	LOG(logINFO) << "  setting fParVcthrCompHi  ->" << fParVcthrCompHi << "<- from sval = " << sval;
      }

      break;
    }
  }
  
  return found; 
}


// ----------------------------------------------------------------------
void PixTestTrim::init() {
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}

// ----------------------------------------------------------------------
void PixTestTrim::bookHist(string name) {
  fDirectory->cd(); 

  TH1D *h1(0);
  fHistList.clear();
  for (int i = 0; i < fPixSetup->getConfigParameters()->getNrocs(); ++i){
    h1 = new TH1D(Form("scanRange_%s_C%d", name.c_str(), i), Form("scanRange_%s_C%d", name.c_str(), i), 255, 0., 255.); 
    h1->SetMinimum(0.); 
    setTitles(h1, name.c_str(), "a.u."); 
    fHistList.push_back(h1); 

    for (unsigned int ip = 0; ip < fPIX.size(); ++ip) {
      h1 = new TH1D(Form("NhitsVs%s_c%d_r%d_C%d", name.c_str(), fPIX[ip].first, fPIX[ip].second, i), 
		    Form("NhitsVs%s_c%d_r%d_C%d", name.c_str(), fPIX[ip].first, fPIX[ip].second, i), 
		    255, 0., 255.); 
      h1->SetMinimum(0.); 
      setTitles(h1, "DAC", "# pixels"); 
      fHistList.push_back(h1); 
    }
   
  }

}


//----------------------------------------------------------
PixTestTrim::~PixTestTrim() {
  LOG(logINFO) << "PixTestTrim dtor";
}


// ----------------------------------------------------------------------
void PixTestTrim::doTest() {
  PixTest::update(); 
  LOG(logINFO) << "PixTestTrim::doTest() ntrig = " << fParNtrig;

  fPIX.clear(); 
  if (fApi) fApi->_dut->testAllPixels(true);
  vector<TH1*> thr0 = scurveMaps("vcal", "TrimThr0", fParNtrig, 7); 


  PixTest::update(); 
}
