#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include "PixUtil.hh"
#include "PixTestFactory.hh"
#include "PixTestFPIXTest.hh"
#include "log.h"

#include <TH2.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestFPIXTest)

// ----------------------------------------------------------------------
PixTestFPIXTest::PixTestFPIXTest(PixSetup *a, std::string name) : PixTest(a, name) {
  PixTest::init();
  init(); 
  LOG(logDEBUG) << "PixTestFPIXTest ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestFPIXTest::PixTestFPIXTest() : PixTest() {
  LOG(logDEBUG) << "PixTestFPIXTest ctor()";
}

// ----------------------------------------------------------------------
bool PixTestFPIXTest::setParameter(string parName, string /*sval*/) {
  bool found(false);
  string stripParName; 
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      if (!parName.compare("deadface")) {
	//	fDeadFace = static_cast<uint16_t>(atoi(sval.c_str())); 
	setToolTips();
      }
      break;
    }
  }
  return found; 
}


// ----------------------------------------------------------------------
void PixTestFPIXTest::init() {
  LOG(logDEBUG) << "PixTestFPIXTest::init()";

  setToolTips();
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}

// ----------------------------------------------------------------------
void PixTestFPIXTest::setToolTips() {
  fTestTip    = string("run the complete FPIXTest")
    ;
  fSummaryTip = string("to be implemented")
    ;
}


// ----------------------------------------------------------------------
void PixTestFPIXTest::bookHist(string name) {
  LOG(logDEBUG) << "nothing done with " << name;
  fDirectory->cd(); 
}


//----------------------------------------------------------
PixTestFPIXTest::~PixTestFPIXTest() {
  LOG(logDEBUG) << "PixTestFPIXTest dtor";
}


// ----------------------------------------------------------------------
void PixTestFPIXTest::doTest() {

  bigBanner(Form("PixTestFPIXTest::doTest()"));

  vector<string> suite;
  suite.push_back("Pretest");
  suite.push_back("alive"); 
  suite.push_back("trim");
  suite.push_back("scurves");
  suite.push_back("phoptimization"); 
  suite.push_back("gainpedestal"); 
  suite.push_back("bb3"); 

  PixTest *t(0); 

  string trimvcal(""); 
  PixTestFactory *factory = PixTestFactory::instance(); 
  for (unsigned int i = 0; i < suite.size(); ++i) {
    t =  factory->createTest(suite[i], fPixSetup);

    if (!suite[i].compare("trim")) {
      trimvcal = t->getParameter("vcal"); 
      fPixSetup->getConfigParameters()->setTrimVcalSuffix(trimvcal, true); 
    }

    if (!suite[i].compare("scurves")) {
      t->setParameter("dumpoutputfile","1");
      t->setParameter("dac","Vcal");
      t->setParameter("daclo","0");
      t->setParameter("dachi","70");
      //t->setParameter("dacs/step","1");
      t->setParameter("ntrig","200");
    }

    t->doTest(); 

    delete t; 
  }

}


