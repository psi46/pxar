
#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include "PixTestDaq.hh"
#include "log.h"
#include "helper.h"
#include "timer.h"

#include "PixUtil.hh"

using namespace std;
using namespace pxar;

ClassImp(PixTestDaq)

// ----------------------------------------------------------------------
PixTestDaq::PixTestDaq(PixSetup *a, std::string name) : PixTest(a, name), fParDelayTBM(0), fRunDaqTrigger(0), fParNtrig(0), fParStretch(0), fParFillTree(0), fParSeconds(0), fParTriggerFrequency(0), fParIter(0) {
  PixTest::init();
  init(); 
  LOG(logDEBUG) << "PixTestDaq ctor(PixSetup &a, string, TGTab *)";
  
  fTree = 0; 
  fPhCal.setPHParameters(fPixSetup->getConfigParameters()->getGainPedestalParameters());
  fPhCalOK = fPhCal.initialized();
}

//----------------------------------------------------------
PixTestDaq::PixTestDaq() : PixTest() {
  LOG(logDEBUG) << "PixTestDaq ctor()";
  fTree = 0; 
}

//----------------------------------------------------------
PixTestDaq::~PixTestDaq() {
	LOG(logDEBUG) << "PixTestDaq dtor";
	fDirectory->cd();
	if (fTree && fParFillTree) fTree->Write();
}

// ----------------------------------------------------------------------
void PixTestDaq::init() {
  LOG(logDEBUG) << "PixTestDaq::init()";
  setToolTips();
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 
}

// ----------------------------------------------------------------------
void PixTestDaq::setToolTips() {
  fTestTip    = string("Run DAQ seconds - data from each run will be added to the same histogram.") ;
  fSummaryTip = string("to be implemented") ;
  fStopTip    = string("Stop DAQ and save data.");
}

// ----------------------------------------------------------------------
bool PixTestDaq::setParameter(string parName, string sval) {
	bool found(false);
	fParOutOfRange = false;
	std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
	for (unsigned int i = 0; i < fParameters.size(); ++i) {
		if (fParameters[i].first == parName) {
			found = true;
			if (!parName.compare("delaytbm")) {
				PixUtil::replaceAll(sval, "checkbox(", "");
				PixUtil::replaceAll(sval, ")", "");
				fParDelayTBM = !(atoi(sval.c_str()) == 0);
				setToolTips();
			}
			if (!parName.compare("filltree")) {
				PixUtil::replaceAll(sval, "checkbox(", "");
				PixUtil::replaceAll(sval, ")", "");
				fParFillTree = !(atoi(sval.c_str()) == 0);
				setToolTips();
			}
			if (!parName.compare("clockstretch")) {  //debug - useful?
				fParStretch = atoi(sval.c_str());
				setToolTips();
			}
			if (!parName.compare("trgfrequency(khz)")){     // trigger frequency in kHz.
				fParTriggerFrequency = atoi(sval.c_str());
				if (fParTriggerFrequency == 0) {
					LOG(logWARNING) << "PixTestDaq::setParameter() trgfrequency must be different from zero";
					found = false; fParOutOfRange = true;
				}
			}
			if (!parName.compare("trgnumber")) {
				fParNtrig = atoi(sval.c_str());
				setToolTips();
			}
			if (!parName.compare("iterations")) {
				fParIter = atoi(sval.c_str());
				setToolTips();
			}
			if (!parName.compare("daqseconds")){
				fParSeconds = atoi(sval.c_str());
			}
		}
	}
	return found;
}

// ----------------------------------------------------------------------
void PixTestDaq::doStop(){
	// Interrupt the test 
	fDaq_loop = false;
	LOG(logINFO) << "Stop pressed. Ending test.";
}

// ----------------------------------------------------------------------
void PixTestDaq::doRunMaskHotPixels() {
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
void PixTestDaq::runCommand(std::string command) {
		
	if (command == "stop")
		doStop();
	else if (!command.compare("maskhotpixels")) {
		doRunMaskHotPixels();
		return;
	}
	else if (!command.compare("rundaqtrg")) {
		fRunDaqTrigger = true;
		doDaqRun();
	}
	else if (!command.compare("rundaqseconds")) {
		fRunDaqTrigger = false;
		doDaqRun();
	}
	else
		LOG(logINFO) << "Command " << command << " not implemented.";
}

// ----------------------------------------------------------------------
void PixTestDaq::bookHist(string name) {

	if (fParFillTree) bookTree();
	fHitMap.clear(); fPhmap.clear(); fPh.clear(); fQmap.clear(); fQ.clear();

	std::vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
	TH1D *h1(0);
	TH2D *h2(0);
	TProfile2D *p2(0);
	for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
		h2 = bookTH2D(Form("hits_C%d", rocIds[iroc]), Form("hits_C%d", rocIds[iroc]), 52, 0., 52., 80, 0., 80.);
		h2->SetMinimum(0.);
		h2->SetDirectory(fDirectory);
		setTitles(h2, "col", "row");
		fHistOptions.insert(make_pair(h2, "colz"));
		fHitMap.push_back(h2);

		p2 = bookTProfile2D(Form("phMap_C%d", rocIds[iroc]), Form("phMap_C%d", rocIds[iroc]), 52, 0., 52., 80, 0., 80.);
		p2->SetMinimum(0.);
		p2->SetDirectory(fDirectory);
		setTitles(p2, "col", "row");
		fHistOptions.insert(make_pair(p2, "colz"));
		fPhmap.push_back(p2);

		h1 = bookTH1D(Form("ph_C%d", rocIds[iroc]), Form("ph_C%d", rocIds[iroc]), 256, 0., 256.);
		h1->SetMinimum(0.);
		h1->SetDirectory(fDirectory);
		setTitles(h1, "ADC", "Entries/bin");
		fPh.push_back(h1);

		p2 = bookTProfile2D(Form("qMap_C%d", rocIds[iroc]), Form("qMap_C%d", rocIds[iroc]), 52, 0., 52., 80, 0., 80.);
		p2->SetMinimum(0.);
		p2->SetDirectory(fDirectory);
		setTitles(p2, "col", "row");
		fHistOptions.insert(make_pair(p2, "colz"));
		fQmap.push_back(p2);

		h1 = bookTH1D(Form("q_C%d", rocIds[iroc]), Form("q_C%d", rocIds[iroc]), 200, 0., 1000.);
		h1->SetMinimum(0.);
		h1->SetDirectory(fDirectory);
		setTitles(h1, "Q [Vcal]", "Entries/bin");
		fQ.push_back(h1);
	}
}

// ----------------------------------------------------------------------
void PixTestDaq::ProcessData(uint16_t numevents){

	LOG(logDEBUG) << "Getting Event Buffer";
	std::vector<pxar::Event> daqdat;

	if (numevents > 0) {
		for (unsigned int i = 0; i < numevents; i++) {
			pxar::Event evt = fApi->daqGetEvent();
			//Check if event is empty?
			if (evt.pixels.size() > 0)
				daqdat.push_back(evt);
		}
	}
	else
		daqdat = fApi->daqGetEventBuffer();

	LOG(logDEBUG) << "Processing Data: " << daqdat.size() << " events.";

	int pixCnt(0);
	int idx(-1);
	uint16_t q;
	vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
	for (std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
		pixCnt += it->pixels.size();

		if (fParFillTree) {
			fTreeEvent.header = it->header;
			fTreeEvent.dac = 0;
			fTreeEvent.trailer = it->trailer;
			fTreeEvent.npix = it->pixels.size();
		}

		for (unsigned int ipix = 0; ipix < it->pixels.size(); ++ipix) {
			idx = getIdxFromId(it->pixels[ipix].roc());
			if(idx == -1) {
				LOG(logWARNING) << "PixTestDaq::ProcessData() wrong 'idx' value --> return";
				return;    			
			}
			fHitMap[idx]->Fill(it->pixels[ipix].column(), it->pixels[ipix].row());
			fPhmap[idx]->Fill(it->pixels[ipix].column(), it->pixels[ipix].row(), it->pixels[ipix].value());
			fPh[idx]->Fill(it->pixels[ipix].value());

			if (fPhCalOK) {
				q = static_cast<uint16_t>(fPhCal.vcal(it->pixels[ipix].roc(), it->pixels[ipix].column(),	
								      it->pixels[ipix].row(), it->pixels[ipix].value()));
			}
			else {
				q = 0;
			}
			fQ[idx]->Fill(q);
			fQmap[idx]->Fill(it->pixels[ipix].column(), it->pixels[ipix].row(), q);
				if (fParFillTree) {
				fTreeEvent.proc[ipix] = it->pixels[ipix].roc();
				fTreeEvent.pcol[ipix] = it->pixels[ipix].column();
				fTreeEvent.prow[ipix] = it->pixels[ipix].row();
				fTreeEvent.pval[ipix] = it->pixels[ipix].value();
				fTreeEvent.pq[ipix] = q;
			}
		}
		if (fParFillTree) fTree->Fill();
	}

  	//to draw the hitsmap as 'online' check.
	TH2D* h2 = (TH2D*)(fHitMap.back());
	h2->Draw(getHistOption(h2).c_str());
	fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);
	PixTest::update();

	LOG(logINFO) << Form("events read:%5ld, pixels seen:%3d, hist entries: %4d",
		daqdat.size(), pixCnt, static_cast<int>(fHitMap[0]->GetEntries()));
}

// ----------------------------------------------------------------------
void PixTestDaq::doDaqRun() {

  PixTest::update();
  fDirectory->cd();

  //Immediately stop if parameters not in range	
  if (fParOutOfRange) return;

  banner(Form("PixTestDaq::doDaqRun() start.") );

  //Set the ClockStretch
  fApi->setClockStretch(0, 0, fParStretch); //Stretch after trigger, 0 delay   //debug - needed?
   
  //Set the histograms:
  if(fHistList.size() == 0) bookHist("daq");  //to book histo only for the first 'doTest' (or after Clear).

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
  
  //To print summary of the number of masked pixels per ROC:
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  LOG(logINFO) << "PixTestDaq:: Number of masked pixels:";
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
	  LOG(logINFO) << "PixTestDaq::    " << fApi->_dut->getNMaskedPixels(static_cast<int>(iroc)) << " - ROC " << static_cast<int>(iroc);
  }  

  // Start the DAQ:
  //::::::::::::::::::::::::::::::::

  int totalPeriod = prepareDaq(fParTriggerFrequency, 50); //new common function defined in PixTest.cc

  //Set the pattern wrt the trigger frequency:
  LOG(logINFO) << "PG set to have trigger frequency = " << fParTriggerFrequency << " kHz";
  
  if (fParDelayTBM) {
	  LOG(logINFO) << "set TBM register delays = 0x40";
	  fApi->setTbmReg("delays", 0x40);
  }

  //Start the DAQ:
  fDaq_loop = true;
  fApi->daqStart();

  //Using number of triggers
  if (fRunDaqTrigger) {
	LOG(logINFO) << "Sending " << fParNtrig << " triggers for " << fParIter << " iterations.";
	for (int i = 0; i < fParIter && fDaq_loop; i++) {		//Send fParNtrig for fParIter
		fApi->daqTrigger(fParNtrig, totalPeriod);
		gSystem->ProcessEvents();
		ProcessData(0);
	}
	fApi->daqStop();

  } else {  //Use seconds

	//Start trigger loop + buffer fill management:
	int finalPeriod = fApi->daqTriggerLoop(totalPeriod);
	LOG(logINFO) << "PixTestDaq:: start TriggerLoop with period " << finalPeriod << " and duration " << fParSeconds << " seconds";
	
	  //To control the buffer filling
	uint8_t perFull;
	uint64_t diff = 0, timepaused = 0, timeff = 0;
	timer t;
	bool TotalTime = false;

	while (fDaq_loop){    //Check every n seconds if buffer is full less then 80%
	  while (fApi->daqStatus(perFull) && perFull < 80 && fDaq_loop) {     //Pause and drain the buffer if almost full.
		  timeff = t.get() - timepaused;
		  if (timeff / 1000 >= fParSeconds) {
			  fDaq_loop = false;
			  TotalTime = true;
			  break;
		  }
		  LOG(logDEBUG) << "buffer not full, at " << (int)perFull << "%";
		  gSystem->ProcessEvents();
	  }
	  if (fDaq_loop){
		  LOG(logINFO) << "Elapsed time: " << timeff / 1000 << " seconds.";
		  LOG(logINFO) << "Buffer almost full, pausing triggers.";
		  fApi->daqTriggerLoopHalt();
		  diff = t.get();
		  ProcessData(0);
		  diff = t.get() - diff;
		  timepaused += diff;
		  LOG(logDEBUG) << "Readout time: " << timepaused / 1000 << " seconds.";
		  LOG(logINFO) << "Resuming triggers for " << fParSeconds - (timeff/1000) << " seconds.";
		  fApi->daqTriggerLoop(finalPeriod);
	  }
	  else {
		  if (TotalTime) { LOG(logINFO) << "PixTestDaq:: total time reached - DAQ stopped."; }
		  fApi->daqStop();
		  ProcessData(0);
	  }
	}
  }
  //::::::::::::::::::::::::::::::
  //DAQ - THE END.

  //to draw and save histograms  //debug - needed all?
  TH1D *h1(0);
  TH2D *h2(0);
  TProfile2D *p2(0);
  copy(fQ.begin(), fQ.end(), back_inserter(fHistList));
  copy(fQmap.begin(), fQmap.end(), back_inserter(fHistList));
  copy(fPh.begin(), fPh.end(), back_inserter(fHistList));
  copy(fPhmap.begin(), fPhmap.end(), back_inserter(fHistList));
  copy(fHitMap.begin(), fHitMap.end(), back_inserter(fHistList));
  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
   	(*il)->Draw((getHistOption(*il)).c_str()); 
  }
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), p2);
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), p2);
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);
  PixTest::update();

  finalCleanup();
  fApi->setClockStretch(0, 0, 0); //No Stretch after trigger, 0 delay
  LOG(logINFO) << "PixTestDaq::doDaqRun() done";
}

// ----------------------------------------------------------------------
void PixTestDaq::doTest() {
	LOG(logINFO) << "PixTestDaq::doTest() start.";
	fRunDaqTrigger = false;   //use daqTriggerLoop() as default for doTest
	doDaqRun();
	LOG(logINFO) << "PixTestDaq::doTest() done";
}
