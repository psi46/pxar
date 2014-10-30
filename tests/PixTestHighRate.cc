
#include <stdlib.h>     /* atof, atoi,itoa */
#include <algorithm>    // std::find
#include <iostream>
#include <fstream>

#include "PixTestHighRate.hh"
#include "log.h"
#include "timer.h"

#include "PixUtil.hh"


#include <TH2.h>
#include <TMath.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestHighRate)

// ----------------------------------------------------------------------
PixTestHighRate::PixTestHighRate(PixSetup *a, std::string name) : PixTest(a, name), 
  fParTriggerFrequency(0), fParRunSeconds(0),  
  fParFillTree(false), fParDelayTBM(false) {
  PixTest::init();
  init(); 
  LOG(logDEBUG) << "PixTestHighRate ctor(PixSetup &a, string, TGTab *)";
  
  fTree = 0; 
  fPhCal.setPHParameters(fPixSetup->getConfigParameters()->getGainPedestalParameters());
  fPhCalOK = fPhCal.initialized();
}


//----------------------------------------------------------
PixTestHighRate::PixTestHighRate() : PixTest() {
  LOG(logDEBUG) << "PixTestHighRate ctor()";
  fTree = 0; 
}


// ----------------------------------------------------------------------
bool PixTestHighRate::setParameter(string parName, string sval) {
  bool found(false);
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      if (!parName.compare("trgfrequency(khz)")) {
	fParTriggerFrequency = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParTriggerFrequency -> " << fParTriggerFrequency;
	setToolTips();
      }
      if (!parName.compare("runseconds")) {
	fParRunSeconds = atoi(sval.c_str()); 
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
void PixTestHighRate::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;

  if (!command.compare("stop")){
     doStop();
  }

  if (!command.compare("maskhotpixels")) {
    doRunMaskHotPixels(); 
    return;
  }

  if (!command.compare("rundaq")) {
    doRunDaq(); 
    return;
  }

  if (!command.compare("xpixelalive")) {
    doXPixelAlive(); 
    return;
  }

  LOG(logDEBUG) << "did not find command ->" << command << "<-";
}

// ----------------------------------------------------------------------
void PixTestHighRate::init() {
  LOG(logDEBUG) << "PixTestHighRate::init()";
  setToolTips();
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}

// ----------------------------------------------------------------------
void PixTestHighRate::setToolTips() {
  fTestTip    = string("Xray vcal calibration test")
    ;
  fSummaryTip = string("to be implemented")
    ;
  fStopTip = string("Stop 'rundaq' and save data.")
    ;
}


// ----------------------------------------------------------------------
void PixTestHighRate::bookHist(string name) {
  fDirectory->cd(); 
  if (fParFillTree) bookTree();  

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  unsigned nrocs = rocIds.size(); 
  
  //-- Sets up the histogram
  TH2D *h2(0);
  for (unsigned int iroc = 0; iroc < nrocs; ++iroc){
    h2 = bookTH2D(Form("hitMap_%s_C%d", name.c_str(), rocIds[iroc]), Form("hitMap_%s_C%d", name.c_str(), rocIds[iroc]), 
		  52, 0., 52., 80, 0., 80.);
    h2->SetMinimum(0.);
    h2->SetDirectory(fDirectory);
    fHistOptions.insert(make_pair(h2,"colz"));
    fHitMap.push_back(h2);
  }
  
  copy(fHitMap.begin(), fHitMap.end(), back_inserter(fHistList));

}


//----------------------------------------------------------
PixTestHighRate::~PixTestHighRate() {
  LOG(logDEBUG) << "PixTestHighRate dtor";
  fDirectory->cd();
  if (fTree && fParFillTree) fTree->Write(); 
}


// ----------------------------------------------------------------------
void PixTestHighRate::doTest() {
  bigBanner(Form("PixTestHighRate::doTest()"));
  //  maskHotPixels();
  doXPixelAlive();
  doRunDaq();
  LOG(logINFO) << "PixTestHighRate::doTest() done ";
}

// ----------------------------------------------------------------------
void PixTestHighRate::doXPixelAlive() {

  banner(Form("PixTestHighRate::xPixelAlive() ntrig = %d, vcal = %d", fParNtrig, fParVcal));
  cacheDacs();

  fDirectory->cd();
  PixTest::update(); 

  //??  fApi->setDAC("ctrlreg", 4);
  fApi->setDAC("vcal", fParVcal);

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  for (unsigned int i = 0; i < fHotPixels.size(); ++i) {
    vector<pair<int, int> > hot = fHotPixels[i]; 
    for (unsigned int ipix = 0; ipix < hot.size(); ++ipix) {
      LOG(logINFO) << "ROC " << getIdFromIdx(i) << " masking hot pixel " << hot[ipix].first << "/" << hot[ipix].second; 
      fApi->_dut->maskPixel(hot[ipix].first, hot[ipix].second, true, getIdFromIdx(i)); 
    }
  }
  maskPixels();

  // -- pattern generator setup without resets
  resetROC();
  fPg_setup.clear();
  vector<pair<string, uint8_t> > pgtmp = fPixSetup->getConfigParameters()->getTbPgSettings();
  for (unsigned i = 0; i < pgtmp.size(); ++i) {
    if (string::npos != pgtmp[i].first.find("resetroc")) continue;
    if (string::npos != pgtmp[i].first.find("resettbm")) continue;
    fPg_setup.push_back(pgtmp[i]);
  }
  if (0) for (unsigned int i = 0; i < fPg_setup.size(); ++i) cout << fPg_setup[i].first << ": " << (int)fPg_setup[i].second << endl;

  fApi->setPatternGenerator(fPg_setup);


 
  pair<vector<TH2D*>,vector<TH2D*> > tests = xEfficiencyMaps("highRate", fParNtrig, FLAG_CHECK_ORDER | FLAG_FORCE_UNMASKED); 
  vector<TH2D*> test2 = tests.first;
  vector<TH2D*> test3 = tests.second;
  vector<int> deadPixel(test2.size(), 0); 
  vector<int> probPixel(test2.size(), 0);
  vector<int> xHits(test3.size(),0);
  vector<int> fidHits(test2.size(),0);   
  vector<int> allHits(test2.size(),0);   
  vector<int> fidPixels(test2.size(),0);   
  for (unsigned int i = 0; i < test2.size(); ++i) {
    fHistOptions.insert(make_pair(test2[i], "colz"));
    fHistOptions.insert(make_pair(test3[i], "colz"));    
    for (int ix = 0; ix < test2[i]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < test2[i]->GetNbinsY(); ++iy) {
        allHits[i] += static_cast<int>(test2[i]->GetBinContent(ix+1, iy+1));
	if ((ix > 1) && (ix < 50) && (iy < 79) && (test2[i]->GetBinContent(ix+1, iy+1) > 0)) {
	  fidHits[i] += static_cast<int>(test2[i]->GetBinContent(ix+1, iy+1));
	  ++fidPixels[i];
	}
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

  copy(test2.begin(), test2.end(), back_inserter(fHistList));
  copy(test3.begin(), test3.end(), back_inserter(fHistList));
  TH2D *h = (TH2D*)(fHistList.back());
  h->Draw(getHistOption(h).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 

  // -- summary printout
  //  int nrocs = fApi->_dut->getNEnabledRocs();
  double sensorArea = 0.015 * 0.010 * 54 * 81; // in cm^2, accounting for larger edge pixels (J. Hoss 2014/10/21)
  string deadPixelString, probPixelString, xHitsString, numTrigsString, 
    fidCalHitsString, allCalHitsString, 
    fidCalEfficiencyString, allCalEfficiencyString, 
    xRayRateString;
  for (unsigned int i = 0; i < probPixel.size(); ++i) {
    probPixelString += Form(" %4d", probPixel[i]); 
    deadPixelString += Form(" %4d", deadPixel[i]);
    xHitsString     += Form(" %4d", xHits[i]);
    allCalHitsString += Form(" %4d", allHits[i]); 
    fidCalHitsString += Form(" %4d", fidHits[i]); 
    int numTrigs = fParNtrig * 4160;
    numTrigsString += Form(" %4d", numTrigs );
    fidCalEfficiencyString += Form(" %.1f", fidHits[i]/static_cast<double>(fidPixels[i]*fParNtrig)*100);
    allCalEfficiencyString += Form(" %.1f", allHits[i]/static_cast<double>(numTrigs)*100);
    xRayRateString += Form(" %.1f", xHits[i]/static_cast<double>(numTrigs)/25./sensorArea*1000.);
  }
  
  LOG(logINFO) << "number of dead pixels (per ROC):    " << deadPixelString;
  LOG(logINFO) << "number of red-efficiency pixels:    " << probPixelString;
  LOG(logINFO) << "number of X-ray hits detected: " << xHitsString;
  LOG(logINFO) << "number of triggers sent (total per ROC): " << numTrigsString;
  LOG(logINFO) << "number of Vcal hits detected: " << allCalHitsString;
  LOG(logINFO) << "Vcal hit fiducial efficiency (%): " << fidCalEfficiencyString;
  LOG(logINFO) << "Vcal hit overall efficiency (%): " << allCalEfficiencyString;
  LOG(logINFO) << "X-ray hit rate [MHz/cm2]: " <<  xRayRateString;
  LOG(logINFO) << "PixTestHighRate::doXPixelAlive() done";
  restoreDacs();

  finalCleanup();
}


// ----------------------------------------------------------------------
void PixTestHighRate::doRunDaq() {

  // -- fill into same(!) histogram when re-running the test
  vector<TH2D*> v = mapsWithString(fHitMap, "daqbbtest"); 
  if (0 == v.size()) {
    bookHist("daqbbtest");
    v = mapsWithString(fHitMap, "daqbbtest"); 
  }

  banner(Form("PixTestHighRate::runDaq() running for %d seconds", fParRunSeconds));

  fDirectory->cd();
  PixTest::update(); 


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

  // -- take data
  fDirectory->cd();
  doHitMap(fParRunSeconds, v); 

  // -- display
  TH2D *h = (TH2D*)(fHistList.back());
  h->Draw(getHistOption(h).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 

  // -- analyze

  string zPixelString(""); 
  for (unsigned int i = 0; i < v.size(); ++i) {
    int cnt(0); 
    LOG(logDEBUG) << "analyzing " << v[i]->GetName(); 
    for (int ix = 0; ix < v[i]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < v[i]->GetNbinsY(); ++iy) {
	if (0 == v[i]->GetBinContent(ix+1, iy+1)) ++cnt;
      }
    }
    zPixelString += Form(" %4d ", cnt);
  }
  
  LOG(logINFO) << "Pixels without X-ray hits (per ROC): " << zPixelString; 
  LOG(logINFO) << "PixTestHighRate::doRunDaq() done";

}



// ----------------------------------------------------------------------
void PixTestHighRate::doHitMap(int nseconds, vector<TH2D*> h) {

  int totalPeriod = prepareDaq(fParTriggerFrequency, 50);
  fApi->daqStart();

  int finalPeriod = fApi->daqTriggerLoop(totalPeriod);
  LOG(logINFO) << "PixTestHighRate::doHitMap start TriggerLoop with trigger frequency " << fParTriggerFrequency 
	       << ", period " << finalPeriod 
	       << " and duration " << nseconds << " seconds";
    
  timer t;
  uint8_t perFull;
  fDaq_loop = true;
  while (fApi->daqStatus(perFull) && fDaq_loop) {
    gSystem->ProcessEvents();
    if (perFull > 80) {
      LOG(logINFO) << "run duration " << t.get()/1000 << " seconds, buffer almost full (" 
		   << (int)perFull << "%), pausing triggers.";
      fApi->daqTriggerLoopHalt();
      fillMap(h);
      LOG(logINFO) << "Resuming triggers.";
	  fApi->daqTriggerLoop(finalPeriod);
    }
    
    if (static_cast<int>(t.get()/1000) >= nseconds)	{
      LOG(logINFO) << "data taking finished, elapsed time: " << t.get()/1000 << " seconds.";
      fDaq_loop = false;
      break;
    }
  }
    
  fApi->daqTriggerLoopHalt();
  fApi->daqStop();

  fillMap(h);
  finalCleanup();
       
}

// ----------------------------------------------------------------------
void PixTestHighRate::fillMap(vector<TH2D*> hist) {

  int pixCnt(0);  
  vector<pxar::Event> daqdat = fApi->daqGetEventBuffer();
  
  for(std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
    pixCnt += it->pixels.size();
    
    for (unsigned int ipix = 0; ipix < it->pixels.size(); ++ipix) {
      hist[getIdxFromId(it->pixels[ipix].roc())]->Fill(it->pixels[ipix].column(), it->pixels[ipix].row());
    }
  }
  LOG(logDEBUG) << "Processing Data: " << daqdat.size() << " events with " << pixCnt << " pixels";
}


// ----------------------------------------------------------------------
void PixTestHighRate::doRunMaskHotPixels() {    
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
void PixTestHighRate::doStop(){
	// Interrupt the test 
	fDaq_loop = false;
	LOG(logINFO) << "Stop pressed. Ending test.";
}
