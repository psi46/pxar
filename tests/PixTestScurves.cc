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
#include "rsstools.hh"

using namespace std;
using namespace pxar;

ClassImp(PixTestScurves)

// ----------------------------------------------------------------------
PixTestScurves::PixTestScurves(PixSetup *a, std::string name) : PixTest(a, name), fParDac(""), fParNtrig(-1), fParNpix(-1), fParDacLo(-1), fParDacHi(-1), fAdjustVcal(1) {
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
      }
      if (!parName.compare("npix")) {
	fParNpix = atoi(sval.c_str()); 
      }
      if (!parName.compare("dac")) {
	fParDac = sval;
      }
      if (!parName.compare("daclo")) {
	fParDacLo = atoi(sval.c_str()); 
      }
      if (!parName.compare("dachi")) {
	fParDacHi = atoi(sval.c_str()); 
      }

      if (!parName.compare("adjustvcal")) {
	PixUtil::replaceAll(sval, "checkbox(", ""); 
	PixUtil::replaceAll(sval, ")", ""); 
	fAdjustVcal = atoi(sval.c_str()); 
	setToolTips();
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

}


//----------------------------------------------------------
PixTestScurves::~PixTestScurves() {
  LOG(logDEBUG) << "PixTestScurves dtor";
  if (fPixSetup->doMoreWebCloning()) output4moreweb();
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
  fDirectory->cd();
  cacheDacs();

  string command(fParDac);
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  if (!command.compare("vthrcomp") && fAdjustVcal) {
    LOG(logINFO) << "adjusting VCAL to have VthrComp average threshold at default VthrComp";
    adjustVcal(); 
  }
  

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  int results(15); 
  int FLAG = FLAG_FORCE_MASKED;
  vector<TH1*> thr0 = scurveMaps(fParDac, "scurve"+fParDac, fParNtrig, fParDacLo, fParDacHi, results, 1, FLAG); 
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
	  
	  h->Fit(f, "qr", "", fPIF->fLo, fPIF->fHi); 
	  
	  double Threshold  = f->GetParameter(0); 
	  double ThresholdE = f->GetParError(0); 
	  cout << Threshold << " +/- " << ThresholdE << endl;
	  PixTest::update();      
	}
      }
    }
  } 

  string dacname(fParDac);
  std::transform(dacname.begin(), dacname.end(), dacname.begin(), ::tolower);
  if (!dacname.compare("vthrcomp")) {
    TH1D *h = (TH1D*)fDirectory->Get("scurveVthrComp_VthrComp_c51_r62_C0");
    

    for (int iroc = 0; iroc < 1; ++iroc) {
      for (int ic = 0; ic < 10; ++ic) {
	for (int ir = 0; ir < 20; ++ir) {
	  h = (TH1D*)fDirectory->Get(Form("scurveVthrComp_VthrComp_c%d_r%d_C%d", ic, ir, iroc)); 
	  if (0 == h) continue;
	  fPIF->fLo = fParDacLo+1;
	  fPIF->fHi = h->FindLastBinAbove(0.5*h->GetMaximum());  
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
void PixTestScurves::adjustVcal() {

  vector<int> vcal; 
  uint16_t FLAGS = FLAG_FORCE_MASKED;

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  unsigned nrocs = rocIds.size();
  
  vector<vector<pair<int, int> > > dead = deadPixels(5); 
  // FIXME: do something with 'dead'!!!

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  fApi->_dut->testPixel(11, 20, true);
  fApi->_dut->maskPixel(11, 20, false);
  
  vector<TH2D *> hv; 
  TH2D *h(0); 
  for (unsigned int iroc = 0; iroc < nrocs; ++iroc){
    h = bookTH2D(Form("adjustVcal_C%d", rocIds[iroc]), Form("adjustVcal_C%d", rocIds[iroc]), 256, 0., 256., 256, 0., 256.); 
    hv.push_back(h); 
  }
  
  int ntrig(5); 

  try{
    
    vector<pair<uint8_t, pair<uint8_t, vector<pixel> > > >  results = 
      fApi->getEfficiencyVsDACDAC("vthrcomp", 0, 255, "vcal", 0, 255, FLAGS, ntrig);
    
    int idx(-1);
    for (unsigned int i = 0; i < results.size(); ++i) {
      pair<uint8_t, pair<uint8_t, vector<pixel> > > v = results[i];
      int idac1 = v.first; 
      pair<uint8_t, vector<pixel> > w = v.second;      
      int idac2 = w.first;
      vector<pixel> wpix = w.second;
      
      for (unsigned ipix = 0; ipix < wpix.size(); ++ipix) {
	idx = getIdxFromId(wpix[ipix].roc());
	hv[idx]->Fill(idac1, idac2, wpix[ipix].value()); 
      }
    }
    
    for (unsigned int iroc = 0; iroc < nrocs; ++iroc){
      hv[iroc]->Draw("colz");
      fHistList.push_back(hv[iroc]); 
      PixTest::update();      
      
      int vcthr = fApi->_dut->getDAC(rocIds[iroc], "vthrcomp");
      TH1D *h0 = hv[iroc]->ProjectionY("h0_px", vcthr, vcthr+1); 
      int vcalthr = h0->FindFirstBinAbove(0.5*ntrig); 
      delete h0; 

      LOG(logDEBUG) << "ROC " << static_cast<int>(rocIds[iroc]) << " vthrcomp = " << vcthr << " -> vcal = " << vcalthr;
      vcal.push_back(vcalthr); 
    }
    
  } catch(pxarException &e){
    LOG(logCRITICAL) << "problem with readout: "<< e.what() << " setting vcal = 100"; 
    for (unsigned int iroc = 0; iroc < nrocs; ++iroc){
      vcal.push_back(100); 
    }
  }
  
  for (unsigned int i = 0; i < nrocs; ++i) {
    fApi->setDAC("vcal", vcal[i], rocIds[i]);
  }
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  
}



// ----------------------------------------------------------------------
void PixTestScurves::output4moreweb() {
  print("PixTestScurves::output4moreweb()"); 

  list<TH1*>::iterator begin = fHistList.begin();
  list<TH1*>::iterator end = fHistList.end();

  TDirectory *pDir = gDirectory; 
  gFile->cd(); 
  for (list<TH1*>::iterator il = begin; il != end; ++il) {
    string name = (*il)->GetName(); 
    if (string::npos == name.find("_V0"))  continue;
    if (string::npos != name.find("dist_"))  continue;
    if (string::npos == name.find("thr_scurve"))  continue;
    if (string::npos != name.find("thr_scurveVthrComp_VthrComp")) {
      PixUtil::replaceAll(name, "thr_scurveVthrComp_VthrComp", "CalThresholdMap"); 
    }
    if (string::npos != name.find("thr_scurveVcal_Vcal")) {
      PixUtil::replaceAll(name, "thr_scurveVcal_Vcal", "VcalThresholdMap"); 
    }
    PixUtil::replaceAll(name, "_V0", ""); 
    TH2D *h = (TH2D*)((*il)->Clone(name.c_str()));
    h->SetDirectory(gDirectory); 
    h->Write(); 
  }
  pDir->cd(); 


}


