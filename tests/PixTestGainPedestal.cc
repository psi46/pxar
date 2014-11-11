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
PixTestGainPedestal::PixTestGainPedestal(PixSetup *a, std::string name) : PixTest(a, name), fParShowFits(0), fParNtrig(-1)  {
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
	PixUtil::replaceAll(sval, "checkbox(", "");
	PixUtil::replaceAll(sval, ")", "");
	fParShowFits = atoi(sval.c_str()); 
      }
      if (!parName.compare("ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
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

  fLpoints.clear();
  fLpoints.push_back(50); 
  fLpoints.push_back(100); 
  fLpoints.push_back(150); 
  fLpoints.push_back(200); 
  fLpoints.push_back(250); 

  fHpoints.clear();
  fHpoints.push_back(30); 
  fHpoints.push_back(50); 
  fHpoints.push_back(70); 
  fHpoints.push_back(90); 
  fHpoints.push_back(200); 

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

  fDirectory->cd();
  PixTest::update(); 
  bigBanner(Form("PixTestGainPedestal::doTest() ntrig = %d", fParNtrig));

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
  uint16_t FLAGS = FLAG_FORCE_MASKED;
  LOG(logDEBUG) << " using FLAGS = "  << (int)FLAGS; 

  cacheDacs();
 
  TH1D *h1(0); 
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  string name; 
  fHists.clear();
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

  //    OutputFile << "Low range:  50 100 150 200 250 " << endl;
  //    OutputFile << "High range:  30  50  70  90 200 " << endl;

  vector<pair<uint8_t, vector<pixel> > > rresult, lresult, hresult; 
  for (unsigned int i = 0; i < fLpoints.size(); ++i) {
    LOG(logINFO) << "scanning low vcal = " << fLpoints[i];
    int cnt(0); 
    bool done = false;
    while (!done){
      try {
	rresult = fApi->getPulseheightVsDAC("vcal", fLpoints[i], fLpoints[i], FLAGS, fParNtrig);
	copy(rresult.begin(), rresult.end(), back_inserter(lresult)); 
	done = true; // got our data successfully
      }
      catch(pxar::DataMissingEvent &e){
	LOG(logCRITICAL) << "problem with readout: "<< e.what() << " missing " << e.numberMissing << " events"; 
	++cnt;
	if (e.numberMissing > 10) done = true; 
      } catch(pxarException &e) {
	LOG(logCRITICAL) << "pXar execption: "<< e.what(); 
	++cnt;
      }
      done = (cnt>2) || done;
    }
  }

  // -- and high range
  fApi->setDAC("ctrlreg", 4);
  for (unsigned int i = 0; i < fHpoints.size(); ++i) {
    LOG(logINFO) << "scanning high vcal = " << fHpoints[i] << " (= " << 7*fHpoints[i] << " in low range)";
    int cnt(0); 
    bool done = false;
    while (!done){
      try {
	rresult = fApi->getPulseheightVsDAC("vcal", fHpoints[i], fHpoints[i], FLAGS, fParNtrig);
	copy(rresult.begin(), rresult.end(), back_inserter(hresult)); 
	done = true; // got our data successfully
      }
      catch(pxar::DataMissingEvent &e){
	LOG(logCRITICAL) << "problem with readout: "<< e.what() << " missing " << e.numberMissing << " events"; 
	++cnt;
	if (e.numberMissing > 10) done = true; 
      } catch(pxarException &e) {
	LOG(logCRITICAL) << "pXar execption: "<< e.what(); 
	++cnt;
      }
      done = (cnt>2) || done;
    }
  }

  double sf(2.), err(0.);
  for (unsigned int i = 0; i < lresult.size(); ++i) {
    int dac = lresult[i].first; 
    vector<pixel> vpix = lresult[i].second;
    for (unsigned int ipx = 0; ipx < vpix.size(); ++ipx) {
      int roc = vpix[ipx].roc();
      int ic = vpix[ipx].column();
      int ir = vpix[ipx].row();
      name = Form("gainPedestal_c%d_r%d_C%d", ic, ir, roc); 
      h1 = fHists[name];
      if (h1) {
	h1->SetBinContent(dac+1, vpix[ipx].value());
	h1->SetBinError(dac+1, (err>1?sf*err:sf)); //FIXME using variance as error
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
      int roc = vpix[ipx].roc();
      int ic = vpix[ipx].column();
      int ir = vpix[ipx].row();
      name = Form("gainPedestal_c%d_r%d_C%d", ic, ir, roc); 
      h1 = fHists[name];
      if (h1) {
	h1->SetBinContent(scaleLo*dac+1, vpix[ipx].value());
	err = vpix[ipx].variance();
	h1->SetBinError(scaleLo*dac+1, (err>1?sf*err:sf)); //FIXME using variance as error
      } else {
	LOG(logDEBUG) << " histogram " << Form("gainPedestal_c%d_r%d_C%d", ic, ir, roc) << " not found";
      }
    } 
  } 

  gStyle->SetOptStat(0); 
  gROOT->ForceStyle();
  h1->Draw(); 
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);

  printHistograms();

  PixTest::update(); 
  restoreDacs();
  LOG(logINFO) << "PixTestGainPedestal::measure() done ";
}




// ----------------------------------------------------------------------
void PixTestGainPedestal::fit() {
  PixTest::update(); 
  fDirectory->cd();


  TH1D *h1(0); 
  TF1 *f(0); 

  vector<vector<gainPedestalParameters> > v;
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  gainPedestalParameters a; a.p0 = a.p1 = a.p2 = a.p3 = 0.;
  TH1D* h(0);
  vector<TH1D*> p1list; 
  for (unsigned int i = 0; i < rocIds.size(); ++i) {
    LOG(logDEBUG) << "Create hist " << Form("gainPedestalP1_C%d", rocIds[i]); 
    h = bookTH1D(Form("gainPedestalP1_C%d", i), Form("gainPedestalP1_C%d", rocIds[i]), 100, 0., 2.); 
    setTitles(h, "p1", "Entries / Bin"); 
    p1list.push_back(h); 
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
    f = fPIF->gpTanH(h1); 
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

    p1list[getIdxFromId(iroc)]->Fill(f->GetParameter(1)); 
  }

  fPixSetup->getConfigParameters()->setGainPedestalParameters(v);

  copy(p1list.begin(), p1list.end(), back_inserter(fHistList));
  h = (TH1D*)(fHistList.back());
  h->Draw();

  string p1MeanString(""), p1RmsString(""); 
  for (unsigned int i = 0; i < p1list.size(); ++i) {
    p1MeanString += Form(" %5.3f", p1list[i]->GetMean()); 
    p1RmsString += Form(" %5.3f", p1list[i]->GetRMS()); 
  }

  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 

  LOG(logINFO) << "PixTestGainPedestal::fit() done"; 
  LOG(logINFO) << "p1 mean: " << p1MeanString; 
  LOG(logINFO) << "p1 RMS:  " << p1RmsString; 
}



// ----------------------------------------------------------------------
void PixTestGainPedestal::saveGainPedestalParameters() {
  fPixSetup->getConfigParameters()->writeGainPedestalParameters();
}

// ----------------------------------------------------------------------
void PixTestGainPedestal::output4moreweb() {
  // -- nothing required here
}


// ----------------------------------------------------------------------
void PixTestGainPedestal::printHistograms() {

  ofstream OutputFile;
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  unsigned nRocs = rocIds.size(); 

  for (unsigned int iroc = 0; iroc < nRocs; ++iroc) {

    OutputFile.open(Form("%s/%s_C%d.dat", fPixSetup->getConfigParameters()->getDirectory().c_str(), 
			 fPixSetup->getConfigParameters()->getGainPedestalFileName().c_str(), 
			 iroc));

    OutputFile << "Pulse heights for the following Vcal values:" << endl;
    OutputFile << "Low range:  ";
    for (unsigned int i = 0; i < fLpoints.size(); ++i) OutputFile << fLpoints[i] << " "; 
    OutputFile << endl;
    OutputFile << "High range:  ";
    for (unsigned int i = 0; i < fHpoints.size(); ++i) OutputFile << fHpoints[i] << " "; 
    OutputFile << endl;
    OutputFile << endl;

    TH1D *h1(0); 
    for (int ic = 0; ic < 52; ++ic) {
      for (int ir = 0; ir < 80; ++ir) {
	h1 = fHists[Form("gainPedestal_c%d_r%d_C%d", ic, ir, iroc)];

	string h1name(h1->GetName()), line(""); 

	for (unsigned int i = 0; i < fLpoints.size(); ++i) {
	  line += Form(" %3d", static_cast<int>(h1->GetBinContent(fLpoints[i]+1))); 
	}
	
	for (unsigned int i = 0; i < fHpoints.size(); ++i) {
	  line += Form(" %3d", static_cast<int>(h1->GetBinContent(7*fHpoints[i]+1))); 
	}
	
	line += Form("    Pix %2d %2d", ic, ir); 
	OutputFile << line << endl;
      }
    }
    OutputFile.close();
  }
}
