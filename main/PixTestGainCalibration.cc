#include <iostream>
#include "PixTestGainCalibration.hh"

using namespace std;

ClassImp(PixTestGainCalibration)

//----------------------------------------------------------
PixTestGainCalibration::PixTestGainCalibration(PixSetup *a, std::string name): PixTest(a, name) {
  cout << "PixTestGainCalibration ctor(PixSetup &, string)" << endl;
}

//----------------------------------------------------------
PixTestGainCalibration::PixTestGainCalibration(): PixTest() {
  cout << "PixTestGainCalibration ctor()" << endl;
}

//----------------------------------------------------------
PixTestGainCalibration::~PixTestGainCalibration() {
  cout << "PixTestGainCalibration dtor()" << endl;
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
  cout << "PixTestGainCalibration::doTest()" << endl;
}
