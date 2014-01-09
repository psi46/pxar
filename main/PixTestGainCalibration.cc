#include <iostream>
#include "PixTestGainCalibration.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestGainCalibration)

//----------------------------------------------------------
PixTestGainCalibration::PixTestGainCalibration(PixSetup *a, std::string name): PixTest(a, name) {
  LOG(logINFO) << "PixTestGainCalibration ctor(PixSetup &, string)";
}

//----------------------------------------------------------
PixTestGainCalibration::PixTestGainCalibration(): PixTest() {
  LOG(logINFO) << "PixTestGainCalibration ctor()";
}

//----------------------------------------------------------
PixTestGainCalibration::~PixTestGainCalibration() {
  LOG(logINFO) << "PixTestGainCalibration dtor()";
}

// ----------------------------------------------------------------------
bool PixTestGainCalibration::setParameter(string parName, string sval) {
  bool found(false);
  for (map<string,string>::iterator imap = fParameters.begin(); imap != fParameters.end(); ++imap) {
    if (0 == imap->first.compare(parName)) {
      found = true; 
      break;
    }
  }
  if (found) {
    fParameters[parName] = sval;
  }

  return found;
}

// ----------------------------------------------------------------------
void PixTestGainCalibration::doTest() {
  LOG(logINFO)<< "PixTestGainCalibration::doTest()";
}
