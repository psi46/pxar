#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include <fstream>

#include <TH1.h>
#include <TRandom.h>
#include <TROOT.h>
#include <TStyle.h>
#include <TMath.h>
#include <TStopwatch.h>

#include "PixTestGainPedestal.hh"
#include "PHCalibration.hh"
#include "PixUtil.hh"
#include "log.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestGainPedestal)

// ----------------------------------------------------------------------
PixTestGainPedestal::PixTestGainPedestal(PixSetup *a, std::string name) : PixTest(a, name), 
  fParNtrig(-1), fParShowFits(0), fParExtended(0), fParDumpHists(0), fVcalStep(-1)  {
  PixTest::init();
  init(); 

//   PHCalibration phc; 
//   phc.setPHParameters(fPixSetup->getConfigParameters()->getGainPedestalParameters());
//   phc.setMode(0); 
//   double ph = phc.phErr(0, 0, 0, 100);
//   double result = phc.vcalErr(0, 0, 0, ph); 
//   int iroc, icol, irow; 
//   for (int i = 0; i < 20; ++i) {
//     iroc = 15.*gRandom->Rndm();
//     icol = 52.*gRandom->Rndm();
//     irow = 80.*gRandom->Rndm();
//     cout << iroc << "/" << icol << "/" << irow << endl;
//     for (int iv = 100; iv < 1000; iv += 200) {
//       ph =  phc.phErr(iroc, icol, irow, iv);
//       result = phc.vcalErr(iroc, icol, irow, ph); 
//       cout << iv << " " << result << " " << ph << endl;
//     }
//   }
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
      if (!parName.compare("ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
      }
      if (!parName.compare("showfits")) {
	PixUtil::replaceAll(sval, "checkbox(", "");
	PixUtil::replaceAll(sval, ")", "");
	fParShowFits = atoi(sval.c_str()); 
      }
      if (!parName.compare("extended")) {
	PixUtil::replaceAll(sval, "checkbox(", "");
	PixUtil::replaceAll(sval, ")", "");
	fParExtended = atoi(sval.c_str()); 
      }
      if (!parName.compare("dumphists")) {
	PixUtil::replaceAll(sval, "checkbox(", "");
	PixUtil::replaceAll(sval, ")", "");
	fParDumpHists = atoi(sval.c_str()); 
      }
      if (!parName.compare("vcalstep")) {
        fVcalStep = atoi(sval.c_str());
        LOG(logDEBUG) << "PixTestGainPedestal::PixTest() fVcalStep = " << fVcalStep;
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

  TStopwatch t;

  gStyle->SetPalette(1);
  fDirectory->cd();
  PixTest::update(); 
  bigBanner(Form("PixTestGainPedestal::doTest() ntrig = %d", fParNtrig));

  measure();
  fit();
  saveGainPedestalParameters();

  int seconds = t.RealTime(); 
  LOG(logINFO) << "PixTestGainPedestal::doTest() done, duration: " << seconds << " seconds";
}


// ----------------------------------------------------------------------
void PixTestGainPedestal::fullTest() {

  TStopwatch t;

  fDirectory->cd();
  PixTest::update(); 
  bigBanner(Form("PixTestGainPedestal::fullTest() ntrig = %d", fParNtrig));

  //  fParDumpHists = 1; 

  measure();
  fit();
  saveGainPedestalParameters();

  int seconds = t.RealTime(); 
  LOG(logINFO) << "PixTestGainPedestal::doTest() done, duration: " << seconds << " seconds";
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


  fLpoints.clear();

  for( int value = 10; value <= 255; value += fVcalStep ){
    fLpoints.push_back(value);
  }

  fHpoints.clear();
  if (1 == fParExtended) {
    fHpoints.push_back(10); //new:  70
    fHpoints.push_back(17); //new: 119
    fHpoints.push_back(24); //new: 168
  }
  fHpoints.push_back(30); 
  fHpoints.push_back(50); 
  fHpoints.push_back(70); 
  fHpoints.push_back(90); 
  if (1 == fParExtended) {
    fHpoints.push_back(120); //new
  }
  fHpoints.push_back(200); 


  cacheDacs();
 
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 

  shist256 *pshistBlock  = new (fPixSetup->fPxarMemory) shist256[16*52*80]; 
  shist256 *ph;
  
  int idx(0);
  fHists.clear();
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    for (unsigned int ic = 0; ic < 52; ++ic) {
      for (unsigned int ir = 0; ir < 80; ++ir) {
	idx = PixUtil::rcr2idx(iroc, ic, ir); 
	ph = pshistBlock + idx;
	fHists.push_back(ph); 
      }
    }
  }

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  maskPixels();

  // -- first low range 
  fApi->setDAC("ctrlreg", 0);

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

  for (unsigned int i = 0; i < lresult.size(); ++i) {
    int dac = lresult[i].first; 
    int dacbin(-1); 
    for (unsigned int v = 0; v < fLpoints.size(); ++v) {
      if (fLpoints[v] == dac) {
	dacbin = v; 
	break;
      }
    }
    vector<pixel> vpix = lresult[i].second;
    for (unsigned int ipx = 0; ipx < vpix.size(); ++ipx) {
      int roc = vpix[ipx].roc();
      int ic = vpix[ipx].column();
      int ir = vpix[ipx].row();

      double val = vpix[ipx].value();
      int idx = PixUtil::rcr2idx(getIdxFromId(roc), ic, ir);
      if (idx > -1) fHists[idx]->fill(dacbin+1, val);

    } 
  } 

  for (unsigned int i = 0; i < hresult.size(); ++i) {
    int dac = hresult[i].first; 
    int dacbin(-1); 
    for (unsigned int v = 0; v < fHpoints.size(); ++v) {
      if (fHpoints[v] == dac) {
	dacbin = 100 + v; 
	break;
      }
    }
    vector<pixel> vpix = hresult[i].second;
    for (unsigned int ipx = 0; ipx < vpix.size(); ++ipx) {
      int roc = vpix[ipx].roc();
      int ic = vpix[ipx].column();
      int ir = vpix[ipx].row();
      double val = vpix[ipx].value();
      int idx = PixUtil::rcr2idx(getIdxFromId(roc), ic, ir);
      if (idx > -1) fHists[idx]->fill(dacbin+1, val);

    } 
  } 

  printHistograms();

  PixTest::update(); 
  restoreDacs();
  LOG(logINFO) << "PixTestGainPedestal::measure() done ";
  dutCalibrateOff();
}




// ----------------------------------------------------------------------
void PixTestGainPedestal::fit() {
  PixTest::update(); 
  fDirectory->cd();

  string name = Form("gainPedestal_c%d_r%d_C%d", 0, 0, 0); 
  TH1D *h1 = bookTH1D(name, name, 1800, 0., 1800.);
  h1->SetMinimum(0);
  h1->SetMaximum(260.); 
  h1->SetNdivisions(506);
  h1->SetMarkerStyle(20);
  h1->SetMarkerSize(1.);

  TF1 *f(0); 

  int mode(0); 
  if (string::npos != fPixSetup->getConfigParameters()->getGainPedestalParameterFileName().find("TanH")) {
    LOG(logDEBUG) << "choosing TanH for PH gain/pedestal fitting"; 
    mode = 1; 
  }


  vector<vector<gainPedestalParameters> > v;
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  gainPedestalParameters a; 
  a.p0 = a.p1 = a.p2 = a.p3 = 0.;
  TH1D* h(0);
  vector<TH1D*> nllist;
  vector<TH1D*> p0list, p1list, p2list, p3list; 
  vector<TH1D*> e0list, e1list, e2list, e3list; 
  double fracErr(0.05); 
  double p0max = (0 == mode? 1000: 0.01);
  double p1max = (0 == mode? 1000: 2.);
  double p2max = (0 == mode? 10: 200.);
  double p3max = (0 == mode? 200: 200.);
  for (unsigned int i = 0; i < rocIds.size(); ++i) {
    LOG(logDEBUG) << "Create hist " << Form("gainPedestalP1_C%d", rocIds[i]); 
    h = bookTH1D(Form("gainPedestalP0_C%d", i), Form("gainPedestalP0_C%d", rocIds[i]), 100, 0., p0max); 
    setTitles(h, "p0", "Entries / Bin"); 
    p0list.push_back(h); 
    h = bookTH1D(Form("gainPedestalP1_C%d", i), Form("gainPedestalP1_C%d", rocIds[i]), 100, 0., p1max); 
    setTitles(h, "p1", "Entries / Bin"); 
    p1list.push_back(h); 
    h = bookTH1D(Form("gainPedestalP2_C%d", i), Form("gainPedestalP2_C%d", rocIds[i]), 100, 0., p2max); 
    setTitles(h, "p2", "Entries / Bin"); 
    p2list.push_back(h); 
    h = bookTH1D(Form("gainPedestalP3_C%d", i), Form("gainPedestalP3_C%d", rocIds[i]), 100, 0., p3max); 
    setTitles(h, "p3", "Entries / Bin"); 
    p3list.push_back(h); 

    h = bookTH1D(Form("P0_relErr_C%d", i), Form("P0_relErr_C%d", rocIds[i]), 100, 0., 1.); 
    e0list.push_back(h); 
    h = bookTH1D(Form("P1_relErr_C%d", i), Form("P1_relErr_C%d", rocIds[i]), 100, 0., 1.); 
    e1list.push_back(h); 
    h = bookTH1D(Form("P2_relErr_C%d", i), Form("P2_relErr_C%d", rocIds[i]), 100, 0., 1.); 
    e2list.push_back(h); 
    h = bookTH1D(Form("P3_relErr_C%d", i), Form("P3_relErr_C%d", rocIds[i]), 100, 0., 1.); 
    e3list.push_back(h); 

    h = bookTH1D(Form("gainPedestalNonLinearity_C%d", i), Form("gainPedestalNonLinearity_C%d", rocIds[i]), 200, 0.5, 1.5); 
    setTitles(h, "non linearity", "Entries / Bin"); 
    nllist.push_back(h); 
    

    vector<gainPedestalParameters> vroc;
    for (unsigned j = 0; j < 4160; ++j) {
      vroc.push_back(a); 
    }
    v.push_back(vroc); 
  }
  
  double nl(0.), ifunction(0.), ipol1(0.), x0(0.), y0(0.), x1(200.), y1(0.); 
  int iroc(0), ic(0), ir(0); 

  for (unsigned int i = 0; i < fHists.size(); ++i) {
    h1->Reset();
    for (int ib = 0; ib < static_cast<int>(fLpoints.size()); ++ib) {
      h1->SetBinContent(fLpoints[ib]+1, fHists[i]->get(ib+1));
      h1->SetBinError(fLpoints[ib]+1, fracErr*fHists[i]->get(ib+1)); 
    }
    for (int ib = 0; ib < static_cast<int>(fHpoints.size()); ++ib) {
      h1->SetBinContent(7*fHpoints[ib]+1, fHists[i]->get(100+ib+1));
      h1->SetBinError(7*fHpoints[ib]+1, fracErr*fHists[i]->get(100+ib+1)); 
    }
    if (0 == mode) {
      f = fPIF->gpErr(h1); 
    } else if (1 == mode) {
      f = fPIF->gpTanH(h1); 
    }
    if (h1->Integral() < 1) continue;
    PixUtil::idx2rcr(i, iroc, ic, ir);
    if (fParShowFits) {
      TH1D *hc = (TH1D*)h1->Clone(Form("gainPedestal_c%d_r%d_C%d", ic, ir, iroc));
      hc->SetTitle(Form("gainPedestal_c%d_r%d_C%d", ic, ir, iroc)); 
      string hcname = hc->GetName();
      LOG(logDEBUG) << hcname; 
      hc->Fit(f, "r");
      fHistList.push_back(hc); 
      PixTest::update(); 
    } else {
      if (fParDumpHists) {
	h1->SetTitle(Form("gainPedestal_c%d_r%d_C%d", ic, ir, iroc)); 
	h1->SetName(Form("gainPedestal_c%d_r%d_C%d", ic, ir, iroc)); 
      }
      h1->Fit(f, "rq");
      if (fParDumpHists) {
	ifunction = f->Integral(0., 200.);
	y0 = f->Eval(x0);
	y1 = f->Eval(x1);
	ipol1 = y0*x1 + 0.5*x1*(y1-y0);
	nl = ifunction/ipol1;
	nllist[getIdxFromId(iroc)]->Fill(nl); 
	h1->SetTitle(Form("%s, if = %6.4f, ip = %6.4f, nl = %6.4f", h1->GetTitle(), ifunction, ipol1, nl)); 

	h1->SetDirectory(fDirectory); 
	h1->Write();
      }
    }

    ifunction = f->Integral(0., 200.);
    y0 = f->Eval(x0);
    y1 = f->Eval(x1);
    ipol1 = y0*x1 + 0.5*x1*(y1-y0);
    nllist[getIdxFromId(iroc)]->Fill(ifunction/ipol1); 



    int idx = ic*80 + ir; 
    v[iroc][idx].p0 = f->GetParameter(0); 
    v[iroc][idx].p1 = f->GetParameter(1); 
    v[iroc][idx].p2 = f->GetParameter(2); 
    v[iroc][idx].p3 = f->GetParameter(3); 

    p0list[getIdxFromId(iroc)]->Fill(f->GetParameter(0)); 
    p1list[getIdxFromId(iroc)]->Fill(f->GetParameter(1)); 
    p2list[getIdxFromId(iroc)]->Fill(f->GetParameter(2)); 
    p3list[getIdxFromId(iroc)]->Fill(f->GetParameter(3)); 

    e0list[getIdxFromId(iroc)]->Fill(f->GetParError(0)/f->GetParameter(0)); 
    e1list[getIdxFromId(iroc)]->Fill(f->GetParError(1)/f->GetParameter(1)); 
    e2list[getIdxFromId(iroc)]->Fill(f->GetParError(2)/f->GetParameter(2)); 
    e3list[getIdxFromId(iroc)]->Fill(f->GetParError(3)/f->GetParameter(3)); 

  }

  fPixSetup->getConfigParameters()->setGainPedestalParameters(v);

  copy(p0list.begin(), p0list.end(), back_inserter(fHistList));
  copy(p1list.begin(), p1list.end(), back_inserter(fHistList));
  copy(p2list.begin(), p2list.end(), back_inserter(fHistList));
  copy(p3list.begin(), p3list.end(), back_inserter(fHistList));

  copy(e0list.begin(), e0list.end(), back_inserter(fHistList));
  copy(e1list.begin(), e1list.end(), back_inserter(fHistList));
  copy(e2list.begin(), e2list.end(), back_inserter(fHistList));
  copy(e3list.begin(), e3list.end(), back_inserter(fHistList));

  copy(nllist.begin(), nllist.end(), back_inserter(fHistList));

  h = (TH1D*)(fHistList.back());
  h->Draw();

  string nlMeanString(""), nlRmsString(""); 
  string p1MeanString(""), p1RmsString(""); 
  for (unsigned int i = 0; i < p1list.size(); ++i) {
    nlMeanString += Form(" %5.3f", nllist[i]->GetMean()); 
    nlRmsString += Form(" %5.3f", nllist[i]->GetRMS()); 
    
    p1MeanString += Form(" %5.3f", p1list[i]->GetMean()); 
    p1RmsString += Form(" %5.3f", p1list[i]->GetRMS()); 
  }
  
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 

  LOG(logINFO) << "PixTestGainPedestal::fit() done"; 
  if (0 == mode) {
    LOG(logINFO) << "non-linearity mean: " << nlMeanString; 
    LOG(logINFO) << "non-linearity RMS:  " << nlRmsString; 
  }  else if (1 == mode) {
    LOG(logINFO) << "p1 mean: " << p1MeanString; 
    LOG(logINFO) << "p1 RMS:  " << p1RmsString; 
  } 
}



// ----------------------------------------------------------------------
void PixTestGainPedestal::saveGainPedestalParameters() {
  fPixSetup->getConfigParameters()->writeGainPedestalParameters();
}


// ----------------------------------------------------------------------
void PixTestGainPedestal::printHistograms() {

  ofstream OutputFile;
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  int nRocs = static_cast<int>(rocIds.size()); 

  for (int iroc = 0; iroc < nRocs; ++iroc) {

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

    int roc(0), ic(0), ir(0); 
    string line(""); 
    for (int i = iroc*4160; i < (iroc+1)*4160; ++i) {
      PixUtil::idx2rcr(i, roc, ic, ir);
      if (roc != iroc) {
	LOG(logDEBUG) << "BIG CONFUSION?! iroc = " << iroc << " roc = " << roc;
      }

      line.clear();
      for (int ib = 0; ib < static_cast<int>(fLpoints.size()); ++ib) {
	line += Form(" %3d", static_cast<int>(fHists[i]->get(ib+1))); 
      }
      
      for (int ib = 0; ib < static_cast<int>(fHpoints.size()); ++ib) {
	line += Form(" %3d", static_cast<int>(fHists[i]->get(100+ib+1))); 
      }
      
      line += Form("    Pix %2d %2d", ic, ir); 
      OutputFile << line << endl;
    }

    OutputFile.close();
  }
}
