#include <iostream>
#include "PixTestGainCalibration.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestGainCalibration)

//----------------------------------------------------------
PixTestGainCalibration::PixTestGainCalibration(PixSetup *a, std::string name): PixTest(a, name) {
  LOG(logINFO) << "PixTestGainCalibration ctor(PixSetup &, string)";
  init(); 
}

//----------------------------------------------------------
PixTestGainCalibration::PixTestGainCalibration(): PixTest() {
  LOG(logINFO) << "PixTestGainCalibration ctor()";
}

//----------------------------------------------------------
PixTestGainCalibration::~PixTestGainCalibration() {
  LOG(logINFO) << "PixTestGainCalibration dtor()";
}

//----------------------------------------------------------
void PixTestGainCalibration::init() {
  LOG(logINFO) << "PixTestGainCalibration::init()";
  
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 
}


// ----------------------------------------------------------------------
void PixTestGainCalibration::setToolTips() {
  fTestTip    = string("measure and fit pulse height vs VCAL\n") + string("TO BE FINISHED!!"); 
  fSummaryTip = string("summary plot to be implemented")
    ;
}

// ----------------------------------------------------------------------
bool PixTestGainCalibration::setParameter(string parName, string sval) {
  bool found(false);

  return found;
}

// ----------------------------------------------------------------------
void PixTestGainCalibration::doTest() {
  LOG(logINFO)<< "PixTestGainCalibration::doTest()";
}
