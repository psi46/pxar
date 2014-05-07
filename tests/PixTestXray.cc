#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include "PixTestXray.hh"
#include "log.h"

#include <TH2.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestXray)

// ----------------------------------------------------------------------
PixTestXray::PixTestXray(PixSetup *a, std::string name) : PixTest(a, name), fParNtrig(1000), fParStretch(0) {
  PixTest::init();
  init(); 
  LOG(logDEBUG) << "PixTestXray ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestXray::PixTestXray() : PixTest() {
  LOG(logDEBUG) << "PixTestXray ctor()";
}

// ----------------------------------------------------------------------
bool PixTestXray::setParameter(string parName, string sval) {
  bool found(false);
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      if (!parName.compare("ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("clockstretch"))
 	fParStretch = atoi(sval.c_str());	
      break;
    }
  }
  return found; 
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
}


// ----------------------------------------------------------------------
void PixTestXray::bookHist(string /*name*/) {
  fDirectory->cd(); 
}


//----------------------------------------------------------
PixTestXray::~PixTestXray() {
  LOG(logDEBUG) << "PixTestXray dtor";
}


// ----------------------------------------------------------------------
void PixTestXray::doTest() {
  
  LOG(logINFO) << "PixTestXray::doTest() start with fParNtrig = " << fParNtrig;

  cacheDacs(); 
  PixTest::update(); 
  fDirectory->cd();
  
  // -- Set the ClockStretch ?????????????????????
  // fApi->setClockStretch(0, 0, fParStretch); // Stretch after trigger, 0 delay
  
  
  // All on!
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(false);
  
  string name("xray-hits");
  TH1D *h1(0);
  vector<TH1D*> hits;
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    h1 = bookTH1D(Form("%s_C%d", name.c_str(), iroc), Form("%s_C%d", name.c_str(), rocIds[iroc]), 256, 0., 256.);
    h1->SetMinimum(0.);
    h1->SetDirectory(fDirectory);
    setTitles(h1, "VthrComp", "Hits");
    hits.push_back(h1);
  }
  

  // -- set 
  fApi->setDAC("vthrcomp", 60); // ???????????

  // -- check for noisy pixels
  //   fApi->daqStart(fPixSetup->getConfigParameters()->getTbPgSettings());
  //   fApi->daqTrigger(100);
  //   fApi->daqStop();
  //   vector<pxar::Event> daqdat = fApi->daqGetEventBuffer();
  //   LOG(logDEBUG) << "Number of events read from board: " << daqdat.size();
  map<string, int> hitMap; 
  string pname; 
  for (int ithr = 0; ithr < 150; ++ithr) {
    fApi->setDAC("vthrcomp", ithr); 
    fApi->daqStart(fPixSetup->getConfigParameters()->getTbPgSettings());
    fApi->daqTrigger(100);
    vector<Event> daqdat = fApi->daqGetEventBuffer();
    LOG(logDEBUG) << "VthrComp = " << ithr << " with daqdat.size() = " << daqdat.size(); 
    for (vector<Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
      if ((*it).pixels.size() > 0) LOG(logDEBUG) << (*it);
      //     for(vector<pixel>::iterator pixit = it->pixels.begin(); pixit != it->pixels.end() ; pixit++) {   
      //       pname = Form("c%d_r%d_C%d", pixit->column, pixit->row, pixit->roc_id); 
      //       hitMap[pname]++;
      //     }
    }
    fApi->daqStop();
  }

  LOG(logDEBUG) << "Pixels readout: " << hitMap.size(); 
  for (map<string, int>::iterator it = hitMap.begin(); it != hitMap.end(); ++it) {
    cout << it->first << ": nhits = " << it->second << endl;
  }
  return;

  // -- mask the noisy pixels
  // FIXME

  // -- scan VthrComp
  map<int, bool> rocDone;
  for (int ithr = 0; ithr < 150; ++ithr) {
    
    LOG(logDEBUG) << "VthrComp = " << ithr; 
    fApi->setDAC("vthrcomp", ithr); 
    fApi->daqStart(fPixSetup->getConfigParameters()->getTbPgSettings());
    fApi->daqTrigger(fParNtrig);
    fApi->daqStop();
    
    vector<pxar::Event> daqdat = fApi->daqGetEventBuffer();
    
    cout << "  number of events read from board: " << daqdat.size() << endl;
    for (vector<Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
      LOG(logDEBUG) << (*it) << std::endl;
      
      for(vector<pixel>::iterator pixit = it->pixels.begin(); pixit != it->pixels.end() ; pixit++) {   
	hits[getIdxFromId(pixit->roc_id)]->Fill(ithr);
      }
    }
  


  
  }
  

  


  PixTest::update();
  restoreDacs(); 

  LOG(logINFO) << "PixTestXray::doTest() done";
}
