
#include <stdlib.h>     /* atof, atoi,itoa */
#include <algorithm>    // std::find
#include <iostream>
#include <fstream>
#include "PixTestHighRate.hh"
#include "log.h"


#include <TH2.h>
#include <TMath.h>
#include "../core/utils/timer.h"

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
bool PixTestHighRate::setTrgFrequency(uint8_t TrgTkDel, int triggerFreq) {
  uint8_t trgtkdel = TrgTkDel;
  
  double period_ns = 1 / (double)triggerFreq * 1000000; // trigger frequency in kHz.
  double fClkDelays = period_ns / 25 - trgtkdel;
  uint16_t ClkDelays = (uint16_t)fClkDelays; //debug -- aprox to def
  
  // -- add right delay between triggers:
  uint16_t i = ClkDelays;
  while (i>255){
    fPg_setup.push_back(make_pair("delay", 255));
    i = i - 255;
  }
  fPg_setup.push_back(make_pair("delay", i));
  
  // -- then send trigger and token:
  fPg_setup.push_back(make_pair("trg", trgtkdel));	// PG_TRG b000010
  fPg_setup.push_back(make_pair("tok", 0));	// PG_TOK
  
  return true;
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
	fParDelayTBM = !(atoi(sval.c_str())==0);
	setToolTips();
      }
      if (!parName.compare("filltree")) {
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

  if (!command.compare("maskhotpixels")) {
    maskHotPixels(); 
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

  fApi->setDAC("ctrlreg", 4);
  fApi->setDAC("vcal", fParVcal);

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
 
  pair<vector<TH2D*>,vector<TH2D*> > tests = xEfficiencyMaps("highRate", fParNtrig, FLAG_CHECK_ORDER | FLAG_FORCE_UNMASKED); 
  vector<TH2D*> test2 = tests.first;
  vector<TH2D*> test3 = tests.second;
  vector<int> deadPixel(test2.size(), 0); 
  vector<int> probPixel(test2.size(), 0);
  vector<int> xHits(test3.size(),0);
  vector<int> vCalHits (test2.size(),0);   
  for (unsigned int i = 0; i < test2.size(); ++i) {
    fHistOptions.insert(make_pair(test2[i], "colz"));
    fHistOptions.insert(make_pair(test3[i], "colz"));    
    for (int ix = 0; ix < test2[i]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < test2[i]->GetNbinsY(); ++iy) {
        vCalHits[i] += static_cast<int>(test2[i]->GetBinContent(ix+1, iy+1));
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
  string deadPixelString, probPixelString, xHitsString, numTrigsString, vCalHitsString,xRayHitEfficiencyString,xRayRateString;
  for (unsigned int i = 0; i < probPixel.size(); ++i) {
    probPixelString += Form(" %4d", probPixel[i]); 
    deadPixelString += Form(" %4d", deadPixel[i]);
    xHitsString     += Form(" %4d", xHits[i]);
    vCalHitsString += Form(" %4d",vCalHits[i]); 
    int numTrigs = fParNtrig * 4160;
    numTrigsString += Form(" %4d", numTrigs );
    xRayHitEfficiencyString += Form(" %.1f", (vCalHits[i]+fParNtrig*deadPixel[i])/static_cast<double>(numTrigs)*100);
    xRayRateString += Form(" %.1f", xHits[i]/static_cast<double>(numTrigs)/25./0.64*1000.);
  }

  LOG(logINFO) << "number of dead pixels (per ROC):    " << deadPixelString;
  LOG(logINFO) << "number of red-efficiency pixels:    " << probPixelString;
  LOG(logINFO) << "number of X-ray hits detected: " << xHitsString;
  LOG(logINFO) << "number of triggers sent (total per ROC): " << numTrigsString;
  LOG(logINFO) << "number of Vcal hits detected: " << vCalHitsString;
  LOG(logINFO) << "Vcal hit detection efficiency (%): " << xRayHitEfficiencyString;
  LOG(logINFO) << "X-ray hit rate [MHz/cm2]: " <<  xRayRateString;
  LOG(logINFO) << "PixTestHighRate::doXPixelAlive() done";
  restoreDacs();
}



// ----------------------------------------------------------------------
void PixTestHighRate::pgToDefault(vector<pair<std::string, uint8_t> > /*pg_setup*/) {
  fPg_setup.clear();
  LOG(logDEBUG) << "PixTestHighRate::PG_Setup clean";
  
  fPg_setup = fPixSetup->getConfigParameters()->getTbPgSettings();
  fApi->setPatternGenerator(fPg_setup);
  LOG(logINFO) << "PixTestHighRate::Xray pg_setup set to default.";
}

// ----------------------------------------------------------------------
void PixTestHighRate::finalCleanup() {
  pgToDefault(fPg_setup);
  
  fPg_setup.clear();
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
      LOG(logDEBUG) << "ROC " << getIdFromIdx(i) << " masking hot pixel " << hot[ipix].first << "/" << hot[ipix].second; 
      fApi->_dut->maskPixel(hot[ipix].first, hot[ipix].second, true, getIdFromIdx(i)); 
    }
  }
  maskPixels();

  // -- take data
  fDirectory->cd();
  doHitMap(fParRunSeconds, fHitMap); 

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

  // -- setup DAQ for data taking
  fPg_setup.clear();
  fPg_setup.push_back(make_pair("resetroc", 0)); // PG_RESR b001000
  uint16_t period = 28;
  fApi->setPatternGenerator(fPg_setup);
  fApi->daqStart();
  fApi->daqTrigger(1, period);
  fApi->daqStop();
  fPg_setup.clear();
  setTrgFrequency(50, fParTriggerFrequency);
  fApi->setPatternGenerator(fPg_setup);
  
  timer t;
  uint8_t perFull;
  fDaq_loop = true;
    
  fApi->daqStart();

  int finalPeriod = fApi->daqTriggerLoop(0);  //period is automatically set to the minimum by Api function
  LOG(logINFO) << "PixTestHighRate::doHitMap start TriggerLoop with trigger frequency " << fParTriggerFrequency 
	       << ", period " << finalPeriod 
	       << " and duration " << nseconds << " seconds";
    
  while (fApi->daqStatus(perFull) && fDaq_loop) {
    gSystem->ProcessEvents();
    if (perFull > 80) {
      LOG(logINFO) << "Buffer almost full, pausing triggers.";
      fApi->daqTriggerLoopHalt();
      fillMap(h);
      LOG(logINFO) << "Resuming triggers.";
      fApi->daqTriggerLoop();
    }
    
    if (static_cast<int>(t.get()/1000) >= nseconds)	{
      LOG(logINFO) << "Elapsed time: " << t.get()/1000 << " seconds.";
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
void PixTestHighRate::maskHotPixels() {

  int NSECONDS(10); 
  int TRGFREQ(100); // in kiloHertz

  fHotPixels.clear(); 

  vector<TH2D*> v = mapsWithString(fHitMap, "hotpixels"); 
  if (0 == v.size()) {
    bookHist("hotpixels");
    v = mapsWithString(fHitMap, "hotpixels"); 
  }
  for (unsigned int i = 0; i < v.size(); ++i) v[i]->Reset();

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(false);

  // -- setup DAQ for data taking
  fPg_setup.clear();
  fPg_setup.push_back(make_pair("resetroc", 0)); // PG_RESR b001000
  uint16_t period = 28;
  fApi->setPatternGenerator(fPg_setup);
  fApi->daqStart();
  fApi->daqTrigger(1, period);
  fApi->daqStop();
  fPg_setup.clear();
  setTrgFrequency(50, TRGFREQ);
  fApi->setPatternGenerator(fPg_setup);
  
  timer t;
  uint8_t perFull;
  fDaq_loop = true;
    
  fApi->daqStart();

  int finalPeriod = fApi->daqTriggerLoop(0);  //period is automatically set to the minimum by Api function
  LOG(logINFO) << "PixTestHighRate::maskHotPixels start TriggerLoop with period " << finalPeriod 
	       << " and duration " << NSECONDS << " seconds and trigger rate " << TRGFREQ << " kHz";
  
  while (fApi->daqStatus(perFull) && fDaq_loop) {
    if (perFull > 80) {
      LOG(logINFO) << "Buffer almost full, pausing triggers.";
      fApi->daqTriggerLoopHalt();
      fillMap(v);
      LOG(logINFO) << "Resuming triggers.";
      fApi->daqTriggerLoop();
    }
    
    if (static_cast<int>(t.get()/1000) >= NSECONDS)	{
      LOG(logINFO) << "Elapsed time: " << t.get()/1000 << " seconds.";
      fDaq_loop = false;
      break;
    }
  }
    
  fApi->daqTriggerLoopHalt();
  
  fApi->daqStop();
  fillMap(v);
  finalCleanup();


  // -- analysis of hit map
  double THR = 1e-5*NSECONDS*TRGFREQ*1000; 
  LOG(logDEBUG) << "hot pixel determination with THR = " << THR; 
  TH2D *h(0); 
  for (unsigned int i = 0; i < v.size(); ++i) {
    h = v[i]; 
    vector<pair<int, int> > hot; 
    for (int ix = 0; ix < h->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < h->GetNbinsY(); ++iy) {
	if (h->GetBinContent(ix+1, iy+1) > THR) {
	  LOG(logDEBUG) << "ROC " << i << " with hot pixel " << ix << "/" << iy << ",  hits = " << h->GetBinContent(ix+1, iy+1);
	  hot.push_back(make_pair(ix, iy)); 
	}
      }
    }
    fHotPixels.push_back(hot); 
  }
  
}
