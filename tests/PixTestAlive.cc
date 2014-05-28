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
    + string("the result is a hitmap, not an efficiency map\n")
    + string("NOTE: VCAL is given in high range!")
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

  addressDecodingTest();
  h1 = (*fDisplayedHist); 
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update(); 

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

  vector<TH2D*> test2 = efficiencyMaps("PixelAlive", fParNtrig, FLAG_FORCE_MASKED); 
  vector<int> deadPixel(test2.size(), 0); 
  vector<int> probPixel(test2.size(), 0); 
  for (unsigned int i = 0; i < test2.size(); ++i) {
    fHistOptions.insert(make_pair(test2[i], "colz"));
    // -- count dead pixels
    for (int ix = 0; ix < test2[i]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < test2[i]->GetNbinsY(); ++iy) {
	if (test2[i]->GetBinContent(ix+1, iy+1) < fParNtrig) {
	  ++probPixel[i];
	  if (test2[i]->GetBinContent(ix+1, iy+1) < 1) {
	    ++deadPixel[i];
	  }
	}
      }
    }
  }

  copy(test2.begin(), test2.end(), back_inserter(fHistList));
  
  TH2D *h = (TH2D*)(fHistList.back());

  h->Draw(getHistOption(h).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 
  restoreDacs();

  // -- summary printout
  string deadPixelString, probPixelString; 
  for (unsigned int i = 0; i < probPixel.size(); ++i) {
    probPixelString += Form(" %4d", probPixel[i]); 
    deadPixelString += Form(" %4d", deadPixel[i]); 
  }
  LOG(logINFO) << "PixTestAlive::aliveTest() done";
  LOG(logINFO) << "number of dead pixels (per ROC): " << deadPixelString;
  LOG(logINFO) << "number of red-efficiency pixels: " << probPixelString;
  
}


// ----------------------------------------------------------------------
void PixTestAlive::maskTest() {

  cacheDacs();
  fDirectory->cd();
  PixTest::update(); 
  banner(Form("PixTestAlive::maskTest() ntrig = %d, vcal = %d", static_cast<int>(fParNtrig), fParVcal));

  fApi->setDAC("ctrlreg", 4);
  fApi->setDAC("vcal", fParVcal);

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(true);

  vector<TH2D*> test2 = efficiencyMaps("MaskTest", fParNtrig, FLAG_FORCE_MASKED); 
  vector<int> maskPixel(test2.size(), 0); 
  for (unsigned int i = 0; i < test2.size(); ++i) {
    fHistOptions.insert(make_pair(test2[i], "colz"));
    // -- go for binary displays
    for (int ix = 0; ix < test2[i]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < test2[i]->GetNbinsY(); ++iy) {
	if (test2[i]->GetBinContent(ix+1, iy+1) > 0) {
	  ++maskPixel[i];
	}
	if (test2[i]->GetBinContent(ix+1, iy+1) == 0) {
	  test2[i]->SetBinContent(ix+1, iy+1, 1);
	} else {
	  test2[i]->SetBinContent(ix+1, iy+1, 0);
	}	  
      }
    }
  }

  copy(test2.begin(), test2.end(), back_inserter(fHistList));
  
  TH2D *h = (TH2D*)(fHistList.back());

  h->Draw(getHistOption(h).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  restoreDacs();
  PixTest::update(); 

  // -- summary printout
  string maskPixelString; 
  for (unsigned int i = 0; i < maskPixel.size(); ++i) {
    maskPixelString += Form(" %4d", maskPixel[i]); 
  }
  LOG(logINFO) << "PixTestAlive::maskTest() done";
  LOG(logINFO) << "number of mask-defect pixels (per ROC): " << maskPixelString;
}


// ----------------------------------------------------------------------
void PixTestAlive::addressDecodingTest() {

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

  vector<TH2D*> test2 = efficiencyMaps("AddressDecodingTest", fParNtrig, FLAG_CHECK_ORDER|FLAG_FORCE_MASKED); 
  vector<int> addrPixel(test2.size(), 0); 
  for (unsigned int i = 0; i < test2.size(); ++i) {
    fHistOptions.insert(make_pair(test2[i], "colz"));
    // -- go for binary displays
    for (int ix = 0; ix < test2[i]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < test2[i]->GetNbinsY(); ++iy) {
	if (test2[i]->GetBinContent(ix+1, iy+1) < 0) {
	  LOG(logDEBUG) << " read col/row = " << ix+1 << "/" << iy+1
			<< " address decoding error";
	  ++addrPixel[i];
	}
	if (test2[i]->GetBinContent(ix+1, iy+1) < 0) {
	  test2[i]->SetBinContent(ix+1, iy+1, 0);
	} else {
	  test2[i]->SetBinContent(ix+1, iy+1, 1);
	}	  
      }
    }
  }

  copy(test2.begin(), test2.end(), back_inserter(fHistList));

  TH2D *h = (TH2D*)(fHistList.back());

  h->Draw(getHistOption(h).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 
  restoreDacs();

  // -- summary printout
  string addrPixelString; 
  for (unsigned int i = 0; i < addrPixel.size(); ++i) {
    addrPixelString += Form(" %4d", addrPixel[i]); 
  }
  LOG(logINFO) << "PixTestAlive::addressDecodingTest() done";
  LOG(logINFO) << "number of address-decoding pixels (per ROC): " << addrPixelString;
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
