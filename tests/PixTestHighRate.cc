
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
bool PixTestHighRate::setTrgFrequency(uint8_t TrgTkDel) {
  uint8_t trgtkdel = TrgTkDel;
  
  double period_ns = 1 / (double)fParTriggerFrequency * 1000000; // trigger frequency in kHz.
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
double PixTestHighRate::meanHit(TH2D *h2) {

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
double PixTestHighRate::noiseLevel(TH2D *h2) {

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
int PixTestHighRate::countHitsAndMaskPixels(TH2D *h2, double noiseLevel, int iroc) {
  
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
void PixTestHighRate::maskHotPixels() {

  if (0 == fHitMap.size()) bookHist("maskHotPixels");

  int ntrig(1), vthrCompMin(5), vthrCompMax(105); 
  banner(Form("PixTestHighRate::maskHotPixels() running with ntrig = %d/step, vthrcomp = %d .. %d", ntrig, vthrCompMin, vthrCompMax));
  cacheDacs();

  vector<uint8_t> vVthrComp = getDacs("vthrcomp");

  fDirectory->cd();

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  
  // -- Check and mask hot pixels
  bool done(false);
  for (int vThrComp = vthrCompMin; vThrComp <= vthrCompMax; vThrComp = vThrComp+10) {
    LOG(logDEBUG) << "setting vthrcomp to " << vThrComp;
    // set vthrcomp
    done = true;
    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
      done = done && (vThrComp > vVthrComp[iroc]); 
      fApi->setDAC("vthrcomp", vThrComp, rocIds[iroc]); 
    }
    if (done) {
      LOG(logDEBUG) << "all ROCs have original VthrComp < vthrcomp; breaking out of loop"; 
      break;
    }

    for (unsigned i = 0; i < fHitMap.size(); ++i) {
      fHitMap[i]->Reset();
    }

    doHitMap(1); 
    for (unsigned int i = 0; i < rocIds.size(); ++i) {
      vector<pair<int,int> > hotPixels = checkHotPixels(fHitMap[i]);
      for (unsigned int j = 0; j<hotPixels.size(); ++j) {
 	LOG(logDEBUG) << "mask hot pixel ROC/col/row: " << static_cast<int>(rocIds[i]) 
 		      << "/" <<  hotPixels[j].first << "/" << hotPixels[j].second; 
 	fApi->_dut->maskPixel(hotPixels[j].first, hotPixels[j].second, true,  rocIds[i]);
 	fApi->_dut->testPixel(hotPixels[j].first, hotPixels[j].second, false, rocIds[i]);
      }
    }

  }


  LOG(logINFO) << "# masked pixels per ROC:"; 
  for (unsigned int i = 0; i < rocIds.size(); ++i) {
    LOG(logINFO) << " ROC " << static_cast<int>(rocIds[i]) << ": " << static_cast<int>(fApi->_dut->getNMaskedPixels(rocIds[i])); 
  }

  restoreDacs();
}



// ----------------------------------------------------------------------
void PixTestHighRate::doHitMap(int nseconds) {

  // -- setup DAQ for data taking
  fPg_setup.clear();
  fPg_setup.push_back(make_pair("resetroc", 0)); // PG_RESR b001000
  uint16_t period = 28;
  fApi->setPatternGenerator(fPg_setup);
  fApi->daqStart();
  fApi->daqTrigger(1, period);
  fApi->daqStop();
  fPg_setup.clear();
  setTrgFrequency(50);
  fApi->setPatternGenerator(fPg_setup);
  
  timer t;
  uint8_t perFull;
  fDaq_loop = true;
    
  fApi->daqStart();

  int finalPeriod = fApi->daqTriggerLoop(0);  //period is automatically set to the minimum by Api function
  LOG(logINFO) << "PixTestHighRate::doRateScan start TriggerLoop with period " << finalPeriod 
	       << " and duration " << nseconds << " seconds";
    
  while (fApi->daqStatus(perFull) && fDaq_loop) {
    gSystem->ProcessEvents();
    if (perFull > 80) {
      LOG(logINFO) << "Buffer almost full, pausing triggers.";
      fApi->daqTriggerLoopHalt();
      readData();
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
  readData();
  finalCleanup();
       
}

// ----------------------------------------------------------------------
void PixTestHighRate::readData() {

  int pixCnt(0);  
  vector<pxar::Event> daqdat;
  
  daqdat = fApi->daqGetEventBuffer();
  
  for(std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
    pixCnt += it->pixels.size();
    
    for (unsigned int ipix = 0; ipix < it->pixels.size(); ++ipix) {
      fHitMap[getIdxFromId(it->pixels[ipix].roc_id)]->Fill(it->pixels[ipix].column, it->pixels[ipix].row);
    }
  }
  LOG(logDEBUG) << "Processing Data: " << daqdat.size() << " events with " << pixCnt << " pixels";
}



// ----------------------------------------------------------------------
void PixTestHighRate::doRunDaq() {

  if (0 == fHitMap.size()) bookHist("daqbbtest");

  banner(Form("PixTestHighRate::runDaq() running for %d seconds", fParRunSeconds));

  fDirectory->cd();
  PixTest::update(); 

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(false);
  fDirectory->cd();
  doHitMap(fParRunSeconds); 

  TH2D *h = (TH2D*)(fHistList.back());
  h->Draw(getHistOption(h).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 

  vector<TH2D*> v = mapsWithString(fHitMap, "daqbbtest"); 
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

