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
      }
      if (!parName.compare("vcal")) {
	fParVcal = atoi(sval.c_str()); 
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

  LOG(logINFO) << "PixTestTrim::doTest() done ";
}

// ----------------------------------------------------------------------
void PixTestTrim::trimTest() {

  bool verbose(false);
  cacheDacs(verbose);
  fDirectory->cd();
  PixTest::update(); 
  banner(Form("PixTestTrim::trimTest() ntrig = %d, vcal = %d", fParNtrig, fParVcal));

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 

  fApi->setDAC("Vtrim", 0);
  fApi->setDAC("ctrlreg", 0);
  fApi->setDAC("Vcal", fParVcal);

  setTrimBits(15);  
  
  // -- determine minimal VthrComp 
  int NTRIG(5);
  map<int, int> rocVthrComp;
  print("VthrComp thr map (minimal VthrComp)"); 
  vector<TH1*> thr0 = scurveMaps("vthrcomp", "TrimThr0", NTRIG, 0, 150, 7); 
  vector<int> minVthrComp = getMinimumVthrComp(thr0, 10, 2.); 
  
  TH2D* h2(0); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    LOG(logINFO) << "ROC " << static_cast<int>(rocIds[iroc]) << " VthrComp = " << minVthrComp[iroc]; 
    fApi->setDAC("VthrComp", static_cast<uint8_t>(minVthrComp[iroc]), rocIds[iroc]); 
    rocVthrComp.insert(make_pair(rocIds[iroc], minVthrComp[iroc])); 
  }

  // -- determine pixel with largest VCAL threshold
  print("Vcal thr map (pixel with maximum Vcal thr)"); 
  vector<TH1*> thr1 = scurveMaps("vcal", "TrimThr1", NTRIG, 0, 150, 1); 

  vector<int> Vcal; 
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  double vcalMean(-1), vcalMin(999.), vcalMax(-1.); 
  string hname("");
  string::size_type s1, s2; 
  int rocid(-1); 
  double NSIGMA(3); 
  map<string, TH2D*> maps; 
  for (unsigned int i = 0; i < thr1.size(); ++i) {
    h2 = (TH2D*)thr1[i]; 
    hname = h2->GetName();

    TH1* d1 = distribution(h2, 256, 0., 256.); 
    vcalMean = d1->GetMean(); 
    vcalMin = d1->GetMean() - NSIGMA*d1->GetRMS();
    if (vcalMin < 0) vcalMin = 0; 
    vcalMax = d1->GetMean() + NSIGMA*d1->GetRMS();
    if (vcalMax > 255) vcalMin = 255; 
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

    fTrimBits[getIdxFromId(rocid)][ix][iy] = 0;
    //??    fTrimBits[getIdxFromId(rocid)][ix][iy] = 1;

    fApi->_dut->testPixel(ix, iy, true, rocid);
    fApi->_dut->maskPixel(ix, iy, false, rocid);

    h2 = bookTH2D(Form("trim_VCAL_VTRIM_C%d", rocid), 
		  Form("trim_VCAL_VTRIM_c%d_r%d_C%d", ix, iy, rocid), 
		  256, 0., 256., 256, 0., 256.); 
    h2->SetMinimum(0.); 
    setTitles(h2, "vcal", "vtrim"); 
    fHistList.push_back(h2);
    fHistOptions.insert(make_pair(h2, "colz")); 
    maps.insert(make_pair(Form("trim_VCAL_VTRIM_C%d", rocid), h2)); 
  }
  setTrimBits();

  // -- determine VTRIM with this pixel
  int cnt(0); 
  bool done(false);
  vector<pair<uint8_t, pair<uint8_t, vector<pixel> > > >  results;
  while (!done) {
    try {
      results = fApi->getEfficiencyVsDACDAC("vcal", 0, 200, "vtrim", 0, 255, FLAG_FORCE_MASKED, NTRIG);
      done = true;
    } catch(pxarException &e) {
      LOG(logCRITICAL) << "pXar execption: "<< e.what(); 
      fNDaqErrors = 666667;
      ++cnt;
    }
    done = (cnt>2) || done;
  }
  
  for (unsigned int i = 0; i < results.size(); ++i) {
    pair<uint8_t, pair<uint8_t, vector<pixel> > > v = results[i];
    int idac1 = v.first; 
    pair<uint8_t, vector<pixel> > w = v.second;      
    int idac2 = w.first;
    vector<pixel> wpix = w.second;
    
    for (unsigned ipix = 0; ipix < wpix.size(); ++ipix) {
      h2 = maps[Form("trim_VCAL_VTRIM_C%d", wpix[ipix].roc())];
      if (h2) {
	h2->Fill(idac1, idac2, wpix[ipix].value()); 
      } else {
	LOG(logDEBUG) << "wrong pixel decoded"; 
      }
    }
  }
  
  map<int, int> rocTrim;
  int vtrim(0);
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    h2 = maps[Form("trim_VCAL_VTRIM_C%d", rocIds[iroc])];
    TH1D *hy = h2->ProjectionY("_py", 150, 150); 
    double thresh = hy->FindLastBinAbove(0.5*NTRIG);
    delete hy; 
    vtrim = static_cast<int>(thresh); 
    double oldThr(99.); 
    for (int itrim = vtrim-20; itrim > 0; --itrim) {
      TH1D *hx = h2->ProjectionX("_px", itrim, itrim); 
      threshold(hx); 
      delete hx; 
      if (fThreshold > fParVcal) {
	if (TMath::Abs(fParVcal - fThreshold) < TMath::Abs(fParVcal - oldThr)) {
	  rocTrim.insert(make_pair(rocIds[iroc], itrim)); 
	  fApi->setDAC("vtrim", itrim, rocIds[iroc]);
	  LOG(logDEBUG) << " vtrim: vcal = " << fThreshold << " < " << fParVcal << " for itrim = " << itrim << "; old thr = " << oldThr
			<< " ... break";
	} else {
	  rocTrim.insert(make_pair(rocIds[iroc], itrim+1)); 
	  fApi->setDAC("vtrim", itrim+1, rocIds[iroc]);
	  LOG(logDEBUG) << "vtrim: vcal = " << fThreshold << " < " << fParVcal << " for itrim+1 = " << itrim+1 << "; old thr = " << oldThr
			<< " ... break";
	}
	break;
      } else {
	oldThr = fThreshold;
      }
    }
  }

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
  vector<TH1*> thr2  = scurveMaps("vcal", "TrimThr2", fParNtrig, 0, 200, 1); 
  double maxthr = getMaximumThreshold(thr2);
  double minthr = getMinimumThreshold(thr2);
  print(Form("TrimStepCorr4 extremal thresholds: %f .. %f", minthr,  maxthr));
  if (maxthr < 245) maxthr += 10; 
  if (minthr > 10)  minthr -= 10; 
  vector<TH1*> thr2a = trimStep("trimStepCorr4", correction, thr2, static_cast<int>(minthr), static_cast<int>(maxthr));


  correction = 2; 
  maxthr = getMaximumThreshold(thr2a);
  minthr = getMinimumThreshold(thr2a);
  print(Form("TrimStepCorr2 extremal thresholds: %f .. %f", minthr,  maxthr));
  if (maxthr < 245) maxthr += 10; 
  if (minthr > 10)  minthr -= 10; 
  vector<TH1*> thr3a = trimStep("trimStepCorr2", correction, thr2a, static_cast<int>(minthr), static_cast<int>(maxthr));
  
  correction = 1; 
  maxthr = getMaximumThreshold(thr3a);
  minthr = getMinimumThreshold(thr3a);
  print(Form("TrimStepCorr1a extremal thresholds: %f .. %f", minthr,  maxthr));
  if (maxthr < 245) maxthr += 10; 
  if (minthr > 10)  minthr -= 10; 
  vector<TH1*> thr4a = trimStep("trimStepCorr1a", correction, thr3a, static_cast<int>(minthr), static_cast<int>(maxthr));
  
  correction = 1; 
  maxthr = getMaximumThreshold(thr4a);
  minthr = getMinimumThreshold(thr4a);
  print(Form("TrimStepCorr1b extremal thresholds: %f .. %f", minthr,  maxthr));
  if (maxthr < 245) maxthr += 10; 
  if (minthr > 10)  minthr -= 10; 
  vector<TH1*> thr5a = trimStep("trimStepCorr1b", correction, thr4a, static_cast<int>(minthr), static_cast<int>(maxthr));

  // -- create trimMap
  for (unsigned int i = 0; i < thr5a.size(); ++i) {
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

  print(Form("TrimThrFinal extremal thresholds: %d .. %d", fParVcal-20,  fParVcal+20));
  vector<TH1*> thrF = scurveMaps("vcal", "TrimThrFinal", fParNtrig, fParVcal-20, fParVcal+20, 9); 
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
  fPixSetup->getConfigParameters()->setTrimVcalSuffix(Form("%d", fParVcal)); 
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
      ix = pix[ipix].column();
      iy = pix[ipix].row();
      fTrimBits[i][ix][iy] = pix[ipix].trim();
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
  vector<TH1*> thr0 = mapsWithString(scurveMaps("Vcal", "TrimBitsThr0", fParNtrig, 0, 200, 7), "thr");
  
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
    thr = mapsWithString(scurveMaps("Vcal", Form("TrimThr_trim%d", btrim[iv]), fParNtrig, 0, static_cast<int>(maxThr)+10, 7), "thr");
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
      h1 = bookTH1D(Form("TrimBit%d_C%d", btrim[i], rocIds[iroc]), Form("TrimBit%d_C%d", btrim[i], rocIds[iroc]), 256, 0., 256); 
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
  vector<TH1*> calNew = scurveMaps("vcal", name, NTRIG, vcalMin, vcalMax, 1); 

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
	fTrimBits[ir][pix[ipix].column()][pix[ipix].row()] = itrim;
      }
      fApi->_dut->updateTrimBits(pix[ipix].column(), pix[ipix].row(), fTrimBits[ir][pix[ipix].column()][pix[ipix].row()], rocIds[ir]);
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
  }
  pDir->cd(); 
}

