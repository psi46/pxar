#include <iostream>
#include "PixTestGainCalibration.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestGainCalibration)

//----------------------------------------------------------
PixTestGainCalibration::PixTestGainCalibration(PixSetup *a, std::string name): PixTest(a, name) {
  LOG(logDEBUG) << "PixTestGainCalibration ctor(PixSetup &, string)";
  init(); 
}

//----------------------------------------------------------
PixTestGainCalibration::PixTestGainCalibration(): PixTest() {
  LOG(logDEBUG) << "PixTestGainCalibration ctor()";
}

//----------------------------------------------------------
PixTestGainCalibration::~PixTestGainCalibration() {
  LOG(logDEBUG) << "PixTestGainCalibration dtor()";
}

//----------------------------------------------------------
void PixTestGainCalibration::init() {
  LOG(logDEBUG) << "PixTestGainCalibration::init()";
  
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
  LOG(logDEBUG) << "nothing done with " << parName << " and " << sval;

  return found;
}

// ----------------------------------------------------------------------
void PixTestGainCalibration::doTest() {
  LOG(logINFO)<< "PixTestGainCalibration::doTest()";
}
