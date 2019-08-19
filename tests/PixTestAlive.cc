#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include "PixUtil.hh"
#include "PixTestAlive.hh"
#include "log.h"

#include <TStopwatch.h>
#include <TStyle.h>
#include <TH2.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestAlive)

// ----------------------------------------------------------------------
PixTestAlive::PixTestAlive(PixSetup *a, std::string name) : PixTest(a, name),
  fParNtrig(1), fParVcal(200),
  fParMaskDeadPixels(false), fParSaveMaskedPixels(false),
  fParMaskFileName("default") {
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

      if (!parName.compare("maskdeadpixels")) {
	PixUtil::replaceAll(sval, "checkbox(", "");
	PixUtil::replaceAll(sval, ")", "");
	fParMaskDeadPixels = !(atoi(sval.c_str())==0);
	setToolTips();
      }
      if (!parName.compare("savemaskfile")) {
	PixUtil::replaceAll(sval, "checkbox(", "");
	PixUtil::replaceAll(sval, ")", "");
	fParSaveMaskedPixels = !(atoi(sval.c_str())==0);
	setToolTips();
      }
      if (!parName.compare("maskfilename")) {
	fParMaskFileName = sval;
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
string PixTestAlive::toolTip(string what) {
  if (string::npos != what.find("alivetest")) return string("checks whether PUC works (after pretest OK!)");
  if (string::npos != what.find("masktest")) return string("checks whether pixels can be masked");
  if (string::npos != what.find("addressdecodingtest")) return string("checks for correct pixel addresses");
  return string("nada");
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

  TStopwatch t;

  fDirectory->cd();
  PixTest::update();
  bigBanner(Form("PixTestAlive::doTest()"));

  aliveTest();
  maskTest();
  addressDecodingTest();

  int seconds = t.RealTime();
  LOG(logINFO) << "PixTestAlive::doTest() done, duration: " << seconds << " seconds";

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
  maskPixels();

  fNDaqErrors = 0;
  vector<TH2D*> test2 = efficiencyMaps("PixelAlive", fParNtrig, FLAG_FORCE_MASKED);
  vector<int> deadPixel(test2.size(), 0);
  vector<int> probPixel(test2.size(), 0);
  for (unsigned int i = 0; i < test2.size(); ++i) {
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

  // -- mask dead pixels (if desired)
  if (fParMaskDeadPixels) {
    TH2D *h(0);
    vector<vector<pair<int, int> > > allDeadPixels;
    for (unsigned int i = 0; i < test2.size(); ++i) {
      h = test2[i];
      vector<pair<int, int> > dead;
      for (int ix = 0; ix < h->GetNbinsX(); ++ix) {
	for (int iy = 0; iy < h->GetNbinsY(); ++iy) {
	  if (h->GetBinContent(ix+1, iy+1) < fParNtrig) {
	    LOG(logDEBUG) << "ROC " << i << " with dead pixel " << ix << "/" << iy;
	    dead.push_back(make_pair(ix, iy));
	  }
	}
      }
      allDeadPixels.push_back(dead);
    }

    for (unsigned int i = 0; i < allDeadPixels.size(); ++i) {
      vector<pair<int, int> > dead = allDeadPixels[i];
      for (unsigned int ipix = 0; ipix < dead.size(); ++ipix) {
	LOG(logINFO) << "ROC " << getIdFromIdx(i) << " masking dead pixel " << dead[ipix].first << "/" << dead[ipix].second;
	fApi->_dut->maskPixel(dead[ipix].first, dead[ipix].second, true, getIdFromIdx(i));
      }
    }

    if (fParSaveMaskedPixels) {
      if (fParMaskFileName == "default") {
	fPixSetup->getConfigParameters()->writeMaskFile(allDeadPixels);
      } else {
	fPixSetup->getConfigParameters()->writeMaskFile(allDeadPixels, fParMaskFileName);
      }
    }
  }



  copy(test2.begin(), test2.end(), back_inserter(fHistList));

  TH2D *h = (TH2D*)(fHistList.back());

  if (h) {
    gStyle->SetPalette(1);
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

  dutCalibrateOff();
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

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  maskPixels();

  vector<int> maskPixel(test2.size(), 0);
  vector<TH2D*> testPixelAlive = mapsWithString("PixelAlive");
  bool deleteIt(false);
  if (testPixelAlive.size() > 0) {
    LOG(logINFO) << "mask vs. old pixelAlive "
		 << testPixelAlive[0]->GetName() << " ..  " << testPixelAlive[testPixelAlive.size()-1]->GetName();
  } else {
    deleteIt = true;
    testPixelAlive = efficiencyMaps("PixelAlive", fParNtrig, FLAG_FORCE_MASKED);
    LOG(logINFO) << "mask vs. new pixelAlive "
		 << testPixelAlive[0]->GetName() << " ..  " << testPixelAlive[testPixelAlive.size()-1]->GetName();
  }

  for (unsigned int i = 0; i < test2.size(); ++i) {
    fHistOptions[test2[i]] = "coltristate";
    test2[i]->SetMinimum(-1.);
    test2[i]->SetMaximum(1.);
    for (int ix = 0; ix < test2[i]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < test2[i]->GetNbinsY(); ++iy) {
	if (test2[i]->GetBinContent(ix+1, iy+1) > 0) {
	  ++maskPixel[i];
	}

	if (test2[i]->GetBinContent(ix+1, iy+1) < 0.5) {
	  if (testPixelAlive[i]->GetBinContent(ix+1, iy+1) > 0) {
	    test2[i]->SetBinContent(ix+1, iy+1, 1);
	  } else {
	    test2[i]->SetBinContent(ix+1, iy+1, 0);
	  }
	} else if (test2[i]->GetBinContent(ix+1, iy+1) > 0.5) {
	  test2[i]->SetBinContent(ix+1, iy+1, -1);
	} else {
	  LOG(logDEBUG) << "how did I get here? Please post an issue to github";
	}


      }
    }
  }

  // -- delete local pixelAlive histograms (but do not touch in case they were taken from fHistList)
  if (deleteIt) {
    for (unsigned int i = 0; i < testPixelAlive.size(); ++i) {
      delete testPixelAlive[i];
    }
  }
  testPixelAlive.clear();

  copy(test2.begin(), test2.end(), back_inserter(fHistList));

  TH2D *h = (TH2D*)(fHistList.back());

  // -- draw it
  if (h) {
    string lopt = getHistOption(h);
    PixUtil::replaceAll(lopt, "tristate", "");
    gStyle->SetPalette(3, fTriStateColors);
    h->Draw(lopt.c_str());
    fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
    PixTest::update();
  }

  restoreDacs();
  // -- summary printout
  string maskPixelString;
  for (unsigned int i = 0; i < maskPixel.size(); ++i) {
    maskPixelString += Form(" %4d", maskPixel[i]);
  }
  LOG(logINFO) << "PixTestAlive::maskTest() done"
	       << (fNDaqErrors>0? Form(" with %d decoding errors", static_cast<int>(fNDaqErrors)):"");
  LOG(logINFO) << "number of mask-defect pixels (per ROC): " << maskPixelString;
  dutCalibrateOff();
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
  maskPixels();

  fNDaqErrors = 0;
  vector<TH2D*> test2 = efficiencyMaps("AddressDecodingTest", fParNtrig, FLAG_CHECK_ORDER|FLAG_FORCE_MASKED);
  vector<int> addrPixel(test2.size(), 0);
  for (unsigned int i = 0; i < test2.size(); ++i) {
    fHistOptions[test2[i]] = "coltristate";
    test2[i]->SetMinimum(-1.);
    test2[i]->SetMaximum(1.);
    // -- binary displays
    for (int ix = 0; ix < test2[i]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < test2[i]->GetNbinsY(); ++iy) {
	if (test2[i]->GetBinContent(ix+1, iy+1) < 0) {
	  LOG(logDEBUG) << " read col/row = " << ix+1 << "/" << iy+1
			<< " address decoding error";
	  ++addrPixel[i];
	}
	if (test2[i]->GetBinContent(ix+1, iy+1) < -0.5) {
	  test2[i]->SetBinContent(ix+1, iy+1, -1);
	} else if (test2[i]->GetBinContent(ix+1, iy+1) > 0.5) {
	  test2[i]->SetBinContent(ix+1, iy+1, 1);
	} else {
	  test2[i]->SetBinContent(ix+1, iy+1, 0);
	}
      }
    }
  }

  copy(test2.begin(), test2.end(), back_inserter(fHistList));

  TH2D *h = (TH2D*)(fHistList.back());

  // -- draw it
  if (h) {
    string lopt = getHistOption(h);
    PixUtil::replaceAll(lopt, "tristate", "");
    gStyle->SetPalette(3, fTriStateColors);
    h->Draw(lopt.c_str());
    fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
    PixTest::update();
  }

  restoreDacs();
  // -- summary printout
  string addrPixelString;
  for (unsigned int i = 0; i < addrPixel.size(); ++i) {
    addrPixelString += Form(" %4d", addrPixel[i]);
  }
  LOG(logINFO) << "PixTestAlive::addressDecodingTest() done"
	       << (fNDaqErrors>0? Form(" with %d decoding errors", static_cast<int>(fNDaqErrors)):"");
  LOG(logINFO) << "number of address-decoding pixels (per ROC): " << addrPixelString;
  dutCalibrateOff();
}
