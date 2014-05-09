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
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      if (!parName.compare("ntrig")) {
	fParNtrig = static_cast<uint16_t>(atoi(sval.c_str())); 
	setToolTips();
      }
      if (!parName.compare("vcal")) {
	fParVcal = atoi(sval.c_str()); 
	setToolTips();
      }
      break;
    }
  }
  return found; 
}



// ----------------------------------------------------------------------
void PixTestAlive::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;
  if (!command.compare("masktest")) {
    maskTest(); 
    return;
  }

  if (!command.compare("alivetest")) {
    aliveTest(); 
    return;
  }

  if (!command.compare("addressdecodingtest")) {
    addressDecodingTest(); 
    return;
  }
  LOG(logDEBUG) << "did not find command ->" << command << "<-";
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
  if (fPixSetup->doMoreWebCloning()) output4moreweb();
}


// ----------------------------------------------------------------------
void PixTestAlive::doTest() {
  if (fPixSetup->isDummy()) {
    dummyAnalysis(); 
    return;
  }

  fDirectory->cd();
  PixTest::update(); 
  bigBanner(Form("PixTestAlive::doTest()"));

  aliveTest();
  TH1 *h1 = (*fDisplayedHist); 
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update(); 

  maskTest();
  h1 = (*fDisplayedHist); 
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update(); 

  if (0) {
  addressDecodingTest();
  h1 = (*fDisplayedHist); 
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update(); 
  }

  LOG(logINFO) << "PixTestScurves::doTest() done ";

}


// ----------------------------------------------------------------------
void PixTestAlive::aliveTest() {
  cacheDacs();
  fDirectory->cd();
  PixTest::update(); 
  banner(Form("PixTestAlive::aliveTest() ntrig = %d, vcal = %d", fParNtrig, fParVcal));

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
  restoreDacs();
  LOG(logINFO) << "PixTestAlive::aliveTest() done";
}


// ----------------------------------------------------------------------
void PixTestAlive::maskTest() {
  if (fPixSetup->isDummy()) {
    dummyAnalysis(); 
    return;
  }

  cacheDacs();
  fDirectory->cd();
  PixTest::update(); 
  banner(Form("PixTestAlive::maskTest() ntrig = %d, vcal = %d", static_cast<int>(fParNtrig), fParVcal));

  fApi->setDAC("ctrlreg", 4);
  fApi->setDAC("vcal", fParVcal);

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(true);

  vector<TH2D*> test2 = efficiencyMaps("MaskTest", fParNtrig); 
  for (unsigned int i = 0; i < test2.size(); ++i) {
    fHistOptions.insert(make_pair(test2[i], "colz"));
  }

  copy(test2.begin(), test2.end(), back_inserter(fHistList));
  
  TH2D *h = (TH2D*)(*fHistList.begin());

  h->Draw(getHistOption(h).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  restoreDacs();
  PixTest::update(); 
  LOG(logINFO) << "PixTestAlive::maskTest() done";
}


// ----------------------------------------------------------------------
void PixTestAlive::addressDecodingTest() {
  if (fPixSetup->isDummy()) {
    dummyAnalysis(); 
    return;
  }

  cacheDacs();
  fDirectory->cd();
  PixTest::update(); 
  banner(Form("PixTestAlive::addressDecodingTest() vcal = %d", static_cast<int>(fParVcal)));

  fApi->setDAC("ctrlreg", 4);
  fApi->setDAC("vcal", fParVcal);

  fDirectory->cd();
  vector<TH2D*> maps;
  TH2D *h2(0); 

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    LOG(logDEBUG) << "Create hist " << Form("addressDecoding_C%d", iroc); 
    h2 = bookTH2D(Form("AddressDecoding_C%d", iroc), Form("AddressDecoding_C%d", rocIds[iroc]), 52, 0., 52., 80, 0., 80.); 
    h2->SetMinimum(-1.); 
    h2->SetMaximum(+1.); 
    fHistOptions.insert(make_pair(h2, "colz"));
    h2->SetDirectory(fDirectory); 
    setTitles(h2, "col", "row"); 
    maps.push_back(h2); 
  }

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  int idx(-1);
  std::vector<pxar::pixel> test = fApi->getEfficiencyMap(FLAG_CHECK_ORDER | FLAG_FORCE_MASKED,1);
  for(std::vector<pxar::pixel>::iterator pix = test.begin(); pix != test.end(); ++pix) {
    idx = getIdxFromId(pix->roc_id);
    h2 = maps[idx];
    if(pix->value < 0) {
      h2->SetBinContent(pix->column+1, pix->row+1, -1);
      LOG(logDEBUG) << " read col/row = " << pix->column << "/" << pix->row 
		    << " address decoding error";

    }
    else { h2->SetBinContent(pix->column+1, pix->row+1, 1.); }
  }

  copy(maps.begin(), maps.end(), back_inserter(fHistList));
  
  TH2D *h = (TH2D*)(*fHistList.begin());

  h->Draw(getHistOption(h).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 
  restoreDacs();
  LOG(logINFO) << "PixTestAlive::addressDecodingTest() done";
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
	h2->SetBinContent(ix+1, iy+1, fParNtrig); 
      }
    }

    fHistList.push_back(h2); 
  }

  TH2D *h = (TH2D*)(*fHistList.begin());
  h->Draw(getHistOption(h).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 
  LOG(logINFO) << "PixTestAlive::dummyAnalysis() done";

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

    name = (*il)->GetName(); 
    PixUtil::replaceAll(name, "PixelAlive", "AddressDecoding"); 
    PixUtil::replaceAll(name, "_V0", ""); 
    h = (TH2D*)((*il)->Clone(name.c_str()));
    h->SetDirectory(gDirectory); 
    h->Write(); 
  }
  pDir->cd(); 
}
