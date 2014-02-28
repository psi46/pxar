#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include "PixUtil.hh"
#include "PixTestAlive.hh"
#include "log.h"

#include <TH2.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestAlive)

// ----------------------------------------------------------------------
PixTestAlive::PixTestAlive(PixSetup *a, std::string name) : PixTest(a, name), fParNtrig(0), fParVcal(-1) {
  PixTest::init();
  init(); 
  LOG(logDEBUG) << "PixTestAlive ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestAlive::PixTestAlive() : PixTest() {
  LOG(logDEBUG) << "PixTestAlive ctor()";
}

// ----------------------------------------------------------------------
bool PixTestAlive::setParameter(string parName, string sval) {
  bool found(false);
  string stripParName; 
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 

      LOG(logDEBUG) << "  ==> parName: " << parName;
      LOG(logDEBUG) << "  ==> sval:    " << sval;
      if (!parName.compare("Ntrig")) {
	fParNtrig = static_cast<uint16_t>(atoi(sval.c_str())); 
	setToolTips();
      }
      if (!parName.compare("Vcal")) {
	fParVcal = atoi(sval.c_str()); 
	setToolTips();
      }
      break;
    }
  }
  return found; 
}


// ----------------------------------------------------------------------
void PixTestAlive::init() {
  LOG(logDEBUG) << "PixTestAlive::init()";

  setToolTips();
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}

// ----------------------------------------------------------------------
void PixTestAlive::setToolTips() {
  fTestTip    = string("send Ntrig \"calibrates\" and count how many hits were measured\n")
    + string("the result is a hitmap, not an efficiency map")
    ;
  fSummaryTip = string("all ROCs are displayed side-by-side. Note the orientation:\n")
    + string("the canvas bottom corresponds to the narrow module side with the cable")
    ;
}


// ----------------------------------------------------------------------
void PixTestAlive::bookHist(string name) {
  fDirectory->cd(); 
  LOG(logDEBUG) << "nothing done with " << name; 
}


//----------------------------------------------------------
PixTestAlive::~PixTestAlive() {
  LOG(logDEBUG) << "PixTestAlive dtor";
}


// ----------------------------------------------------------------------
void PixTestAlive::doTest() {
  if (fPixSetup->isDummy()) {
    dummyAnalysis(); 
    return;
  }

  PixTest::update(); 
  fDirectory->cd();
  LOG(logINFO) << "PixTestAlive::doTest() ntrig = " << int(fParNtrig);
  PixTest::update(); 

  fApi->setDAC("ctrlreg", 4);
  fApi->setDAC("vcal", fParVcal);

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  vector<TH2D*> test2 = efficiencyMaps("PixelAlive", fParNtrig); 
  for (unsigned int i = 0; i < test2.size(); ++i) {
    fHistOptions.insert(make_pair(test2[i], "colz"));
  }

  copy(test2.begin(), test2.end(), back_inserter(fHistList));
  
  TH2D *h = (TH2D*)(*fHistList.begin());

  h->Draw(getHistOption(h).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 
  LOG(logINFO) << "PixTestAlive::doTest() done";
}


// ----------------------------------------------------------------------
void PixTestAlive::dummyAnalysis() {
  string name("PixelAlive"); 
  TH2D *h2(0); 
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    fId2Idx.insert(make_pair(rocIds[iroc], iroc)); 
    h2 = bookTH2D(Form("%s_C%d", name.c_str(), iroc), Form("%s_C%d", name.c_str(), rocIds[iroc]), 52, 0., 52., 80, 0., 80.); 
    h2->SetMinimum(0.); 
    h2->SetDirectory(fDirectory); 
    setTitles(h2, "col", "row"); 
    fHistOptions.insert(make_pair(h2, "colz"));
    
    for (int ix = 0; ix < 52; ++ix) {
      for (int iy = 0; iy < 80; ++iy) {
	h2->Fill(ix, iy, fParNtrig); 
      }
    }

    fHistList.push_back(h2); 
  }

  TH2D *h = (TH2D*)(*fHistList.begin());
  h->Draw(getHistOption(h).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 
  LOG(logINFO) << "PixTestAlive::dummyAnalysis() done";

  output4moreweb();
}


// ----------------------------------------------------------------------
void PixTestAlive::output4moreweb() {
  list<TH1*>::iterator begin = fHistList.begin();
  list<TH1*>::iterator end = fHistList.end();
  
  TDirectory *pDir = gDirectory; 
  gFile->cd(); 
  for (list<TH1*>::iterator il = begin; il != end; ++il) {
    string name = (*il)->GetName(); 
    PixUtil::replaceAll(name, "PixelAlive", "PixelMap"); 
    PixUtil::replaceAll(name, "_V0", ""); 
    TH2D *h = (TH2D*)((*il)->Clone(name.c_str()));
    h->SetDirectory(gDirectory); 
    h->Write(); 
  }
  pDir->cd(); 
}
