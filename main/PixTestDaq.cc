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
  string stripParName; 
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      
      LOG(logDEBUG) << "  ==> parName: " << parName;
      LOG(logDEBUG) << "  ==> sval:    " << sval;
      if (!parName.compare("Ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
	setToolTips();
      }
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


  // All on!
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(false);

  // Set some pixels up for getting calibrate signals:
  for (int i = 0; i < 3; ++i) {
    fApi->_dut->testPixel(i, 5, true);
    fApi->_dut->testPixel(i, 6, true);
  }

    
  // Start the DAQ:
  fApi->daqStart(fPixSetup->getConfigParameters()->getTbPgSettings());
  
  // Send the triggers:
  fApi->daqTrigger(fParNtrig);
   
  // Stop the DAQ:
  fApi->daqStop();

  // And read out the full buffer:
  vector<uint16_t> daqdat = fApi->daqGetBuffer();

  cout << "Size of data read from board: " << daqdat.size() << endl;
  // Count triggers:
  uint32_t daq_check_triggers(0);
  int lcnt(0); 
  for(vector<uint16_t>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
    cout << Form("%4X ", (*it));
    ++lcnt;
    if(((*it) & 0xF000) > 0x4000 || (*it) == 0x0080) {
      ++daq_check_triggers;
      cout << endl;
    }
  }
  cout << endl;
  
  cout << "Found " << daq_check_triggers << " triggers in data, expected " << fParNtrig << endl;


  LOG(logINFO) << "PixTestDaq::doTest() done";
}
