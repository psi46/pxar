#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include <fstream>

#include <TH1.h>
#include <TRandom.h>
#include <TROOT.h>
#include <TStyle.h>
#include <TMath.h>

#include "PixTestGainPedestal.hh"
#include "PixUtil.hh"
#include "log.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestGainPedestal)

// ----------------------------------------------------------------------
PixTestGainPedestal::PixTestGainPedestal(PixSetup *a, std::string name) : PixTest(a, name), fParShowFits(0), fParNtrig(-1), fParNpointsLo(-1), fParNpointsHi(-1) {
  PixTest::init();
  init(); 
}


//----------------------------------------------------------
PixTestGainPedestal::PixTestGainPedestal() : PixTest() {
  //  LOG(logDEBUG) << "PixTestGainPedestal ctor()";
}

// ----------------------------------------------------------------------
bool PixTestGainPedestal::setParameter(string parName, string sval) {
  bool found(false);
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());
      if (!parName.compare("showfits")) {
	fParShowFits = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParShowFits  ->" << fParShowFits << "<- from sval = " << sval;
      }
      if (!parName.compare("ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParNtrig  ->" << fParNtrig << "<- from sval = " << sval;
      }
      if (!parName.compare("npointslo")) {
	fParNpointsLo = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParNpointsLo  ->" << fParNpointsLo << "<- from sval = " << sval;
      }

      if (!parName.compare("npointshi")) {
	fParNpointsHi = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParNpointsHi  ->" << fParNpointsHi << "<- from sval = " << sval;
      }

      setToolTips();
      break;
    }
  }
  
  return found; 
}


// ----------------------------------------------------------------------
void PixTestGainPedestal::setToolTips() {
  fTestTip    = string(Form("measure and fit pulseheight vs VCAL (combining low- and high-range)\n")); 
  fSummaryTip = string("all ROCs are displayed side-by-side. Note the orientation:")
    + string("\nthe canvas bottom corresponds to the narrow module side with the cable")
    ;
}

// ----------------------------------------------------------------------
void PixTestGainPedestal::init() {

  setToolTips(); 

  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}

// ----------------------------------------------------------------------
void PixTestGainPedestal::bookHist(string /*name*/) {
  fDirectory->cd(); 
  //  fHistList.clear();

}


//----------------------------------------------------------
PixTestGainPedestal::~PixTestGainPedestal() {
  LOG(logDEBUG) << "PixTestGainPedestal dtor";
}


// ----------------------------------------------------------------------
void PixTestGainPedestal::doTest() {
  if (fPixSetup->isDummy()) {
    dummyAnalysis(); 
    return;
  }

  fDirectory->cd();
  PixTest::update(); 
  bigBanner(Form("PixTestGainPedestal::doTest() ntrig = %d, npointsLo = %d, npointHi = %d", 
		 fParNtrig, fParNpointsLo, fParNpointsHi));

  measure();
  fit();
  saveGainPedestalParameters();
}


// ----------------------------------------------------------------------
void PixTestGainPedestal::runCommand(string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;
  if (!command.compare("measure")) {
    measure(); 
    return;
  }
  if (!command.compare("fit")) {
    fit(); 
    return;
  }
  if (!command.compare("save")) {
    saveGainPedestalParameters(); 
    return;
  }
  return;
}



// ----------------------------------------------------------------------
void PixTestGainPedestal::measure() {
  uint16_t FLAGS = FLAG_FORCE_MASKED | FLAG_FORCE_SERIAL;
  LOG(logDEBUG) << " using FLAGS = "  << (int)FLAGS; 

  cacheDacs();


  TH1D *h1(0); 
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  string name; 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    for (unsigned int ix = 0; ix < 52; ++ix) {
      for (unsigned int iy = 0; iy < 80; ++iy) {
	name = Form("gainPedestal_c%d_r%d_C%d", ix, iy, rocIds[iroc]); 
	h1 = bookTH1D(name, name, 1800, 0., 1800.);
	h1->SetMinimum(0);
	h1->SetMaximum(260.); 
	h1->SetNdivisions(506);
	h1->SetMarkerStyle(20);
	h1->SetMarkerSize(1.);
	setTitles(h1, "Vcal [low range]", "PH [ADC]"); 
	fHists.insert(make_pair(name, h1)); 
	fHistList.push_back(h1);
      }
    }
  }

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  // -- first low range 
  fApi->setDAC("ctrlreg", 0);

  vector<pair<uint8_t, vector<pixel> > > rresult, lresult, hresult; 
  int step = 256/fParNpointsLo;
  for (int i = 0; i < fParNpointsLo; ++i) {
    LOG(logDEBUG) << "scanning vcal = " << i*step;
    rresult = fApi->getPulseheightVsDAC("vcal", i*step, i*step, FLAGS, fParNtrig);
    copy(rresult.begin(), rresult.end(), back_inserter(lresult)); 
  }

  // -- and high range
  fApi->setDAC("ctrlreg", 4);
  step = 256/fParNpointsHi;
  for (int i = 0; i < fParNpointsHi; ++i) {
    LOG(logDEBUG) << "scanning vcal = " << 10 + i*step;
    rresult = fApi->getPulseheightVsDAC("vcal", 10 + i*step, 10 + i*step, FLAGS, fParNtrig);
    copy(rresult.begin(), rresult.end(), back_inserter(hresult)); 
  }

  for (unsigned int i = 0; i < lresult.size(); ++i) {
    int dac = lresult[i].first; 
    vector<pixel> vpix = lresult[i].second;
    for (unsigned int ipx = 0; ipx < vpix.size(); ++ipx) {
      int roc = vpix[ipx].roc_id;
      int ic = vpix[ipx].column;
      int ir = vpix[ipx].row;
      name = Form("gainPedestal_c%d_r%d_C%d", ic, ir, roc); 
      h1 = fHists[name];
      if (h1) {
	h1->SetBinContent(dac+1, vpix[ipx].value);
	h1->SetBinError(dac+1, 0.05*vpix[ipx].value); //FIXME constant 5% error assumption!?
      } else {
	LOG(logDEBUG) << " histogram " << Form("gainPedestal_c%d_r%d_C%d", ic, ir, roc) << " not found";
      }
    } 
  } 

  int scaleLo(7); 
  for (unsigned int i = 0; i < hresult.size(); ++i) {
    int dac = hresult[i].first; 
    vector<pixel> vpix = hresult[i].second;
    for (unsigned int ipx = 0; ipx < vpix.size(); ++ipx) {
      int roc = vpix[ipx].roc_id;
      int ic = vpix[ipx].column;
      int ir = vpix[ipx].row;
      name = Form("gainPedestal_c%d_r%d_C%d", ic, ir, roc); 
      h1 = fHists[name];
      if (h1) {
	h1->SetBinContent(scaleLo*dac+1, vpix[ipx].value);
	h1->SetBinError(scaleLo*dac+1, 0.05*vpix[ipx].value); //FIXME constant 5% error assumption!?
      } else {
	LOG(logDEBUG) << " histogram " << Form("gainPedestal_c%d_r%d_C%d", ic, ir, roc) << " not found";
      }
    } 
  } 

  gStyle->SetOptStat(0); 
  gROOT->ForceStyle();
  h1->Draw(); 
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);

//   int RFLAG(7); 
//   vector<TH1*> thr0 = scurveMaps("vcal", "gainpedestal", fParNtrig, 0, 255, RFLAG, 2); 
//   TH1 *h1 = (*fDisplayedHist); 
//   h1->Draw(getHistOption(h1).c_str());
  PixTest::update(); 
  restoreDacs();
  LOG(logINFO) << "PixTestGainPedestal::gainPedestal() done ";
}




// ----------------------------------------------------------------------
void PixTestGainPedestal::fit() {
  PixTest::update(); 
  fDirectory->cd();


  TH1D *h1 = (*fHists.begin()).second; 
  TF1 *f = fPIF->gpTanH(h1); 

  vector<vector<gainPedestalParameters> > v;
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  gainPedestalParameters a; a.p0 = a.p1 = a.p2 = a.p3 = 0.;
  for (unsigned int i = 0; i < rocIds.size(); ++i) {
    vector<gainPedestalParameters> vroc;
    for (unsigned j = 0; j < 4160; ++j) {
      vroc.push_back(a); 
    }
    v.push_back(vroc); 
  }
  
  map<string, TH1D*>::iterator hend = fHists.end(); 
  int ic, ir, iroc, idx; 
  for (map<string, TH1D*>::iterator i = fHists.begin(); i != hend; ++i) {
    h1 = (*i).second; 
    if (h1->GetEntries() < 1) continue;
    string h1name = h1->GetName();
    if (fParShowFits) {
      LOG(logDEBUG) << h1name; 
      h1->Fit(f, "r");
      PixTest::update(); 
    } else {
      h1->Fit(f, "rq");
    }
    sscanf(h1name.c_str(), "gainPedestal_c%d_r%d_C%d", &ic, &ir, &iroc); 
    idx = ic*80 + ir; 
    v[iroc][idx].p0 = f->GetParameter(0); 
    v[iroc][idx].p1 = f->GetParameter(1); 
    v[iroc][idx].p2 = f->GetParameter(2); 
    v[iroc][idx].p3 = f->GetParameter(3); 

  }

  fPixSetup->getConfigParameters()->setGainPedestalParameters(v);
  LOG(logDEBUG) << "done with fitting"; 
}



// ----------------------------------------------------------------------
void PixTestGainPedestal::saveGainPedestalParameters() {
  fPixSetup->getConfigParameters()->writeGainPedestalParameters();
}

// ----------------------------------------------------------------------
void PixTestGainPedestal::output4moreweb() {
  // -- nothing required here
}
