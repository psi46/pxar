#include <iostream>
#include <algorithm>

#include "PixTestFactory.hh"
#include "log.h"

#include "PixTestCurrentVsDac.hh"
#include "PixTestIV.hh"
#include "PixTestAlive.hh"
#include "PixTestTbm.hh"
#include "PixTestDacScan.hh"
#include "PixTestDacDacScan.hh"
#include "PixTestTrim.hh"
#include "PixTestScurves.hh"
#include "PixTestGainPedestal.hh"
#include "PixTestSetup.hh"
#include "PixTestPretest.hh"
#include "PixTestTiming.hh"
#include "PixTestPattern.hh"
#include "PixTestDaq.hh"
#include "PixTestXray.hh"
#include "PixTestHighRate.hh"
#include "PixTestPh.hh"
#include "PixTestPhOptimization.hh"
#include "PixTestPhOpt.hh"
#include "PixTestBBMap.hh"
#include "PixTestBareModule.hh"
#include "PixTestFullTest.hh"

using namespace std;
using namespace pxar;


PixTestFactory* PixTestFactory::fInstance = 0;


// ----------------------------------------------------------------------
PixTestFactory* PixTestFactory::instance() {
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
  if( !name.compare("iv" ) ) return new PixTestIV(a, "IV" );
  if (!name.compare("dacscan")) return new PixTestDacScan(a, "DacScan");
  if (!name.compare("dacdacscan")) return new PixTestDacDacScan(a, "DacDacScan");
  if (!name.compare("pixelalive")) return new PixTestAlive(a, "PixelAlive");
  if (!name.compare("alive")) return new PixTestAlive(a, "PixelAlive"); // synonym
  if (!name.compare("pretest")) return new PixTestPretest(a, "Pretest");
  if (!name.compare("timing")) return new PixTestTiming(a, "Timing");
  if (!name.compare("pattern")) return new PixTestPattern(a, "Pattern");
  if (!name.compare("scurves")) return new PixTestScurves(a, "Scurves");
  if (!name.compare("gainpedestal")) return new PixTestGainPedestal(a, "GainPedestal");
  if (!name.compare("setup")) return new PixTestSetup(a, "Setup");
  if (!name.compare("tbm")) return new PixTestTbm(a, "Tbm");
  if (!name.compare("trim")) return new PixTestTrim(a, "Trim");
  if (!name.compare("daq")) return new PixTestDaq(a, "DAQ");
  if (!name.compare("xray")) return new PixTestXray(a, "Xray");
  if (!name.compare("highrate")) return new PixTestHighRate(a, "HighRate");
  if (!name.compare("ph")) return new PixTestPh(a, "Ph");
  if (!name.compare("phoptimization")) return new PixTestPhOptimization(a, "PhOptimization");
  if (!name.compare("phopt")) return new PixTestPhOpt(a, "PhOpt");
  if (!name.compare("bumpbonding")) return new PixTestBBMap(a, "BumpBonding");
  if (!name.compare("baremodule")) return new PixTestBareModule(a, "BareModule");
  if (!name.compare("fulltest")) return new PixTestFullTest(a, "FullTest");
  return 0;
}
