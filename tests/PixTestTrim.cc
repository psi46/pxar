#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include <fstream>

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
PixTestTrim::PixTestTrim(PixSetup *a, std::string name) : PixTest(a, name), fParVcal(-1), fParNtrig(-1) {
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
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());
      if (!parName.compare("ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParNtrig  ->" << fParNtrig << "<- from sval = " << sval;
      }
      if (!parName.compare("vcal")) {
	fParVcal = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParVcal  ->" << fParVcal << "<- from sval = " << sval;
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
  if (!command.compare("trim")) {
    trimTest(); 
    return;
  }
  if (!command.compare("getvthrcompthr")) {
    getVthrCompThr(); 
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

  fDirectory->cd();
  PixTest::update(); 
  bigBanner(Form("PixTestTrim::doTest()"));

  trimTest(); 
  TH1 *h1 = (*fDisplayedHist); 
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update(); 

  trimBitTest();
  h1 = (*fDisplayedHist); 
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update(); 

  getVthrCompThr();

  LOG(logINFO) << "PixTestTrim::doTest() done ";
}

// ----------------------------------------------------------------------
void PixTestTrim::trimTest() {

  bool verbose(false);
  cacheDacs(verbose);
  fDirectory->cd();
  PixTest::update(); 
  banner(Form("PixTestTrim::trimTest() ntrig = %d, vcal = %d", fParNtrig, fParVcal));

  double NSIGMA(2); 


  fPIX.clear(); 

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 

  fApi->setDAC("Vtrim", 0);
  fApi->setDAC("ctrlreg", 0);
  fApi->setDAC("Vcal", fParVcal);

  setTrimBits(15);  
  
  // -- determine minimal VthrComp 
  map<int, int> rocVthrComp;
  print("VthrComp thr map (minimal VthrComp)"); 
  vector<TH1*> thr0 = scurveMaps("vthrcomp", "TrimThr0", fParNtrig, 0, 200, 1); 
  vector<int> minVthrComp = getMinimumVthrComp(thr0, 10, 2.); 
  string bla("TRIM determined minimal VthrComp: ");
  LOG(logDEBUG) << bla; 
  for (unsigned int i = 0; i < minVthrComp.size(); ++i) LOG(logDEBUG) << "  " << minVthrComp[i]; 
  
  TH2D* h2(0); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    LOG(logDEBUG) << " ROC " << (int)rocIds[iroc] << " setting VthrComp = " << (int)(minVthrComp[iroc]);
    fApi->setDAC("VthrComp", static_cast<uint8_t>(minVthrComp[iroc]), rocIds[iroc]); 
    rocVthrComp.insert(make_pair(rocIds[iroc], minVthrComp[iroc])); 
  }

  // -- determine pixel with largest VCAL threshold
  print("Vcal thr map (pixel with maximum Vcal thr)"); 
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
  map<int, int> rocTrim;
  int itrim(0), vcalHi(200); 
  do {
    if (rocDone.size() == rocIds.size()) break;
    fApi->setDAC("vtrim", itrim);
    vector<pair<uint8_t, vector<pixel> > > results;
    int cnt(0); 
    bool done(false);
    while (!done) {
      try {
	results = fApi->getEfficiencyVsDAC("vcal", 0, vcalHi, FLAG_FORCE_SERIAL | FLAG_FORCE_MASKED, 10);
	done = true;
      } catch(pxarException &e) {
	LOG(logCRITICAL) << "pXar execption: "<< e.what(); 
	++cnt;
      }
      done = (cnt>5) || done;
    }

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
	rocTrim.insert(make_pair(rocIds[iroc], itrim)); 
	LOG(logDEBUG) << "---------> " << fThreshold << " < " << fParVcal << " for itrim = " << itrim << " ... break";
      }
    }

    if (minThr < fParVcal + 8) {
      ++itrim; 
    } else {
      itrim += 10;
    }

    vcalHi = static_cast<int>(maxThr) + 40; 

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
  int NTRIG(fParNtrig); 
  vector<TH1*> thr2  = scurveMaps("vcal", "TrimThr2", fParNtrig, 0, 200, 1); 
  vector<TH1*> thro = mapsWithString(thr2, "thr_");
  double maxthr = getMaximumThreshold(thro);
  double minthr = getMinimumThreshold(thro);
  print(Form("TrimThr2 extremal thresholds: %f .. %f", minthr,  maxthr));
  if (maxthr < 245) maxthr += 10; 
  if (minthr > 10)  minthr -= 10; 
  vector<TH1*> thr2a = trimStep("trimStepCorr4", correction, thro, static_cast<int>(minthr), static_cast<int>(maxthr));


  correction = 2; 
  vector<TH1*> thr3  = scurveMaps("vcal", "TrimThr3", NTRIG, static_cast<int>(minthr), static_cast<int>(maxthr), 1); 
  thro.clear(); 
  thro = mapsWithString(thr3, "thr_");
  maxthr = getMaximumThreshold(thro);
  minthr = getMinimumThreshold(thro);
  print(Form("TrimThr3 extremal thresholds: %f .. %f", minthr,  maxthr));
  if (maxthr < 245) maxthr += 10; 
  if (minthr > 10)  minthr -= 10; 
  vector<TH1*> thr3a = trimStep("trimStepCorr2", correction, thro, static_cast<int>(minthr), static_cast<int>(maxthr));
  
  correction = 1; 
  vector<TH1*> thr4  = scurveMaps("vcal", "TrimThr4", NTRIG, static_cast<int>(minthr), static_cast<int>(maxthr), 1); 
  thro.clear(); 
  thro = mapsWithString(thr4, "thr_");
  maxthr = getMaximumThreshold(thro);
  minthr = getMinimumThreshold(thro);
  print(Form("TrimThr4 extremal thresholds: %f .. %f", minthr,  maxthr));
  if (maxthr < 245) maxthr += 10; 
  if (minthr > 10)  minthr -= 10; 
  vector<TH1*> thr4a = trimStep("trimStepCorr1a", correction, thro, static_cast<int>(minthr), static_cast<int>(maxthr));
  
  correction = 1; 
  vector<TH1*> thr5  = scurveMaps("vcal", "TrimThr5", NTRIG, static_cast<int>(minthr), static_cast<int>(maxthr), 1); 
  thro.clear(); 
  thro = mapsWithString(thr5, "thr_");
  maxthr = getMaximumThreshold(thro);
  minthr = getMinimumThreshold(thro);
  print(Form("TrimThr5 extremal thresholds: %f .. %f", minthr,  maxthr));
  if (maxthr < 245) maxthr += 10; 
  if (minthr > 10)  minthr -= 10; 
  vector<TH1*> thr5a = trimStep("trimStepCorr1b", correction, thro, static_cast<int>(minthr), static_cast<int>(maxthr));

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

    TH1* d1 = distribution(h2, 16, 0., 16.); 
    fHistList.push_back(d1); 
  }

  vector<TH1*> thrF = scurveMaps("vcal", "TrimThrFinal", fParNtrig, fParVcal-20, fParVcal+20, 3); 
  string trimMeanString, trimRmsString; 
  for (unsigned int i = 0; i < thrF.size(); ++i) {
    hname = thrF[i]->GetName();
    // -- skip sig_ and thn_ histograms
    if (string::npos == hname.find("dist_thr_")) continue;
    trimMeanString += Form("%6.2f ", thrF[i]->GetMean()); 
    trimRmsString += Form("%6.2f ", thrF[i]->GetRMS()); 
  }

  TH1 *h1 = (*fDisplayedHist); 
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update(); 
  restoreDacs();
  string vtrimString, vthrcompString; 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    fApi->setDAC("vtrim", rocTrim[rocIds[iroc]], rocIds[iroc]);
    vtrimString += Form("%3d ", rocTrim[rocIds[iroc]]); 
    fApi->setDAC("vthrcomp", rocVthrComp[rocIds[iroc]], rocIds[iroc]);
    vthrcompString += Form("%3d ", rocVthrComp[rocIds[iroc]]); 
  }

  // -- save into files
  saveDacs();
  saveTrimBits();
  
  // -- summary printout
  LOG(logINFO) << "PixTestAlive::trimTest() done";
  LOG(logINFO) << "vtrim:     " << vtrimString; 
  LOG(logINFO) << "vthrcomp:  " << vthrcompString; 
  LOG(logINFO) << "vcal mean: " << trimMeanString; 
  LOG(logINFO) << "vcal RMS:  " << trimRmsString; 
}


// ----------------------------------------------------------------------
void PixTestTrim::trimBitTest() {

  cacheDacs();

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  unsigned int nrocs = rocIds.size();

  // -- cache trim bits
  for (unsigned int i = 0; i < nrocs; ++i) {
    vector<pixelConfig> pix = fApi->_dut->getEnabledPixels(rocIds[i]);
    int ix(-1), iy(-1); 
    for (unsigned int ipix = 0; ipix < pix.size(); ++ipix) {
      ix = pix[ipix].column;
      iy = pix[ipix].row;
      fTrimBits[i][ix][iy] = pix[ipix].trim;
    }
  }
  
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

  fDirectory->cd();
  PixTest::update(); 
  banner(Form("PixTestTrim::trimBitTest() ntrig = %d, vtrims = %d %d %d %d", 
	      fParNtrig, vtrim[0], vtrim[1], vtrim[2], vtrim[3]));

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

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
    thr = mapsWithString(scurveMaps("Vcal", Form("TrimThr_trim%d", btrim[iv]), fParNtrig, 0, static_cast<int>(maxThr)+10, 1), "thr");
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
  restoreDacs();
  setTrimBits(); 
  LOG(logINFO) << "PixTestTrim::trimBitTest() done "; 
  
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
void PixTestTrim::output4moreweb() {
  print("PixTestTrim::output4moreweb()"); 
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
      continue;
    }

    if (string::npos != name.find("TrimBit11")) {
      PixUtil::replaceAll(name, "_V0", ""); 
      TH1D *h = (TH1D*)((*il)->Clone(name.c_str()));
      h->SetDirectory(gDirectory); 
      h->Write(); 
      continue;
    }

    if (string::npos != name.find("TrimBit13")) {
      PixUtil::replaceAll(name, "_V0", ""); 
      TH1D *h = (TH1D*)((*il)->Clone(name.c_str()));
      h->SetDirectory(gDirectory); 
      h->Write(); 
      continue;
    }

    if (string::npos != name.find("TrimBit14")) {
      PixUtil::replaceAll(name, "_V0", ""); 
      TH1D *h = (TH1D*)((*il)->Clone(name.c_str()));
      h->SetDirectory(gDirectory); 
      h->Write(); 
      continue;
    }

    if (string::npos != name.find("TrimMap")) {
      PixUtil::replaceAll(name, "_V0", ""); 
      TH2D *h = (TH2D*)((*il)->Clone(name.c_str()));
      h->SetDirectory(gDirectory); 
      h->Write(); 
      continue;
    }

    //dist_thr_TrimThrFinal_vcal_C0_V0;1
    //VcalThresholdMap_C0Distribution
    if (string::npos != name.find("dist_thr_TrimThrFinal_vcal")) {
      PixUtil::replaceAll(name, "dist_thr_", ""); 
      PixUtil::replaceAll(name, "TrimThrFinal_vcal", "VcalThresholdTrimmedMap"); 
      PixUtil::replaceAll(name, "_V0", "Distribution"); 
      TH1D *h = (TH1D*)((*il)->Clone(name.c_str()));
      h->SetDirectory(gDirectory); 
      h->Write(); 
      continue;
    }

    if (string::npos != name.find("thr_TrimThrFinal_vcal")) {
      PixUtil::replaceAll(name, "thr_TrimThrFinal_vcal", "VcalThresholdTrimmedMap"); 
      PixUtil::replaceAll(name, "_V0", ""); 
      TH2D *h = (TH2D*)((*il)->Clone(name.c_str()));
      h->SetDirectory(gDirectory); 
      h->Write(); 
      continue;
    }



    // VcalThresholdMap_C{ChipNo}Distribution

  }
  pDir->cd(); 
}

// ----------------------------------------------------------------------
void PixTestTrim::getVthrCompThr() {

  /*First determine whether there are hot pixels by starting at low vthrcomp
  and then raising, and checking for outliers
  Next, loop through different Vthrcomp values to determine a threshold
  value for Vthrcomp below which no hits are seen without X-ray source
  Second bit may not be necessary anymore, but code already exists for this
  and it probably wont take long, and does not hurt results, but arguable
  improves them 
  */

  banner(Form("PixTestPretest::doVthrCompThr() ntrig = %d, vcal = %d", fParNtrig, fParVcal));
  cacheDacs();
  
  fDirectory->cd();

  fApi->setDAC("ctrlreg", 4);
  fApi->setDAC("vcal", fParVcal);
  fFinal = 0;
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  
  //-- Check and mask hot pixels
  //loop through vthrcomp
  for (int vThrComp = 90; vThrComp < 96; vThrComp += 1) {
    LOG(logDEBUG) << "setting vthrcomp to " << vThrComp;
    //set vthrcomp
    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
      fApi->setDAC("vthrcomp", vThrComp,iroc); 
    }
    //pixel alive
    pair<vector<TH2D*>,vector<TH2D*> > tests = xEfficiencyMaps("xrayAlive", fParNtrig, FLAG_CHECK_ORDER | FLAG_FORCE_UNMASKED); 
    vector<TH2D*> test1 = tests.second;
    //check for hot pixels
    for (unsigned int i = 0; i < test1.size(); ++i){
      vector<pair<int,int> > hotPixels = checkHotPixels(test1[i]);
      for (unsigned int i=0; i<hotPixels.size();++i) {
	fApi->_dut->maskPixel(hotPixels[i].first, hotPixels[i].second, true, rocIds[i]);
      }
    }
  }

  //-- Calcualte VthrComp

  restoreDacs();
  cacheDacs();
 
  vector<int> currVthrComp(rocIds.size(),0);
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    currVthrComp[iroc] = fApi->_dut->getDAC(iroc,"vthrcomp");
  }
  
  //Leave -1 at end of vthrCompValues[]!!
  int vthrCompValues[] = {100,97,96,95,94,93,92,91,90,85,80,75,70,65,60,55,50,45,40,35,30,25,20,15,10,5,0,-1};
  int len = sizeof(vthrCompValues)/ sizeof(*vthrCompValues);
  vector<int> index(len,0); // lets each ROC have track where it is in vthrCompValues
  
  vector<int> xHits(rocIds.size(),1);
  string  currVthrCompString = getVthrCompString(rocIds, currVthrComp); 
  LOG(logINFO) << "VthrComp Loop Running";
  LOG(logINFO) << "#### VthrComp of ROCs (per ROC): " << currVthrCompString;

  //initially check the default values
  xHits = xPixelAliveSingleSweep();
  if(fFinal){
    LOG(logINFO) << "Conducting Second Check";
    xHits = xPixelAliveSingleSweep();
    if(fFinal){
      LOG(logINFO) << "Conducting Third Check";
      xHits = xPixelAliveSingleSweep();
      if (!fFinal){
        LOG(logINFO) << "Failed Third Check";
      }
    }
    LOG(logINFO) << "Failed Second Check";
  }

  
  if(!fFinal){ 
  // if initial value was accepted, don't need anything more
    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){   
      while(currVthrComp[iroc] < vthrCompValues[index[iroc]]){
        //set the vthrcomp of each ROC to first in list greater than default
        index[iroc] += 1;
      }
      currVthrComp[iroc] = vthrCompValues[index[iroc]];
      fApi->setDAC("vthrcomp", currVthrComp[iroc],iroc);  
    }    
  }

  
  //Loop through values in hardcoded list vthrCompValues
  while(!fFinal){
    currVthrCompString = getVthrCompString(rocIds, currVthrComp); 
    LOG(logINFO) << "#### VthrComp of ROCs (per ROC): " << currVthrCompString;
    xHits = xPixelAliveSingleSweep();
    if(fFinal){
      LOG(logINFO) << "Conducting Second Check";
      xHits = xPixelAliveSingleSweep();
      if(fFinal){
        LOG(logINFO) << "Conducting Third Check";
        xHits = xPixelAliveSingleSweep();
        if (!fFinal){
          LOG(logINFO) << "Failed Third Check";
        }
        continue;
      } else{
      LOG(logINFO) << "Failed Second Check";
      }
    }        
    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
      //Only lower vthrcomp if individual ROC still has hits
      if (xHits[iroc] != 0){
        index[iroc]+=1;
        if (vthrCompValues[index[iroc]]==-1){
          //If we get through vthrCompValues list, then exit
          LOG(logERROR) << "Detected unexpected hits for minimum VthrComp value = " 
			<< vthrCompValues[index[iroc]-1] << "for ROC # " << iroc;
          LOG(logERROR) << "Now exiting program, consider removing ROC, or modifying vthrCompValues[] in this file";
	  //          exit(EXIT_FAILURE);
        }
        currVthrComp[iroc] = vthrCompValues[index[iroc]]; 
        fApi->setDAC("vthrcomp", currVthrComp[iroc],iroc);          
      }
    }
  }
  LOG(logINFO) << "Finished VthrComp scan for all ROCs";
  LOG(logINFO) << "VthrComp thresholds for ROCs in order: " << currVthrCompString;
  
  ofstream OutputFile;
  OutputFile.open(Form("%s/%s", fPixSetup->getConfigParameters()->getDirectory().c_str(), "xraythr-vthrcomp.dat"));
  unsigned nRocs = rocIds.size(); 
  for (unsigned int iroc = 0; iroc < nRocs; ++iroc) {
    OutputFile << static_cast<int>(currVthrComp[iroc]) << endl;
  }
  OutputFile.close();
  
  restoreDacs();
  LOG(logINFO) << "PixTestPretest::getVthrCompThr() done";
    
}

// ----------------------------------------------------------------------
vector<int> PixTestTrim::xPixelAliveSingleSweep() { 
  
  pair<vector<TH2D*>,vector<TH2D*> > tests = xEfficiencyMaps("xrayAlive", fParNtrig, FLAG_CHECK_ORDER | FLAG_FORCE_UNMASKED); 
  vector<TH2D*> test2 = tests.first;
  vector<TH2D*> test3 = tests.second;
  vector<int> deadPixel(test2.size(), 0); 
  vector<int> probPixel(test2.size(), 0);
  vector<int> xHits(test3.size(),0);
  for (unsigned int i = 0; i < test2.size(); ++i) {
    fHistOptions.insert(make_pair(test2[i], "colz"));
    fHistOptions.insert(make_pair(test3[i], "colz"));    
    for (int ix = 0; ix < test2[i]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < test2[i]->GetNbinsY(); ++iy) {
	// -- count dead pixels
	if (test2[i]->GetBinContent(ix+1, iy+1) < fParNtrig) {
	  ++probPixel[i];
	  if (test2[i]->GetBinContent(ix+1, iy+1) < 1) {
	    ++deadPixel[i];
	  }  
	}
	// -- Count X-ray hits detected
	if (test3[i]->GetBinContent(ix+1,iy+1)>0){
	  xHits[i] += static_cast<int> (test3[i]->GetBinContent(ix+1,iy+1));
	}
      }
    }
  }

  for (unsigned int i = 0; i < xHits.size(); ++i){
    if (xHits[i] !=0){
      fFinal=0;
      break;
    }
    //Go into final check mode only if all entries are zero
    fFinal = 1;
  }
  if (fFinal){
  //Print Pixel Alive for final VthrComp and single pixel run
    copy(test2.begin(), test2.end(), back_inserter(fHistList));
  }
  copy(test3.begin(), test3.end(), back_inserter(fHistList));

  TH2D *h = (TH2D*)(fHistList.back());

  h->Draw(getHistOption(h).c_str());
  
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 
 
  // -- summary printout
  string deadPixelString, probPixelString, xHitsString;
  int acceptDead(10); //check if there are too many dead pixels, sign of test having run poorly
  for (unsigned int i = 0; i < probPixel.size(); ++i) {
    probPixelString += Form(" %4d", probPixel[i]); 
    deadPixelString += Form(" %4d", deadPixel[i]);
    xHitsString     += Form(" %4d", xHits[i]);
    if (probPixel[i] > acceptDead) {
      LOG(logERROR) << "Likely problem with ROC #" << i << " as there are " << probPixel[i] << " dead pixels";
    }
  }

  LOG(logINFO) << "number of dead pixels (per ROC):    " << deadPixelString;
  LOG(logINFO) << "number of red-efficiency pixels:    " << probPixelString;
  LOG(logINFO) << "number of unexpected hits detected: " << xHitsString;
  return xHits;
}
 

// ----------------------------------------------------------------------
string PixTestTrim::getVthrCompString(vector<uint8_t>rocIds,vector<int> VthrComp ) {
  string vthrCompString = "";
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    vthrCompString += Form(" %4d", VthrComp[iroc]);
  }
  return vthrCompString;
}

