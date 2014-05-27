
// Record an IV curve (only available if built with HV Power supply support)

#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>

#include <TH1.h>

#include "PixTestIV.hh"
#include "log.h"
#include "helper.h"
#ifdef BUILD_HV
#include "hvsupply.h"
#endif

using namespace pxar;

ClassImp(PixTestIV)

PixTestIV::PixTestIV( PixSetup *a, std::string name )
  : PixTest(a, name), fParVoltageMax(150), fParVoltageStep(5)
{
  PixTest::init();
  init();
}

PixTestIV::PixTestIV() : PixTest() {}

bool PixTestIV::setParameter(std::string parName, std::string sval) {
  bool found(false);

  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true;
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());
      if(!parName.compare("voltagemax")) {
	fParVoltageMax = atoi(sval.c_str());
	setToolTips();
      }
      if(!parName.compare("voltagestep")) {
	fParVoltageStep = atoi(sval.c_str());
	setToolTips();
      }
      break;
    }
  }
  return found;
}

void PixTestIV::init() {
  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory )
    fDirectory = gFile->mkdir( fName.c_str() );
  fDirectory->cd();
}

void PixTestIV::bookHist(std::string /*name*/) {
  fDirectory->cd();
}

PixTestIV::~PixTestIV() {
  std::list<TH1*>::iterator il;
  fDirectory->cd();
  for( il = fHistList.begin(); il != fHistList.end(); ++il) {
    LOG(logDEBUG) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory);
    (*il)->Write();
  }
}

void PixTestIV::doTest() {
  fDirectory->cd();

  TH1D *h1(0);
  h1 = bookTH1D("IVcurve","IV curve",fParVoltageMax, 0, fParVoltageMax);
  h1->SetMinimum(0.);
  h1->SetStats(0.);
  setTitles(h1, "U [V]", "I [uA]");

  PixTest::update();

#ifndef BUILD_HV
  LOG(logERROR) << "Not build with HV supply support.";
#else
  LOG(logINFO) << "Starting IV curve measurement...";
  pxar::hvsupply * myHvSupply = new pxar::hvsupply();
  myHvSupply->setCurrentLimit(50);
  
  // Loop over Voltage:
  for(unsigned int voltage = 0; voltage <= fParVoltageMax; voltage += fParVoltageStep) {
    myHvSupply->setVoltage(voltage);
    mDelay(2000); // Delay 2sec
    h1->SetBinContent(voltage+1, myHvSupply->getCurrent()*10E6);
  }
#endif

  fHistList.push_back(h1);
  h1->Draw();
  PixTest::update();
}
