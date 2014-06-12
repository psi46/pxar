#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include "PixUtil.hh"
#include "PixTestFactory.hh"
#include "PixTestFullTest.hh"
#include "log.h"

#include <TH2.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestFullTest)

// ----------------------------------------------------------------------
PixTestFullTest::PixTestFullTest(PixSetup *a, std::string name) : PixTest(a, name) {
  PixTest::init();
  init(); 
  LOG(logDEBUG) << "PixTestFullTest ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestFullTest::PixTestFullTest() : PixTest() {
  LOG(logDEBUG) << "PixTestFullTest ctor()";
}

// ----------------------------------------------------------------------
bool PixTestFullTest::setParameter(string parName, string /*sval*/) {
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
void PixTestFullTest::init() {
  LOG(logDEBUG) << "PixTestFullTest::init()";

  setToolTips();
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}

// ----------------------------------------------------------------------
void PixTestFullTest::setToolTips() {
  fTestTip    = string("run the complete FullTest")
    ;
  fSummaryTip = string("to be implemented")
    ;
}


// ----------------------------------------------------------------------
void PixTestFullTest::bookHist(string name) {
  LOG(logDEBUG) << "nothing done with " << name;
  fDirectory->cd(); 
}


//----------------------------------------------------------
PixTestFullTest::~PixTestFullTest() {
  LOG(logDEBUG) << "PixTestFullTest dtor";
}


// ----------------------------------------------------------------------
void PixTestFullTest::doTest() {

  bigBanner(Form("PixTestFullTest::doTest()"));
  //  fPixSetup->setMoreWebCloning(true);

  vector<string> suite;
  suite.push_back("alive"); 
  suite.push_back("bumpbonding"); 
  suite.push_back("scurves");
  suite.push_back("trim"); 
  suite.push_back("phoptimization"); 
  suite.push_back("gainpedestal"); 

  PixTest *t(0); 

  string trimvcal(""); 
  PixTestFactory *factory = PixTestFactory::instance(); 
  for (unsigned int i = 0; i < suite.size(); ++i) {
    t =  factory->createTest(suite[i], fPixSetup);

    if (!suite[i].compare("trim")) {
      trimvcal = t->getParameter("vcal"); 
      fPixSetup->getConfigParameters()->setTrimVcalSuffix(trimvcal); 
    }

    t->doTest(); 

    delete t; 
  }

  //  fPixSetup->setMoreWebCloning(false);
}


