#include <iostream>

#include "PixTestFactory.hh"

#include "PixTestAlive.hh"
#include "PixTestGainCalibration.hh"

using namespace std;


PixTestFactory* PixTestFactory::fInstance = 0; 


// ----------------------------------------------------------------------
PixTestFactory* PixTestFactory::instance() {
  cout << "PixTestFactory* PixTestFactory::instance()" << endl;
  if (0 == fInstance) {
    fInstance = new PixTestFactory;
  }
  return fInstance; 
}



// ----------------------------------------------------------------------
PixTestFactory::PixTestFactory() {
  cout << "PixTestFactory::PixTestFactory()" << endl;
}

// ----------------------------------------------------------------------
PixTestFactory::~PixTestFactory() {
  cout << "PixTestFactory::~PixTestFactory()" << endl;
}

// ----------------------------------------------------------------------
PixTest* PixTestFactory::createTest(string name, PixSetup *a) {
  
  if (!name.compare("pixelalive")) return new PixTestAlive(a, "PixelAlive"); 
  if (!name.compare("gaincalibration")) return new PixTestGainCalibration(a, "GainCalibration"); 
  return 0; 
}
