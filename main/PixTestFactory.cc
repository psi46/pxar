#include <iostream>

#include "PixTestFactory.hh"
#include "log.h"

#include "PixTestAlive.hh"
#include "PixTestGainCalibration.hh"

using namespace std;
using namespace pxar;


PixTestFactory* PixTestFactory::fInstance = 0; 


// ----------------------------------------------------------------------
PixTestFactory* PixTestFactory::instance() {
  LOG(logINFO) << "PixTestFactory* PixTestFactory::instance()";
  if (0 == fInstance) {
    fInstance = new PixTestFactory;
  }
  return fInstance; 
}



// ----------------------------------------------------------------------
PixTestFactory::PixTestFactory() {
  LOG(logINFO) << "PixTestFactory::PixTestFactory()";
}

// ----------------------------------------------------------------------
PixTestFactory::~PixTestFactory() {
  LOG(logINFO) << "PixTestFactory::~PixTestFactory()";
}

// ----------------------------------------------------------------------
PixTest* PixTestFactory::createTest(string name, PixSetup *a) {
  
  if (!name.compare("pixelalive")) return new PixTestAlive(a, "PixelAlive"); 
  if (!name.compare("gaincalibration")) return new PixTestGainCalibration(a, "GainCalibration"); 
  return 0; 
}
