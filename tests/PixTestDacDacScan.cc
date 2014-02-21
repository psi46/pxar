#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include "PixTestDacDacScan.hh"
#include "log.h"

#include <TH2.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestDacDacScan)

// ----------------------------------------------------------------------
PixTestDacDacScan::PixTestDacDacScan(PixSetup *a, std::string name) : 
PixTest(a, name), fParNtrig(-1), fParDAC1("nada"), fParDAC2("nada"), fParLoDAC1(-1), fParHiDAC1(-1), fParLoDAC2(-1), fParHiDAC2(-1) {
  PixTest::init();
  init(); 
  LOG(logDEBUG) << "PixTestDacDacScan ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestDacDacScan::PixTestDacDacScan() : PixTest() {
  LOG(logDEBUG) << "PixTestDacDacScan ctor()";
}

// ----------------------------------------------------------------------
bool PixTestDacDacScan::setParameter(string parName, string sval) {
  bool found(false);
  string str1, str2; 
  string::size_type s1;
  int pixc, pixr; 
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());
      if (!parName.compare("Ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("DAC1")) {
	fParDAC1 = sval; 
	setToolTips();
      }
      if (!parName.compare("DAC2")) {
	fParDAC2 = sval; 
	setToolTips();
      }
      if (!parName.compare("DAC1LO")) {
	fParLoDAC1 = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("DAC1HI")) {
	fParHiDAC1 = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("DAC2LO")) {
	fParLoDAC2 = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("DAC2HI")) {
	fParHiDAC2 = atoi(sval.c_str()); 
	setToolTips();
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
void PixTestDacDacScan::init() {
  LOG(logDEBUG) << "PixTestDacDacScan::init()";

  setToolTips(); 
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}


// ----------------------------------------------------------------------
void PixTestDacDacScan::setToolTips() {
  fTestTip    = string(Form("scan the two DACs %s vs %s and ",  fParDAC1.c_str(), fParDAC2.c_str()))
    + string("determine the number of hits for each setting")
    + string("\nNOTE: There is currently a limitation that the total number of scanned points is less than 2^14!")
    ;
  fSummaryTip = string("summary plot to be implemented")
    ;

}

// ----------------------------------------------------------------------
void PixTestDacDacScan::bookHist(string name) {
  fDirectory->cd(); 

}


//----------------------------------------------------------
PixTestDacDacScan::~PixTestDacDacScan() {
  LOG(logDEBUG) << "PixTestDacDacScan dtor";
}


// ----------------------------------------------------------------------
void PixTestDacDacScan::doTest() {
  fDirectory->cd();
  PixTest::update(); 
  LOG(logINFO) << "PixTestDacDacScan::doTest() ntrig = " << fParNtrig;
  //FIXME  clearHist();

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  for (unsigned int i = 0; i < fPIX.size(); ++i) {
    if (fPIX[i].first > -1)  {
      fApi->_dut->testPixel(fPIX[i].first, fPIX[i].second, true);
      fApi->_dut->maskPixel(fPIX[i].first, fPIX[i].second, false);
    }
  }
  string name = fParDAC1 + string(":") + fParDAC2; 
  bookHist(name);
  // -- FIXME also applies to the next few lines
  string::size_type s1 = name.find(":"); 
  string dac1 = name.substr(0, s1); 
  string dac2 = name.substr(s1+1); 
  LOG(logDEBUG) << "PixTestDacDacScan for dacs ->" << dac1 << "<- and ->" << dac2 << "<-";
  
  //  TH1D *h1(0);
  TH2D *h2(0);
  map<string, TH2D*> maps;
  for (unsigned int i = 0; i < fPixSetup->getConfigParameters()->getNrocs(); ++i){
    for (unsigned int ip = 0; ip < fPIX.size(); ++ip) {
      h2 = bookTH2D(Form("nhits_%s_%s_c%d_r%d_C%d", dac1.c_str(), dac2.c_str(), fPIX[ip].first, fPIX[ip].second, i), 
		    Form("nhits_%s_%s_c%d_r%d_C%d", dac1.c_str(), dac2.c_str(), fPIX[ip].first, fPIX[ip].second, i), 
		    255, 0., 255., 255, 0., 255.); 
      h2->SetMinimum(0.); 
      setTitles(h2, dac1.c_str(), dac2.c_str()); 
      fHistList.push_back(h2);
      fHistOptions.insert(make_pair(h2, "colz")); 
      maps.insert(make_pair(Form("nhits_%s_%s_c%d_r%d_C%d", dac1.c_str(), dac2.c_str(), fPIX[ip].first, fPIX[ip].second, i), h2)); 
    }
   
  }


  // -- FIXME This code is crap. Replace as in PixelAlive

  vector<pair<uint8_t, pair<uint8_t, vector<pixel> > > > 
    results = fApi->getEfficiencyVsDACDAC(fParDAC1, fParLoDAC1, fParHiDAC1, 
					  fParDAC2, fParLoDAC2, fParHiDAC2, 
					  0, fParNtrig);

  TH2D *h(0); 
  for (unsigned int ichip = 0; ichip < fPixSetup->getConfigParameters()->getNrocs(); ++ichip) {
    for (unsigned int i = 0; i < results.size(); ++i) {
      pair<uint8_t, pair<uint8_t, vector<pixel> > > v = results[i];
      int idac1 = v.first; 
      pair<uint8_t, vector<pixel> > w = v.second;      
      int idac2 = w.first;
      vector<pixel> wpix = w.second;

      for (unsigned ipix = 0; ipix < wpix.size(); ++ipix) {
	if (wpix[ipix].roc_id == ichip) {
	  h = maps[Form("nhits_%s_%s_c%d_r%d_C%d", fParDAC1.c_str(), fParDAC2.c_str(), wpix[ipix].column, wpix[ipix].row, ichip)];
	  if (h) {
	    h->Fill(idac1+1, idac2+1, wpix[ipix].value); 
	  } else {
	    LOG(logDEBUG) << "XX did not find " << Form("nhits_%s_%s_c%d_r%d_C%d", 
						       fParDAC1.c_str(), fParDAC2.c_str(), wpix[ipix].column, wpix[ipix].row, ichip);
	  }

	}
      }

    }

    fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
    if (h) h->Draw(getHistOption(h).c_str());
    PixTest::update(); 
  }

}
