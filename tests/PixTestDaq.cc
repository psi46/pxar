#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include "PixTestDaq.hh"
#include "log.h"

#include <TH2.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestDaq)

// ----------------------------------------------------------------------
PixTestDaq::PixTestDaq(PixSetup *a, std::string name) : PixTest(a, name), fParNtrig(-1) {
  PixTest::init();
  init(); 
  LOG(logDEBUG) << "PixTestDaq ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestDaq::PixTestDaq() : PixTest() {
  LOG(logDEBUG) << "PixTestDaq ctor()";
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
      if (!parName.compare("clockstretch"))
 	fParStretch = atoi(sval.c_str());	
      if (!parName.compare("counthits"))
	fParCount = !(atoi(sval.c_str())==0);
      break;
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
  fTestTip    = string("run DAQ")
    ;
  fSummaryTip = string("to be implemented")
    ;
}


// ----------------------------------------------------------------------
void PixTestDaq::bookHist(string name) {
  fDirectory->cd(); 
  LOG(logDEBUG) << "nothing done with " << name; 
}


//----------------------------------------------------------
PixTestDaq::~PixTestDaq() {
  LOG(logDEBUG) << "PixTestDaq dtor";
}


// ----------------------------------------------------------------------
void PixTestDaq::doTest() {

  LOG(logINFO) << "PixTestDaq::doTest() start with fParNtrig = " << fParNtrig;

  PixTest::update(); 
  fDirectory->cd();

  //Set the ClockStretch

  fApi->setClockStretch(0, 0, fParStretch); // Stretch after trigger, 0 delay
   

  // All on!
  fApi->_dut->testAllPixels(false);
  //fApi->_dut->maskAllPixels(true);
  /*
  // Set some pixels up for getting calibrate signals:
  for (int i = 0; i < 3; ++i) {
  fApi->_dut->testPixel(i, 5, true);
  fApi->_dut->maskPixel(i, 5, false);
  fApi->_dut->testPixel(i, 6, true);
  fApi->_dut->maskPixel(i, 6, false);
  }
  */
  
  vector<TH2D*> hits;
  vector<TH2D*> phmap;
  vector<TH1D*> ph;
  TH1D *h1(0); 
  TH2D *h2(0); 

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    h2 = bookTH2D(Form("hits_C%d", rocIds[iroc]), Form("hits_C%d", rocIds[iroc]), 52, 0., 52., 80, 0., 80.);
    h2->SetMinimum(0.);
    h2->SetDirectory(fDirectory);
    setTitles(h2, "col", "row");
    hits.push_back(h2);

    h2 = bookTH2D(Form("phMap_C%d", rocIds[iroc]), Form("ph_C%d", rocIds[iroc]), 52, 0., 52., 80, 0., 80.);
    h2->SetMinimum(0.);
    h2->SetDirectory(fDirectory);
    setTitles(h2, "col", "row");
    phmap.push_back(h2);

    h1 = bookTH1D(Form("ph_C%d", rocIds[iroc]), Form("ph_C%d", rocIds[iroc]), 256, 0., 256.);
    h1->SetMinimum(0.);
    h1->SetDirectory(fDirectory);
    setTitles(h1, "ADC", "Entries/bin");
    ph.push_back(h1);
  }

  copy(hits.begin(), hits.end(), back_inserter(fHistList));
  copy(phmap.begin(), phmap.end(), back_inserter(fHistList));
  copy(ph.begin(), ph.end(), back_inserter(fHistList));

  for (int iter = 0; iter < fParIter; iter++) {
    int pixCnt(0); 
    // Start the DAQ:
    fApi->daqStart(fPixSetup->getConfigParameters()->getTbPgSettings());
    
    // Send the triggers:
    fApi->daqTrigger(fParNtrig);
    
    // Stop the DAQ:
    fApi->daqStop();
    
    vector<pxar::Event> daqdat = fApi->daqGetEventBuffer();
    
    for(std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
      pixCnt += it->pixels.size(); 
      //      LOG(logDEBUG) << (*it);
      for(vector<pixel>::iterator pixit = it->pixels.begin(); pixit != it->pixels.end(); pixit++) {   
	hits[getIdxFromId(pixit->roc_id)]->Fill(pixit->column,pixit->row);
	phmap[getIdxFromId(pixit->roc_id)]->SetBinContent(pixit->column,pixit->row,pixit->value);
	ph[getIdxFromId(pixit->roc_id)]->Fill(pixit->value);
      }
    }
    
    cout << Form("Run %4d", iter) << Form(" # events read: %4d, pixels seen in all events: %3d, hist entries: %4d", 
					  daqdat.size(), pixCnt, 
					  static_cast<int>(hits[0]->GetEntries())) 
	 << endl;
    
    hits[0]->Draw("colz");
    PixTest::update();
  
  }
  fApi->setClockStretch(0, 0, 0); // No Stretch after trigger, 0 delay
  
  
  LOG(logDEBUG) << "Filled histograms..." ;

  h2 = (TH2D*)(*fHistList.begin());

  h2->Draw("colz");
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);

  PixTest::update();
 

  LOG(logINFO) << "PixTestDaq::doTest() done";
}
