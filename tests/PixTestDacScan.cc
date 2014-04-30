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
  PixTest::init();
  init(); 
  //  LOG(logINFO) << "PixTestDacScan ctor(PixSetup &a, string, TGTab *)";
  for (unsigned int i = 0; i < fPIX.size(); ++i) {
    LOG(logDEBUG) << "  setting fPIX" << i <<  " ->" << fPIX[i].first << "/" << fPIX[i].second;
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
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());
      if (!parName.compare("PHmap")) {
	fParPHmap = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("Ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("DAC")) {
	fParDAC = sval; 
	setToolTips();
      }
      if (!parName.compare("DACLO")) {
	fParLoDAC = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("DACHI")) {
	fParHiDAC = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("PIX")) {
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
      if (!parName.compare("PIX1")) {
	LOG(logWARNING) << "please change parameter name from PIX1 to PIX (can appear multiple times)"; 
      }
      if (!parName.compare("PIX2")) {
	LOG(logWARNING) << "please change parameter name from PIX2 to PIX (can appear multiple times)"; 
      }
      if (!parName.compare("PIX3")) {
	LOG(logWARNING) << "please change parameter name from PIX3 to PIX (can appear multiple times)"; 
      }
      if (!parName.compare("PIX4")) {
	LOG(logWARNING) << "please change parameter name from PIX4 to PIX (can appear multiple times)"; 
      }

      break;
    }
  }
  
  return found; 
}


// ----------------------------------------------------------------------
void PixTestDacScan::init() {
  setToolTips(); 
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}


// ----------------------------------------------------------------------
void PixTestDacScan::setToolTips() {
  fTestTip    = string(Form("scan the DAC %s and determine the number of hits vs DAC value\n", fParDAC.c_str()))
    ;
  fSummaryTip = string("summary plot to be implemented")
    ;

}

// ----------------------------------------------------------------------
void PixTestDacScan::bookHist(string /*name*/) {
  fDirectory->cd(); 
}


//----------------------------------------------------------
PixTestDacScan::~PixTestDacScan() {
  LOG(logDEBUG) << "PixTestDacScan dtor";
}


// ----------------------------------------------------------------------
void PixTestDacScan::doTest() {
  uint16_t FLAGS = FLAG_FORCE_SERIAL | FLAG_FORCE_MASKED; // required for manual loop over ROCs
  fDirectory->cd();
  TH1D *h1(0);
  vector<TH1D*> vhist;
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  map<string, TH1D*> hmap; 
  string name(fParPHmap?"ph":"nhits"); 
  string hname;
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    for (unsigned int ip = 0; ip < fPIX.size(); ++ip) {
      hname = Form("%s_%s_c%d_r%d_C%d", name.c_str(), fParDAC.c_str(), fPIX[ip].first, fPIX[ip].second, rocIds[iroc]);
      h1 = bookTH1D(hname.c_str(), hname.c_str(), 256, 0., 256.); 
      h1->SetMinimum(0.); 
      setTitles(h1, Form("%s [DAC]", fParDAC.c_str()), (fParPHmap?"average PH":"readouts"));
      if (ip > 0) fHistOptions.insert(make_pair(h1, "same"));
      hmap[hname] = h1;
      fHistList.push_back(h1); 
    }
   
  }


  PixTest::update(); 
  LOG(logINFO) << "PixTestDacScan::doTest() ntrig = " << fParNtrig;

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  vector<pair<uint8_t, vector<pixel> > > rresults, results;
  for (unsigned int i = 0; i < fPIX.size(); ++i) {
    if (fPIX[i].first > -1)  {
      fApi->_dut->testPixel(fPIX[i].first, fPIX[i].second, true);
      fApi->_dut->maskPixel(fPIX[i].first, fPIX[i].second, false);

      bool done = false;
      while (!done) {
	int cnt(0); 
	try{
	  if (0 == fParPHmap) {
	    rresults = fApi->getEfficiencyVsDAC(fParDAC, fParLoDAC, fParHiDAC, FLAGS, fParNtrig);
	    done = true;
	  } else {
	    rresults = fApi->getPulseheightVsDAC(fParDAC, fParLoDAC, fParHiDAC, FLAGS, fParNtrig);
	    done = true;
	  }
	} catch(DataMissingEvent &e){
	  LOG(logDEBUG) << "problem with readout: "<< e.what() << " missing " << e.numberMissing << " events"; 
	  ++cnt;
	  if (e.numberMissing > 10) done = true; 
	}
	done = (cnt>5) || done;
      }

      copy(rresults.begin(), rresults.end(), back_inserter(results)); 
      fApi->_dut->testPixel(fPIX[i].first, fPIX[i].second, false);
      fApi->_dut->maskPixel(fPIX[i].first, fPIX[i].second, true);
    }
  }

  LOG(logDEBUG) << " dacscandata.size(): " << results.size();
  TH1D *h(0); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    for (unsigned int i = 0; i < results.size(); ++i) {
      pair<uint8_t, vector<pixel> > v = results[i];
      int idac = v.first; 
      
      vector<pixel> vpix = v.second;
      for (unsigned int ipix = 0; ipix < vpix.size(); ++ipix) {
	if (vpix[ipix].roc_id == rocIds[iroc]) {
	  hname = Form("%s_%s_c%d_r%d_C%d", name.c_str(), fParDAC.c_str(), vpix[ipix].column, vpix[ipix].row, rocIds[iroc]);
	  h = hmap[hname];
	  if (h) {
	    h->Fill(idac, vpix[ipix].value); 
	  } else {
	    LOG(logDEBUG) << "XX did not find "  << hname; 
	  }
	}
	
      }
    }
    
  }


  fDisplayedHist = fHistList.begin();
  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    (*il)->Draw((getHistOption(*il)).c_str()); 
  }
  PixTest::update(); 

}

// ----------------------------------------------------------------------
TH1* PixTestDacScan::moduleMap(string histname) {
  LOG(logDEBUG) << "PixTestDacScan::moduleMap histname: " << histname; 
  TH1* h0 = (*fDisplayedHist);
  if (!h0->InheritsFrom(TH1::Class())) {
    return 0; 
  }

  TH1D *h1 = (TH1D*)h0; 
  string h1name = h1->GetName();
  string::size_type s1 = h1name.rfind("_c"); 
  string barename = h1name.substr(0, s1);
  LOG(logDEBUG) << "h1->GetName() = " << h1name << " -> " << barename; 

  TH1 *h(0);
  string hname;
  int cycle(-1); 
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){

    h1  = bookTH1D(Form("%s_C%d", barename.c_str(), rocIds[iroc]), Form("%s_C%d", barename.c_str(), rocIds[iroc]), 
		   h0->GetNbinsX(), h0->GetBinLowEdge(1), h0->GetBinLowEdge(h0->GetNbinsX()+1)); 
    if (0 == iroc) cycle = -1 + histCycle(Form("%s_C%d", barename.c_str(), rocIds[iroc])); 

    list<TH1*>::iterator hbeg = fHistList.begin();
    list<TH1*>::iterator hend = fHistList.end();
    for (list<TH1*>::iterator il = hbeg; il != hend; ++il) {
      h = (*il);
      hname = h->GetName();
      if (string::npos != hname.find(Form("%s", barename.c_str()))) {
	if (string::npos != hname.find(Form("_C%d", rocIds[iroc]))) {
	  if (string::npos != hname.find(Form("_V%d", cycle))) {
	    for (int i = 1; i < h->GetNbinsX(); ++i) {
	      h1->Fill(h->GetBinCenter(i), h->GetBinContent(i)); 
	    }
	  }
	}
      }
    }
    fHistList.push_back(h1); 
  }

  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);

  if (h1) h1->Draw();
  update(); 
  return h1; 
}
