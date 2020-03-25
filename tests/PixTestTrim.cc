#include <stdlib.h>
#include <algorithm>
#include <iostream>
#include <fstream>

#include <TH1.h>
#include <TRandom.h>
#include <TMarker.h>
#include <TStopwatch.h>
#include <TStyle.h>

#include "PixTestTrim.hh"
#include "PixUtil.hh"
#include "log.h"
#include "helper.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestTrim)

// ----------------------------------------------------------------------
PixTestTrim::PixTestTrim(PixSetup *a, std::string name) : PixTest(a, name), fParVcal(35), fParNtrig(1) {
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
  if (!command.compare("dotest")) {
    doTest();
    return;
  }
  if (!command.compare("fulltest")) {
    doTest();
    return;
  }
  LOG(logDEBUG) << "did not find command ->" << command << "<-";
}


// ----------------------------------------------------------------------
string PixTestTrim::toolTip(string what) {
  if (string::npos != what.find("trimbits")) return string("check that all trim bits are functional");
  if (string::npos != what.find("trim")) return string("run the trim test");
  return string("nada");
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
}


// ----------------------------------------------------------------------
void PixTestTrim::doTest() {

  TStopwatch t;

  fDirectory->cd();
  PixTest::update();
  bigBanner(Form("PixTestTrim::doTest()"));

  fProblem = false;
  trimTest();
  if (fProblem) {
    LOG(logINFO) << "PixTestTrim::doTest() aborted because of problem ";
    return;
  }
  TH1 *h1 = (*fDisplayedHist);
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update();

  fProblem = false;
  trimBitTest();
  if (fProblem) {
    LOG(logINFO) << "PixTestTrim::doTest() aborted because of problem ";
    return;
  }
  h1 = (*fDisplayedHist);
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update();

  int seconds = t.RealTime();
  LOG(logINFO) << "PixTestTrim::doTest() done, duration: " << seconds << " seconds";
}

// ----------------------------------------------------------------------
void PixTestTrim::trimTest() {

  gStyle->SetPalette(1);
  bool verbose(false);
  cacheDacs(verbose);
  fDirectory->cd();
  PixTest::update();
  banner(Form("PixTestTrim::trimTest() ntrig = %d, vcal = %d", fParNtrig, fParVcal));

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  maskPixels();

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();

  fApi->setDAC("Vtrim", 0);
  fApi->setVcalLowRange();
  fApi->setDAC("Vcal", fParVcal);

  setTrimBits(15);

  // -- determine minimal VthrComp
  int NTRIG(fParNtrig);
  if (NTRIG < 5) NTRIG = 5;
  map<int, int> rocVthrComp;
  print("VthrComp thr map (minimal VthrComp)");
  vector<TH1*> thr0 = scurveMaps("vthrcomp", "TrimThr0", NTRIG, 10, 200, -1, -1, 7);
  PixTest::update();
  if (thr0.size()/3 != rocIds.size()) {
    LOG(logERROR) << "scurve map size " << thr0.size() << " does not agree with number of enabled ROCs " << rocIds.size();
    fProblem = true;
    return;
  }
  vector<int> minVthrComp = getMinimumVthrComp(thr0, 10, 2.);
  if(fParVcal<=30){         // for very low thresholds set VcThr slightly lower, because after trimming threshold shifts a bit downwards
    for(unsigned int i=0; i<minVthrComp.size();  i++) minVthrComp[i]-=5;
  }

  TH2D* h2(0);
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    LOG(logINFO) << "ROC " << static_cast<int>(rocIds[iroc]) << " VthrComp = " << minVthrComp[iroc];
    fApi->setDAC("VthrComp", static_cast<uint8_t>(minVthrComp[iroc]), rocIds[iroc]);
    rocVthrComp.insert(make_pair(rocIds[iroc], minVthrComp[iroc]));
  }

  // -- determine pixel with largest VCAL threshold
  print("Vcal thr map (pixel with maximum Vcal thr)");
  vector<TH1*> thr1 = scurveMaps("vcal", "TrimThr1", NTRIG, 10, 160, -1, -1, 1);
  PixTest::update();
  if (thr1.size() != rocIds.size()) {
    LOG(logERROR) << "scurve map size " << thr1.size() << " does not agree with number of enabled ROCs " << rocIds.size() << endl;
    fProblem = true;
    return;
  }

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

    // -- empty bins are ignored (Jamie)
    TH1* d1 = distribution(h2, 255, 1., 256.);
    vcalMean = d1->GetMean();
    vcalMin = d1->GetMean() - NSIGMA*d1->GetRMS();
    if (vcalMin < 0) vcalMin = 0;
    vcalMax = d1->GetMean() + NSIGMA*d1->GetRMS();
    if (vcalMax > 255) vcalMax = 255;
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
  PixTest::update();

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
  maskPixels();

  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    for (int ix = 0; ix < 52; ++ix) {
      for (int iy = 0; iy < 80; ++iy) {
	fTrimBits[iroc][ix][iy] = 7;
      }
    }
  }
  setTrimBits();
  PixTest::update();

  // -- set trim bits
  int correction = 4;
  int initialCorrectionThresholdMax = 150;
  vector<TH1*> thr2  = scurveMaps("vcal", "TrimThr2", fParNtrig, 0, initialCorrectionThresholdMax, -1, -1, 1);

  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    for (int ix = 0; ix < 52; ++ix) {
      for (int iy = 0; iy < 80; ++iy) {
        if (thr2[iroc]->GetBinContent(1+ix,1+iy) > initialCorrectionThresholdMax - 2) {
          thr2[iroc]->SetBinContent(1+ix,1+iy,0);
        }
      }
    }
  }

  if (thr2.size() != rocIds.size()) {
    LOG(logERROR) << "scurve map thr2 size " << thr2.size() << " does not agree with number of enabled ROCs " << rocIds.size();
    fProblem = true;
    return;
  }
  double maxthr = getMaximumThreshold(thr2);
  double minthr = getMinimumThreshold(thr2);
  print(Form("TrimStepCorr4 extremal thresholds: %f .. %f", minthr,  maxthr));
  if (maxthr < 245) maxthr += 10;
  if (minthr > 10)  minthr -= 10;
  vector<TH1*> thr2a = trimStep("trimStepCorr4", correction, thr2, static_cast<int>(minthr), static_cast<int>(maxthr));
  PixTest::update();
  if (thr2a.size() != rocIds.size()) {
    LOG(logERROR) << "scurve map thr2a size " << thr2a.size() << " does not agree with number of enabled ROCs " << rocIds.size();
    fProblem = true;
    return;
  }


  correction = 2;
  maxthr = getMaximumThreshold(thr2a);
  minthr = getMinimumThreshold(thr2a);
  print(Form("TrimStepCorr2 extremal thresholds: %f .. %f", minthr,  maxthr));
  if (maxthr < 245) maxthr += 10;
  if (minthr > 10)  minthr -= 10;
  vector<TH1*> thr3a = trimStep("trimStepCorr2", correction, thr2a, static_cast<int>(minthr), static_cast<int>(maxthr));
  PixTest::update();
  if (thr3a.size() != rocIds.size()) {
    LOG(logERROR) << "scurve map thr3a size " << thr3a.size() << " does not agree with number of enabled ROCs " << rocIds.size();
    fProblem = true;
    return;
  }

  correction = 1;
  maxthr = getMaximumThreshold(thr3a);
  minthr = getMinimumThreshold(thr3a);
  print(Form("TrimStepCorr1a extremal thresholds: %f .. %f", minthr,  maxthr));
  if (maxthr < 245) maxthr += 10;
  if (minthr > 10)  minthr -= 10;
  vector<TH1*> thr4a = trimStep("trimStepCorr1a", correction, thr3a, static_cast<int>(minthr), static_cast<int>(maxthr));
  PixTest::update();
  if (thr4a.size() != rocIds.size()) {
    LOG(logERROR) << "scurve map thr4a size " << thr4a.size() << " does not agree with number of enabled ROCs " << rocIds.size();
    fProblem = true;
    return;
  }

  correction = 1;
  maxthr = getMaximumThreshold(thr4a);
  minthr = getMinimumThreshold(thr4a);
  print(Form("TrimStepCorr1b extremal thresholds: %f .. %f", minthr,  maxthr));
  if (maxthr < 245) maxthr += 10;
  if (minthr > 10)  minthr -= 10;
  vector<TH1*> thr5a = trimStep("trimStepCorr1b", correction, thr4a, static_cast<int>(minthr), static_cast<int>(maxthr));
  PixTest::update();
  if (thr5a.size() != rocIds.size()) {
    LOG(logERROR) << "scurve map thr5a size " << thr5a.size() << " does not agree with number of enabled ROCs " << rocIds.size();
    fProblem = true;
    return;
  }

  // -- create trimMap
  string trimbitsMeanString(""), trimbitsRmsString("");
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

    trimbitsMeanString += Form("%6.2f ", d1->GetMean());
    trimbitsRmsString += Form("%6.2f ", d1->GetRMS());

    fHistList.push_back(d1);
  }

  print(Form("TrimThrFinal extremal thresholds: %d .. %d", fParVcal-20,  fParVcal+20));
  vector<TH1*> thrF = scurveMaps("vcal", "TrimThrFinal", fParNtrig, fParVcal-20, fParVcal+20, -1, -1, 9);
  PixTest::update();
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
  fPixSetup->getConfigParameters()->setTrimVcalSuffix(Form("%d", fParVcal), true);
  saveDacs();
  saveTrimBits();

  // -- summary printout
  LOG(logINFO) << "PixTestTrim::trimTest() done";
  LOG(logINFO) << "vtrim:     " << vtrimString;
  LOG(logINFO) << "vthrcomp:  " << vthrcompString;
  LOG(logINFO) << "vcal mean: " << trimMeanString;
  LOG(logINFO) << "vcal RMS:  " << trimRmsString;
  LOG(logINFO) << "bits mean: " << trimbitsMeanString;
  LOG(logINFO) << "bits RMS:  " << trimbitsRmsString;

  dutCalibrateOff();
}


// ----------------------------------------------------------------------
void PixTestTrim::trimBitTest() {

  cacheDacs();

  int NTRIG = static_cast<int>(0.5*fParNtrig);
  if (NTRIG < 5) NTRIG = 5;
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

  // fApi->setDAC("Vcal", 250);
  //  fApi->setDAC("Vthrcomp", 70);
  // findWorkingPixel();
  // setVthrCompCalDel();

  vector<int>vtrim;
  vtrim.push_back(255);
  vtrim.push_back(254);
  vtrim.push_back(130);
  vtrim.push_back(60);

  vector<int>btrim;
  btrim.push_back(14);
  btrim.push_back(13);
  btrim.push_back(11);
  btrim.push_back(7);

  fDirectory->cd();
  PixTest::update();
  banner(Form("PixTestTrim::trimBitTest() ntrig = %d, vtrims = %d %d %d %d",
	      NTRIG, vtrim[0], vtrim[1], vtrim[2], vtrim[3]));

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  maskPixels();

  vector<vector<TH1*> > steps;

  ConfigParameters *cp = fPixSetup->getConfigParameters();
  // -- start untrimmed
  bool ok = cp->setTrimBits(15);
  if (!ok) {
    LOG(logWARNING) << "could not set trim bits to " << 15;
    fProblem = true;
    return;
  }
  LOG(logDEBUG) << "trimBitTest initDUT with trim bits = 15" ;
  for (vector<uint8_t>::size_type iroc = 0; iroc < rocIds.size(); ++iroc) {
    fApi->_dut->updateTrimBits(cp->getRocPixelConfig(rocIds[iroc]), rocIds[iroc]);
  }


  fApi->setVcalLowRange();
  fApi->setDAC("Vtrim", 0);
  LOG(logDEBUG) << "trimBitTest determine threshold map without trims ";
  vector<TH1*> thr0 = mapsWithString(scurveMaps("Vcal", "TrimBitsThr0", NTRIG, 0, 199, -1, -1, 7), "thr");

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
    thr = mapsWithString(scurveMaps("Vcal", Form("TrimThr_trim%d", btrim[iv]),
				    NTRIG, 0, 220, -1, -1, 7), "thr");
    // thr = mapsWithString(scurveMaps("Vcal", Form("TrimThr_trim%d", btrim[iv]),
    // 				    NTRIG, 0, static_cast<int>(maxThr)+10, -1, -1, 7),
    // 			 "thr");
    maxThr = getMaximumThreshold(thr);
    if (maxThr > 245.) maxThr = 245.;
    steps.push_back(thr);
  }

  // -- and now determine threshold difference
  TH1 *h1(0);
  double dthr(0.);
  vector<vector<double> > meanThrDiff, rmsThrDiff;
  for (unsigned int i = 0; i < steps.size(); ++i) {
    thr = steps[i];
    vector<double> mean, rms;
    for (unsigned int iroc = 0; iroc < thr.size(); ++iroc) {
      h1 = bookTH1D(Form("TrimBit%d_C%d", btrim[i], rocIds[iroc]),
		    Form("TrimBit%d_C%d", btrim[i], rocIds[iroc]),
		    256, 0., 256);
      for (int ix = 0; ix < 52; ++ix) {
	for (int iy = 0; iy < 80; ++iy) {
	  // -- check that this is not a dead pixel without initial threshold
	  if (thr0[iroc]->GetBinContent(ix+1, iy+1) > 0) {
	    dthr = thr0[iroc]->GetBinContent(ix+1, iy+1) - thr[iroc]->GetBinContent(ix+1, iy+1);
	    h1->Fill(dthr);
	  }
	}
      }
      LOG(logDEBUG) << "ROC " << iroc
		    << " step " << i
		    << ": thr difference mean: " << h1->GetMean()
		    << ", thr difference RMS: " << h1->GetRMS();
      mean.push_back(h1->GetMean());
      rms.push_back(h1->GetRMS());
      fHistList.push_back(h1);
    }
    meanThrDiff.push_back(mean);
    rmsThrDiff.push_back(rms);
    mean.clear();
    rms.clear();
  }

  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);

  if (h1) h1->Draw();
  PixTest::update();
  restoreDacs();
  setTrimBits();
  LOG(logINFO) << "PixTestTrim::trimBitTest() done ";

  for (unsigned int i = 0; i < meanThrDiff.size(); ++i) {
    string vmeanString(Form("step %d: diff(thr) mean = ", i)), vrmsString(Form("step %d: diff(thr) rms =  ", i));
    for (unsigned int iroc = 0; iroc < meanThrDiff[i].size(); ++iroc) {
      vmeanString += Form(" %4.2f ", meanThrDiff[i].at(iroc));
      vrmsString +=  Form(" %4.2f ", rmsThrDiff[i].at(iroc));
    }
    LOG(logINFO) << vmeanString;
    LOG(logINFO) << vrmsString;
  }
  dutCalibrateOff();
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

  int NTRIG(fParNtrig);

  if (vcalMin < 0) vcalMin = 0;
  if (vcalMin > 255) vcalMax = 255;

  int nTrigAlive = 50;
  int vcalHigh = fParVcal + 200;
  if (vcalHigh > 250) {
    vcalHigh = 250;
  }
  fApi->setDAC("Vcal", vcalHigh);
  vector<TH2D*> effHighVcal = efficiencyMaps("PixelAliveHighVcal", nTrigAlive, FLAG_FORCE_MASKED);

  int trim(1);
  int trimBitsOld[16][52][80];
  for (unsigned int i = 0; i < calOld.size(); ++i) {
    for (int ix = 0; ix < 52; ++ix) {
      for (int iy = 0; iy < 80; ++iy) {
	trimBitsOld[i][ix][iy] = fTrimBits[i][ix][iy];
        if (effHighVcal[i]->GetBinContent(ix+1, iy+1) < nTrigAlive * 0.95) {
          trim = fTrimBits[i][ix][iy] + correction;
          if (trim > 15) trim = 15;
          fTrimBits[i][ix][iy] = trim;
        } else {
          if (calOld[i]->GetBinContent(ix + 1, iy + 1) > fParVcal) {
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
  }

  setTrimBits();
  vector<TH1*> calNew = scurveMaps("vcal", name, NTRIG, vcalMin, vcalMax, -1, -1, 1);
  if (calNew.size() != calOld.size()) {
    LOG(logERROR) << "scurve map size " << calNew.size() << " does not agree with previous number " << calOld.size();
    fProblem = true;
    return calNew;
  }

  // -- check that things got better, else revert and leave up to next correction round
  for (unsigned int i = 0; i < calOld.size(); ++i) {
    for (int ix = 0; ix < 52; ++ix) {
      for (int iy = 0; iy < 80; ++iy) {

        if (effHighVcal[i]->GetBinContent(ix+1, iy+1) < nTrigAlive * 0.95) {
          calNew[i]->SetBinContent(ix+1, iy+1, 0); // threshold too low!
        }
        if (calNew[i]->GetBinContent(ix+1, iy+1) < 5
	    || calNew[i]->GetBinContent(ix+1, iy+1) < vcalMin + 2
	    || calNew[i]->GetBinContent(ix+1, iy+1) > vcalMax - 2) {
          if (trimBitsOld[i][ix][iy] > fTrimBits[i][ix][iy]) {
            fTrimBits[i][ix][iy] = trimBitsOld[i][ix][iy];
          }
          calNew[i]->SetBinContent(ix + 1, iy + 1, 0);
        } else {
          if (TMath::Abs(calOld[i]->GetBinContent(ix + 1, iy + 1) - fParVcal) <
              TMath::Abs(calNew[i]->GetBinContent(ix + 1, iy + 1) - fParVcal)) {
            trim = trimBitsOld[i][ix][iy];
            calNew[i]->SetBinContent(ix + 1, iy + 1, calOld[i]->GetBinContent(ix + 1, iy + 1));
          } else {
            trim = fTrimBits[i][ix][iy];
          }
          fTrimBits[i][ix][iy] = trim;
        }
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
void PixTestTrim::findWorkingPixel() {

  gStyle->SetPalette(1);
  fDirectory->cd();
  PixTest::update();
  banner(Form("PixTestTrim::findWorkingPixel()"));


  vector<pair<int, int> > pixelList;
  pixelList.push_back(make_pair(12,22));
  pixelList.push_back(make_pair(5,5));
  pixelList.push_back(make_pair(15,26));
  pixelList.push_back(make_pair(20,32));
  pixelList.push_back(make_pair(25,36));
  pixelList.push_back(make_pair(30,42));
  pixelList.push_back(make_pair(35,50));
  pixelList.push_back(make_pair(40,60));
  pixelList.push_back(make_pair(45,70));
  pixelList.push_back(make_pair(50,75));

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  uint16_t FLAGS = FLAG_FORCE_MASKED;

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  TH2D *h2(0);
  map<string, TH2D*> maps;

  bool gofishing(false);
  vector<pair<uint8_t, pair<uint8_t, vector<pixel> > > >  rresults;
  int ic(-1), ir(-1);
  for (unsigned int ifwp = 0; ifwp < pixelList.size(); ++ifwp) {
    gofishing = false;
    ic = pixelList[ifwp].first;
    ir = pixelList[ifwp].second;
    fApi->_dut->testPixel(ic, ir, true);
    fApi->_dut->maskPixel(ic, ir, false);

    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
      h2 = bookTH2D(Form("fwp_c%d_r%d_C%d", ic, ir, rocIds[iroc]),
		    Form("fwp_c%d_r%d_C%d", ic, ir, rocIds[iroc]),
		    256, 0., 256., 256, 0., 256.);
      h2->SetMinimum(0.);
      h2->SetDirectory(fDirectory);
      fHistOptions.insert(make_pair(h2, "colz"));
      maps.insert(make_pair(Form("fwp_c%d_r%d_C%d", ic, ir, rocIds[iroc]), h2));
    }

    rresults.clear();
    try{
      rresults = fApi->getEfficiencyVsDACDAC("caldel", 0, 255, "vthrcomp", 0, 255, FLAGS, 5);
    } catch(pxarException &e) {
      LOG(logCRITICAL) << "pXar execption: "<< e.what();
      gofishing = true;
    }

    fApi->_dut->testPixel(ic, ir, false);
    fApi->_dut->maskPixel(ic, ir, true);
    if (gofishing) continue;

    string hname;
    for (unsigned i = 0; i < rresults.size(); ++i) {
      pair<uint8_t, pair<uint8_t, vector<pixel> > > v = rresults[i];
      int idac1 = v.first;
      pair<uint8_t, vector<pixel> > w = v.second;
      int idac2 = w.first;
      vector<pixel> wpix = w.second;
      for (unsigned ipix = 0; ipix < wpix.size(); ++ipix) {
	hname = Form("fwp_c%d_r%d_C%d", ic, ir, wpix[ipix].roc());
	if (maps.count(hname) > 0) {
	  maps[hname]->Fill(idac1, idac2, wpix[ipix].value());
	} else {
	  LOG(logDEBUG) << "bad pixel address decoded: " << hname << ", skipping";
	}
      }
    }


    bool okVthrComp(false), okCalDel(false);
    bool okAllRocs(true);
    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
      okVthrComp = okCalDel = false;
      hname = Form("fwp_c%d_r%d_C%d", ic, ir, rocIds[iroc]);
      h2 = maps[hname];

      h2->Draw("colz");
      PixTest::update();

      TH1D *hy = h2->ProjectionY("_py", 5, h2->GetNbinsX());
      double vcthrMax = hy->GetMaximum();
      double bottom   = hy->FindFirstBinAbove(0.5*vcthrMax);
      double top      = hy->FindLastBinAbove(0.5*vcthrMax);
      double vthrComp = top - 50;
      delete hy;
      if (vthrComp > bottom) {
	okVthrComp = true;
      }

      TH1D *hx = h2->ProjectionX("_px", vthrComp, vthrComp);
      double cdMax   = hx->GetMaximum();
      double cdFirst = hx->GetBinLowEdge(hx->FindFirstBinAbove(0.5*cdMax));
      double cdLast  = hx->GetBinLowEdge(hx->FindLastBinAbove(0.5*cdMax));
      delete hx;
      if (cdLast - cdFirst > 30) {
	okCalDel = true;
      }

      if (!okVthrComp || !okCalDel) {
	okAllRocs = false;
	LOG(logINFO) << hname << " does not pass: vthrComp = " << vthrComp
		     << " Delta(CalDel) = " << cdLast - cdFirst << ((ifwp != pixelList.size() - 1) ? ", trying another" : ".");
	break;
      } else{
	LOG(logDEBUG) << hname << " OK, with vthrComp = " << vthrComp << " and Delta(CalDel) = " << cdLast - cdFirst;
      }
    }
    if (okAllRocs) {
      for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
	string name = Form("fwp_c%d_r%d_C%d", ic, ir, rocIds[iroc]);
	TH2D *h = maps[name];
	fHistList.push_back(h);
	h->Draw(getHistOption(h).c_str());
	fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
	PixTest::update();
      }
      break;
    } else {
      for (map<string, TH2D*>::iterator il = maps.begin(); il != maps.end(); ++il) {
	delete (*il).second;
      }
      maps.clear();
    }
  }

  if (maps.size()) {
    LOG(logINFO) << "Found working pixel in all ROCs: col/row = " << ic << "/" << ir;
    clearSelectedPixels();
    fPIX.push_back(make_pair(ic, ir));
    addSelectedPixels(Form("%d,%d", ic, ir));
  } else {
    LOG(logINFO) << "Something went wrong...";
    LOG(logINFO) << "Didn't find a working pixel in all ROCs.";
    for (size_t iroc = 0; iroc < rocIds.size(); iroc++) {
      LOG(logINFO) << "our roc list from in the dut: " << static_cast<int>(rocIds[iroc]);
    }
  }

  dutCalibrateOff();
}


// ----------------------------------------------------------------------
void PixTestTrim::setVthrCompCalDel() {
  uint16_t FLAGS = FLAG_FORCE_MASKED;

  fStopTest = false;
  gStyle->SetPalette(1);
  fDirectory->cd();
  PixTest::update();
  banner(Form("PixTestTrim::setVthrCompCalDel()"));

  string name("trimVthrCompCalDel");

  fApi->setVcalLowRange();
  fApi->setDAC("Vcal", 250);

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();

  TH1D *h1(0);
  h1 = bookTH1D(Form("pretestCalDel"), Form("pretestCalDel"), rocIds.size(), 0., rocIds.size());
  h1->SetMinimum(0.);
  h1->SetDirectory(fDirectory);
  setTitles(h1, "ROC", "CalDel DAC");

  TH2D *h2(0);

  vector<int> calDel(rocIds.size(), -1);
  vector<int> vthrComp(rocIds.size(), -1);
  vector<int> calDelE(rocIds.size(), -1);

  int ip = 0;

  fApi->_dut->testPixel(fPIX[ip].first, fPIX[ip].second, true);
  fApi->_dut->maskPixel(fPIX[ip].first, fPIX[ip].second, false);

  map<int, TH2D*> maps;
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    h2 = bookTH2D(Form("%s_c%d_r%d_C%d", name.c_str(), fPIX[ip].first, fPIX[ip].second, rocIds[iroc]),
		  Form("%s_c%d_r%d_C%d", name.c_str(), fPIX[ip].first, fPIX[ip].second, rocIds[iroc]),
		  255, 0., 255., 255, 0., 255.);
    fHistOptions.insert(make_pair(h2, "colz"));
    maps.insert(make_pair(rocIds[iroc], h2));
    h2->SetMinimum(0.);
    h2->SetMaximum(fParNtrig);
    h2->SetDirectory(fDirectory);
    setTitles(h2, "CalDel", "VthrComp");
  }

  bool done = false;
  vector<pair<uint8_t, pair<uint8_t, vector<pixel> > > >  rresults;
  int cnt(0);
  while (!done) {
    rresults.clear();
    gSystem->ProcessEvents();
    if (fStopTest) break;

    try{
      rresults = fApi->getEfficiencyVsDACDAC("caldel", 0, 255, "vthrcomp", 0, 255, FLAGS, 5);
      done = true;
    } catch(DataMissingEvent &e){
      LOG(logCRITICAL) << "problem with readout: "<< e.what() << " missing " << e.numberMissing << " events";
      ++cnt;
      if (e.numberMissing > 10) done = true;
    } catch(pxarException &e) {
      LOG(logCRITICAL) << "pXar execption: "<< e.what();
      ++cnt;
    }
    done = (cnt>5) || done;
  }

  fApi->_dut->testPixel(fPIX[ip].first, fPIX[ip].second, false);
  fApi->_dut->maskPixel(fPIX[ip].first, fPIX[ip].second, true);

  for (unsigned i = 0; i < rresults.size(); ++i) {
    pair<uint8_t, pair<uint8_t, vector<pixel> > > v = rresults[i];
    int idac1 = v.first;
    pair<uint8_t, vector<pixel> > w = v.second;
    int idac2 = w.first;
    vector<pixel> wpix = w.second;
    for (unsigned ipix = 0; ipix < wpix.size(); ++ipix) {
      maps[wpix[ipix].roc()]->Fill(idac1, idac2, wpix[ipix].value());
    }
  }
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    h2 = maps[rocIds[iroc]];
    TH1D *hy = h2->ProjectionY("_py", 5, h2->GetNbinsX());
    double vcthrMax = hy->GetMaximum();
    double bottom   = hy->FindFirstBinAbove(0.5*vcthrMax);
    double top      = hy->FindLastBinAbove(0.5*vcthrMax);
    delete hy;

    double fracCalDel(0.5), offsetVthrComp(20.);
    vthrComp[iroc] = bottom + offsetVthrComp;

    TH1D *h0 = h2->ProjectionX("_px", vthrComp[iroc], vthrComp[iroc]);
    double cdMax   = h0->GetMaximum();
    double cdFirst = h0->GetBinLowEdge(h0->FindFirstBinAbove(0.5*cdMax));
    double cdLast  = h0->GetBinLowEdge(h0->FindLastBinAbove(0.5*cdMax));
    calDelE[iroc] = static_cast<int>(cdLast - cdFirst);
    calDel[iroc] = static_cast<int>(cdFirst + fracCalDel*calDelE[iroc]);
    TMarker *pm = new TMarker(calDel[iroc], vthrComp[iroc], 21);
    pm->SetMarkerColor(kWhite);
    pm->SetMarkerSize(2);
    h2->GetListOfFunctions()->Add(pm);
    pm = new TMarker(calDel[iroc], vthrComp[iroc], 7);
    pm->SetMarkerColor(kBlack);
    pm->SetMarkerSize(0.2);
    h2->GetListOfFunctions()->Add(pm);
    delete h0;

    h1->SetBinContent(rocIds[iroc]+1, calDel[iroc]);
    h1->SetBinError(rocIds[iroc]+1, 0.5*calDelE[iroc]);
    LOG(logDEBUG) << "CalDel: " << calDel[iroc] << " +/- " << 0.5*calDelE[iroc];

    h2->Draw(getHistOption(h2).c_str());
    PixTest::update();

    fHistList.push_back(h2);
  }

  fHistList.push_back(h1);

  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);
  PixTest::update();

  string caldelString(""), vthrcompString("");
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    if (calDel[iroc] > 0) {
      fApi->setDAC("CalDel", calDel[iroc], rocIds[iroc]);
      caldelString += Form("  %4d", calDel[iroc]);
    } else {
      caldelString += Form(" _%4d", fApi->_dut->getDAC(rocIds[iroc], "caldel"));
    }
    fApi->setDAC("VthrComp", vthrComp[iroc], rocIds[iroc]);
    vthrcompString += Form("  %4d", vthrComp[iroc]);
  }

  // -- summary printout
  LOG(logINFO) << "PixTestTrim::setVthrCompCalDel() done";
  LOG(logINFO) << "CalDel:   " << caldelString;
  LOG(logINFO) << "VthrComp: " << vthrcompString;

  dutCalibrateOff();
}
