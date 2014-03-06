#include <iostream>
#include <fstream>
#include <stdlib.h>     /* atof, atoi */

#include <TKey.h>
#include <TClass.h>
#include <TMinuit.h>
#include <TMath.h>
#include <TStyle.h>

#include "PixTest.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTest)

// ----------------------------------------------------------------------
PixTest::PixTest(PixSetup *a, string name) {
  //  LOG(logINFO) << "PixTest ctor(PixSetup, string)";
  fPIF            = new PixInitFunc(); 
  fPixSetup       = a;
  fApi            = a->getApi(); 
  fTestParameters = a->getPixTestParameters(); 
  fCacheDac = "nada"; 
  fCacheVal.clear();    

  fName = name;
  setToolTips();
  fParameters = a->getPixTestParameters()->getTestParameters(name); 
  //  init(a, name); 
}

// ----------------------------------------------------------------------
PixTest::PixTest() {
  //  LOG(logINFO) << "PixTest ctor()";
  
}


// ----------------------------------------------------------------------
void PixTest::init() {
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    setParameter(fParameters[i].first, fParameters[i].second); 
  }
}

// ----------------------------------------------------------------------
string PixTest::stripPos(string name) {
  string::size_type m1 = name.find("::"); 
  name = name.substr(m1+2); 
  return name; 
}

// ----------------------------------------------------------------------
void PixTest::setToolTips() {
  fTestTip = "generic tool tip for a test";
  fSummaryTip = "generic tool tip for a the summary plot";
}



// ----------------------------------------------------------------------
void PixTest::bookHist(string name) {
  LOG(logDEBUG) << "Nothing done with " << name; 
}


// ----------------------------------------------------------------------
void PixTest::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "Nothing done with " << command; 
}


// ----------------------------------------------------------------------
void PixTest::resetDirectory() {
  fDirectory = gFile->GetDirectory(fName.c_str()); 
}


// ----------------------------------------------------------------------
int PixTest::pixelThreshold(string dac, int ntrig, int dacmin, int dacmax) {
  TH1D *h = new TH1D("h1", "h1", 256, 0., 256.); 

  vector<pair<uint8_t, vector<pixel> > > results = fApi->getEfficiencyVsDAC(dac, dacmin, dacmax, 0, ntrig);
  int val(0); 
  for (unsigned int idac = 0; idac < results.size(); ++idac) {
    int dacval = results[idac].first; 
    for (unsigned int ipix = 0; ipix < results[idac].second.size(); ++ipix) {
      val = results[idac].second[ipix].value;
      h->Fill(dacval, val);
    }
  }
  int thr = simpleThreshold(h); 
  delete h; 
  return thr; 

}

// ----------------------------------------------------------------------
// result & 0x1 == 1 -> return maps 
// result & 0x2 == 2 -> return distributions (projections) of maps
// result & 0x4 == 4 -> write to file: all pixel histograms with outlayer threshold/sigma
vector<TH1*> PixTest::scurveMaps(string dac, string name, int ntrig, int dacmin, int dacmax, int result) {
  vector<TH1*>          rmaps; 
  vector<vector<TH1*> > maps; 
  vector<TH1*>          resultMaps; 

  TH1* h1(0); 
  fDirectory->cd();

  map<int, int> id2idx; // map the ROC ID onto the index of the ROC
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    id2idx.insert(make_pair(rocIds[iroc], iroc)); 
    rmaps.clear();
    for (unsigned int ic = 0; ic < 52; ++ic) {
      for (unsigned int ir = 0; ir < 80; ++ir) {
	h1 = new TH1D(Form("%s_%s_c%d_r%d_C%d", name.c_str(), dac.c_str(), ic, ir, rocIds[iroc]), 
		      Form("%s_%s_c%d_r%d_C%d", name.c_str(), dac.c_str(), ic, ir, rocIds[iroc]), 
		      256, 0., 256.);
	rmaps.push_back(h1); 
      }
    }
    maps.push_back(rmaps); 
  }
  
  cache(dac); 
  

  int ic, ir, iroc, val; 
  vector<pair<uint8_t, vector<pixel> > > results = fApi->getEfficiencyVsDAC(dac, dacmin, dacmax, 0, ntrig); 
  for (unsigned int idac = 0; idac < results.size(); ++idac) {
    int dac = results[idac].first; 
    for (unsigned int ipix = 0; ipix < results[idac].second.size(); ++ipix) {
      ic =   results[idac].second[ipix].column; 
      ir =   results[idac].second[ipix].row; 
      iroc = results[idac].second[ipix].roc_id; 
      val =  results[idac].second[ipix].value;
      maps[id2idx[iroc]][ic*80+ir]->Fill(dac, val);
    }
  }

  fPIF->fLo = dacmin; 
  fPIF->fHi = dacmax; 

  TH1* h2(0), *h3(0); 
  string fname("SCurveData");
  ofstream OUT;
  string line; 
  string empty("32  93   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0 ");
  for (unsigned int iroc = 0; iroc < maps.size(); ++iroc) {
    rmaps.clear();
    rmaps = maps[iroc];
    h2 = new TH2D(Form("thr_%s_%s_C%d", name.c_str(), dac.c_str(), rocIds[iroc]), 
		  Form("thr_%s_%s_C%d", name.c_str(), dac.c_str(), rocIds[iroc]), 
		  52, 0., 52., 80, 0., 80.); 

    h3 = new TH2D(Form("sig_%s_%s_C%d", name.c_str(), dac.c_str(), rocIds[iroc]), 
		  Form("sig_%s_%s_C%d", name.c_str(), dac.c_str(), rocIds[iroc]), 
		  52, 0., 52., 80, 0., 80.); 

    OUT.open(Form("%s/%s_C%d.dat", fPixSetup->getConfigParameters()->getDirectory().c_str(), fname.c_str(), iroc));
    OUT << "Mode 1" << endl;
    for (unsigned int i = 0; i < rmaps.size(); ++i) {
      if (rmaps[i]->GetSumOfWeights() < 1) {
	delete rmaps[i];
	OUT << empty << endl;
	continue;
      }
      bool ok = threshold(rmaps[i]); 
      if (!ok) {
	LOG(logINFO) << "  failed fit for " << rmaps[i]->GetName() << ", adding to list of hists";
      }
      ic = i/80; 
      ir = i%80; 
      h2->SetBinContent(ic+1, ir+1, fThreshold); 
      h2->SetBinError(ic+1, ir+1, fThresholdE); 

      h3->SetBinContent(ic+1, ir+1, fSigma); 
      h3->SetBinError(ic+1, ir+1, fSigmaE); 

      // -- write file
      int NSAMPLES(32); 
      int ibin = rmaps[i]->FindBin(fThreshold); 
      int bmin = ibin - 15;
      line = Form("%2d %3d", NSAMPLES, bmin); 
      for (int ix = bmin; ix < bmin + 33; ++ix) {
	line += string(Form(" %3d", static_cast<int>(rmaps[i]->GetBinContent(ix)))); 
      }
      OUT << line << endl;

      if (result & 0x4) {
	cout << "add " << rmaps[i]->GetName() << endl;
	fHistList.push_back(rmaps[i]);
      }
      // -- write all hists to file if requested
      if (0 && result & 0x4) {
	rmaps[i]->SetDirectory(fDirectory); 
	rmaps[i]->Write();
	delete rmaps[i];
      }
    }
    OUT.close();

    if (result & 0x1) {
      resultMaps.push_back(h2); 
      fHistList.push_back(h2); 
      resultMaps.push_back(h3); 
      fHistList.push_back(h3); 
    }

    bool zeroSuppressed(fApi->_dut->getNEnabledPixels(0) < 4000?true:false);
    if (result & 0x2) {
      TH1* d1 = distribution((TH2D*)h2, 100, 0., 100., zeroSuppressed); 
      resultMaps.push_back(d1); 
      fHistList.push_back(d1); 
      TH1* d2 = distribution((TH2D*)h3, 100, 0., 4., zeroSuppressed); 
      resultMaps.push_back(d2); 
      fHistList.push_back(d2); 
    }

  }
  
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);

  if (h2) h2->Draw("colz");
  PixTest::update(); 

  restore(dac); 

  return resultMaps; 
}

// ----------------------------------------------------------------------
vector<TH2D*> PixTest::efficiencyMaps(string name, uint16_t ntrig) {

  vector<pixel> results; 
  if (fApi) results = fApi->getEfficiencyMap(0, ntrig);
  LOG(logDEBUG) << " eff result size = " << results.size(); 

  fDirectory->cd(); 
  vector<TH2D*> maps;
  TH2D *h2(0); 

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    h2 = bookTH2D(Form("%s_C%d", name.c_str(), iroc), Form("%s_C%d", name.c_str(), rocIds[iroc]), 52, 0., 52., 80, 0., 80.); 
    h2->SetMinimum(0.); 
    h2->SetDirectory(fDirectory); 
    setTitles(h2, "col", "row"); 
    maps.push_back(h2); 
  }
  
  int idx(-1); 
  for (unsigned int i = 0; i < results.size(); ++i) {
    idx = fId2Idx[results[i].roc_id];
    h2 = maps[idx];
    if (h2) {
      if (h2->GetBinContent(results[i].column+1, results[i].row+1) > 0) {
	LOG(logINFO) << "ROC/row/col = " << int(results[i].roc_id) << "/" << int(results[i].column) << "/" << int(results[i].row) 
		      << " with = " << h2->GetBinContent(results[i].column+1, results[i].row+1)
		      << " now adding " << static_cast<float>(results[i].value);
      }
      h2->Fill(results[i].column, results[i].row, static_cast<float>(results[i].value)); 
    }
  }

  return maps; 
}



// ----------------------------------------------------------------------
vector<TH1*> PixTest::thrMaps(string dac, string name, int ntrig) {

  vector<TH2D*>  maps; 
  vector<TH1*>   resultMaps; 

  TH1* h1(0); 
  fDirectory->cd();

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    h1 = bookTH2D(Form("thr_%s_%s_C%d", name.c_str(), dac.c_str(), iroc), 
		  Form("thr_%s_%s_C%d", name.c_str(), dac.c_str(), iroc), 
		  52, 0., 52., 80, 0., 80.);
    resultMaps.push_back(h1); 
    fHistOptions.insert(make_pair(h1, "colz"));
  }
  
  int ic, ir, iroc, val; 
  LOG(logDEBUG) << "start threshold map for dac = " << dac; 

  std::vector<pixel> results = fApi->getThresholdMap(dac, FLAG_RISING_EDGE, ntrig);
  LOG(logDEBUG) << "finished threshold map for dac = " << dac << " results size = " << results.size(); 
  for (unsigned int ipix = 0; ipix < results.size(); ++ipix) {
    ic =   results[ipix].column; 
    ir =   results[ipix].row; 
    iroc = fId2Idx[results[ipix].roc_id]; 
    val =  results[ipix].value;
    ((TH2D*)resultMaps[iroc])->Fill(ic, ir, val); 
  }
  copy(resultMaps.begin(), resultMaps.end(), back_inserter(fHistList));
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);
  if (h1) h1->Draw(getHistOption(h1).c_str());
  PixTest::update(); 
  
  return resultMaps; 
}



// ----------------------------------------------------------------------
bool PixTest::setParameter(string parName, string value) {
  LOG(logDEBUG) << " PixTest::setParameter wrong function" << parName << " " << value;
  return false;
}


// ----------------------------------------------------------------------
bool PixTest::getParameter(std::string parName, int &ival) {
  bool found(false);
  //  FIXME This is likely not the intended behavior
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (0 == fParameters[i].first.compare(parName)) {
      found = true; 
      ival = atoi(fParameters[i].first.c_str()); 
      break;
    }
  }
  return found; 
}


  // ----------------------------------------------------------------------
bool PixTest::getParameter(std::string parName, float &fval) {
  bool found(false);
  //  FIXME This is likely not the intended behavior
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (0 == fParameters[i].first.compare(parName)) {
      found = true; 
      fval = atof(fParameters[i].first.c_str()); 
      break;
    }
  }
  return found; 
}


// ----------------------------------------------------------------------
void PixTest::dumpParameters() {
  LOG(logINFO) << "Parameters for test" << getName();
  //  FIXME This is likely not the intended behavior
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    LOG(logINFO) << fParameters[i].first << ": " << fParameters[i].second;
  }
}


// ----------------------------------------------------------------------
PixTest::~PixTest() {
  LOG(logDEBUG) << "PixTestBase dtor(), writing out histograms";
  std::list<TH1*>::iterator il; 
  fDirectory->cd(); 
  for (il = fHistList.begin(); il != fHistList.end(); ++il) {
    //    LOG(logINFO) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory); 
    (*il)->Write(); 
  }
}

// ----------------------------------------------------------------------
void PixTest::testDone() {
  //  LOG(logINFO) << "PixTest::testDone()";
  Emit("testDone()"); 
}

// ----------------------------------------------------------------------
void PixTest::update() {
  //  cout << "PixTest::update()" << endl;
  Emit("update()"); 
}


// ----------------------------------------------------------------------
void PixTest::doTest() {
  //  LOG(logINFO) << "PixTest::doTest()";
}


// ----------------------------------------------------------------------
void PixTest::doAnalysis() {
  //  LOG(logINFO) << "PixTest::doAnalysis()";
}


// ----------------------------------------------------------------------
TH1* PixTest::nextHist() {
  if (fHistList.size() == 0) return 0; 
  std::list<TH1*>::iterator itmp = fDisplayedHist;  
  ++itmp;
  if (itmp == fHistList.end()) {
    // -- wrap around and point to first histogram in list
    fDisplayedHist = fHistList.begin(); 
    return (*fDisplayedHist); 
  } else {
    ++fDisplayedHist; 
    return (*fDisplayedHist); 
  }
}

// ----------------------------------------------------------------------
TH1* PixTest::previousHist() {
  if (fHistList.size() == 0) return 0; 
  if (fDisplayedHist == fHistList.begin()) {
    // -- wrap around and point to last histogram in list
    fDisplayedHist = fHistList.end(); 
    --fDisplayedHist;
    return (*fDisplayedHist); 
  } else {
    --fDisplayedHist; 
    return (*fDisplayedHist); 
  }

}

// ----------------------------------------------------------------------
void PixTest::setTitles(TH1 *h, const char *sx, const char *sy, float size, 
			float xoff, float yoff, float lsize, int font) {
  if (h == 0) {
    LOG(logDEBUG) << " Histogram not defined";
  } else {
    h->SetXTitle(sx);                  h->SetYTitle(sy); 
    if (fPixSetup->useRootLogon()) {
      h->SetTitleOffset(gStyle->GetTitleOffset("X"), "x");  h->SetTitleOffset(gStyle->GetTitleOffset("Y"), "y");
      h->SetTitleSize(gStyle->GetTitleSize("X"), "x");        h->SetTitleSize(gStyle->GetTitleSize("Y"), "y");
      h->SetLabelSize(gStyle->GetLabelSize("X"), "x");       h->SetLabelSize(gStyle->GetLabelSize("Y"), "y");
      h->SetLabelFont(gStyle->GetLabelFont());        h->SetLabelFont(gStyle->GetLabelFont(), "y");
      h->GetXaxis()->SetTitleFont(gStyle->GetTitleFont("X")); h->GetYaxis()->SetTitleFont(gStyle->GetTitleFont("Y"));
      h->SetNdivisions(gStyle->GetNdivisions("X"), "X");
    } else {
      h->SetTitleOffset(xoff, "x");      h->SetTitleOffset(yoff, "y");
      h->SetTitleSize(size, "x");        h->SetTitleSize(size, "y");
      h->SetLabelSize(lsize, "x");       h->SetLabelSize(lsize, "y");
      h->SetLabelFont(font, "x");        h->SetLabelFont(font, "y");
      h->GetXaxis()->SetTitleFont(font); h->GetYaxis()->SetTitleFont(font);
      h->SetNdivisions(508, "X");
    }
  }
}

// ----------------------------------------------------------------------
void PixTest::clearHistList() {
  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    //    (*il)->Reset();
    delete (*il);
  }
  fHistList.clear();
}


// ----------------------------------------------------------------------
int PixTest::simpleThreshold(TH1 *h) {
  return h->GetBinCenter(h->FindFirstBinAbove(0.5*h->GetMaximum()));
}


// ----------------------------------------------------------------------
bool PixTest::threshold(TH1 *h) {
  TF1 *f = fPIF->errScurve(h); 
  h->Fit(f, "qr", "", fPIF->fLo, fPIF->fHi); 

  fThreshold  = f->GetParameter(0); 
  fThresholdE = f->GetParError(0); 
  fSigma      = 1./(TMath::Sqrt(2.)*f->GetParameter(1)); 
  fSigmaE     = fSigma * f->GetParError(1) / f->GetParameter(1);

  if (fThreshold < 0 || strcmp(gMinuit->fCstatu.Data(), "CONVERGED ")) {

    LOG(logINFO) << "s-curve fit did not converge for histogram " << h->GetTitle(); 

    int ibin = h->FindFirstBinAbove(0.); 
    int jbin = h->FindFirstBinAbove(0.9*h->GetMaximum());
    fThreshold = h->GetBinCenter(0.5*(ibin+jbin)); 
    fThresholdE = 1+(TMath::Abs(ibin-jbin)); 

    fSigma = fThresholdE; 
    fSigmaE = fThresholdE; 
  }

  return true;
}


// ----------------------------------------------------------------------
TH1D *PixTest::distribution(TH2D* h2, int nbins, double xmin, double xmax, bool zeroSuppressed) {
  TH1D *h1 = new TH1D(Form("dist_%s", h2->GetName()), Form("dist_%s", h2->GetName()), nbins, xmin, xmax); 
  for (int ix = 0; ix < h2->GetNbinsX(); ++ix) {
    for (int iy = 0; iy < h2->GetNbinsY(); ++iy) {
      if (zeroSuppressed && h2->GetBinContent(ix+1, iy+1) < 1e-6) continue;
      h1->Fill(h2->GetBinContent(ix+1, iy+1)); 
    }
  }
  return h1; 
}


// ----------------------------------------------------------------------
void PixTest::cache(string dacname) {
  if (!fCacheDac.compare("nada")) {
    fCacheDac = dacname; 
  } else {
    LOG(logWARNING) << "Error: cached " << fCacheDac << ", not yet restored";
  }
  
  LOG(logDEBUG) << "Cache " << dacname;
  for (unsigned int i = 0; i < fPixSetup->getConfigParameters()->getNrocs(); ++i){
    fCacheVal.push_back(fApi->_dut->getDAC(i, dacname)); 
  }
}


// ----------------------------------------------------------------------
void PixTest::restore(string dacname) {
  if (dacname.compare(fCacheDac)) {
    LOG(logWARNING) << "Error: restoring " << dacname << ", but cached " << fCacheDac;
    return;
  }

  LOG(logDEBUG) << "Restore " << dacname;
  for (unsigned int i = 0; i < fPixSetup->getConfigParameters()->getNrocs(); ++i){
    fApi->setDAC(dacname, fCacheVal[i], i); 
  }
  
  fCacheVal.clear(); 
  fCacheDac = "nada"; 
}


// ----------------------------------------------------------------------
TH1* PixTest::moduleMap(string histname) {
  // FIXME? Loop over fHistList instead of using Directory->Get()?
  LOG(logDEBUG) << "moduleMap histname: " << histname; 
  TH1* h0 = (*fDisplayedHist);
  if (!h0->InheritsFrom(TH2::Class())) {
    return 0; 
  }
  TH2D *h1 = (TH2D*)h0; 
  string h1name = h1->GetName();
  string::size_type s1 = h1name.rfind("_C"); 
  string barename = h1name.substr(0, s1);
  string h2name = barename + string("_mod"); 
  LOG(logDEBUG) << "h1->GetName() = " << h1name << " -> " << h2name; 
  TH2D *h2 = bookTH2D(h2name.c_str(), h2name.c_str(), 2*80, 0., 2*80., 8*52, 0., 8*52); 
  fHistOptions.insert(make_pair(h2, "colz"));
  int cycle(-1); 
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    if (0 == iroc) cycle = -1 + histCycle(Form("%s_C%d", barename.c_str(), rocIds[iroc])); 
    TH2D *hroc = (TH2D*)fDirectory->Get(Form("%s_C%d_V%d", barename.c_str(), rocIds[iroc], cycle)); 
    if (hroc) fillMap(h2, hroc, rocIds[iroc]); 
  }

  fHistList.push_back(h2); 
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);

  if (h2) h2->Draw(getHistOption(h2).c_str());
  update(); 
  return h2; 
}

// ----------------------------------------------------------------------
void PixTest::fillMap(TH2D *hmod, TH2D *hroc, int iroc) {
  int mxOffset(iroc<8?159:0), mxDirection(iroc<8?-1:+1); 
  int myOffset(iroc<8?52*(iroc%8):52*(16-iroc)-1), myDirection(iroc<8?1:-1);

  int mx, my; 
  for (int rx = 0; rx < hroc->GetNbinsX(); ++rx) {
    for (int ry = 0; ry < hroc->GetNbinsY(); ++ry) {
      mx = mxOffset + mxDirection*ry;
      my = myOffset + myDirection*rx;
      //       if (0) cout << "mod x: " << mx
      // 		  << " mod y: " << my
      // 		  << endl;
      hmod->Fill(mx, my, hroc->GetBinContent(rx+1, ry+1)); 
    }
  }

}



// ----------------------------------------------------------------------
void PixTest::sparseRoc(int npix) {
  
  if (!fApi) return;

  int cnt(0); 
  if (npix < 11) {
    for (int i = 0; i < npix; ++i) {
      fApi->_dut->testPixel(5*i, 5*i, true);  
      fApi->_dut->maskPixel(5*i, 5*i, false);  
    }
    return;
  } else if (npix < 101) {
    for (int i = 0; i < 50; ++i) {
      fApi->_dut->testPixel(i, 5 + 0.5*i, true);  
      fApi->_dut->maskPixel(i, 5 + 0.5*i, false);  
      ++cnt;
      fApi->_dut->testPixel(i, 15 + 0.5*i, true);  
      fApi->_dut->maskPixel(i, 15 + 0.5*i, false);  
      ++cnt;
      if (cnt == npix) return;
    }
  } else if (npix < 1001) {
    for (int i = 0; i < 50; ++i) {
      for (int j = 0; j < 10; ++j) {
	fApi->_dut->testPixel(i, i + 2*j, true);  
	fApi->_dut->maskPixel(i, i + 2*j, false);  
	fApi->_dut->testPixel(i, i + 5*j, true);  
	fApi->_dut->maskPixel(i, i + 5*j, false);  
	++cnt; 
	if (cnt == npix) return;
      }
    }
  } else{
    fApi->_dut->testAllPixels(true);
    fApi->_dut->maskAllPixels(false);
  }
}


// ----------------------------------------------------------------------
bool PixTest::selectedRoc(int iroc) {
  vector<uint8_t> v = fApi->_dut->getEnabledRocIDs();
  if (v.end() == find(v.begin(), v.end(), iroc)) {
    return false;
  }
  return true;
}


// ----------------------------------------------------------------------
void PixTest::setId2Idx(std::map<int, int> a) {
  fId2Idx = a;
}


// ----------------------------------------------------------------------
TH1D* PixTest::bookTH1D(std::string sname, std::string title, int nbins, double xmin, double xmax) {
  int cnt = histCycle(sname); 
  LOG(logDEBUG) << "bookTH1D " << Form("%s_V%d", sname.c_str(), cnt);
  return new TH1D(Form("%s_V%d", sname.c_str(), cnt), Form("%s (V%d)", title.c_str(), cnt), nbins, xmin, xmax); 
} 


// ----------------------------------------------------------------------
TH2D* PixTest::bookTH2D(std::string sname, std::string title, int nbinsx, double xmin, double xmax, 
			int nbinsy, double ymin, double ymax) {
  int cnt = histCycle(sname); 
  LOG(logDEBUG) << "bookTH2D " << Form("%s_V%d", sname.c_str(), cnt);
  return new TH2D(Form("%s_V%d", sname.c_str(), cnt), Form("%s (V%d)", title.c_str(), cnt), nbinsx, xmin, xmax, nbinsy, ymin, ymax); 
}


// ----------------------------------------------------------------------
int PixTest::histCycle(string hname) {
  TH1* h(0); 
  int cnt(0); 
  h = (TH1*)fDirectory->FindObject(Form("%s_V%d", hname.c_str(), cnt));
  while (h) {
    ++cnt;
    h = (TH1*)fDirectory->FindObject(Form("%s_V%d", hname.c_str(), cnt));
  }
  return cnt;
}

// ----------------------------------------------------------------------
string PixTest::getHistOption(TH1* h) {   
  map<TH1*, string>::iterator end = fHistOptions.end(); 
  for (map<TH1*, string>::iterator pos = fHistOptions.begin(); 
       pos != end; ++pos) {
    if (h == pos->first) return pos->second;
  }
  return string("");
}


// ----------------------------------------------------------------------
vector<int> PixTest::getMaximumVthrComp(int ntrig, double frac, int reserve) {

  vector<pair<uint8_t, vector<pixel> > > scans = fApi->getEfficiencyVsDAC("vthrcomp", 0, 255, 0, ntrig);
  LOG(logDEBUG) << " getMaximumVthrComp.size(): " << scans.size();

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  vector<TH1*> scanHists;
  vector<int> npixels; 
  TH1* h1;
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    h1 = bookTH1D(Form("maxVthrComp_C%d", rocIds[iroc]),  Form("maxVthrComp_C%d", rocIds[iroc]),  255, 0., 255.);
    scanHists.push_back(h1); 
    npixels.push_back(fApi->_dut->getNEnabledPixels(rocIds[iroc])); 
  }

  int idx(-1); 
  for (unsigned int i = 0; i < scans.size(); ++i) {
    pair<uint8_t, vector<pixel> > v = scans[i];
    int idac = v.first; 
    
    vector<pixel> vpix = v.second;
    for (unsigned int ipix = 0; ipix < vpix.size(); ++ipix) {
      idx = fId2Idx[vpix[ipix].roc_id]; 
      scanHists[idx]->Fill(idac, vpix[ipix].value); 
    }
  }

  vector<int> results; 
  for (unsigned int i = 0; i < scanHists.size(); ++i) {
    scanHists[i]->Draw();
    update();
    bool onPlateau(false); 
    int idac(1); 
    int plateau = ntrig*npixels[i];
    for (idac = 1; idac < 255; ++idac) {
      if (scanHists[i]->GetBinContent(idac) > frac*plateau
	  && scanHists[i]->GetBinContent(idac+1) > frac*plateau
	  && scanHists[i]->GetBinContent(idac+2) > frac*plateau
	  && scanHists[i]->GetBinContent(idac+3) > frac*plateau
	  ) {
	onPlateau = true;
      } else {
	if (onPlateau) {
	  break;
	}
      }
    }
    idac -= reserve; 
    fHistList.push_back(scanHists[i]); 
    results.push_back(idac); 
  }
  return results;
}


// ----------------------------------------------------------------------
int PixTest::getIdFromIdx(int idx) {
  map<int, int>::iterator end = fId2Idx.end(); 
  for (map<int, int>::iterator il = fId2Idx.begin(); il != end; ++il) {
    if (il->second == idx) return il->first;
  }
  return -1; 
}


// ----------------------------------------------------------------------
void PixTest::dummyAnalysis() {
  
}


// ----------------------------------------------------------------------
void PixTest::output4moreweb() {

}
