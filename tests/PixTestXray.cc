#include <stdlib.h>     /* atof, atoi,itoa */
#include <algorithm>    // std::find
#include <iostream>
#include <fstream>

#include "PixTestXray.hh"
#include "log.h"
#include "timer.h"

#include "PixUtil.hh"

#include <TH2.h>
#include <TMath.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestXray)

// ----------------------------------------------------------------------
PixTestXray::PixTestXray(PixSetup *a, std::string name) : PixTest(a, name), 
  fParSource("nada"), fParTriggerFrequency(0), fParRunSeconds(0), fParStepSeconds(0), 
  fParVthrCompMin(0), fParVthrCompMax(0),  fParFillTree(false), fParDelayTBM(false) {
  PixTest::init();
  init(); 
  LOG(logDEBUG) << "PixTestXray ctor(PixSetup &a, string, TGTab *)";

  fTree = 0; 
  fPhCal.setPHParameters(fPixSetup->getConfigParameters()->getGainPedestalParameters());
  fPhCalOK = fPhCal.initialized();

}


//----------------------------------------------------------
PixTestXray::PixTestXray() : PixTest() {
  LOG(logDEBUG) << "PixTestXray ctor()";
  fTree = 0; 
}


// ----------------------------------------------------------------------
bool PixTestXray::setParameter(string parName, string sval) {
  bool found(false);
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      if (!parName.compare("source")) {
	fParSource = sval; 
	setToolTips();
      }
      if (!parName.compare("vthrcompmin")) {
	fParVthrCompMin = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("vthrcompmax")) {
	fParVthrCompMax = atoi(sval.c_str()); 
	setToolTips();
      }	
      if (!parName.compare("trgfrequency(khz)")) {
	fParTriggerFrequency = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("runseconds")) {
	fParRunSeconds = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("stepseconds")) {
	fParStepSeconds = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("delaytbm")) {
	PixUtil::replaceAll(sval, "checkbox(", "");
	PixUtil::replaceAll(sval, ")", "");
	fParDelayTBM = !(atoi(sval.c_str())==0);
	setToolTips();
      }
      if (!parName.compare("filltree")) {
	PixUtil::replaceAll(sval, "checkbox(", "");
	PixUtil::replaceAll(sval, ")", "");
	fParFillTree = !(atoi(sval.c_str())==0);
	setToolTips();
      }
      
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
void PixTestXray::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;

  if (!command.compare("stop")){
     doStop();
  }

  if (!command.compare("maskhotpixels")) {
    doRunMaskHotPixels(); 
    return;
  }

  if (!command.compare("ratescan")) {
    doRateScan(); 
    return;
  }

  if (!command.compare("phrun")) {
    doPhRun(); 
    return;
  }

  LOG(logDEBUG) << "did not find command ->" << command << "<-";
}

// ----------------------------------------------------------------------
void PixTestXray::init() {
  LOG(logDEBUG) << "PixTestXray::init()";
  setToolTips();
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}

// ----------------------------------------------------------------------
void PixTestXray::setToolTips() {
  fTestTip    = string("Xray vcal calibration test")
    ;
  fSummaryTip = string("to be implemented")
    ;
  fStopTip = string("Stop 'phrun' and save data.")
    ;
}


// ----------------------------------------------------------------------
vector<TH2D*> PixTestXray::bookHotPixelMap() {
  fDirectory->cd(); 
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  unsigned nrocs = rocIds.size(); 
  vector<TH2D*> v; 
  for (unsigned int iroc = 0; iroc < nrocs; ++iroc){
    TH2D *h2 = bookTH2D(Form("hotPixelMap_C%d", rocIds[iroc]), Form("hotPixelMap_C%d", rocIds[iroc]), 
			52, 0., 52., 80, 0., 80.);
    h2->SetMinimum(0.);
    h2->SetDirectory(fDirectory);
    fHotPixelMap.push_back(h2);
    fHistOptions.insert(make_pair(h2,"colz"));
    v.push_back(h2); 
  }
  copy(fHotPixelMap.begin(), fHotPixelMap.end(), back_inserter(fHistList));
  return v; 
}  


// ----------------------------------------------------------------------
void PixTestXray::bookHist(string name) {
  fDirectory->cd(); 
  if (fParFillTree) bookTree();  
  
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  unsigned nrocs = rocIds.size(); 
  
  //-- Sets up the histogram
  TH1D *h1(0);
  TH2D *h2(0);
  for (unsigned int iroc = 0; iroc < nrocs; ++iroc){
    h1 = bookTH1D(Form("hits_%s_C%d", name.c_str(), rocIds[iroc]), Form("hits_%s_C%d", name.c_str(), rocIds[iroc]), 
		  256, 0., 256.);
    h1->SetMinimum(0.);
    h1->SetDirectory(fDirectory);
    setTitles(h1, "VthrComp", "Hits");
    fHits.push_back(h1);

    h1 = bookTH1D(Form("mpix_%s_C%d", name.c_str(), rocIds[iroc]), Form("mpix_%s_C%d", name.c_str(), rocIds[iroc]), 
		  256, 0., 256.);
    h1->SetMinimum(0.);
    h1->SetDirectory(fDirectory);
    setTitles(h1, "VthrComp", "maskedpixels");
    fMpix.push_back(h1);
    
    h2 = bookTH2D(Form("hitMap_%s_C%d", name.c_str(), rocIds[iroc]), Form("hitMap_%s_C%d", name.c_str(), rocIds[iroc]), 
		  52, 0., 52., 80, 0., 80.);
    h2->SetMinimum(0.);
    h2->SetDirectory(fDirectory);
    fHitMap.push_back(h2);
  }

  copy(fHits.begin(), fHits.end(), back_inserter(fHistList));
  copy(fMpix.begin(), fMpix.end(), back_inserter(fHistList));
  copy(fHitMap.begin(), fHitMap.end(), back_inserter(fHistList));
}


//----------------------------------------------------------
PixTestXray::~PixTestXray() {
  LOG(logDEBUG) << "PixTestXray dtor";
  fDirectory->cd();
  if (fTree && fParFillTree) fTree->Write(); 
}


// ----------------------------------------------------------------------
void PixTestXray::doTest() {

  bigBanner(Form("PixTestXray::doTest()"));
  doPhRun();
  //  doRateScan();

  LOG(logINFO) << "PixTestXray::doTest() done ";

}


// ----------------------------------------------------------------------
void PixTestXray::doPhRun() {

  banner(Form("PixTestXray::doPhRun() fParRunSeconds = %d", fParRunSeconds));

  PixTest::update(); 
  fDirectory->cd();

  // -- unmask entire chip and then mask hot pixels
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(false);
  for (unsigned int i = 0; i < fHotPixels.size(); ++i) {
    vector<pair<int, int> > hot = fHotPixels[i]; 
    for (unsigned int ipix = 0; ipix < hot.size(); ++ipix) {
      LOG(logINFO) << "ROC " << getIdFromIdx(i) << " masking hot pixel " << hot[ipix].first << "/" << hot[ipix].second; 
      fApi->_dut->maskPixel(hot[ipix].first, hot[ipix].second, true, getIdFromIdx(i)); 
    }
  }
  maskPixels();

  
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();

  if (0 == fQ.size()) {
    if (fParFillTree) bookTree(); 
    TH1D *h1(0); 
    TH2D *h2(0);
    TH1D *h3(0); 
    TProfile2D *p2(0); 
    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
      h2 = bookTH2D(Form("hMap_%s_C%d", fParSource.c_str(), rocIds[iroc]), 
		    Form("hMap_%s_C%d", fParSource.c_str(), rocIds[iroc]), 
		    52, 0., 52., 80, 0., 80.);
      h2->SetMinimum(0.);
      h2->SetDirectory(fDirectory);
      setTitles(h2, "col", "row");
      fHistOptions.insert(make_pair(h2,"colz"));
      fHmap.push_back(h2);


      p2 = bookTProfile2D(Form("qMap_%s_C%d", fParSource.c_str(), rocIds[iroc]), 
			  Form("qMap_%s_C%d", fParSource.c_str(), rocIds[iroc]), 
			  52, 0., 52., 80, 0., 80.);
      p2->SetMinimum(0.);
      p2->SetDirectory(fDirectory);
      setTitles(p2, "col", "row");
      fHistOptions.insert(make_pair(p2,"colz"));
      fQmap.push_back(p2);

      p2 = bookTProfile2D(Form("phMap_%s_C%d", fParSource.c_str(), rocIds[iroc]), 
			  Form("phMap_%s_C%d", fParSource.c_str(), rocIds[iroc]), 
			  52, 0., 52., 80, 0., 80.);
      p2->SetMinimum(0.);
      p2->SetDirectory(fDirectory);
      setTitles(p2, "col", "row");
      fHistOptions.insert(make_pair(p2,"colz"));
      fPHmap.push_back(p2);
      
      h1 = bookTH1D(Form("q_%s_C%d", fParSource.c_str(), rocIds[iroc]), 
		    Form("q_%s_C%d", fParSource.c_str(), rocIds[iroc]), 
		    2000, 0., 2000.);
      h1->SetMinimum(0.);
      h1->SetDirectory(fDirectory);
      setTitles(h1, "Q [Vcal]", "Entries/bin");
      fQ.push_back(h1);

      h1 = bookTH1D(Form("ph_%s_C%d", fParSource.c_str(), rocIds[iroc]), 
		    Form("ph_%s_C%d", fParSource.c_str(), rocIds[iroc]), 
		    256, 0., 256.);
      h1->SetMinimum(0.);
      h1->SetDirectory(fDirectory);
      setTitles(h1, "PH [ADC]", "Entries/bin");
      fPH.push_back(h1);
    }
    h3 = bookTH1D(Form("ntrig_%s", fParSource.c_str()),
	Form("ntrig_%s", fParSource.c_str()),1,0,1);
    h3->SetDirectory(fDirectory);
    fTriggers.push_back(h3);

    copy(fHmap.begin(), fHmap.end(), back_inserter(fHistList));
    copy(fQmap.begin(), fQmap.end(), back_inserter(fHistList));
    copy(fQ.begin(), fQ.end(), back_inserter(fHistList));
    copy(fPHmap.begin(), fPHmap.end(), back_inserter(fHistList));
    copy(fPH.begin(), fPH.end(), back_inserter(fHistList));
    copy(fTriggers.begin(), fTriggers.end(), back_inserter(fHistList));
  }

  int totalPeriod = prepareDaq(fParTriggerFrequency, 50);
  fApi->daqStart();

  if (fParDelayTBM) {
    LOG(logINFO) << "set TBM register delays = 0x40";
    fApi->setTbmReg("delays", 0x40);
  }

  int finalPeriod = fApi->daqTriggerLoop(totalPeriod);
  LOG(logINFO) << "PixTestXray::doPhRun start TriggerLoop with trigger frequency " << fParTriggerFrequency 
	       << ", period "  << finalPeriod 
	       << " and duration " << fParRunSeconds << " seconds";
  
  uint8_t perFull;
  timer t;
  fDaq_loop = true;
  while (fApi->daqStatus(perFull) && fDaq_loop) {
    gSystem->ProcessEvents();
    if (perFull > 80) {
      LOG(logINFO) << "run duration " << t.get()/1000 << " seconds, buffer almost full (" 
		   << (int)perFull << "%), pausing triggers.";
      fApi->daqTriggerLoopHalt();
      processData(0);
      LOG(logINFO) << "Resuming triggers.";
	  fApi->daqTriggerLoop(finalPeriod);
    }

    if (static_cast<int>(t.get())/1000 >= fParRunSeconds)	{
      LOG(logINFO) << "data taking finished, elapsed time: " << t.get()/1000 << " seconds."; 
      fDaq_loop = false;
      break;
    }
  }

  fApi->daqTriggerLoopHalt();
  fApi->daqStop();

  processData(0);

  finalCleanup();
  fQ[0]->Draw();
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), fQ[0]);
  PixTest::update();

  LOG(logINFO) << "PixTestXray::doPhRun() done";

}


// ----------------------------------------------------------------------
void PixTestXray::doRateScan() {
  
  banner(Form("PixTestXray::doRateScan() fParStepSeconds = %d, vthrcom = %d .. %d", fParStepSeconds, fParVthrCompMin, fParVthrCompMax));
  cacheDacs(); 

  if (1) {
  fPg_setup.clear();
  fPg_setup = fPixSetup->getConfigParameters()->getTbPgSettings();
  fApi->setPatternGenerator(fPg_setup);

  PixTest::update(); 
  fDirectory->cd();

  if (0 == fHits.size()) bookHist("xrayVthrCompScan");


  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(false);
  

  int totalPeriod = prepareDaq(fParTriggerFrequency, 50);

  // -- scan VthrComp  
  for (fVthrComp = fParVthrCompMin; fVthrComp <= fParVthrCompMax;  ++fVthrComp) {
    for (unsigned i = 0; i < fHitMap.size(); ++i) {
      fHitMap[i]->Reset();
    }
    timer t;
    uint8_t perFull;
    fApi->setDAC("vthrcomp", fVthrComp);
    fDaq_loop = true;
    
    LOG(logINFO)<< "Starting Loop with VthrComp = " << fVthrComp;
    fApi->daqStart();

	int finalPeriod = fApi->daqTriggerLoop(totalPeriod);
    LOG(logINFO) << "PixTestXray::doRateScan start TriggerLoop with period " << finalPeriod << " and duration " << fParStepSeconds << " seconds";
    
    while (fApi->daqStatus(perFull) && fDaq_loop) {
      gSystem->ProcessEvents();
      if (perFull > 80) {
	LOG(logINFO) << "Buffer almost full, pausing triggers.";
	fApi->daqTriggerLoopHalt();
	readData();
	LOG(logINFO) << "Resuming triggers.";
	fApi->daqTriggerLoop(finalPeriod);
      }
      
      if (static_cast<int>(t.get()/1000) >= fParStepSeconds)	{
	LOG(logINFO) << "Elapsed time: " << t.get()/1000 << " seconds.";
	fDaq_loop = false;
	break;
      }
    }
    
    fApi->daqTriggerLoopHalt();
    
    fApi->daqStop();
    readData();
    

       
    analyzeData();

    fHits[0]->Draw();
    fDisplayedHist = find(fHistList.begin(), fHistList.end(), fHits[0]);
    PixTest::update();

  }
  }

  if (0) {
    //    TFile *f = TFile::Open("testROC/pxar_firstXray30_90.root");
    //    TFile *f = TFile::Open("testROC/pxar_Ag_Vcal_10_70_10s.root");
    TFile *f = TFile::Open("testROC/pxar_Mo_Vcal_30_80_10s.root");
    
    TH1D *h1 = ((TH1D*)f->Get("Xray/hits_xrayVthrCompScan_C0_V0"));
    TH1D *h2 = (TH1D*)h1->Clone("local");
    h2->SetDirectory(0);
    f->Close();
    fHits.push_back(h2);
    copy(fHits.begin(), fHits.end(), back_inserter(fHistList));
  
    fHits[0]->Draw();
    fDisplayedHist = find(fHistList.begin(), fHistList.end(), fHits[0]);
    PixTest::update();
    return;
  }


  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  unsigned nrocs = rocIds.size(); 
  double lo(-1.), hi(-1.);
  for (unsigned int i = 0; i < nrocs; ++i) {
    TF1 *f1 = fPIF->xrayScan(fHits[i]);
    f1->GetRange(lo, hi); 
    fHits[i]->Fit(f1, "lr", "", lo, hi); 
    double thr = f1->GetParameter(0); 
    if (thr < 0 || thr > 255.) thr = 0.;
    uint8_t ithr = static_cast<uint8_t>(thr); 
    LOG(logINFO) << "ROC " << static_cast<int>(rocIds[i]) << " with VthrComp threshold = " << thr << " -> " << static_cast<int>(ithr); 
    fApi->setDAC("vthrcomp", ithr, rocIds[i]); 
    PixTest::update();
  }

  finalCleanup();

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  vector<TH1*> thr0 = scurveMaps("vcal", "xrayScan", 5, 0, 255, 9); 

  fHits[0]->Draw();
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), fHits[0]);
  PixTest::update();

  string hname(""), scurvesMeanString(""), scurvesRmsString(""); 
  for (unsigned int i = 0; i < thr0.size(); ++i) {
    hname = thr0[i]->GetName();
    // -- skip sig_ and thn_ histograms
    if (string::npos == hname.find("dist_thr_")) continue;
    scurvesMeanString += Form("%6.2f ", thr0[i]->GetMean()); 
    scurvesRmsString += Form("%6.2f ", thr0[i]->GetRMS()); 
  }

  LOG(logINFO) << "PixTestXray::doTest() done";
  LOG(logINFO) << "vcal mean: " << scurvesMeanString; 
  LOG(logINFO) << "vcal RMS:  " << scurvesRmsString; 

  LOG(logINFO) << "PixTestXray::doRateScan() done";

}

// ----------------------------------------------------------------------
void PixTestXray::readDataOld() {

  int pixCnt(0);  
  vector<pxar::Event> daqdat;
  
  daqdat = fApi->daqGetEventBuffer();
  
  for(std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
    pixCnt += it->pixels.size();
    
    for (unsigned int ipix = 0; ipix < it->pixels.size(); ++ipix) {
      fHitMap[getIdxFromId(it->pixels[ipix].roc())]->Fill(it->pixels[ipix].column(), it->pixels[ipix].row());
    }
  }
  LOG(logDEBUG) << "Processing Data: " << daqdat.size() << " events with " << pixCnt << " pixels";
}

// ----------------------------------------------------------------------
void PixTestXray::analyzeData() {

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  unsigned nrocs = rocIds.size(); 

  int cnt(0); 
  for (unsigned int i = 0; i < nrocs; ++i) {
    double cut = meanHit(fHitMap[i]); 
    cut = noiseLevel(fHitMap[i]); 
    cnt = countHitsAndMaskPixels(fHitMap[i], cut, rocIds[i]); 
    fHits[i]->SetBinContent(fVthrComp+1, cnt); 
    int mpix = fApi->_dut->getNMaskedPixels(rocIds[i]);
    fMpix[i]->SetBinContent(fVthrComp+1, mpix); 
  }


}


// ----------------------------------------------------------------------
double PixTestXray::meanHit(TH2D *h2) {

  TH1D *h1 = new TH1D("h1", "h1", 1000, 0., 1000.); 

  for (int ix = 0; ix < h2->GetNbinsX(); ++ix) {
    for (int iy = 0; iy < h2->GetNbinsY(); ++iy) {
      h1->Fill(h2->GetBinContent(ix+1, iy+1)); 
    }
  }
  
  double mean = h1->GetMean(); 
  delete h1; 
  LOG(logDEBUG) << "hist " << h2->GetName() << " mean hits = " << mean; 
  return mean; 
}

// ----------------------------------------------------------------------
double PixTestXray::noiseLevel(TH2D *h2) {

  TH1D *h1 = new TH1D("h1", "h1", 1000, 0., 1000.); 

  for (int ix = 0; ix < h2->GetNbinsX(); ++ix) {
    for (int iy = 0; iy < h2->GetNbinsY(); ++iy) {
      h1->Fill(h2->GetBinContent(ix+1, iy+1)); 
    }
  }
  
  // -- skip bin for zero hits!
  int noise(-1);
  for (int ix = 1; ix < h1->GetNbinsX(); ++ix) {
    if (h1->GetBinContent(ix+1) < 1) {
      noise = ix; 
      break;
    }
  }

  int lastbin(1);
  for (int ix = 1; ix < h1->GetNbinsX(); ++ix) {
    if (h1->GetBinContent(ix+1) > 1) {
      lastbin = ix; 
    }
  }


  delete h1; 
  LOG(logINFO) << "hist " << h2->GetName() 
	       << " (maximum: " << h2->GetMaximum() << ") "
	       << " noise level = " << noise << " last bin above 1: " << lastbin; 
  return lastbin; 
}


// ----------------------------------------------------------------------
int PixTestXray::countHitsAndMaskPixels(TH2D *h2, double noiseLevel, int iroc) {
  
  int cnt(0); 
  double entries(0.); 
  for (int ix = 0; ix < h2->GetNbinsX(); ++ix) {
    for (int iy = 0; iy < h2->GetNbinsY(); ++iy) {
      entries = h2->GetBinContent(ix+1, iy+1);
      if (entries > noiseLevel) {
	fApi->_dut->maskPixel(ix, iy, true, iroc); 
	LOG(logINFO) << "ROC " << iroc << " masking pixel " << ix << "/" << iy 
		     << " with #hits = " << entries << " (cut: " << noiseLevel << ")"; 
      } else {
	cnt += static_cast<int>(entries);
      }
    }
  }
  return cnt; 
}


// ----------------------------------------------------------------------
void PixTestXray::readData() {

  int pixCnt(0);  
  vector<pxar::Event> daqdat = fApi->daqGetEventBuffer();
  
  for (std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
    pixCnt += it->pixels.size();

    if (fParFillTree) {
      fTreeEvent.header           = it->header; 
      fTreeEvent.dac              = 0;
      fTreeEvent.trailer          = it->trailer; 
      fTreeEvent.npix             = it->pixels.size();
    }

    int idx(0); 
    double q(0.);
    for (unsigned int ipix = 0; ipix < it->pixels.size(); ++ipix) {   
      idx = getIdxFromId(it->pixels[ipix].roc());
      if (fPhCalOK) {
	q = fPhCal.vcal(it->pixels[ipix].roc(), 
			it->pixels[ipix].column(), 
			it->pixels[ipix].row(), 
			it->pixels[ipix].value());
      } else {
	q = 0;
      }
      fHitMap[idx]->Fill(it->pixels[ipix].column(), it->pixels[ipix].row());
      fQ[idx]->Fill(q);
      fQmap[idx]->Fill(it->pixels[ipix].column(), it->pixels[ipix].row(), q);

      fPHmap[idx]->Fill(it->pixels[ipix].column(), it->pixels[ipix].row(), it->pixels[ipix].value());
      fPH[idx]->Fill(it->pixels[ipix].value());
	
      if (fParFillTree) {
	fTreeEvent.proc[ipix] = it->pixels[ipix].roc(); 
	fTreeEvent.pcol[ipix] = it->pixels[ipix].column(); 
	fTreeEvent.prow[ipix] = it->pixels[ipix].row(); 
	fTreeEvent.pval[ipix] = it->pixels[ipix].value(); 
	fTreeEvent.pq[ipix]   = q;
      }
    }
    
    if (fParFillTree) fTree->Fill();
    
  }
  LOG(logDEBUG) << "Processing Data: " << daqdat.size() << " events with " << pixCnt << " pixels";
}


// ----------------------------------------------------------------------
void PixTestXray::processData(uint16_t numevents) {
  fDirectory->cd();
  PixTest::update();
  
  int pixCnt(0);
  LOG(logDEBUG) << "Getting Event Buffer";
  vector<pxar::Event> daqdat;
   
  if (numevents > 0) {
    for (unsigned int i = 0; i < numevents ; i++) {
      pxar::Event evt = fApi->daqGetEvent();
      //Check if event is empty?
      if(evt.pixels.size() > 0)
	daqdat.push_back(evt);
    }
  }
  else {
    daqdat = fApi->daqGetEventBuffer();
  }

  LOG(logDEBUG) << "Processing Data: " << daqdat.size() << " events.";
  
  int idx(-1); 
  uint16_t q; 
  for (std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
    pixCnt += it->pixels.size(); 
    
    if (fParFillTree) {
      fTreeEvent.header           = it->header; 
      fTreeEvent.dac              = 0;
      fTreeEvent.trailer          = it->trailer; 
      fTreeEvent.npix             = it->pixels.size();
    }

    for (unsigned int ipix = 0; ipix < it->pixels.size(); ++ipix) {   
      idx = getIdxFromId(it->pixels[ipix].roc());

      if (fPhCalOK) {
	q = fPhCal.vcal(it->pixels[ipix].roc(), 
			it->pixels[ipix].column(), 
			it->pixels[ipix].row(), 
			it->pixels[ipix].value());
      } else {
	q = 0;
      }
      fHmap[idx]->Fill(it->pixels[ipix].column(), it->pixels[ipix].row());
      fQ[idx]->Fill(q);
      fQmap[idx]->Fill(it->pixels[ipix].column(), it->pixels[ipix].row(), q);

      fPHmap[idx]->Fill(it->pixels[ipix].column(), it->pixels[ipix].row(), it->pixels[ipix].value());
      fPH[idx]->Fill(it->pixels[ipix].value());
	
      if (fParFillTree) {
	fTreeEvent.proc[ipix] = it->pixels[ipix].roc(); 
	fTreeEvent.pcol[ipix] = it->pixels[ipix].column(); 
	fTreeEvent.prow[ipix] = it->pixels[ipix].row(); 
	fTreeEvent.pval[ipix] = it->pixels[ipix].value(); 
	fTreeEvent.pq[ipix]   = q;
      }
    }
    
    if (fParFillTree) fTree->Fill();
  }
  
  LOG(logDEBUG) << Form(" # events read: %6ld, pixels seen in all events: %3d", daqdat.size(), pixCnt);
  
  fHmap[0]->Draw("colz");
  fTriggers[0]->SetBinContent(1, fTriggers[0]->GetBinContent(1) + daqdat.size());
  PixTest::update();
}


// ----------------------------------------------------------------------
void PixTestXray::doRunMaskHotPixels() {    
  PixTest::update(); 
  vector<TH2D*> v = mapsWithString(fHitMap, "hotpixels"); 
  if (0 == v.size()) {
    bookHist("hotpixels");
    v = mapsWithString(fHitMap, "hotpixels"); 
  }
  for (unsigned int i = 0; i < v.size(); ++i) v[i]->Reset();
  maskHotPixels(v); 
  // -- display
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), v[0]);
  v[0]->Draw("colz");
  PixTest::update(); 
  return;
}


// ----------------------------------------------------------------------
void PixTestXray::doStop(){
	// Interrupt the test 
	fDaq_loop = false;
	LOG(logINFO) << "Stop pressed. Ending test.";
}

