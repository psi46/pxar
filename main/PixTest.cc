#include <iostream>
#include <stdlib.h>     /* atof, atoi */

#include <TMinuit.h>
#include <TMath.h>

#include "PixTest.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTest)

// ----------------------------------------------------------------------
PixTest::PixTest(PixSetup *a, string name) {
  //  LOG(logINFO) << "PixTest ctor(PixSetup, string)";
  init(a, name); 
}

// ----------------------------------------------------------------------
PixTest::PixTest() {
  //  LOG(logINFO) << "PixTest ctor()";
  
}


// ----------------------------------------------------------------------
void PixTest::init(PixSetup *a, string name) {
  //  LOG(logINFO)  << "PixTest::init()";
  fPIF            = new PixInitFunc(); 
  fPixSetup       = a;
  fApi            = a->getApi(); 
  fTestParameters = a->getPixTestParameters(); 
  fCacheDac = "nada"; 
  fCacheVal.clear();    

  fName = name;
  setToolTips();
  fParameters = a->getPixTestParameters()->getTestParameters(name); 

  for (map<string,string>::iterator imap = fParameters.begin(); imap != fParameters.end(); ++imap) {
    setParameter(imap->first, imap->second); 
  }
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
  for (unsigned int i = 0; i < fPixSetup->getConfigParameters()->getNrocs(); ++i){
    rmaps.clear();
    for (unsigned int ic = 0; ic < 52; ++ic) {
      for (unsigned int ir = 0; ir < 80; ++ir) {
	h1 = new TH1D(Form("%s_%s_c%d_r%d_C%d", name.c_str(), dac.c_str(), ic, ir, i), 
		      Form("%s_%s_c%d_r%d_C%d", name.c_str(), dac.c_str(), ic, ir, i), 
		      256, 0., 256.);
	rmaps.push_back(h1); 
      }
    }
    maps.push_back(rmaps); 
  }

  cache(dac); 


  int ic, ir, iroc, val; 
  if (0) {
    vector<pixel>  results;
    for (int idac = dacmin; idac < dacmax; ++idac) {
      LOG(logINFO) << dac << ": " << idac;
      results.clear(); 
      fApi->setDAC(dac, idac);
      results = fApi->getEfficiencyMap(0, ntrig);
      for (unsigned int i = 0; i < results.size(); ++i) {
	ic = results[i].column; 
	ir = results[i].row; 
	iroc = results[i].roc_id; 
	val = results[i].value;
	maps[iroc][ic*80+ir]->Fill(idac, val);
      }
    }
  }
  if (1) {
    vector<pair<uint8_t, vector<pixel> > > results = fApi->getEfficiencyVsDAC(dac, dacmin, dacmax, 0, ntrig); 
    for (unsigned int idac = 0; idac < results.size(); ++idac) {
      int dac = results[idac].first; 
      for (unsigned int ipix = 0; ipix < results[idac].second.size(); ++ipix) {
	ic =   results[idac].second[ipix].column; 
	ir =   results[idac].second[ipix].row; 
	iroc = results[idac].second[ipix].roc_id; 
	val =  results[idac].second[ipix].value;
	maps[iroc][ic*80+ir]->Fill(dac, val);
      }
    }
  }

  fPIF->fLo = dacmin; 
  fPIF->fHi = dacmax; 

  TH1* h2(0), *h3(0); 
  for (unsigned int iroc = 0; iroc < maps.size(); ++iroc) {
    rmaps.clear();
    rmaps = maps[iroc];
    h2 = new TH2D(Form("thr_%s_%s_C%d", name.c_str(), dac.c_str(), iroc), 
		  Form("thr_%s_%s_C%d", name.c_str(), dac.c_str(), iroc), 
		  52, 0., 52., 80, 0., 80.); 

    h3 = new TH2D(Form("sig_%s_%s_C%d", name.c_str(), dac.c_str(), iroc), 
		  Form("sig_%s_%s_C%d", name.c_str(), dac.c_str(), iroc), 
		  52, 0., 52., 80, 0., 80.); 

    for (unsigned int i = 0; i < rmaps.size(); ++i) {
      if (rmaps[i]->GetSumOfWeights() < 1) {
	delete rmaps[i];
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
vector<TH2D*> PixTest::efficiencyMaps(string name, int ntrig) {

  vector<pixel> results; 
  if (fApi) results = fApi->getEfficiencyMap(0, ntrig);

  fDirectory->cd(); 
  vector<TH2D*> maps;
  TH2D *h2(0); 
  for (unsigned int i = 0; i < fPixSetup->getConfigParameters()->getNrocs(); ++i){
    h2 = new TH2D(Form("%s_C%d", name.c_str(), i), Form("%s_C%d", name.c_str(), i), 52, 0., 52., 80, 0., 80.); 
    h2->SetMinimum(0.); 
    h2->SetDirectory(fDirectory); 
    setTitles(h2, "col", "row"); 
    maps.push_back(h2); 
  }
  
  for (unsigned int i = 0; i < results.size(); ++i) {
    //     cout << "results[i].roc_id = " << int(results[i].roc_id) 
    // 	 << " col/row = " << int(results[i].column) << "/" << int(results[i].row)
    // 	 << endl;
    h2 = maps[results[i].roc_id];
    if (h2) h2->Fill(results[i].column, results[i].row, static_cast<float>(results[i].value)); 
  }

  return maps; 
}



// ----------------------------------------------------------------------
vector<TH1*> PixTest::thrMaps(string dac, string name, int ntrig) {

  vector<TH2D*>  maps; 
  vector<TH1*>   resultMaps; 

  TH1* h1(0); 
  fDirectory->cd();
  for (unsigned int i = 0; i < fPixSetup->getConfigParameters()->getNrocs(); ++i){
    h1 = new TH2D(Form("thr_%s_%s_C%d", name.c_str(), dac.c_str(), i), 
		  Form("thr_%s_%s_C%d", name.c_str(), dac.c_str(), i), 
		  52, 0., 52., 80, 0., 80.);
    resultMaps.push_back(h1); 
  }
  
  int ic, ir, iroc, val; 
  LOG(logDEBUG) << "start threshold map for dac = " << dac; 
  std::vector<pixel> results = fApi->getThresholdMap(dac, 0, ntrig);
  LOG(logDEBUG) << "finished threshold map for dac = " << dac; 
  for (unsigned int ipix = 0; ipix < results.size(); ++ipix) {
    ic =   results[ipix].column; 
    ir =   results[ipix].row; 
    iroc = results[ipix].roc_id; 
    val =  results[ipix].value;
    //    LOG(logDEBUG) << resultMaps[iroc]->GetName() << " roc/col/row = " << iroc << "/" << ic << "/" << ir << " val = " << val;
    ((TH2D*)resultMaps[iroc])->Fill(ic, ir, val); 
  }
  copy(resultMaps.begin(), resultMaps.end(), back_inserter(fHistList));
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);
  if (h1) h1->Draw("colz");
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
  for (map<string,string>::iterator imap = fParameters.begin(); imap != fParameters.end(); ++imap) {
    if (0 == imap->first.compare(parName)) {
      found = true; 
      break;
    }
  }
  if (found) {
    ival = atoi(fParameters[parName].c_str()); 
  }
  return found; 
}


  // ----------------------------------------------------------------------
bool PixTest::getParameter(std::string parName, float &fval) {
  bool found(false);
  for (map<string,string>::iterator imap = fParameters.begin(); imap != fParameters.end(); ++imap) {
    if (0 == imap->first.compare(parName)) {
      found = true; 
      break;
    }
  }
  if (found) {
    fval = atof(fParameters[parName].c_str()); 
  }
  return found; 
}


// ----------------------------------------------------------------------
void PixTest::dumpParameters() {
  LOG(logINFO) << "Parameters for test" << getName();
  for (map<string,string>::iterator imap = fParameters.begin(); imap != fParameters.end(); ++imap) {
    LOG(logINFO) << imap->first << ": " << imap->second;
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
    h->SetTitleOffset(xoff, "x");      h->SetTitleOffset(yoff, "y");
    h->SetTitleSize(size, "x");        h->SetTitleSize(size, "y");
    h->SetLabelSize(lsize, "x");       h->SetLabelSize(lsize, "y");
    h->SetLabelFont(font, "x");        h->SetLabelFont(font, "y");
    h->GetXaxis()->SetTitleFont(font); h->GetYaxis()->SetTitleFont(font);
    h->SetNdivisions(508, "X");
  }
}

// ----------------------------------------------------------------------
void PixTest::clearHist() {
  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    (*il)->Reset();
  }
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
TH2D* PixTest::moduleMap(string histname) {
  LOG(logDEBUG) << "moduleMap histname: " << histname; 
  TH1* h0 = (*fDisplayedHist);
  if (!h0->InheritsFrom(TH2::Class())) {
    return 0; 
  }
  TH2D *h1 = (TH2D*)h0; 
  string h1name = h1->GetName();
  string::size_type s1 = h1name.find("_C"); 
  string barename = h1name.substr(0, s1);
  string h2name = barename + string("_mod"); 
  LOG(logDEBUG) << "h1->GetName() = " << h1name << " -> " << h2name; 
  TH2D *h2 = new TH2D(h2name.c_str(), h2name.c_str(), 
		      2*80, 0., 2*80., 8*52, 0., 8*52); 
  for (unsigned int ichip = 0; ichip < fPixSetup->getConfigParameters()->getNrocs(); ++ichip) {
    TH2D *hroc = (TH2D*)fDirectory->Get(Form("%s_C%d", barename.c_str(), ichip)); 
    if (hroc) fillMap(h2, hroc, ichip); 
  }

  fHistList.push_back(h2); 
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);

  if (h2) h2->Draw();
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
    }
    return;
  } else if (npix < 101) {
    for (int i = 0; i < 50; ++i) {
      fApi->_dut->testPixel(i, 5 + 0.5*i, true);  
      ++cnt;
      fApi->_dut->testPixel(i, 15 + 0.5*i, true);  
      ++cnt;
      if (cnt == npix) return;
    }
  } else if (npix < 1001) {
    for (int i = 0; i < 50; ++i) {
      for (int j = 0; j < 10; ++j) {
	fApi->_dut->testPixel(i, i + 2*j, true);  
	fApi->_dut->testPixel(i, i + 5*j, true);  
	++cnt; 
	if (cnt == npix) return;
      }
    }
  } else{
    fApi->_dut->testAllPixels(true);
  }
}
