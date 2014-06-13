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
PixTestDaq::PixTestDaq(PixSetup *a, std::string name) : PixTest(a, name), fParNtrig(-1) {
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

void PixTestDaq::stop()
{
  // Interrupt the test 
  fDaq_loop = false;
 
  LOG(logINFO)<< "Stop pressed. Ending test.";

}
bool PixTestDaq::setTrgFrequency(uint8_t TrgTkDel)
{
	uint8_t trgtkdel = TrgTkDel;

	double period_ns = 1 / (double)fParTriggerFrequency * 1000000; // trigger frequency in kHz.
	double fClkDelays = period_ns / 25 - trgtkdel;
	uint16_t ClkDelays = (uint16_t)fClkDelays;  //debug -- aprox to def

	//add right delay between triggers:
	uint16_t i = ClkDelays;

	if(fParResetROC) {
	
	   fPg_setup.push_back(make_pair("resetroc",15));
	   ClkDelays -= 15;			

	}

	while (i>255){
		//cout << i << endl; //debug
		fPg_setup.push_back(make_pair("delay", 255));
		i = i - 255;
	}
	fPg_setup.push_back(make_pair("delay", i));
	//cout << i << endl; //debug

	//then send trigger and token:
	fPg_setup.push_back(make_pair("trg", trgtkdel));	// PG_TRG  b000010
	fPg_setup.push_back(make_pair("tok", 0));	// PG_TOK  		

	return true;
}

// ----------------------------------------------------------------------
bool PixTestDaq::setParameter(string parName, string sval) {
  bool found(false);
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
	fParDelayTBM = !(atoi(sval.c_str())==0);
	setToolTips();
      }
      if (!parName.compare("filltree")) {
	fParFillTree = !(atoi(sval.c_str())==0);
	setToolTips();
      }
      if (!parName.compare("trgfrequency(khz)")){   // trigger frequency in kHz.
	 fParTriggerFrequency = atoi(sval.c_str());
	 LOG(logDEBUG) << "  setting fParTriggerFrequency -> " << fParTriggerFrequency;
      }
      if (!parName.compare("seconds")){
		fParSeconds = atoi(sval.c_str());
		LOG(logDEBUG) << "  setting Seconds -> " << fParSeconds;
		if (fParSeconds < 0) {
				  LOG(logWARNING) << "PixTestDaq::setParameter() seconds must be positive";
		}
      }
    }
  }
  return found;
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

  fTestTip    = string("run DAQ") ;
  fSummaryTip = string("Show summary plot") ;
  fStopTip    = string("Stop DAQ");
}


// ----------------------------------------------------------------------
void PixTestDaq::bookHist(string name) {
  fDirectory->cd(); 
  LOG(logDEBUG) << "nothing done with " << name; 
}


//----------------------------------------------------------
PixTestDaq::~PixTestDaq() {
  LOG(logDEBUG) << "PixTestDaq dtor, saving tree ... ";
  fDirectory->cd();
  if (fTree && fParFillTree) fTree->Write(); 
}

// ----------------------------------------------------------------------
void PixTestDaq::pgToDefault(vector<pair<std::string, uint8_t> > /*pg_setup*/) {
  fPg_setup.clear();
  LOG(logDEBUG) << "PixTestPattern::PG_Setup clean";
  
  fPg_setup = fPixSetup->getConfigParameters()->getTbPgSettings();
  fApi->setPatternGenerator(fPg_setup);
  LOG(logINFO) << "PixTestPattern::       pg_setup set to default.";
}

// ----------------------------------------------------------------------
void PixTestDaq::doTest() {

  LOG(logINFO) << "PixTestDaq::doTest() start with fParNtrig = " << fParNtrig;

  PixTest::update(); 
  fDirectory->cd();

  fPg_setup.clear();

  //Set the ClockStretch
  fApi->setClockStretch(0, 0, fParStretch); // Stretch after trigger, 0 delay
   

  // Load pixel mask
  

  if (fParFillTree) bookTree(); 

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  TH1D *h1(0); 
  TH2D *h2(0); 
  TProfile2D *p2(0); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    h2 = bookTH2D(Form("hits_C%d", rocIds[iroc]), Form("hits_C%d", rocIds[iroc]), 52, 0., 52., 80, 0., 80.);
    h2->SetMinimum(0.);
    h2->SetDirectory(fDirectory);
    setTitles(h2, "col", "row");
    fHistOptions.insert(make_pair(h2,"colz"));
    fHits.push_back(h2);

    p2 = bookTProfile2D(Form("phMap_C%d", rocIds[iroc]), Form("phMap_C%d", rocIds[iroc]), 52, 0., 52., 80, 0., 80.);
    p2->SetMinimum(0.);
    p2->SetDirectory(fDirectory);
    setTitles(p2, "col", "row");
    fHistOptions.insert(make_pair(p2,"colz"));
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
    fHistOptions.insert(make_pair(p2,"colz"));
    fQmap.push_back(p2);

    h1 = bookTH1D(Form("q_C%d", rocIds[iroc]), Form("q_C%d", rocIds[iroc]), 200, 0., 1000.);
    h1->SetMinimum(0.);
    h1->SetDirectory(fDirectory);
    setTitles(h1, "Q [Vcal]", "Entries/bin");
    fQ.push_back(h1);


  }

  copy(fHits.begin(), fHits.end(), back_inserter(fHistList));
  copy(fPhmap.begin(), fPhmap.end(), back_inserter(fHistList));
  copy(fPh.begin(), fPh.end(), back_inserter(fHistList));
  copy(fQmap.begin(), fQmap.end(), back_inserter(fHistList));
  copy(fQ.begin(), fQ.end(), back_inserter(fHistList));

  //first send only a RES:
  fPg_setup.push_back(make_pair("resetroc", 0));     // PG_RESR b001000 
  uint16_t period = 28;

  // Set the pattern generator:
  fApi->setPatternGenerator(fPg_setup);

  fApi->daqStart();

  //send only one trigger to reset:
  fApi->daqTrigger(1, period);
  LOG(logINFO) << "PixTestDaq::RES sent once ";

  fApi->daqStop();
  //fApi->daqClear();

  fPg_setup.clear();
  LOG(logINFO) << "PixTestDaq::PG_Setup clean";

  if(fParDelayTBM)
  	fApi->setTbmReg("delays",0x40); //FPix timing

//set the pattern wrt the trigger frequency:
  LOG(logINFO) << "PG set to have trigger frequency = " << fParTriggerFrequency << " kHz";
  if (!setTrgFrequency(20)){
	  FinalCleaning();
	  return;
  }

  //set pattern generator:
  fApi->setPatternGenerator(fPg_setup);

   //If using number of triggers
  if(fParNtrig > 0) {
	
	// Start the DAQ:
	fApi->daqStart();

	for( int i = 0 ; i < fParIter ; i++) {
  	    		    
    	      // Send the triggers:
    	      fApi->daqTrigger(fParNtrig);
   	
	      ProcessData(0);	
	      gSystem->ProcessEvents();	
	}
	fApi->daqStop();

  } else {  // Use seconds

  //start trigger loop + buffer fill management:
  fDaq_loop = true;

  fApi->daqStart();

  int finalPeriod = fApi->daqTriggerLoop(0);  //period is automatically set to the minimum by Api function
  LOG(logINFO) << "PixTestDaq:: start TriggerLoop with period " << finalPeriod << " and duration " << fParSeconds << " seconds";

  //to control the buffer filling
  uint8_t perFull;
  timer t;
  while (fDaq_loop)    //check every 2 seconds if buffer is full less then 80%
  {
	  // Pause and drain the buffer if almost full.
	  while (fApi->daqStatus(perFull) && perFull < 80) {
		  
		  LOG(logINFO) << "Elapsed time: " << t.get() / 1000 << " seconds.";
		  if (t.get() / 1000 >= fParSeconds)       {
			  fDaq_loop = false;
			  break;
		  }

		  LOG(logINFO) << "buffer not full, at " << (int)perFull << "%";
		  gSystem->ProcessEvents();
		  ProcessData();
	  }

	  if (fDaq_loop){
		  LOG(logINFO) << "Buffer almost full, pausing triggers.";
		  fApi->daqTriggerLoopHalt();
		  ProcessData(0);
		  LOG(logINFO) << "Resuming triggers.";
		  fApi->daqTriggerLoop(0);
	  }

	  else {
	  LOG(logINFO) << "PixTestDaq:: total time reached - DAQ stopped.";
	  fApi->daqStop();
	  ProcessData(0);
	  }
  }

  }

  FinalCleaning();

  fApi->setClockStretch(0, 0, 0); // No Stretch after trigger, 0 delay
  
  LOG(logDEBUG) << "Filled histograms..." ;

  h2 = (TH2D*)(*fHistList.begin());

  h2->Draw("colz");
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);

  PixTest::update();

  LOG(logINFO) << "PixTestDaq::doTest() done";
}

void PixTestDaq::ProcessData(uint16_t numevents)
{

    int pixCnt(0);

    LOG(logDEBUG) << "Getting Event Buffer";

    vector<pxar::Event> daqdat;
    
    if( numevents > 0) {
	for (unsigned int i = 0; i < numevents ; i++) {
		pxar::Event evt = fApi->daqGetEvent();
		//Check if event is empty?
		if(evt.pixels.size() > 0)
			daqdat.push_back(evt);
	}
    }
    else
	daqdat = fApi->daqGetEventBuffer();

    LOG(logDEBUG) << "Processing Data: " << daqdat.size() << " events.";
    
    int idx(-1); 
    uint16_t q; 
    for(std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
      pixCnt += it->pixels.size(); 
      //      LOG(logDEBUG) << (*it);

      if (fParFillTree) {
	fTreeEvent.header           = it->header; 
	fTreeEvent.dac              = 0;
	fTreeEvent.trailer          = it->trailer; 
	fTreeEvent.numDecoderErrors = it->numDecoderErrors;
	fTreeEvent.npix = it->pixels.size();
      }

      for (unsigned int ipix = 0; ipix < it->pixels.size(); ++ipix) {   
	idx = getIdxFromId(it->pixels[ipix].roc_id);
	fHits[idx]->Fill(it->pixels[ipix].column, it->pixels[ipix].row);
	fPhmap[idx]->Fill(it->pixels[ipix].column, 
			  it->pixels[ipix].row, 
			  it->pixels[ipix].value);
	
	fPh[idx]->Fill(it->pixels[ipix].value);

	if (fPhCalOK) {
	  q = static_cast<uint16_t>(fPhCal.vcal(it->pixels[ipix].roc_id, 
						it->pixels[ipix].column, 
						it->pixels[ipix].row, 
						it->pixels[ipix].value));
	} else {
	  q = 0;
	}
	fQ[idx]->Fill(q);
	fQmap[idx]->Fill(it->pixels[ipix].column, it->pixels[ipix].row, q);
	
	
	if (fParFillTree) {
	  fTreeEvent.proc[ipix] = it->pixels[ipix].roc_id; 
	  fTreeEvent.pcol[ipix] = it->pixels[ipix].column; 
	  fTreeEvent.prow[ipix] = it->pixels[ipix].row; 
	  fTreeEvent.pval[ipix] = it->pixels[ipix].value; 
	  fTreeEvent.pq[ipix]   = q;
	}
      }

      if (fParFillTree) fTree->Fill();
    }
    
    cout << Form("Run ") << Form(" # events read: %6ld, pixels seen in all events: %3d, hist entries: %4d", 
					  daqdat.size(), pixCnt, 
					  static_cast<int>(fHits[0]->GetEntries())) 
	 << endl;
    
    fHits[0]->Draw("colz");
    PixTest::update();
  

}

void PixTestDaq::FinalCleaning() {
	// Reset the pg_setup to default value.
	pgToDefault(fPg_setup);

	//clean local variables:
	//fPIX.clear();
	//fPIXm.clear();
	fPg_setup.clear();
}
void PixTestDaq::runCommand(std::string command) {

	if(command == "stop")
		stop();
	else
	   LOG(logINFO) << "Command " << command << " not implemented.";
}


