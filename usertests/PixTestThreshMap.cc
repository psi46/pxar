#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include "PixTestThreshMap.hh"
#include "log.h"

#include <TH2.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestThreshMap)

// ----------------------------------------------------------------------
PixTestThreshMap::PixTestThreshMap(PixSetup *a, std::string name) : PixTest(a, name) {
  PixTest::init();
  init(); 
  LOG(logDEBUG) << "PixTestThreshMap ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestThreshMap::PixTestThreshMap() : PixTest() {
  LOG(logDEBUG) << "PixTestThreshMap ctor()";
}

// ----------------------------------------------------------------------
bool PixTestThreshMap::setParameter(string parName, string sval) {
  bool found(false);
  string stripParName; 
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 

      LOG(logDEBUG) << "  ==> parName: " << parName;
      LOG(logDEBUG) << "  ==> sval:    " << sval;
      if (!parName.compare("dac")) {
	fParDac = sval.c_str();
	setToolTips();
      }
      if (!parName.compare("ntrig")) {
	fParNtrig = static_cast<uint16_t>(atoi(sval.c_str())); 
	setToolTips();
      }
      if (!parName.compare("thresholdpercent")) {
	fParThresholdLevel = static_cast<uint8_t>(atoi(sval.c_str())); 
	setToolTips();
      }
      if (!parName.compare("daclo")) {
	fParLoDAC = static_cast<uint16_t>(atoi(sval.c_str())); 
	setToolTips();
      }
      if (!parName.compare("dachi")) {
	fParHiDAC = static_cast<uint16_t>(atoi(sval.c_str())); 
	setToolTips();
      }
      if (!parName.compare("stepsize")) {
	fParStepSize = static_cast<uint16_t>(atoi(sval.c_str())); 
	setToolTips();
      }
      if (!parName.compare("risingedge")) {
	fParRisingEdge = atoi(sval.c_str()) != 0;
	setToolTips();
      }
      if (!parName.compare("cals")) {
	fParCalS = atoi(sval.c_str()) != 0;
	setToolTips();
      }
      break;
    }
  }
  return found; 
}


// ----------------------------------------------------------------------
void PixTestThreshMap::init() {
  LOG(logINFO) << "PixTestThreshMap::init()";

  setToolTips();
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd();

}

// ----------------------------------------------------------------------
void PixTestThreshMap::setToolTips() {
  fTestTip    = string("send Ntrig \"calibrates\" and count how many hits were measured\n")
    + string("the result is a hitmap, not an efficiency map")
    ;
  fSummaryTip = string("all ROCs are displayed side-by-side. Note the orientation:\n")
    + string("the canvas bottom corresponds to the narrow module side with the cable")
    ;
}


// ----------------------------------------------------------------------
void PixTestThreshMap::bookHist(string name) {
  fDirectory->cd(); 
  LOG(logDEBUG) << "nothing done with " << name; 
}


//----------------------------------------------------------
PixTestThreshMap::~PixTestThreshMap() {
  LOG(logDEBUG) << "PixTestThreshMap dtor";
}


// ----------------------------------------------------------------------
void PixTestThreshMap::doTest() {
  PixTest::update(); 
  fDirectory->cd();
  LOG(logINFO) << "PixTestThreshMap::doTest() ntrig = " << int(fParNtrig);
  PixTest::update(); 

 fDirectory->cd();
  vector<TH2D*> maps;
  vector<TH1D*> hResults;
  TH2D *h2(0);
  string name("Thresh");

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();

  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    id2idx.insert(make_pair(rocIds[iroc], iroc));
    std::string rising = (fParRisingEdge ? "Rising" : "Falling");
    std::string cals = (fParCalS ? "_CalS" : "");
    h2 = bookTH2D(Form("%s_C%d", name.c_str(), iroc), Form("%s%d_%s_%s%s_C%d", name.c_str(), fParThresholdLevel, fParDac.c_str(), rising.c_str(), cals.c_str(), rocIds[iroc]), 52, 0., 52., 80, 0., 80.);
    h2->SetMinimum(0.);
    h2->SetDirectory(fDirectory);
    setTitles(h2, "col", "row");
    maps.push_back(h2);
  }
  
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  LOG(logINFO) << "Recording Threshold Map...";
  uint16_t flags = 0;
  if(fParRisingEdge) flags |= FLAG_RISING_EDGE;
  if(fParCalS) flags |= FLAG_CALS;
  std::vector<pixel> results = fApi->getThresholdMap(fParDac, fParStepSize, fParLoDAC,fParHiDAC,fParThresholdLevel,flags, fParNtrig);
   
  LOG(logINFO) << "Pixels returned: " << results.size();

  for(std::vector<pixel>::size_type idx = 0; idx < results.size() ; idx++) {
    maps[id2idx[results[idx].roc_id]]->SetBinContent(results[idx].column+1,results[idx].row+1,results[idx].getValue());
  }

  for (unsigned int i = 0; i < maps.size(); ++i) {
    fHistOptions.insert(make_pair(maps[i], "colz"));
  }

  copy(maps.begin(), maps.end(), back_inserter(fHistList));
  
  TH2D *h = (TH2D*)(fHistList.back());
  h->Draw(getHistOption(h).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);

  PixTest::update(); 
  LOG(logINFO) << "PixTestThreshMap::doTest() done";
}


