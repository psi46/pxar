
#include <stdlib.h>     /* atof, atoi,itoa */
#include <algorithm>    // std::find
#include <iostream>
#include <fstream>

#include "PixTestHighRate.hh"
#include "log.h"
#include "TStopwatch.h"

#include "PixUtil.hh"


#include <TH2.h>
#include <TMath.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestHighRate)

// ----------------------------------------------------------------------
PixTestHighRate::PixTestHighRate(PixSetup *a, std::string name) : PixTest(a, name),
  fParTriggerFrequency(0), fParRunSeconds(0), fParTriggerDelay(20),
  fParFillTree(false), fParDelayTBM(false), fParNtrig(5), fParVcal(200), 
  fParMaskFileName("default"), fParSaveMaskedPixels(0)   {
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
  string str1, str2; 
  string::size_type s1;
  int pixc, pixr; 
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true;
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
      if (!parName.compare("trgfrequency(khz)")) {
	fParTriggerFrequency = atoi(sval.c_str());
	LOG(logDEBUG) << "  setting fParTriggerFrequency -> " << fParTriggerFrequency;
	setToolTips();
      }
      if (!parName.compare("runseconds")) {
	fParRunSeconds = atoi(sval.c_str());
	setToolTips();
      }
      if (!parName.compare("triggerdelay")) {
	fParTriggerDelay = atoi(sval.c_str());
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
      if (!parName.compare("pix") || !parName.compare("pix1") ) {
	s1 = sval.find(",");
	if (string::npos != s1) {
	  str1 = sval.substr(0, s1);
	  pixc = atoi(str1.c_str());
	  str2 = sval.substr(s1+1);
	  pixr = atoi(str2.c_str());
	  clearSelectedPixels();
	  fPIX.push_back( make_pair(pixc, pixr) );
	  addSelectedPixels(sval); 
	} else {
	  clearSelectedPixels();
	  addSelectedPixels("-1,-1"); 
	  LOG(logDEBUG) << "  clear fPIX: " << fPIX.size(); 
	}
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

  if (!command.compare("caldelscan")) {
    doCalDelScan();
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
  doRunMaskHotPixels();
  doCalDelScan();
  doXPixelAlive();
  doRunDaq();
  LOG(logINFO) << "PixTestHighRate::doTest() done ";
}

// ----------------------------------------------------------------------
void PixTestHighRate::doCalDelScan() {

  uint16_t FLAGS = FLAG_FORCE_MASKED;

  int ntrig(10); 
  banner(Form("PixTestHighRate::calDelScan() ntrig = %d, vcal = %d", ntrig, fParVcal));
  cacheDacs();

  fDirectory->cd();
  PixTest::update();

  fApi->setDAC("vcal", fParVcal);

  // -- do local short 1D caldel scan
  // --------------------------------
  int ip = 0; 
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  fApi->_dut->testPixel(fPIX[ip].first, fPIX[ip].second, true);
  fApi->_dut->maskPixel(fPIX[ip].first, fPIX[ip].second, false);
  
  bool done = false;
  vector<pair<uint8_t, vector<pixel> > >  results;
  while (!done) {
    results.clear(); 
    int cnt(0); 
    try{
      results = fApi->getEfficiencyVsDAC("caldel", 0, 255, FLAGS, 3);
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
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);


  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  TH1D *h1(0);
  vector<TH1D*> maps;
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    h1 = bookTH1D(Form("hrCalDelScan_C%d", rocIds[iroc]), Form("hrCalDelScan_C%d", rocIds[iroc]), 256, 0., 256.);
    h1->SetMinimum(0.); 
    h1->SetDirectory(fDirectory); 
    fHistList.push_back(h1); 
    maps.push_back(h1); 
  }
  
  int idx(-1); 
  for (unsigned int i = 0; i < results.size(); ++i) {
    int caldel = results[i].first; 
    vector<pixel> pixels = results[i].second; 
    for (unsigned int ipix = 0; ipix < pixels.size(); ++ipix) {
      idx = getIdxFromId(pixels[ipix].roc());
      h1 = maps[idx]; 
      if (h1) {
	h1->Fill(caldel, pixels[ipix].value()); 
      } else {
	LOG(logDEBUG) << "no histogram found for ROC " << pixels[ipix].roc() << " with index " << idx; 
      }
    }
  }
      
  vector<int> calDelLo(rocIds.size(), -1); 
  vector<int> calDelHi(rocIds.size(), -1); 
  int DeltaCalDelMax(-1), reserve(1); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    double cdMax   = maps[iroc]->GetMaximum();
    calDelLo[iroc] = static_cast<int>(maps[iroc]->GetBinLowEdge(maps[iroc]->FindFirstBinAbove(0.8*cdMax) + reserve)); 
    calDelHi[iroc] = static_cast<int>(maps[iroc]->GetBinLowEdge(maps[iroc]->FindLastBinAbove(0.8*cdMax) - reserve));
    if (calDelHi[iroc] - calDelLo[iroc] > DeltaCalDelMax) {
      DeltaCalDelMax = calDelHi[iroc] - calDelLo[iroc]; 
    }
  }


  // -- now to xEfficiencyMap vs CalDel
  // ----------------------------------
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


  vector<pair<int, double> > calDelMax(rocIds.size(), make_pair(-1, -1)); // (CalDel,meanEff)
  vector<TH1D*> calDelEffHist;
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    h1 = bookTH1D(Form("HR_CalDelScan_eff_C%d", getIdFromIdx(iroc)),  
		  Form("HR_CalDelScan_eff_C%d", getIdFromIdx(iroc)), 256, 0., 256);
    calDelEffHist.push_back(h1); 
  }

  int nsteps(20); 
  for (int istep = 0; istep < nsteps; ++istep) {
    // -- set CalDel per ROC 
    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
      int caldel = calDelLo[iroc] + istep*(calDelHi[iroc]-calDelLo[iroc])/(nsteps-1);
      fApi->setDAC("CalDel", caldel, rocIds[iroc]);
    }
    
    
    pair<vector<TH2D*>,vector<TH2D*> > tests = xEfficiencyMaps(Form("HR_xeff_CalDelScan_step%d", istep), 
							       ntrig, FLAG_CHECK_ORDER | FLAG_FORCE_UNMASKED);
    vector<TH2D*> test2 = tests.first;
    for (unsigned int iroc = 0; iroc < test2.size(); ++iroc) {
      fHistOptions.insert(make_pair(test2[iroc], "colz"));
      h1 = bookTH1D(Form("HR_CalDelScan_step%d_C%d", istep, getIdFromIdx(iroc)),  
		    Form("HR_CalDelScan_step%d_C%d", istep, getIdFromIdx(iroc)),  201, 0., 1.005);
      fHistList.push_back(h1); 
      for (int ix = 0; ix < test2[iroc]->GetNbinsX(); ++ix) {
	for (int iy = 0; iy < test2[iroc]->GetNbinsY(); ++iy) {
	  h1->Fill(test2[iroc]->GetBinContent(ix+1, iy+1)/ntrig);
	}
      }
      int caldel = fApi->_dut->getDAC(rocIds[iroc], "CalDel");
      calDelEffHist[iroc]->SetBinContent(caldel, h1->GetMean()); 

      if (h1->GetMean() > calDelMax[iroc].second) {
 	calDelMax[iroc].first  = caldel; 
 	calDelMax[iroc].second = h1->GetMean();
      }
    }

    for (unsigned int i = 0; i < tests.first.size(); ++i) {
      delete tests.first[i]; 
      delete tests.second[i]; 
    }
    tests.first.clear();
    tests.second.clear();
  }

  restoreDacs();
  for (unsigned int i = 0; i < calDelMax.size(); ++i) {
    LOG(logDEBUG) << "roc " << Form("%2d", i) << ": caldel = " << calDelMax[i].first << " eff = " << calDelMax[i].second;
    fApi->setDAC("CalDel", calDelMax[i].first, rocIds[i]);
  }

  copy(calDelEffHist.begin(), calDelEffHist.end(), back_inserter(fHistList));

  calDelEffHist[0]->Draw();  
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), calDelEffHist[0]);
  PixTest::update(); 


}


// ----------------------------------------------------------------------
void PixTestHighRate::doXPixelAlive() {

  banner(Form("PixTestHighRate::xPixelAlive() ntrig = %d, vcal = %d", fParNtrig, fParVcal));
  cacheDacs();


  // -- cache triggerdelay
  vector<pair<string, uint8_t> > oldDelays = fPixSetup->getConfigParameters()->getTbSigDelays();
  bool foundIt(false);
  for (unsigned int i = 0; i < oldDelays.size(); ++i) {
    if (oldDelays[i].first == "triggerdelay") {
      foundIt = true;
    }
    LOG(logDEBUG) << " old set: " << oldDelays[i].first << ": " << (int)oldDelays[i].second;
  }

  vector<pair<string, uint8_t> > delays = fPixSetup->getConfigParameters()->getTbSigDelays();
  if (!foundIt) {
    delays.push_back(make_pair("triggerdelay", fParTriggerDelay));
    oldDelays.push_back(make_pair("triggerdelay", 0));
  } else {
    for (unsigned int i = 0; i < delays.size(); ++i) {
      if (delays[i].first == "triggerdelay") {
	delays[i].second = fParTriggerDelay;
      }
    }
  }

  for (unsigned int i = 0; i < delays.size(); ++i) {
    LOG(logDEBUG) << " setting: " << delays[i].first << ": " << (int)delays[i].second;
  }
  fApi->setTestboardDelays(delays);


  fDirectory->cd();
  PixTest::update();

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
  TH1D *h1(0), *h2(0); 
  for (unsigned int i = 0; i < test2.size(); ++i) {
    fHistOptions.insert(make_pair(test2[i], "colz"));
    fHistOptions.insert(make_pair(test3[i], "colz"));
    h1 = bookTH1D(Form("HR_Overall_Efficiency_C%d", getIdFromIdx(i)),  Form("HR_Overall_Efficiency_C%d", getIdFromIdx(i)),  201, 0., 1.005);
    fHistList.push_back(h1); 
    h2 = bookTH1D(Form("HR_Fiducial_Efficiency_C%d", getIdFromIdx(i)),  Form("HR_Fiducial_Efficiency_C%d", getIdFromIdx(i)),  201, 0., 1.005);
    fHistList.push_back(h2); 
    
    for (int ix = 0; ix < test2[i]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < test2[i]->GetNbinsY(); ++iy) {
        allHits[i] += static_cast<int>(test2[i]->GetBinContent(ix+1, iy+1));
	h1->Fill(test2[i]->GetBinContent(ix+1, iy+1)/fParNtrig);
	if ((ix > 0) && (ix < 51) && (iy < 79) && (test2[i]->GetBinContent(ix+1, iy+1) > 0)) {
	  fidHits[i] += static_cast<int>(test2[i]->GetBinContent(ix+1, iy+1));
	  ++fidPixels[i];
	  h2->Fill(test2[i]->GetBinContent(ix+1, iy+1)/fParNtrig);
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

  LOG(logINFO) << "number of dead pixels (per ROC): " << deadPixelString;
  LOG(logINFO) << "number of red-efficiency pixels: " << probPixelString;
  LOG(logINFO) << "number of X-ray hits detected:   " << xHitsString;
  LOG(logINFO) << "number of triggers sent (total per ROC): " << numTrigsString;
  LOG(logINFO) << "number of Vcal hits detected: " << allCalHitsString;
  LOG(logINFO) << "Vcal hit fiducial efficiency (%): " << fidCalEfficiencyString;
  LOG(logINFO) << "Vcal hit overall efficiency (%): " << allCalEfficiencyString;
  LOG(logINFO) << "X-ray hit rate [MHz/cm2]: " <<  xRayRateString;
  LOG(logINFO) << "PixTestHighRate::doXPixelAlive() done";
  restoreDacs();

  // -- reset sig delays
  fApi->setTestboardDelays(oldDelays);
  for (unsigned int i = 0; i < oldDelays.size(); ++i) {
    LOG(logDEBUG) << " resetting: " << oldDelays[i].first << ": " << (int)oldDelays[i].second;
  }

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

  // -- cache triggerdelay
  vector<pair<string, uint8_t> > oldDelays = fPixSetup->getConfigParameters()->getTbSigDelays();
  bool foundIt(false);
  for (unsigned int i = 0; i < oldDelays.size(); ++i) {
    if (oldDelays[i].first == "triggerdelay") {
      foundIt = true;
    }
    LOG(logDEBUG) << " old set: " << oldDelays[i].first << ": " << (int)oldDelays[i].second;
  }

  vector<pair<string, uint8_t> > delays = fPixSetup->getConfigParameters()->getTbSigDelays();
  if (!foundIt) {
    delays.push_back(make_pair("triggerdelay", fParTriggerDelay));
    oldDelays.push_back(make_pair("triggerdelay", 0));
  } else {
    for (unsigned int i = 0; i < delays.size(); ++i) {
      if (delays[i].first == "triggerdelay") {
	delays[i].second = fParTriggerDelay;
      }
    }
  }

  for (unsigned int i = 0; i < delays.size(); ++i) {
    LOG(logDEBUG) << " setting: " << delays[i].first << ": " << (int)delays[i].second;
  }
  fApi->setTestboardDelays(delays);

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

  // -- reset sig delays
  fApi->setTestboardDelays(oldDelays);
  for (unsigned int i = 0; i < oldDelays.size(); ++i) {
    LOG(logDEBUG) << " resetting: " << oldDelays[i].first << ": " << (int)oldDelays[i].second;
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
	       << " kHz, period " << finalPeriod
	       << " and duration " << nseconds << " seconds";

  TStopwatch t;
  uint8_t perFull;
  fDaq_loop = true;
  int seconds(0); 
  while (fApi->daqStatus(perFull) && fDaq_loop) {
    gSystem->ProcessEvents();
    if (perFull > 80) {
      seconds = t.RealTime(); 
      LOG(logINFO) << "run duration " << seconds << " seconds, buffer almost full ("
		   << (int)perFull << "%), pausing triggers.";
      fApi->daqTriggerLoopHalt();
      fillMap(h);
      LOG(logINFO) << "Resuming triggers.";
      t.Start(kFALSE);
      fApi->daqTriggerLoop(finalPeriod);
    }

    seconds = t.RealTime(); 
    t.Start(kFALSE);
    if (static_cast<int>(seconds) >= nseconds)	{
      LOG(logINFO) << "data taking finished, elapsed time: " << seconds << " seconds.";
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

  if (fParSaveMaskedPixels) {
    if (fParMaskFileName == "default") {
      fPixSetup->getConfigParameters()->writeMaskFile(fHotPixels); 
    } else {
      fPixSetup->getConfigParameters()->writeMaskFile(fHotPixels, fParMaskFileName); 
    }
  }

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
