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
  //  LOG(logINFO) << "PixTestDacScan ctor(PixSetup &a, string, TGTab *)";
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
      LOG(logINFO) << "  ==> parName: " << parName;
      LOG(logINFO) << "  ==> sval:    " << sval;
      if (!parName.compare("Ntrig")) fParNtrig = atoi(sval.c_str()); 
      if (!parName.compare("DAC")) fParDAC = sval; 
      if (!parName.compare("PIX1")) {
	s1 = sval.find(","); 
	if (string::npos != s1) {
	  str1 = sval.substr(0, s1); 
	  pixc = atoi(str1.c_str()); 
	  str2 = sval.substr(s1+1); 
	  pixr = atoi(str2.c_str()); 
	  fPIX1 = make_pair(pixc, pixr); 
	} else {
	  fPIX1 = make_pair(-1, -1); 
	}
	cout << "PIX1: ->" << str1 << "<- ->" << str2 << "<-  => c/r = " << pixc << "/" << pixr << endl;
      }
      if (!parName.compare("PIX2")) {
	s1 = sval.find(","); 
	if (string::npos != s1) {
	  str1 = sval.substr(0, s1); 
	  pixc = atoi(str1.c_str()); 
	  str2 = sval.substr(s1+1); 
	  pixr = atoi(str2.c_str()); 
	} else {
	  fPIX2 = make_pair(-1, -1); 
	}
	cout << "PIX2: ->" << str1 << "<- ->" << str2 << "<-  => c/r = " << pixc << "/" << pixr << endl;
      }
      if (!parName.compare("PIX3")) {
	s1 = sval.find(","); 
	if (string::npos != s1) {
	  str1 = sval.substr(0, s1); 
	  pixc = atoi(str1.c_str()); 
	  str2 = sval.substr(s1+1); 
	  pixr = atoi(str2.c_str()); 
	} else {
	  fPIX3 = make_pair(-1, -1); 
	}
	cout << "PIX3: ->" << str1 << "<- ->" << str2 << "<-  => c/r = " << pixc << "/" << pixr << endl;
      }
      if (!parName.compare("PIX4")) {
	s1 = sval.find(","); 
	if (string::npos != s1) {
	  str1 = sval.substr(0, s1); 
	  pixc = atoi(str1.c_str()); 
	  str2 = sval.substr(s1+1); 
	  pixr = atoi(str2.c_str()); 
	} else {
	  fPIX4 = make_pair(-1, -1); 
	}
	cout << "PIX4: ->" << str1 << "<- ->" << str2 << "<-  => c/r = " << pixc << "/" << pixr << endl;
      }


      break;
    }
  }
  return found; 
}


// ----------------------------------------------------------------------
void PixTestDacScan::init() {
  //  LOG(logINFO) << "PixTestDacScan::init()";

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
    setTitles(h1, "DAC", "# pixels"); 
    fHistList.push_back(h1); 
  }

}


//----------------------------------------------------------
PixTestDacScan::~PixTestDacScan() {
  //  LOG(logINFO) << "PixTestDacScan dtor";
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
  PixTest::update(); 
  LOG(logINFO) << "PixTestDacScan::doTest() ntrig = " << fParNtrig;
  clearHist();
  // -- FIXME: Should/could separate better test from display?
  uint16_t flag(0); 
  fApi->_dut->testAllPixels(false);
  if (fPIX1.first > -1)  fApi->_dut->testPixel(fPIX1.first, fPIX1.first, true);
  if (fPIX2.first > -1)  fApi->_dut->testPixel(fPIX2.first, fPIX2.first, true);
  if (fPIX3.first > -1)  fApi->_dut->testPixel(fPIX3.first, fPIX3.first, true);

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
