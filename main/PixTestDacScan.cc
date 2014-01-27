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
PixTestDacScan::PixTestDacScan(PixSetup *a, std::string name) : PixTest(a, name), fParNtrig(-1), fParDAC("nada"), fParLoDAC(-1), fParHiDAC(-1) {
  PixTest::init(a, name);
  init(); 
  //  LOG(logINFO) << "PixTestDacScan ctor(PixSetup &a, string, TGTab *)";
  for (unsigned int i = 0; i < fPIX.size(); ++i) {
    LOG(logINFO) << "  setting fPIX" << i <<  " ->" << fPIX[i].first << "/" << fPIX[i].second;
  }
}


//----------------------------------------------------------
PixTestDacScan::PixTestDacScan() : PixTest() {
  //  LOG(logINFO) << "PixTestDacScan ctor()";
}

// ----------------------------------------------------------------------
bool PixTestDacScan::setParameter(string parName, string sval) {
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
      if (!parName.compare("DAC")) {
	fParDAC = sval; 
	LOG(logINFO) << "  setting fParDAC  ->" << fParDAC << "<- from sval = " << sval;
      }
      if (!parName.compare("DACLO")) {
	fParLoDAC = atoi(sval.c_str()); 
	LOG(logINFO) << "  setting fParLoDAC  ->" << fParLoDAC << "<- from sval = " << sval;
      }
      if (!parName.compare("DACHI")) {
	fParHiDAC = atoi(sval.c_str()); 
	LOG(logINFO) << "  setting fParHiDAC  ->" << fParHiDAC << "<- from sval = " << sval;
      }
      if (!parName.compare("PIX1")) {
	s1 = sval.find(","); 
	if (string::npos != s1) {
	  str1 = sval.substr(0, s1); 
	  pixc = atoi(str1.c_str()); 
	  str2 = sval.substr(s1+1); 
	  pixr = atoi(str2.c_str()); 
	  fPIX.push_back(make_pair(pixc, pixr)); 
	} else {
	  fPIX.push_back(make_pair(-1, -1)); 
	}
      }
      if (!parName.compare("PIX2")) {
	s1 = sval.find(","); 
	if (string::npos != s1) {
	  str1 = sval.substr(0, s1); 
	  pixc = atoi(str1.c_str()); 
	  str2 = sval.substr(s1+1); 
	  pixr = atoi(str2.c_str()); 
	  fPIX.push_back(make_pair(pixc, pixr)); 
	} else {
	  fPIX.push_back(make_pair(-1, -1)); 
	}
      }
      if (!parName.compare("PIX3")) {
	s1 = sval.find(","); 
	if (string::npos != s1) {
	  str1 = sval.substr(0, s1); 
	  pixc = atoi(str1.c_str()); 
	  str2 = sval.substr(s1+1); 
	  pixr = atoi(str2.c_str()); 
	  fPIX.push_back(make_pair(pixc, pixr)); 
	} else {
	  fPIX.push_back(make_pair(-1, -1)); 
	}
      }
      if (!parName.compare("PIX4")) {
	s1 = sval.find(","); 
	if (string::npos != s1) {
	  str1 = sval.substr(0, s1); 
	  pixc = atoi(str1.c_str()); 
	  str2 = sval.substr(s1+1); 
	  pixr = atoi(str2.c_str()); 
	  fPIX.push_back(make_pair(pixc, pixr)); 
	} else {
	  fPIX.push_back(make_pair(-1, -1)); 
	}
      }
      // FIXME: remove/update from fPIX if the user removes via the GUI!

      break;
    }
  }
  
  return found; 
}


// ----------------------------------------------------------------------
void PixTestDacScan::init() {
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}

// ----------------------------------------------------------------------
void PixTestDacScan::bookHist(string name) {
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
PixTestDacScan::~PixTestDacScan() {
  LOG(logINFO) << "PixTestDacScan dtor";
}


// ----------------------------------------------------------------------
void PixTestDacScan::doTest() {
  PixTest::update(); 
  LOG(logINFO) << "PixTestDacScan::doTest() ntrig = " << fParNtrig;
  //FIXME  clearHist();
  // -- FIXME: Should/could separate better test from display?
  uint16_t flag(0); 
  fApi->_dut->testAllPixels(false);
  for (unsigned int i = 0; i < fPIX.size(); ++i) {
    if (fPIX[i].first > -1)  fApi->_dut->testPixel(fPIX[i].first, fPIX[i].second, true);
  }
  bookHist(fParDAC);

  // -- FIXME This code is crap. Replace as in PixelAlive

  vector<pair<uint8_t, vector<pixel> > > results = fApi->getEfficiencyVsDAC(fParDAC, fParLoDAC, fParHiDAC, 0, fParNtrig);
  LOG(logINFO) << " dacscandata.size(): " << results.size();
  TH1D *h(0), *hsummary(0); 
  for (int ichip = 0; ichip < fPixSetup->getConfigParameters()->getNrocs(); ++ichip) {
    hsummary = (TH1D*)fDirectory->Get(Form("scanRange_%s_C%d", fParDAC.c_str(), ichip));
    for (unsigned int i = 0; i < results.size(); ++i) {
      pair<uint8_t, vector<pixel> > v = results[i];
      int idac = v.first; 
      if (hsummary) {
	hsummary->SetBinContent(idac+1, 1); 
      } else {
	LOG(logINFO) << "XX did not find " << Form("scanRange_%s_C%d", fParDAC.c_str(), ichip);
      }

      vector<pixel> vpix = v.second;
      for (unsigned int ipix = 0; ipix < vpix.size(); ++ipix) {
	if (vpix[ipix].roc_id == ichip) {
	  h = (TH1D*)fDirectory->Get(Form("NhitsVs%s_c%d_r%d_C%d", fParDAC.c_str(), vpix[ipix].column, vpix[ipix].row, ichip));
	  if (h) {
	    h->SetBinContent(idac+1, vpix[ipix].value); 
	  } else {
	    LOG(logINFO) << "XX did not find " << Form("NhitsVs%S_c%d_r%d_C%d", fParDAC.c_str(), vpix[ipix].column, vpix[ipix].row, ichip);
	  }
	}
	
      }
      fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);

    }

    if (h) h->Draw();
    PixTest::update(); 

  }
}
