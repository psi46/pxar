#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include "PixTestDaq.hh"
#include "log.h"
#include "helper.h"
#include "timer.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestDaq)

// ----------------------------------------------------------------------
PixTestDaq::PixTestDaq(PixSetup *a, std::string name) : PixTest(a, name), fParNtrig(0), fParStretch(0), fParFillTree(0), fParSeconds(0), fParTriggerFrequency(0), fParIter(0), fParDelayTBM(0), fParResetROC(0) {
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
	LOG(logDEBUG) << "PixTestDaq dtor, saving tree ... ";
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
  fTestTip    = string("Run DAQ - data from each run will be added to the same histogram.") ;
  fSummaryTip = string("to be implemented") ;
  fStopTip    = string("Stop DAQ and save data.");
}

// ----------------------------------------------------------------------
void PixTestDaq::bookHist(string name) {
	fDirectory->cd();
	LOG(logDEBUG) << "nothing done with " << name;
}

// ----------------------------------------------------------------------
void PixTestDaq::stop(){
	// Interrupt the test 
	fDaq_loop = false;
	LOG(logINFO) << "Stop pressed. Ending test.";
}

// ----------------------------------------------------------------------
void PixTestDaq::runCommand(std::string command) {

	if (command == "stop")
		stop();
	else
		LOG(logINFO) << "Command " << command << " not implemented.";
}

// ----------------------------------------------------------------------
bool PixTestDaq::setParameter(string parName, string sval) {
	bool found(false);
	singPixEffTest = true;
	fSPixRow = 25;
	fSPixCol = 40;
	fSPixRoc = 10;
	fParOutOfRange = false;
	std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
	for (unsigned int i = 0; i < fParameters.size(); ++i) {
		if (fParameters[i].first == parName) {
			found = true;
			if (!parName.compare("ntrig")) {
				fParNtrig = atoi(sval.c_str());
				setToolTips();
			}
			if (!parName.compare("iterations")) {
				fParIter = atoi(sval.c_str());
				setToolTips();
			}
			if (!parName.compare("clockstretch")) {
				fParStretch = atoi(sval.c_str());
				setToolTips();
			}
			if (!parName.compare("delaytbm")) {
				fParDelayTBM = !(atoi(sval.c_str()) == 0);
				setToolTips();
			}
			if (!parName.compare("filltree")) {
				fParFillTree = !(atoi(sval.c_str()) == 0);
				setToolTips();
			}
			if (!parName.compare("trgfrequency(khz)")){   // trigger frequency in kHz.
				fParTriggerFrequency = atoi(sval.c_str());
				LOG(logDEBUG) << "  setting fParTriggerFrequency -> " << fParTriggerFrequency;
				if (fParTriggerFrequency == 0) {
					LOG(logWARNING) << "PixTestDaq::setParameter() trgfrequency must be different from zero";
					found = false; fParOutOfRange = true;
				}
			}
			if (!parName.compare("seconds")){
				fParSeconds = atoi(sval.c_str());
				LOG(logDEBUG) << "  setting Seconds -> " << fParSeconds;
			}
		}
	}
	return found;
}

//----------------------------------------------------------
bool PixTestDaq::setTrgFrequency(uint8_t TrgTkDel){

	uint8_t trgtkdel = TrgTkDel;
	double period_ns = 1 / (double)fParTriggerFrequency * 1000000; // trigger frequency in kHz.
	double fClkDelays = period_ns / 25 - trgtkdel;
	uint16_t ClkDelays = (uint16_t)fClkDelays;  //debug -- aprox to def
	uint16_t i = ClkDelays;

	//add right delay between triggers:
	if (fParResetROC) {
		fPg_setup.push_back(make_pair("resetroc", 15));
		ClkDelays -= 15;
	}
	while (i>255){
		fPg_setup.push_back(make_pair("delay", 255));
		i = i - 255;
	}
	fPg_setup.push_back(make_pair("delay", i));

	//then send trigger and token:
	fPg_setup.push_back(make_pair("trg", trgtkdel));
	fPg_setup.push_back(make_pair("tok", 0));

	return true;
}

// ----------------------------------------------------------------------
void PixTestDaq::pgToDefault() {
  fPg_setup.clear();
  LOG(logDEBUG) << "PixTestPattern::PG_Setup clean";
  
  fPg_setup = fPixSetup->getConfigParameters()->getTbPgSettings();
  fApi->setPatternGenerator(fPg_setup);
  LOG(logINFO) << "PixTestPattern::       pg_setup set to default.";
}

// ----------------------------------------------------------------------
void PixTestDaq::setHistos(){
	
	if (fParFillTree) bookTree();
	fHits.clear(); fPhmap.clear(); fPh.clear(); fQmap.clear(); fQ.clear();

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
		fHits.push_back(h2);

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
			//fTriggerCount++;
			//Check if event is empty?
			if (evt.pixels.size() > 0)
				daqdat.push_back(evt);
		}
	}
	else{
		daqdat = fApi->daqGetEventBuffer();
		if( daqdat.size() == 0 ) return;
		//fTriggerCount += daqdat.size();
	}
	LOG(logDEBUG) << "Processing Data: " << daqdat.size() << " events.";

	int pixCnt(0);
	int idx(-1);
	uint16_t q;
	int entries = 0;
	vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
	for (std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
		pixCnt += it->pixels.size();
		fTriggerCount++;
		if (fParFillTree) {
			fTreeEvent.header = it->header;
			fTreeEvent.dac = 0;
			fTreeEvent.trailer = it->trailer;
			fTreeEvent.numDecoderErrors = it->numDecoderErrors;
			fTreeEvent.npix = it->pixels.size();
		}

		for (unsigned int ipix = 0; ipix < it->pixels.size(); ++ipix) {
			idx = getIdxFromId(it->pixels[ipix].roc_id);
			if(idx == -1) {
			  LOG(logWARNING) << "PixTestDaq::ProcessData() wrong 'idx' value";
			} else {
			fHits[idx]->Fill(it->pixels[ipix].column, it->pixels[ipix].row);
			fPhmap[idx]->Fill(it->pixels[ipix].column, it->pixels[ipix].row, it->pixels[ipix].getValue());
			fPh[idx]->Fill(it->pixels[ipix].getValue());
			
			entries++;
			
			int myrow = static_cast<int>( it->pixels[ipix].row );
			int mycol = static_cast<int>( it->pixels[ipix].column );	
			int myroc = static_cast<int>( it->pixels[ipix].roc_id );
			//LOG( logDEBUG ) << "ROW: " << myrow << " COL: " << mycol << " ROC: " << myroc;
			//LOG( logDEBUG ) << "ROW: " << fSPixRow << " COL: " << fSPixCol << " ROC: " << fSPixRoc;
			if( myroc == fSPixRoc ) if( myrow == fSPixRow ) if( mycol == fSPixCol ) fSPixCount = fSPixCount + 1;
			
			if (fPhCalOK) {
				q = static_cast<uint16_t>(fPhCal.vcal(it->pixels[ipix].roc_id, it->pixels[ipix].column,	
								      it->pixels[ipix].row, it->pixels[ipix].getValue()));
			}
			else {
				q = 0;
			}
			fQ[idx]->Fill(q);
			fQmap[idx]->Fill(it->pixels[ipix].column, it->pixels[ipix].row, q);
				if (fParFillTree) {
				fTreeEvent.proc[ipix] = it->pixels[ipix].roc_id;
				fTreeEvent.pcol[ipix] = it->pixels[ipix].column;
				fTreeEvent.prow[ipix] = it->pixels[ipix].row;
				fTreeEvent.pval[ipix] = it->pixels[ipix].getValue();
				fTreeEvent.pq[ipix] = q;
			}
			}
		}
		if (fParFillTree) fTree->Fill();
	}

  	//to draw the hitsmap as 'online' check.
	TH2D* h2 = (TH2D*)(fHits.back());
	h2->Draw(getHistOption(h2).c_str());
	fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);
	PixTest::update();

	//entries = static_cast<int>( fHits[0]->GetEntries() );
	LOG(logINFO) << Form("events read: %6ld, pixels seen: %3d, hist entries: %4d",
	        daqdat.size(), pixCnt, entries);         
		//daqdat.size(), pixCnt,	static_cast<int>(fHits[0]->GetEntries()));	
}

// ----------------------------------------------------------------------
void PixTestDaq::FinalCleaning() {

	// Reset the pg_setup to default value.
	pgToDefault();
	//clean local variables:
	fPg_setup.clear();
}

// ----------------------------------------------------------------------
void PixTestDaq::doTest() {

  PixTest::update();
  fDirectory->cd();
  fPg_setup.clear();
  fTriggerCount = 0;
  fSPixCount = 0;

  //Immediately stop if parameters not in range	
  if (fParOutOfRange) return;
  
  LOG(logINFO) << "PixTestDaq::doTest() start.";

  //Set the ClockStretch
  fApi->setClockStretch(0, 0, fParStretch); //Stretch after trigger, 0 delay
   
  //Set the histograms:
  if(fHistList.size() == 0) setHistos();  //to book histo only for the first 'doTest' (or after Clear).


  fApi->_dut->testAllPixels( false );
  if( singPixEffTest ) {
  	fApi->_dut->maskPixel( fSPixCol, fSPixRow, false, fSPixRoc );
  	fApi->_dut->testPixel( fSPixCol, fSPixRow, true, fSPixRoc );
  }

  //To print on shell the number of masked pixels per ROC:
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  LOG(logINFO) << "PixTestDaq::Number of masked pixels:";
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
	  LOG(logINFO) << "PixTestDaq::    ROC " << static_cast<int>(iroc) << ": " << fApi->_dut->getNMaskedPixels(static_cast<int>(iroc));
  }  

// Start the DAQ:
//::::::::::::::::::::::::::::::::

//First send only a RES:
  fPg_setup.push_back(make_pair("resetroc", 0));     // PG_RESR b001000 
  uint16_t period = 28;

//Set the pattern generator:
  if( ! singPixEffTest ) { fApi->setPatternGenerator(fPg_setup);} else { 
	pgToDefault();
  	LOG( logINFO ) << "Pattern Generator set to default KUtest";
  }
  fApi->daqStart();

//Send only one trigger to reset:
  fApi->daqTrigger(1, period);
  LOG(logINFO) << "PixTestDaq::RES sent once ";

  fApi->daqStop();
  //fApi->daqClear();

  fPg_setup.clear();
  LOG(logINFO) << "PixTestDaq::PG_Setup clean";

  if(fParDelayTBM)
  	fApi->setTbmReg("delays",0x40); //FPix timing

//Set the pattern wrt the trigger frequency:
//  LOG(logINFO) << "PG set to have trigger frequency = " << fParTriggerFrequency << " kHz";
  if (!setTrgFrequency(20)){
	  FinalCleaning();
	  return;
  }

//Set pattern generator:
  if( ! singPixEffTest ){ fApi->setPatternGenerator(fPg_setup);} else { 
  	pgToDefault();
  	LOG(logINFO) << " Pattern generator set to default KUtest";
  }
//Start the DAQ:
  fApi->daqStart();
//If using number of triggers
  if(fParNtrig > 0) {
	uint32_t fLoopCount = 0;
      	int fPgPeriod = 250;
	uint8_t perFull = 0;
	LOG(logINFO) << "PixTestDaq:: start TriggerLoop with period " << fPgPeriod << " and duration " << fParIter*fParNtrig << " events";
	double t = static_cast<double>(fPgPeriod * fParIter * fParNtrig ) / 40000000;
	LOG(logINFO) << "And an approxamate run time of : " << t  << " seconds.";
	for ( int i = 0; i < fParIter; i++) {
			LOG(logINFO) << "Sending " << fParNtrig << " triggers.";
		        fApi->daqTrigger(fParNtrig, fPgPeriod);
		//check buffer status
			fApi->daqStatus(perFull);
		//process events
			if( perFull > 50 ){
				LOG(logINFO) << "Buffer at : " << (int)perFull << "%, Processing Data.";
				ProcessData(0);
				fApi->daqStatus(perFull);
			}
                LOG(logINFO) << "Buffer at : " << (int)perFull << "%";
		fLoopCount += fParNtrig;
		LOG(logINFO) << fLoopCount << " of " << fParNtrig* fParIter << " triggers sent.";
		LOG(logINFO) << fTriggerCount << " of " << fParNtrig * fParIter << " events processed.";
	}
	fApi->daqStop();
	ProcessData(0);
        LOG(logINFO) << fLoopCount << " of " << fParNtrig * fParIter << " triggers sent.";
        LOG(logINFO) << fTriggerCount << " of " << fParNtrig * fParIter << " events processed.";
	
  } else {  //Use seconds

//Start trigger loop + buffer fill management:
	int fPgPeriod = fApi->daqTriggerLoop( 250 );  //Period is automatically set to the minimum by Api function -> which is too small use 250
	LOG(logINFO) << "PixTestDaq:: start TriggerLoop with period " << fPgPeriod << " and duration " << fParSeconds << " seconds";
        LOG(logINFO) << "For a total approximte Trigger Count of : " << (int) (fParSeconds * 40000000 )/ fPgPeriod;
//To control the buffer filling
	uint8_t perFull = 0;
	uint64_t diff = 0, timepaused = 0, timeff = 0;
	timer t;
	fDaq_loop = true;
	while (fDaq_loop){    //Check every n seconds if buffer is full less then 80%
		fApi->daqStatus( perFull );
		timeff = t.get() - timepaused;
		if (timeff / 1000 >= fParSeconds) {
			fDaq_loop = false;
			break;
		}
		if( timeff%1000 == 0 ) {
			LOG(logINFO) << "Elapsed time: " << timeff / 1000 << " seconds.";
		  	LOG(logINFO) << "buffer not full, at " << (int)perFull << "%";
		}
		if( perFull > 80 ){
			LOG(logINFO) << "Buffer almost full, pausing triggers.";
			fApi->daqTriggerLoopHalt();
			diff = t.get();
			ProcessData(0);
			diff = t.get() - diff;
			timepaused += diff;
			LOG(logDEBUG) << "Readout time: " << timepaused / 1000 << " seconds.";
			LOG(logINFO) << "Resuming triggers for " << fParSeconds - (timeff/1000) << " seconds.";
			fApi->daqTriggerLoop( 250 );
	   	}
	}
	LOG(logINFO) << "PixTestDaq:: total time reached - DAQ stopped.";// -b-
	fApi->daqTriggerLoopHalt();
	fApi->daqStop();
	ProcessData(0);
  }

  LOG( logINFO) << "Ending Daq Readout::";
  LOG( logINFO) << "Total Trigger = " << (int) fTriggerCount;
  if( singPixEffTest ) {
  	LOG( logINFO) << "Pixel Count = " << (int) fSPixCount;
  	LOG( logINFO) << "Efficency = " << static_cast<double>(fSPixCount)/fTriggerCount;
  }
  //std::vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  for (unsigned int i = 0 ; i < rocIds.size() ; i++){
  	LOG(logINFO) << Form( "Rate %d (MHz/cm^2): %.1f",i,fHits[i]->Integral(1,52)/static_cast<double>(fTriggerCount)/25./0.64*1000.);
  }

  //::::::::::::::::::::::::::::::
  //DAQ - THE END.

  //to draw and save histograms
  TH1D *h1(0);
  TH2D *h2(0);
  TProfile2D *p2(0);
  copy(fQ.begin(), fQ.end(), back_inserter(fHistList));
  copy(fQmap.begin(), fQmap.end(), back_inserter(fHistList));
  copy(fPh.begin(), fPh.end(), back_inserter(fHistList));
  copy(fPhmap.begin(), fPhmap.end(), back_inserter(fHistList));
  copy(fHits.begin(), fHits.end(), back_inserter(fHistList));
  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
   	(*il)->Draw((getHistOption(*il)).c_str()); 
  }
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), p2);
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), p2);
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);
  PixTest::update();

  FinalCleaning();
  fApi->setClockStretch(0, 0, 0); //No Stretch after trigger, 0 delay
  LOG(logINFO) << "PixTestDaq::doTest() is done";
}
