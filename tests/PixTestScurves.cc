#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include <fstream>

#include <TH1.h>
#include <TRandom.h>
#include <TMath.h>

#include "PixTestScurves.hh"
#include "PixUtil.hh"
#include "log.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestScurves)

// ----------------------------------------------------------------------
PixTestScurves::PixTestScurves(PixSetup *a, std::string name) : PixTest(a, name), fParDac(""), fParNtrig(-1), fParNpix(-1), fParDacLo(-1), fParDacHi(-1) {
  PixTest::init();
  init(); 
}


//----------------------------------------------------------
PixTestScurves::PixTestScurves() : PixTest() {
  //  LOG(logDEBUG) << "PixTestScurves ctor()";
}

// ----------------------------------------------------------------------
bool PixTestScurves::setParameter(string parName, string sval) {
  bool found(false);
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());
      if (!parName.compare("ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParNtrig  ->" << fParNtrig << "<- from sval = " << sval;
      }
      if (!parName.compare("npix")) {
	fParNpix = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParNpix  ->" << fParNpix << "<- from sval = " << sval;
      }
      if (!parName.compare("dac")) {
	fParDac = sval;
	LOG(logDEBUG) << "  setting fParDac  ->" << fParDac << "<- from sval = " << sval;
      }
      if (!parName.compare("daclo")) {
	fParDacLo = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParDacLo  ->" << fParDacLo << "<- from sval = " << sval;
      }
      if (!parName.compare("dachi")) {
	fParDacHi = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParDacHi  ->" << fParDacHi << "<- from sval = " << sval;
      }

      setToolTips();
      break;
    }
  }
  
  return found; 
}


// ----------------------------------------------------------------------
void PixTestScurves::setToolTips() {
  fTestTip    = string(Form("measure and fit s-curves for DAC %s\n", fParDac.c_str())); 
  fSummaryTip = string("all ROCs are displayed side-by-side. Note the orientation:")
    + string("\nthe canvas bottom corresponds to the narrow module side with the cable")
    + string("\nexplanations for all plots: ")
    + string("thr_* shows the map of the s-curve thresholds")
    + string("\nsig_* shows the map of the s-curve widths")
    + string("\ndist_* shows the distribution/projections of the threshold and width maps")
    ;
}

// ----------------------------------------------------------------------
void PixTestScurves::init() {

  setToolTips(); 

  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}

// ----------------------------------------------------------------------
void PixTestScurves::bookHist(string name) {
  fDirectory->cd(); 

  LOG(logDEBUG) << "nothing done with " << name;
  //  fHistList.clear();

}


//----------------------------------------------------------
PixTestScurves::~PixTestScurves() {
  LOG(logDEBUG) << "PixTestScurves dtor";
}


// ----------------------------------------------------------------------
void PixTestScurves::doTest() {

  fDirectory->cd();
  PixTest::update(); 
  bigBanner(Form("PixTestScurves::doTest() ntrig = %d", fParNtrig));

  fParDac = "VthrComp"; 
  fParDacLo = 0; 
  fParDacHi = 250;
  scurves();

  fParDac = "Vcal"; 
  fParDacLo = 0; 
  fParDacHi = 250;
  scurves();
}


// ----------------------------------------------------------------------
void PixTestScurves::runCommand(string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;
  if (!command.compare("thrmap")) {
    thrMap(); 
    return;
  }
  if (!command.compare("fits")) {
    fitS(); 
    return;
  }
  if (!command.compare("scurves")) {
    scurves(); 
    return;
  }
  return;
}


// ----------------------------------------------------------------------
void PixTestScurves::scurves() {
  cacheDacs();
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  int results(7); 
  vector<TH1*> thr0 = scurveMaps(fParDac, "scurve"+fParDac, fParNtrig, fParDacLo, fParDacHi, results, 1); 
  TH1 *h1 = (*fDisplayedHist); 
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update(); 
  restoreDacs();

  string hname(""), scurvesMeanString(""), scurvesRmsString(""); 
  for (unsigned int i = 0; i < thr0.size(); ++i) {
    hname = thr0[i]->GetName();
    // -- skip sig_ and thn_ histograms
    if (string::npos == hname.find("dist_thr_")) continue;
    scurvesMeanString += Form("%6.2f ", thr0[i]->GetMean()); 
    scurvesRmsString += Form("%6.2f ", thr0[i]->GetRMS()); 
  }

  LOG(logINFO) << "PixTestScurves::scurves() done ";
  LOG(logINFO) << Form("%s mean: ", fParDac.c_str()) << scurvesMeanString; 
  LOG(logINFO) << Form("%s RMS:  ", fParDac.c_str()) << scurvesRmsString; 

}


// ----------------------------------------------------------------------
void PixTestScurves::thrMap() {
  cacheDacs();
  PixTest::update(); 
  fDirectory->cd();

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  LOG(logINFO) << "PixTestScurves::thrMap() start: " 
	       << fParDac << ": " << fParDacLo << " .. " << fParDacHi
	       << " ntrig = " << fParNtrig;
  vector<TH1*> thr1 = thrMaps(fParDac, "thr"+fParDac, fParDacLo, fParDacHi, fParNtrig); 

  PixTest::update(); 
  restoreDacs();
  LOG(logINFO) << "PixTestScurves::thrMap() done ";

}


// ----------------------------------------------------------------------
void PixTestScurves::fitS() {
  PixTest::update(); 
  fDirectory->cd();

  if (!fParDac.compare("Vcal")) {
    TH1D *h = (TH1D*)fDirectory->Get("scurveVcal_Vcal_c51_r62_C0");
    
    fPIF->fLo = fParDacLo+1;
    fPIF->fHi = fParDacHi; 
    TF1  *f = fPIF->errScurve(h); 
    f->SetLineColor(kBlue); 
    
    for (int iroc = 0; iroc < 1; ++iroc) {
      for (int ic = 0; ic < 2; ++ic) {
	for (int ir = 0; ir < 10; ++ir) {
	  
	  h = (TH1D*)fDirectory->Get(Form("scurveVcal_Vcal_c%d_r%d_C%d", ic, ir, iroc)); 
	  if (0 == h) continue;
	  
	  cout << h->GetTitle() << endl;
	  h->Fit(f, "qr", "", fPIF->fLo, fPIF->fHi); 
	  
	  double Threshold  = f->GetParameter(0); 
	  double ThresholdE = f->GetParError(0); 
	  cout << Threshold << " +/- " << ThresholdE << endl;
	  PixTest::update();      
	}
      }
    }
  } 

  if (!fParDac.compare("VthrComp")) {
    TH1D *h = (TH1D*)fDirectory->Get("scurveVthrComp_VthrComp_c51_r62_C0");
    
    fPIF->fLo = fParDacLo+1;
    fPIF->fHi = h->FindLastBinAbove(0.5*h->GetMaximum());  

    for (int iroc = 0; iroc < 1; ++iroc) {
      for (int ic = 0; ic < 10; ++ic) {
	for (int ir = 0; ir < 20; ++ir) {
	  h = (TH1D*)fDirectory->Get(Form("scurveVthrComp_VthrComp_c%d_r%d_C%d", ic, ir, iroc)); 
	  if (0 == h) continue;
	  
	  cout << h->GetTitle() << endl;
	  TF1  *f = fPIF->errScurve(h); 
	  f->SetLineColor(kBlue); 
	  if (fPIF->doNotFit()) {
	  } else {
	    h->Fit(f, "qr", "", fPIF->fLo, fPIF->fHi); 
	  }
	  double Threshold  = f->GetParameter(0); 
	  double ThresholdE = f->GetParError(0); 
	  cout << Threshold << " +/- " << ThresholdE << endl;
	  PixTest::update();      
	  fHistList.push_back(h); 
	}
      }
    }
    fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  } 

}


// ----------------------------------------------------------------------
void PixTestScurves::output4moreweb() {
  // -- nothing required here
}
