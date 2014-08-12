#include <stdlib.h>    
#include <algorithm>   
#include <iostream>
#include <fstream>

#include <TH1.h>
#include <TMath.h>
#include <TTime.h>
#include <TPad.h>

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
PixTestIV::PixTestIV(PixSetup *a, string name) : PixTest(a, name), 
                                                 fParVoltageStart(0),
                                                 fParVoltageStop(150), 
                                                 fParVoltageStep(5), 
                                                 fParDelay(1), 
                                                 fStop(false), 
                                                 fParPort("") {
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
      if(!parName.compare("voltagestart")) {
        fParVoltageStart = atof(sval.c_str());
        setToolTips();
      }
      if(!parName.compare("voltagestop")) {
        fParVoltageStop = atof(sval.c_str());
        setToolTips();
      }
      if(!parName.compare("voltagestep")) {
        fParVoltageStep = atof(sval.c_str());
        setToolTips();
      }
      if(!parName.compare("delay(seconds)")) {
        fParDelay = atof(sval.c_str());
        setToolTips();
      }
      if(!parName.compare("port")) {
        fParPort = sval;
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
void PixTestIV::stop() {
  fStop = true;
  LOG(logINFO) << "Stop pressed. Ending test.";
}


// ----------------------------------------------------------------------
PixTestIV::~PixTestIV() {
  LOG(logDEBUG) << "PixTestIV dtor";
  if (fPixSetup->doMoreWebCloning()) output4moreweb();
}

// ----------------------------------------------------------------------
void PixTestIV::doTest() {

#ifndef BUILD_HV
  LOG(logERROR) << "Not built with HV supply support.";
  return;
#else

  fDirectory->cd();
  
  int numMeasurements = ceil(fabs((fParVoltageStart - fParVoltageStop)/fParVoltageStep));
  
  TH1D *h1(0);
  double vMin = min(fParVoltageStart, fParVoltageStop);
  double vMax = max(fParVoltageStart, fParVoltageStop)+fabs(fParVoltageStep);
  h1 = bookTH1D("IVcurve", "IV curve", numMeasurements, vMin, vMax);
  h1->SetMinimum(1.e-2);
  h1->SetMarkerStyle(20);
  h1->SetMarkerSize(1.3);
  h1->SetStats(0.);
  setTitles(h1, "-U [V]", "-I [uA]");

  vector<double> voltageMeasurements;
  vector<double> currentMeasurements;
  vector<TTimeStamp> timeStamps;
  
  PixTest::update();
  if(gPad) gPad->SetLogy(true);
  
  LOG(logINFO) << "Starting IV curve measurement...";
  pxar::HVSupply *hv = new pxar::HVSupply(fParPort.c_str());
  hv->setCurrentLimit(100);

  hv->sweepStart(fParVoltageStart,fParVoltageStop,fParVoltageStep,fParDelay);
  while(hv->sweepRunning()){
    double voltSet, voltRead, current;
    hv->sweepRead(voltSet, voltRead, current);
    voltageMeasurements.push_back(voltRead);
    currentMeasurements.push_back(current);
    h1->Fill(voltSet, current);
    
    TTimeStamp ts;
    ts.Set();
    timeStamps.push_back(ts);
    
    LOG(logINFO) << Form("V = %4f (meas: %+7.2f) I = %4.2e uA %s", 
                         voltSet, voltRead, current, fTimeStamp->AsString("c"));
    
    h1->Draw("p");
    PixTest::update();
    gSystem->ProcessEvents();
    if(fStop) break;
  }

  fHistList.push_back(h1);
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);
  h1->Draw("p");
  PixTest::update();
  
  writeOutput(voltageMeasurements, currentMeasurements, timeStamps);
  LOG(logINFO) << "PixTestIV::doTest() done ";
#endif
}

void PixTestIV::writeOutput(vector<double>       &voltageMeasurements,
                            vector<double>       &currentMeasurements,
                            vector<TTimeStamp>   &timeStamps){  
  ofstream OutputFile;
  OutputFile.open(Form("%s/ivCurve.log", fPixSetup->getConfigParameters()->getDirectory().c_str())); 
  OutputFile << "# IV test from "   << timeStamps[0].AsString("l") << endl;
  OutputFile << "#voltage(V)\tcurrent(A)\ttimestamp" << endl;
  
  unsigned int numMeasurements = voltageMeasurements.size();
  for (unsigned int i = 0; i < numMeasurements; i++) {
    OutputFile << Form("%+8.3f\t%+e\t%s", voltageMeasurements[i],
                                          currentMeasurements[i],
                                          timeStamps[i].AsString("c"))
               << endl; 
  }
  OutputFile.close();
}

