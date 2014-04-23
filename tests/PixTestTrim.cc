#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>

#include <TH1.h>
#include <TRandom.h>

#include "PixTestTrim.hh"
#include "PixUtil.hh"
#include "log.h"
#include "helper.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestTrim)

// ----------------------------------------------------------------------
PixTestTrim::PixTestTrim(PixSetup *a, std::string name) : PixTest(a, name), fParVcal(-1), fParNtrig(-1), fParVthrCompLo(-1), fParVthrCompHi(-1), fParVcalLo(-1), fParVcalHi(-1) {
  PixTest::init();
  init(); 
  //  LOG(logINFO) << "PixTestTrim ctor(PixSetup &a, string, TGTab *)";
  for (unsigned int i = 0; i < fPIX.size(); ++i) {
    LOG(logDEBUG) << "  setting fPIX" << i <<  " ->" << fPIX[i].first << "/" << fPIX[i].second;
  }
}


//----------------------------------------------------------
PixTestTrim::PixTestTrim() : PixTest() {
  //  LOG(logINFO) << "PixTestTrim ctor()";
}

// ----------------------------------------------------------------------
bool PixTestTrim::setParameter(string parName, string sval) {
  bool found(false);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());
      if (!parName.compare("Ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParNtrig  ->" << fParNtrig << "<- from sval = " << sval;
      }
      if (!parName.compare("Vcal")) {
	fParVcal = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParVcal  ->" << fParVcal << "<- from sval = " << sval;
      }
      if (!parName.compare("VcalLo")) {
	fParVcalLo = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParVcalLo  ->" << fParVcalLo << "<- from sval = " << sval;
      }
      if (!parName.compare("VcalHi")) {
	fParVcalHi = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParVcalHi  ->" << fParVcalHi << "<- from sval = " << sval;
      }
      if (!parName.compare("VthrCompLo")) {
	fParVthrCompLo = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParVthrCompLo  ->" << fParVthrCompLo << "<- from sval = " << sval;
      }
      if (!parName.compare("VthrCompHi")) {
	fParVthrCompHi = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParVthrCompHi  ->" << fParVthrCompHi << "<- from sval = " << sval;
      }

      break;
    }
  }
  
  return found; 
}


// ----------------------------------------------------------------------
void PixTestTrim::init() {
  setToolTips(); 
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}


// ----------------------------------------------------------------------
void PixTestTrim::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;
  if (!command.compare("trimbits")) {
    trimBitTest(); 
    return;
  }
  LOG(logDEBUG) << "did not find command ->" << command << "<-";
}


// ----------------------------------------------------------------------
void PixTestTrim::setToolTips() {
  fTestTip    = string(Form("trimming results in a uniform in-time threshold\n")
		       + string("TO BE FINISHED!!"))
    ;
  fSummaryTip = string("summary plot to be implemented")
    ;
}

// ----------------------------------------------------------------------
void PixTestTrim::bookHist(string name) {
  fDirectory->cd(); 

  LOG(logDEBUG) << "nothing done with " << name;
}


//----------------------------------------------------------
PixTestTrim::~PixTestTrim() {
  LOG(logDEBUG) << "PixTestTrim dtor";
  if (fPixSetup->doMoreWebCloning()) output4moreweb();
}


// ----------------------------------------------------------------------
void PixTestTrim::doTest() {
  if (fPixSetup->isDummy()) {
    dummyAnalysis(); 
    return;
  }

  double NSIGMA(2); 

  fDirectory->cd();
  PixTest::update(); 
  bigBanner(Form("PixTestTrim::doTest() ntrig = %d, vcal = %d", fParNtrig, fParVcal));

  fPIX.clear(); 

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 

  fApi->setDAC("Vtrim", 0);
  pxar::mDelay(1000);
  fApi->setDAC("ctrlreg", 0);
  pxar::mDelay(1000);
  fApi->setDAC("Vcal", fParVcal);
  pxar::mDelay(1000);

  setTrimBits(15);  
  
  // -- determine minimal VthrComp 
  banner("VthrComp thr map (minimal VthrComp)"); 
  vector<TH1*> thr0 = scurveMaps("vthrcomp", "TrimThr0", fParNtrig, 0, 200, 1); 
  vector<int> minVthrComp = getMinimumVthrComp(thr0, 10, 2.); 
  string bla("TRIM determined minimal VthrComp: ");
  LOG(logDEBUG) << bla; 
  for (unsigned int i = 0; i < minVthrComp.size(); ++i) LOG(logDEBUG) << "  " << minVthrComp[i]; 
  
  vector<int> VthrComp; 
  TH2D* h2(0); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    VthrComp.push_back(static_cast<int>(minVthrComp[iroc])); 
    LOG(logDEBUG) << " ROC " << (int)rocIds[iroc] << " setting VthrComp = " << (int)(minVthrComp[iroc]);
    fApi->setDAC("VthrComp", static_cast<uint8_t>(minVthrComp[iroc]), rocIds[iroc]); 
  }

  // -- determine pixel with largest VCAL threshold
  banner("Vcal thr map (pixel with maximum Vcal thr)"); 
  vector<TH1*> thr1 = scurveMaps("vcal", "TrimThr1", fParNtrig, 0, 200, 1); 

  vector<int> Vcal; 
  // -- switch off all pixels (and enable one pixel per ROC in loop below!)
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  double vcalMean(-1), vcalMin(999.), vcalMax(-1.); 
  string hname("");
  string::size_type s1, s2; 
  int rocid(-1); 
  for (unsigned int i = 0; i < thr1.size(); ++i) {
    h2 = (TH2D*)thr1[i]; 
    hname = h2->GetName();
    // -- skip sig_ and thn_ histograms
    if (string::npos == hname.find("thr_")) continue;

    TH1* d1 = distribution(h2, 256, 0., 256.); 
    vcalMean = d1->GetMean(); 
    vcalMin = d1->GetMean() - NSIGMA*d1->GetRMS();
    vcalMax = d1->GetMean() + NSIGMA*d1->GetRMS();
    delete d1; 

    s1 = hname.rfind("_C");
    s2 = hname.rfind("_V");
    rocid = atoi(hname.substr(s1+2, s2-s1-2).c_str()); 
    double maxVcal(-1.), vcal(0.); 
    int ix(-1), iy(-1); 
    for (int ic = 0; ic < h2->GetNbinsX(); ++ic) {
      for (int ir = 0; ir < h2->GetNbinsY(); ++ir) {
	vcal = h2->GetBinContent(ic+1, ir+1); 
	if (vcal > vcalMin && vcal > maxVcal && vcal < vcalMax) {
	  maxVcal = vcal;
	  ix = ic; 
	  iy = ir; 
	}
      }
    }

    LOG(logINFO) << "   roc " << getIdxFromId(rocid) << " with ID = " << rocid 
		 << "  has maximal Vcal " << maxVcal << " for pixel " << ix << "/" << iy
		 << " mean/min/max = " << vcalMean << "/" << vcalMin << "/" <<vcalMax;
    Vcal.push_back(static_cast<int>(maxVcal)); 

    fTrimBits[getIdxFromId(rocid)][ix][iy] = 1;
    fApi->_dut->testPixel(ix, iy, true, rocid);
    fApi->_dut->maskPixel(ix, iy, false, rocid);
  }
  setTrimBits();

  TH1D *h = new TH1D("trimh", "trimh", 256, 0., 256.); 
  vector<int> rocDone;
  int itrim(0), vcalHi(200); 
  do {
    if (rocDone.size() == rocIds.size()) break;
    fApi->setDAC("vtrim", itrim);
    vector<pair<uint8_t, vector<pixel> > > results = fApi->getEfficiencyVsDAC("vcal", 0, vcalHi, FLAG_FORCE_SERIAL | FLAG_FORCE_MASKED, 10);
    double minThr(999.), maxThr(-99); 
    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
      if (rocDone.end() != find(rocDone.begin(), rocDone.end(), rocIds[iroc])) continue;
      
      fillDacHist(results, h, -1, -1, rocIds[iroc]);
      threshold(h); 
      
      h->Draw(); 
      PixTest::update(); 
      
      LOG(logDEBUG) << "itrim = " << itrim << " for ROC " << (int)rocIds[iroc] << " -> vcal thr = " << fThreshold;
      if (fThreshold < minThr) minThr = fThreshold; 
      if (fThreshold > maxThr) maxThr = fThreshold; 
      if (fThreshold < fParVcal) {
	fApi->setDAC("vtrim", itrim, rocIds[iroc]);
	rocDone.push_back(rocIds[iroc]); 
	LOG(logDEBUG) << "---------> " << fThreshold << " < " << fParVcal << " for itrim = " << itrim << " ... break";
      }
    }

    if (minThr < fParVcal + 8) {
      ++itrim; 
    } else {
      itrim += 10;
    }

    vcalHi = maxThr + 40; 

  } while(itrim < 256);
  delete h; 

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    for (int ix = 0; ix < 52; ++ix) {
      for (int iy = 0; iy < 80; ++iy) {
	fTrimBits[iroc][ix][iy] = 7; 
      }
    }
  }
  setTrimBits();  

  // -- set trim bits
  int correction = 4;
  int NTRIG(5); 
  vector<TH1*> thr2  = scurveMaps("vcal", "TrimThr2", fParNtrig, 0, 200, 1); 
  vector<TH1*> thro = mapsWithString(thr2, "thr_");
  double maxthr = getMaximumThreshold(thro);
  double minthr = getMinimumThreshold(thro);
  banner(Form("TrimThr2 extremal thresholds: %f .. %f", minthr,  maxthr));
  if (maxthr < 245) maxthr += 10; 
  if (minthr > 10)  minthr -= 10; 
  vector<TH1*> thr2a = trimStep("trimStepCorr4", correction, thro, minthr, maxthr);


  correction = 2; 
  vector<TH1*> thr3  = scurveMaps("vcal", "TrimThr3", NTRIG, minthr, maxthr, 1); 
  thro.clear(); 
  thro = mapsWithString(thr3, "thr_");
  maxthr = getMaximumThreshold(thro);
  minthr = getMinimumThreshold(thro);
  banner(Form("TrimThr3 extremal thresholds: %f .. %f", minthr,  maxthr));
  if (maxthr < 245) maxthr += 10; 
  if (minthr > 10)  minthr -= 10; 
  vector<TH1*> thr3a = trimStep("trimStepCorr2", correction, thro, minthr, maxthr);
  
  correction = 1; 
  vector<TH1*> thr4  = scurveMaps("vcal", "TrimThr4", NTRIG, minthr, maxthr, 1); 
  thro.clear(); 
  thro = mapsWithString(thr4, "thr_");
  maxthr = getMaximumThreshold(thro);
  minthr = getMinimumThreshold(thro);
  banner(Form("TrimThr4 extremal thresholds: %f .. %f", minthr,  maxthr));
  if (maxthr < 245) maxthr += 10; 
  if (minthr > 10)  minthr -= 10; 
  vector<TH1*> thr4a = trimStep("trimStepCorr1a", correction, thro, minthr, maxthr);
  
  correction = 1; 
  vector<TH1*> thr5  = scurveMaps("vcal", "TrimThr5", NTRIG, minthr, maxthr, 1); 
  thro.clear(); 
  thro = mapsWithString(thr5, "thr_");
  maxthr = getMaximumThreshold(thro);
  minthr = getMinimumThreshold(thro);
  banner(Form("TrimThr5 extremal thresholds: %f .. %f", minthr,  maxthr));
  if (maxthr < 245) maxthr += 10; 
  if (minthr > 10)  minthr -= 10; 
  vector<TH1*> thr5a = trimStep("trimStepCorr1b", correction, thro, minthr, maxthr);

  // -- create trimMap
  for (unsigned int i = 0; i < thro.size(); ++i) {
    h2 = bookTH2D(Form("TrimMap_C%d", i), 
		  Form("TrimMap_C%d", i), 
		  52, 0., 52., 80, 0., 80.);
    for (int ix = 0; ix < 52; ++ix) {
      for (int iy = 0; iy < 80; ++iy) {
	h2->SetBinContent(ix+1, iy+1, fTrimBits[i][ix][iy]); 
      }
    }
    
    fHistList.push_back(h2); 
    fHistOptions.insert(make_pair(h2, "colz"));
  }

  vector<TH1*> thrF = scurveMaps("vcal", "TrimThrFinal", 5, fParVcal-20, fParVcal+20, 3); 
    
  PixTest::update(); 
}


// ----------------------------------------------------------------------
void PixTestTrim::trimBitTest() {

  LOG(logINFO) << "trimBitTest start "; 

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  
  vector<int>vtrim; 
  vtrim.push_back(255);
  vtrim.push_back(240);
  vtrim.push_back(150);
  vtrim.push_back(100);

  vector<int>btrim; 
  btrim.push_back(14);
  btrim.push_back(13);
  btrim.push_back(11);
  btrim.push_back(7);

  vector<vector<TH1*> > steps; 

  ConfigParameters *cp = fPixSetup->getConfigParameters();
  // -- start untrimmed
  bool ok = cp->setTrimBits(15);
  if (!ok) {
    LOG(logWARNING) << "could not set trim bits to " << 15; 
    return;
  }
  fApi->setDAC("CtrlReg", 0); 
  fApi->setDAC("Vtrim", 0); 
  LOG(logDEBUG) << "trimBitTest determine threshold map without trims "; 
  vector<TH1*> thr0 = mapsWithString(scurveMaps("Vcal", "TrimBitsThr0", fParNtrig, 0, 200, 1), "thr");
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  
  // -- now loop over all trim bits
  vector<TH1*> thr;
  double maxThr = getMaximumThreshold(thr0); 
  if (maxThr > 245.) maxThr = 245.; 
  for (unsigned int iv = 0; iv < vtrim.size(); ++iv) {
    thr.clear();
    ok = cp->setTrimBits(btrim[iv]);
    if (!ok) {
      LOG(logWARNING) << "could not set trim bits to " << btrim[iv]; 
      continue;
    }

    LOG(logDEBUG) << "trimBitTest initDUT with trim bits = " << btrim[iv]; 
    for (vector<uint8_t>::size_type iroc = 0; iroc < rocIds.size(); ++iroc) {
      fApi->_dut->updateTrimBits(cp->getRocPixelConfig(rocIds[iroc]), rocIds[iroc]);
    }

    fApi->setDAC("Vtrim", vtrim[iv]); 
    LOG(logDEBUG) << "trimBitTest threshold map with trim = " << btrim[iv]; 
    thr = mapsWithString(scurveMaps("Vcal", Form("TrimThr_trim%d", btrim[iv]), fParNtrig, 0, maxThr+10, (iv<3?1:5)), "thr");
    maxThr = getMaximumThreshold(thr); 
    if (maxThr > 245.) maxThr = 245.; 
    steps.push_back(thr); 
  }
  
  // -- and now determine threshild difference
  TH1 *h1(0); 
  double dthr(0.);
  for (unsigned int i = 0; i < steps.size(); ++i) {
    thr = steps[i];
    for (unsigned int iroc = 0; iroc < thr.size(); ++iroc) {
      h1 = bookTH1D(Form("TrimBit%d_C%d", btrim[i], iroc), Form("TrimBit%d_C%d", btrim[i], iroc), 256, 0., 256); 
      for (int ix = 0; ix < 52; ++ix) {
	for (int iy = 0; iy < 80; ++iy) {
	  dthr = thr0[iroc]->GetBinContent(ix+1, iy+1) - thr[iroc]->GetBinContent(ix+1, iy+1);
	  h1->Fill(dthr); 
	}
      }
      fHistList.push_back(h1); 
    }
  }

  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);

  if (h1) h1->Draw();
  PixTest::update(); 

  LOG(logINFO) << "trimBitTest done "; 
  
}



// ----------------------------------------------------------------------
int PixTestTrim::adjustVtrim() {
  int vtrim = -1;
  int thr(255), thrOld(255);
  int ntrig(10); 
  do {
    vtrim++;
    fApi->setDAC("Vtrim", vtrim);
    thrOld = thr;
    thr = pixelThreshold("Vcal", ntrig, 0, 100); 
    LOG(logDEBUG) << vtrim << " thr " << thr;
  }
  while (((thr > fParVcal) || (thrOld > fParVcal) || (thr < 10)) && (vtrim < 200));
  vtrim += 5;
  fApi->setDAC("Vtrim", vtrim);
  LOG(logINFO) << "Vtrim set to " <<  vtrim;
  return vtrim;
}


// ----------------------------------------------------------------------
vector<TH1*> PixTestTrim::trimStep(string name, int correction, vector<TH1*> calOld, int vcalMin, int vcalMax) {

  int NTRIG(4); 

  if (vcalMin < 0) vcalMin = 0; 
  if (vcalMin > 255) vcalMax = 255; 
  
  int trim(1); 
  int trimBitsOld[16][52][80];
  for (unsigned int i = 0; i < calOld.size(); ++i) {
    cout << " ---> " << calOld[i]->GetName() << endl;
    for (int ix = 0; ix < 52; ++ix) {
      for (int iy = 0; iy < 80; ++iy) {
	trimBitsOld[i][ix][iy] = fTrimBits[i][ix][iy];
	if (calOld[i]->GetBinContent(ix+1, iy+1) > fParVcal) {
	  trim = fTrimBits[i][ix][iy] - correction; 
	} else {
	  trim = fTrimBits[i][ix][iy] + correction; 
	}
	if (trim < 1) trim = 1; 
	if (trim > 15) trim = 15; 
	fTrimBits[i][ix][iy] = trim; 
      }
    }
  } 	

  setTrimBits(); 
  vector<TH1*> tmp = scurveMaps("vcal", name, NTRIG, vcalMin, vcalMax, 1); 
  vector<TH1*> calNew = mapsWithString(tmp, "thr_"); 

  // -- check that things got better, else revert and leave up to next correction round
  for (unsigned int i = 0; i < calOld.size(); ++i) {
    for (int ix = 0; ix < 52; ++ix) {
      for (int iy = 0; iy < 80; ++iy) {
	if (TMath::Abs(calOld[i]->GetBinContent(ix+1, iy+1) - fParVcal) < TMath::Abs(calNew[i]->GetBinContent(ix+1, iy+1) - fParVcal)) {
	  trim = trimBitsOld[i][ix][iy];
	  calNew[i]->SetBinContent(ix+1, iy+1, calOld[i]->GetBinContent(ix+1, iy+1)); 
	} else {
	  trim = fTrimBits[i][ix][iy]; 
	}
	fTrimBits[i][ix][iy] = trim; 
	//	if (calNew[i]->GetBinContent(ix+1, iy+1) > 0) 
	//	  cout << "roc/col/row: " << i << "/" << ix << "/" << iy << " vcalThr = " << calNew[i]->GetBinContent(ix+1, iy+1) << endl;
      }
    }
  } 	
  setTrimBits(); 

  return calNew;
}


// ----------------------------------------------------------------------
void PixTestTrim::setTrimBits(int itrim) {
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int ir = 0; ir < rocIds.size(); ++ir){
    vector<pixelConfig> pix = fApi->_dut->getEnabledPixels(rocIds[ir]);
    for (unsigned int ipix = 0; ipix < pix.size(); ++ipix) {
      if (itrim > -1) {
	fTrimBits[ir][pix[ipix].column][pix[ipix].row] = itrim;
      }
      fApi->_dut->updateTrimBits(pix[ipix].column, pix[ipix].row, fTrimBits[ir][pix[ipix].column][pix[ipix].row], rocIds[ir]);
    }
  }
}


// ----------------------------------------------------------------------
void PixTestTrim::dummyAnalysis() {
  vector<string> names;
  names.push_back("TrimBit7"); 
  names.push_back("TrimBit11"); 
  names.push_back("TrimBit13"); 
  names.push_back("TrimBit15"); 
  TH1D *h1(0); 
  TH2D *h2(0); 
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    for (unsigned int in = 0; in < names.size(); ++in) {
      fId2Idx.insert(make_pair(rocIds[iroc], iroc)); 
      h1 = bookTH1D(Form("%s_C%d", names[in].c_str(), iroc), Form("%s_C%d", names[in].c_str(), rocIds[iroc]), 256, 0., 256.); 
      h1->SetMinimum(0.); 
      h1->SetDirectory(fDirectory); 
      setTitles(h1, "delta(Thr)", "pixels"); 
      
      for (int ix = 0; ix < 52; ++ix) {
	for (int iy = 0; iy < 80; ++iy) {
	  h1->Fill(gRandom->Gaus(60., 2.)); 
	}
      }
      
      fHistList.push_back(h1); 
    }
  }

  string name("TrimMap");
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    fId2Idx.insert(make_pair(rocIds[iroc], iroc)); 
    h2 = bookTH2D(Form("%s_C%d", name.c_str(), iroc), Form("%s_C%d", name.c_str(), rocIds[iroc]), 52, 0., 52., 80, 0., 80.); 
    h2->SetMinimum(0.); 
    h2->SetDirectory(fDirectory); 
    setTitles(h2, "col", "row"); 
    fHistOptions.insert(make_pair(h2, "colz"));

    double x; 
    for (int ix = 0; ix < 52; ++ix) {
      for (int iy = 0; iy < 80; ++iy) {
	x = gRandom->Gaus(7., 3.);
	if (x < 0) x = 0.;
	if (x > 15) x = 15.;
	h2->SetBinContent(ix+1, iy+1, static_cast<int>(x)); 
      }
    }
    
    fHistList.push_back(h2); 
  }


  name = "TrimThr5";
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    fId2Idx.insert(make_pair(rocIds[iroc], iroc)); 
    h2 = bookTH2D(Form("%s_C%d", name.c_str(), iroc), Form("%s_C%d", name.c_str(), rocIds[iroc]), 52, 0., 52., 80, 0., 80.); 
    h2->SetMinimum(0.); 
    h2->SetDirectory(fDirectory); 
    setTitles(h2, "col", "row"); 
    fHistOptions.insert(make_pair(h2, "colz"));

    for (int ix = 0; ix < 52; ++ix) {
      for (int iy = 0; iy < 80; ++iy) {
	h2->SetBinContent(ix+1, iy+1, static_cast<int>(gRandom->Gaus(40., 2.))); 
      }
    }
    h1 =  distribution(h2, 256, 0., 256.);
    
    fHistList.push_back(h2); 
    fHistList.push_back(h1); 
  }



  TH2D *h = (TH2D*)(*fHistList.begin());
  h->Draw(getHistOption(h).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 
  LOG(logINFO) << "PixTestTrim::dummyAnalysis() done";




}


// ----------------------------------------------------------------------
void PixTestTrim::output4moreweb() {
  list<TH1*>::iterator begin = fHistList.begin();
  list<TH1*>::iterator end = fHistList.end();
  
  TDirectory *pDir = gDirectory; 
  gFile->cd(); 
  for (list<TH1*>::iterator il = begin; il != end; ++il) {
    string name = (*il)->GetName(); 
    if (string::npos != name.find("TrimBit7")) {
      PixUtil::replaceAll(name, "_V0", ""); 
      TH1D *h = (TH1D*)((*il)->Clone(name.c_str()));
      h->SetDirectory(gDirectory); 
      h->Write(); 
    }

    if (string::npos != name.find("TrimBit11")) {
      PixUtil::replaceAll(name, "_V0", ""); 
      TH1D *h = (TH1D*)((*il)->Clone(name.c_str()));
      h->SetDirectory(gDirectory); 
      h->Write(); 
    }

    if (string::npos != name.find("TrimBit13")) {
      PixUtil::replaceAll(name, "_V0", ""); 
      TH1D *h = (TH1D*)((*il)->Clone(name.c_str()));
      h->SetDirectory(gDirectory); 
      h->Write(); 
    }

    if (string::npos != name.find("TrimBit15")) {
      PixUtil::replaceAll(name, "_V0", ""); 
      TH1D *h = (TH1D*)((*il)->Clone(name.c_str()));
      h->SetDirectory(gDirectory); 
      h->Write(); 
    }

    if (string::npos != name.find("TrimMap")) {
      PixUtil::replaceAll(name, "_V0", ""); 
      TH2D *h = (TH2D*)((*il)->Clone(name.c_str()));
      h->SetDirectory(gDirectory); 
      h->Write(); 
    }

    if (string::npos != name.find("dist_TrimThr5")) {
      PixUtil::replaceAll(name, "dist_", ""); 
      PixUtil::replaceAll(name, "TrimThr5", "VcalThresholdMap"); 
      PixUtil::replaceAll(name, "_V0", "Distribution"); 
      TH2D *h = (TH2D*)((*il)->Clone(name.c_str()));
      h->SetDirectory(gDirectory); 
      h->Write(); 
    } else if (string::npos != name.find("TrimThr5")) {
      PixUtil::replaceAll(name, "TrimThr5", "VcalThresholdMap"); 
      PixUtil::replaceAll(name, "_V0", ""); 
      TH2D *h = (TH2D*)((*il)->Clone(name.c_str()));
      h->SetDirectory(gDirectory); 
      h->Write(); 
    }




  }
  pDir->cd(); 
}
