
// switchyard for gui tests

#include <iostream>

#include "PixTestFactory.hh"
#include "log.h"

#include "PixTestCurrentVsDac.hh"
#include "PixTestAlive.hh"
#include "PixTestTbm.hh"
#include "PixTestDacScan.hh"
#include "PixTestDacDacScan.hh"
#include "PixTestTrim.hh"
#include "PixTestScurves.hh"
#include "PixTestSetup.hh"
#include "PixTestSetVana.hh"
#include "PixTestSetVthrComp.hh"
#include "PixTestSetCalDel.hh"
#include "PixTestPhDacScan.hh"
#include "PixTestPhMap.hh"
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
  
  if( !name.compare("CurVsDac" ) ) return new PixTestCurrentVsDac(a, "CurVsDac" ); 
  if (!name.compare("PixelAlive")) return new PixTestAlive(a, "PixelAlive"); 
  if (!name.compare("Tbm")) return new PixTestTbm(a, "Tbm"); 
  if (!name.compare("DacScan")) return new PixTestDacScan(a, "DacScan"); 
  if (!name.compare("DacDacScan")) return new PixTestDacDacScan(a, "DacDacScan"); 
  if (!name.compare("Trim")) return new PixTestTrim(a, "Trim"); 
  if (!name.compare("Scurves")) return new PixTestScurves(a, "Scurves"); 
  if (!name.compare("Setup")) return new PixTestSetup(a, "Setup"); 
  if (!name.compare( "SetVana")) return new PixTestSetVana(a, "SetVana"); 
  if( !name.compare( "SetVthrComp" ) ) return new PixTestSetVthrComp( a, "SetVthrComp" ); 
  if (!name.compare( "SetCalDel")) return new PixTestSetCalDel(a, "SetCalDel"); 
  if( !name.compare( "PhVsDac" ) ) return new PixTestPhDacScan( a, "PhVsDac" ); 
  if( !name.compare( "PhMap" ) ) return new PixTestPhMap( a, "PhMap" ); 
  if (!name.compare("GainCalibration")) return new PixTestGainCalibration(a, "GainCalibration"); 
  return 0; 
}
