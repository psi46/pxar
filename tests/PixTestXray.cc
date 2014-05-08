#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include "PixTestXray.hh"
#include "log.h"

#include <TH2.h>
#include <TMath.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestXray)

// ----------------------------------------------------------------------
PixTestXray::PixTestXray(PixSetup *a, std::string name) : PixTest(a, name), fParNtrig(1000), fParIter(10), fParStretch(0), fParVthrCompMin(0), fParVthrCompMax(0) {
  PixTest::init();
  init(); 
  LOG(logDEBUG) << "PixTestXray ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestXray::PixTestXray() : PixTest() {
  LOG(logDEBUG) << "PixTestXray ctor()";
}

// ----------------------------------------------------------------------
bool PixTestXray::setParameter(string parName, string sval) {
  bool found(false);
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      if (!parName.compare("ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("vthrcompmin")) {
	fParVthrCompMin = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("vthrcompmax")) {
	fParVthrCompMax = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("iterations")) {
	fParIter = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("clockstretch"))
 	fParStretch = atoi(sval.c_str());	
      break;
    }
  }
  return found; 
}


// ----------------------------------------------------------------------
void PixTestXray::init() {
  LOG(logDEBUG) << "PixTestXray::init()";

  setToolTips();
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}

// ----------------------------------------------------------------------
void PixTestXray::setToolTips() {
  fTestTip    = string("Xray vcal calibration test")
    ;
  fSummaryTip = string("to be implemented")
    ;
}


// ----------------------------------------------------------------------
void PixTestXray::bookHist(string /*name*/) {
  fDirectory->cd(); 
}


//----------------------------------------------------------
PixTestXray::~PixTestXray() {
  LOG(logDEBUG) << "PixTestXray dtor";
}


// ----------------------------------------------------------------------
void PixTestXray::doTest() {
  
  LOG(logINFO) << "PixTestXray::doTest() start with fParNtrig = " << fParNtrig;

  cacheDacs(); 
  PixTest::update(); 
  fDirectory->cd();
  
  // -- Set the ClockStretch ?????????????????????
  // fApi->setClockStretch(0, 0, fParStretch); // Stretch after trigger, 0 delay
  
  
  // All on!
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(false);
  
  string name("xray-hits");
  TH1D *h1(0);
  vector<TH1D*> hits;
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    h1 = bookTH1D(Form("%s_C%d", name.c_str(), iroc), Form("%s_C%d", name.c_str(), rocIds[iroc]), 256, 0., 256.);
    h1->SetMinimum(0.);
    h1->SetDirectory(fDirectory);
    setTitles(h1, "VthrComp", "Hits");
    hits.push_back(h1);
  }
  copy(hits.begin(), hits.end(), back_inserter(fHistList));
  

  // -- set 
  fApi->setDAC("vthrcomp", 60); // ???????????

  // -- check for noisy pixels
  // FIXME DO SOMETHING

  // -- scan VthrComp
  map<string, int> hitMap; 
  string pname; 
  vector<Event> daqdat, idat;
  for (int ithr = fParVthrCompMin; ithr < fParVthrCompMax; ++ithr) {
    int badCount(0), pixCount(0), pCount(0); 
    hitMap.clear();
    daqdat.clear();
    fApi->setDAC("vthrcomp", ithr); 
    for (int iter = 0; iter < fParIter; ++iter) {
      idat.clear();
      fApi->daqStart(fPixSetup->getConfigParameters()->getTbPgSettings());
      fApi->daqTrigger(fParNtrig);
      idat = fApi->daqGetEventBuffer();
      badCount += fApi->daqGetNDecoderErrors(); 
      fApi->daqStop();
      copy(idat.begin(), idat.end(), back_inserter(daqdat));
    }

    for (vector<Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
      for(vector<pixel>::iterator pixit = it->pixels.begin(); pixit != it->pixels.end() ; pixit++) {   
	pname = Form("C%d", pixit->roc_id); 
	hitMap[pname]++;
	++pixCount;
      }
    }
    pCount = pixCount + badCount;
    LOG(logDEBUG) << Form("VthrComp = %3d with daqdat.size() = %6ld #pixels = %d (good: %d bad: %d)", 
			  ithr, daqdat.size(), pCount, pixCount, badCount); 
    for (map<string, int>::iterator it = hitMap.begin(); it != hitMap.end(); ++it) {
      cout << "hitmap name = " << (*it).first << " entries: " << (*it).second  << endl;
    }
    int maxRoc(-1), maxCnt(-1); 
    
    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
      pname = Form("C%d", rocIds[iroc]); 
      h1 = hits[getIdxFromId(iroc)];
      if (h1) {
	h1->SetBinContent(ithr+1, hitMap[pname]);
	h1->SetBinError(ithr+1, TMath::Sqrt(hitMap[pname]));
	if (h1->GetEntries() > maxCnt) {
	  maxCnt = static_cast<int>(h1->GetEntries());
	  maxRoc = iroc; 
	}
      }
    }
    hits[getIdxFromId(maxRoc)]->Draw("hist");
    PixTest::update(); 
  }

  // -- fit hit distribution


  // -- determine equivalent Vcal threshold
  // FIXME DO SOMETHING
  if (0) {
    fApi->setDAC("vthrcomp", 80); // ???????????
    fApi->_dut->testAllPixels(true);
    fApi->_dut->maskAllPixels(false);
    vector<TH1*> thr0 = scurveMaps("vcal", "xrayVcal", 4, 0, 250, 3, 1, 0); 
    for (unsigned int i = 0; i < thr0.size(); ++i) {
      LOG(logDEBUG) << Form(" ROC %2d, VCAL thr = %4.3f", getIdFromIdx(i), thr0[i]->GetMean()); 
    }
  }

  if (fHistList.size() > 0) {
    TH1 *h = (TH1*)(*fHistList.begin());
    h->Draw(getHistOption(h).c_str());
    fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
    PixTest::update(); 
  } else {
    LOG(logDEBUG) << "no histogram produced, this is probably a bug";
  }

  restoreDacs(); 

  LOG(logINFO) << "PixTestXray::doTest() done";
}
