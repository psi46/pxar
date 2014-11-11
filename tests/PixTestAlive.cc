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

  LOG(logINFO) << "PixTestAlive::doTest() done ";

}


// ----------------------------------------------------------------------
void PixTestAlive::aliveTest() {
  cacheDacs();
  fDirectory->cd();
  PixTest::update(); 
  string ctrlregstring = getDacsString("ctrlreg"); 
  banner(Form("PixTestAlive::aliveTest() ntrig = %d, vcal = %d (ctrlreg = %s)", 
	      static_cast<int>(fParNtrig), static_cast<int>(fParVcal), ctrlregstring.c_str()));

  fApi->setDAC("vcal", fParVcal);

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  fNDaqErrors = 0; 
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

  if (h) {
    h->Draw(getHistOption(h).c_str());
    fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
    PixTest::update(); 
  }
  restoreDacs();

  // -- summary printout
  string deadPixelString, probPixelString; 
  for (unsigned int i = 0; i < probPixel.size(); ++i) {
    probPixelString += Form(" %4d", probPixel[i]); 
    deadPixelString += Form(" %4d", deadPixel[i]); 
  }
  LOG(logINFO) << "PixTestAlive::aliveTest() done" 
	       << (fNDaqErrors>0? Form(" with %d decoding errors", static_cast<int>(fNDaqErrors)):"");
  LOG(logINFO) << "number of dead pixels (per ROC): " << deadPixelString;
  LOG(logDEBUG) << "number of red-efficiency pixels: " << probPixelString;

  //  PixTest::hvOff();
  //  PixTest::powerOff(); 
}


// ----------------------------------------------------------------------
void PixTestAlive::maskTest() {

  cacheDacs();
  fDirectory->cd();
  PixTest::update(); 

  string ctrlregstring = getDacsString("ctrlreg"); 
  banner(Form("PixTestAlive::maskTest() ntrig = %d, vcal = %d (ctrlreg = %s)", 
	      static_cast<int>(fParNtrig), static_cast<int>(fParVcal), ctrlregstring.c_str()));

  fApi->setDAC("vcal", fParVcal);

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(true);

  fNDaqErrors = 0; 
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
  LOG(logINFO) << "PixTestAlive::maskTest() done" 
	       << (fNDaqErrors>0? Form(" with %d decoding errors", static_cast<int>(fNDaqErrors)):"");
  LOG(logINFO) << "number of mask-defect pixels (per ROC): " << maskPixelString;
}


// ----------------------------------------------------------------------
void PixTestAlive::addressDecodingTest() {

  cacheDacs();
  fDirectory->cd();
  PixTest::update(); 
  string ctrlregstring = getDacsString("ctrlreg"); 
  banner(Form("PixTestAlive::addressDecodingTest() ntrig = %d, vcal = %d (ctrlreg = %s)", 
	      static_cast<int>(fParNtrig), static_cast<int>(fParVcal), ctrlregstring.c_str()));

  fApi->setDAC("vcal", fParVcal);

  fDirectory->cd();

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  fNDaqErrors = 0; 
  vector<TH2D*> test2 = efficiencyMaps("AddressDecodingTest", fParNtrig, FLAG_CHECK_ORDER|FLAG_FORCE_MASKED); 
  vector<int> addrPixel(test2.size(), 0); 
  for (unsigned int i = 0; i < test2.size(); ++i) {
    fHistOptions.insert(make_pair(test2[i], "colz"));
    // -- binary displays
    for (int ix = 0; ix < test2[i]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < test2[i]->GetNbinsY(); ++iy) {
	if (test2[i]->GetBinContent(ix+1, iy+1) < 0) {
	  LOG(logDEBUG) << " read col/row = " << ix+1 << "/" << iy+1
			<< " address decoding error";
	  ++addrPixel[i];
	}
	if (test2[i]->GetBinContent(ix+1, iy+1) <= 0) {
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
  LOG(logINFO) << "PixTestAlive::addressDecodingTest() done" 
	       << (fNDaqErrors>0? Form(" with %d decoding errors", static_cast<int>(fNDaqErrors)):"");
  LOG(logINFO) << "number of address-decoding pixels (per ROC): " << addrPixelString;
}

// ----------------------------------------------------------------------
void PixTestAlive::output4moreweb() {
  print("PixTestAlive::output4moreweb()"); 

  list<TH1*>::iterator begin = fHistList.begin();
  list<TH1*>::iterator end = fHistList.end();

  // -- merge mask test into pixelalive map
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int i = 0; i < rocIds.size(); ++i) {
    TH2D *h0(0), *h1(0); 
    for (list<TH1*>::iterator il = begin; il != end; ++il) {
      string name = (*il)->GetName(); 
      if (string::npos != name.find(Form("MaskTest_C%d", rocIds[i]))) h0 = (TH2D*)(*il); 
      if (string::npos != name.find(Form("PixelAlive_C%d", rocIds[i]))) h1 = (TH2D*)(*il); 
      if (h0 && h1) break;
    }
    if (!h0 || !h1) continue; 
    LOG(logDEBUG) << "merge " << h0->GetName() << " into " << h1->GetName(); 
    for (int ix = 0; ix < h0->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < h0->GetNbinsY(); ++iy) {
	if (0 == h0->GetBinContent(ix+1, iy+1)) h1->SetBinContent(ix+1, iy+1, -1.*h1->GetBinContent(ix+1, iy+1));
      }
    }
  }

  TDirectory *pDir = gDirectory; 
  gFile->cd(); 
  for (list<TH1*>::iterator il = begin; il != end; ++il) {
    string name = (*il)->GetName(); 
    if (string::npos == name.find("_V0"))  continue;
    if (string::npos != name.find("MaskTest"))  continue;
    if (string::npos != name.find("AddressDecodingTest")) {
      PixUtil::replaceAll(name, "AddressDecodingTest", "AddressDecoding"); 
    }
    if (string::npos != name.find("PixelAlive")) {
      PixUtil::replaceAll(name, "PixelAlive", "PixelMap"); 
    }
    PixUtil::replaceAll(name, "_V0", ""); 
    TH2D *h = (TH2D*)((*il)->Clone(name.c_str()));
    h->SetDirectory(gDirectory); 
    h->Write(); 
  }
  pDir->cd(); 
}
