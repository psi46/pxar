
// Record an IV curve (only available if built with HV Power supply support)

#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include <fstream>

#include <TH1.h>
#include <TMath.h>

#include "PixTestIV.hh"
#include "log.h"
#include "helper.h"
#ifdef BUILD_HV
#include "hvsupply.h"
#endif

using namespace pxar;
using namespace std;

ClassImp(PixTestIV)

// ----------------------------------------------------------------------
PixTestIV::PixTestIV(PixSetup *a, string name) : PixTest(a, name), fParVoltageMax(150), fParVoltageStep(5), fParDelay(1) {
  PixTest::init();
  init();
}

// ----------------------------------------------------------------------
PixTestIV::PixTestIV() : PixTest() {}


// ----------------------------------------------------------------------
bool PixTestIV::setParameter(string parName, string sval) {
  bool found(false);

  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true;
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());
      if(!parName.compare("voltagemin")) {
	fParVoltageMin = atoi(sval.c_str());
	setToolTips();
      }
      if(!parName.compare("voltagemax")) {
	fParVoltageMax = atoi(sval.c_str());
	setToolTips();
      }
      if(!parName.compare("voltagestep")) {
	fParVoltageStep = atoi(sval.c_str());
	setToolTips();
      }
      if(!parName.compare("delay")) {
	fParDelay = atoi(sval.c_str());
	setToolTips();
      }
      break;
    }
  }
  return found;
}

// ----------------------------------------------------------------------
void PixTestIV::init() {
  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory )
    fDirectory = gFile->mkdir( fName.c_str() );
  fDirectory->cd();
}

// ----------------------------------------------------------------------
void PixTestIV::bookHist(string /*name*/) {
  fDirectory->cd();
}

// ----------------------------------------------------------------------
PixTestIV::~PixTestIV() {
  LOG(logDEBUG) << "PixTestIV dtor";
  if (fPixSetup->doMoreWebCloning()) output4moreweb();
}

// ----------------------------------------------------------------------
void PixTestIV::doTest() {

#ifndef BUILD_HV
  LOG(logERROR) << "Not build with HV supply support.";
  return;
#endif

  fDirectory->cd();

  TH1D *h1(0);
  h1 = bookTH1D("IVcurve", "IV curve", fParVoltageMax+1, 0, fParVoltageMax+1);
  h1->SetMinimum(0.);
  h1->SetMarkerStyle(20);
  h1->SetMarkerSize(1.5);
  h1->SetStats(0.);
  setTitles(h1, "U [V]", "I [uA]");

  PixTest::update();

  LOG(logINFO) << "Starting IV curve measurement...";
  pxar::hvsupply *hv = new pxar::hvsupply();
  hv->hvOn();
  double vOld = hv->getVoltage();
  LOG(logDEBUG) << "HV supply has default voltage: " << vOld; 
  hv->setCurrentLimit(50);
  
  // -- loop over voltage:
  double voltMeasured(-1.), amps(-1.);
  int tripped(-1); 
  for(int voltSet = fParVoltageMin; voltSet <= fParVoltageMax; voltSet += fParVoltageStep) {
    hv->setVoltage(voltSet);
    // -- get within 1V of specified voltage. Try at most 5 times.
    int ntry(0);
    while (ntry < 5) {
      mDelay(fParDelay*1000); 
      voltMeasured = hv->getVoltage(); 
      if (TMath::Abs(voltSet + voltMeasured) < 0.5) break; // assume that voltMeasured is negative!
      ++ntry;
    }
    if (hv->tripped()) {
      LOG(logCRITICAL) << "HV supply tripped, aborting IV test"; 
      tripped = voltSet;
      break;
    }
    mDelay(fParDelay*1000); 
    amps = hv->getCurrent()*1E6;
    LOG(logDEBUG) << Form("V = %3d (meas: %+7.2f) I = %4.2e uA (ntry = %d)", 
			  voltSet, voltMeasured, amps, ntry);
    h1->Fill(TMath::Abs(voltSet), TMath::Abs(amps));
  }

  // -- ramp down voltage
  for (int voltSet = fParVoltageMax - 2*fParVoltageStep; voltSet > 100; voltSet -= 2*fParVoltageStep) {
    LOG(logDEBUG) << "ramping down voltage, Vset = " << voltSet;
    hv->setVoltage(voltSet);
  }
  hv->setVoltage(vOld);
  delete hv;

  fHistList.push_back(h1);
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);
  h1->Draw("p");
  PixTest::update();

  ofstream OutputFile;
  OutputFile.open(Form("%s/iv.dat", fPixSetup->getConfigParameters()->getDirectory().c_str())); 
  OutputFile << "Voltage [V] Current [A]" << endl << endl;

  for (int voltSet = fParVoltageMin; voltSet <= fParVoltageMax; voltSet += fParVoltageStep) {
    if (tripped > -1 && voltSet > tripped) break;
    OutputFile << Form("%e %e", static_cast<double>(voltSet), 1.e-6*h1->GetBinContent(h1->FindBin(voltSet))) << endl; 
  }
  OutputFile.close();
}
