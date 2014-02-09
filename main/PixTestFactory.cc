
// switchyard for gui tests

#include <iostream>
#include <algorithm>

#include "PixTestFactory.hh"
#include "log.h"

#include "PixTestDacScanCurrent.hh"
#include "PixTestPretest.hh"
//#include "PixTestSetTrim.hh"
#include "PixTestSetPh.hh"
#include "PixTestMap.hh"
#include "PixTestMapPh.hh"
#include "PixTestMapThr.hh"
#include "PixTestDacScanPix.hh"
#include "PixTestDacScanPh.hh"
#include "PixTestDacScanThr.hh"
#include "PixTestDacScanRoc.hh"

#include "PixTestDacDacScan.hh"

#include "PixTestSetup.hh"
#include "PixTestDaq.hh"
#include "PixTestTbm.hh"
#include "PixTestTrim.hh"
#include "PixTestScurves.hh"
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

  if( !name.compare("curvsdac" ) ) return new PixTestDacScanCurrent(a, "CurVsDac" );
  if (!name.compare("pretest")) return new PixTestPretest(a, "Pretest");
  //  if( !name.compare( "settrim" ) ) return new PixTestSetTrim( a, "SetTrim" ); 
  if( !name.compare( "setph" ) ) return new PixTestSetPh( a, "SetPh" ); 
  if( !name.compare( "mapalive" ) ) return new PixTestMap( a, "MapAlive" ); 
  if( !name.compare( "mapph" ) ) return new PixTestMapPh( a, "MapPh" ); 
  if( !name.compare( "mapthr" ) ) return new PixTestMapThr( a, "MapThr" ); 
  if( !name.compare( "dacpix" ) ) return new PixTestDacScanPix( a, "DacPix" ); 
  if( !name.compare( "phvsdac" ) ) return new PixTestDacScanPh( a, "PhVsDac" ); 
  if( !name.compare( "thrvsdac" ) ) return new PixTestDacScanThr( a, "ThrVsDac" ); 
  if( !name.compare( "dacroc" ) ) return new PixTestDacScanRoc( a, "DacRoc" ); 

  if (!name.compare("dacdacscan")) return new PixTestDacDacScan(a, "DacDacScan");

  if (!name.compare("setup")) return new PixTestSetup(a, "Setup");
  if (!name.compare("daq")) return new PixTestDaq(a, "DAQ");
  if (!name.compare("tbm")) return new PixTestTbm(a, "Tbm");
  if (!name.compare("trim")) return new PixTestTrim(a, "Trim");
  if (!name.compare("scurves")) return new PixTestScurves(a, "Scurves");
  if (!name.compare("gaincalibration")) return new PixTestGainCalibration(a, "GainCalibration");
  return 0;
}
