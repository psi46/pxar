
// switchyard for gui tests

#include <iostream>
#include <algorithm>

#include "PixTestFactory.hh"
#include "log.h"

#include "PixTestCurrentVsDac.hh"
#include "PixTestAlive.hh"
#include "PixTestTbm.hh"
#include "PixTestDacScan.hh"
#include "PixTestDacDacScan.hh"
#include "PixTestPhDacScan.hh"
#include "PixTestTrim.hh"
#include "PixTestScurves.hh"
#include "PixTestSetup.hh"
#include "PixTestPretest.hh"
#include "PixTestGainCalibration.hh"

using namespace std;
using namespace pxar;


PixTestFactory* PixTestFactory::fInstance = 0; 


// ----------------------------------------------------------------------
PixTestFactory* PixTestFactory::instance() {
  LOG(logDEBUG) << "PixTestFactory* PixTestFactory::instance()";
  if (0 == fInstance) {
    fInstance = new PixTestFactory;
  }
  return fInstance; 
}



// ----------------------------------------------------------------------
PixTestFactory::PixTestFactory() {
  LOG(logDEBUG) << "PixTestFactory::PixTestFactory()";
}

// ----------------------------------------------------------------------
PixTestFactory::~PixTestFactory() {
  LOG(logDEBUG) << "PixTestFactory::~PixTestFactory()";
}

// ----------------------------------------------------------------------
PixTest* PixTestFactory::createTest(string name, PixSetup *a) {
  ::transform(name.begin(), name.end(), name.begin(), ::tolower);

  if( !name.compare("curvsdac" ) ) return new PixTestCurrentVsDac(a, "CurVsDac" ); 
  if (!name.compare("dacscan")) return new PixTestDacScan(a, "DacScan"); 
  if (!name.compare("dacdacscan")) return new PixTestDacDacScan(a, "DacDacScan"); 
  if (!name.compare("phdacscan")) return new PixTestPhDacScan(a, "PhDacScan"); 
  if (!name.compare("gaincalibration")) return new PixTestGainCalibration(a, "GainCalibration"); 
  if (!name.compare("pixelalive")) return new PixTestAlive(a, "PixelAlive"); 
  if (!name.compare("pretest")) return new PixTestPretest(a, "Pretest"); 
  if (!name.compare("scurves")) return new PixTestScurves(a, "Scurves"); 
  if (!name.compare("setup")) return new PixTestSetup(a, "Setup"); 
  if (!name.compare("tbm")) return new PixTestTbm(a, "Tbm"); 
  if (!name.compare("trim")) return new PixTestTrim(a, "Trim"); 
  return 0; 
}
