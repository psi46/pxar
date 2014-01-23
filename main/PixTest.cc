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

  fName = name;
  fParameters = a->getPixTestParameters()->getTestParameters(name); 

  for (map<string,string>::iterator imap = fParameters.begin(); imap != fParameters.end(); ++imap) {
    setParameter(imap->first, imap->second); 
  }
}


// ----------------------------------------------------------------------
void PixTest::bookHist(string name) {

}


// ----------------------------------------------------------------------
// result & 0x1 == 1 -> return maps 
// result & 0x2 == 2 -> return distributions (projections) of maps
// result & 0x4 == 4 -> return all pixel histograms with outlayer threshold/sigma
// result & 0x8 == 8 -> return all pixel histograms 
vector<TH1*> PixTest::scurveMaps(string dac, string name, int ntrig, int result) {
  vector<TH1*>          rmaps; 
  vector<vector<TH1*> > maps; 
  vector<TH1*>          resultMaps; 

  fHistList.clear();

  cout << "book histograms" << endl;
  TH1* h1(0); 
  for (int i = 0; i < fPixSetup->getConfigParameters()->getNrocs(); ++i){
    rmaps.clear();
    for (int ic = 0; ic < 52; ++ic) {
      for (int ir = 0; ir < 80; ++ir) {
	h1 = new TH1D(Form("%s_%s_c%d_r%d_C%d", name.c_str(), dac.c_str(), ic, ir, i), 
		      Form("%s_%s_c%d_r%d_C%d", name.c_str(), dac.c_str(), ic, ir, i), 
		      256, 0., 256.);
	rmaps.push_back(h1); 
      }
    }
    maps.push_back(rmaps); 
  }

  vector<uint8_t> cache; 
  for (int i = 0; i < fPixSetup->getConfigParameters()->getNrocs(); ++i){
    cache.push_back(fApi->_dut->getDAC(i, dac)); 
  }

  for (unsigned i = 0; i < cache.size(); ++i) {
    cout << " roc " << i << " " << dac << ": " << cache[i] << endl;
  }

  cout << "api::getEfficiencyMap" << endl;

  vector<pixel>  results;
  int ic, ir, iroc, val; 
  int dacmin(20), dacmax(80); 
  for (int idac = dacmin; idac < dacmax; ++idac) {
    results.clear(); 
    cout << " idac = " << idac << endl;
    fApi->setDAC(dac, idac);
    results = fApi->getEfficiencyMap(0, ntrig);
    for (unsigned int i = 0; i < results.size(); ++i) {
      ic = results[i].column; 
      ir = results[i].row; 
      iroc = results[i].roc_id; 
      val = results[i].value;
      maps[iroc][ic*80+ir]->SetBinContent(idac, val);
    }
  }

  fPIF->fLo = dacmin; 
  fPIF->fHi = dacmax; 

  TH1* h2(0), *h3(0); 
  for (unsigned int iroc = 0; iroc < maps.size(); ++iroc) {
    rmaps = maps[iroc];
    h2 = new TH2D(Form("thr_%s_%s_C%d", name.c_str(), dac.c_str(), iroc), 
		  Form("thr_%s_%s_C%d", name.c_str(), dac.c_str(), iroc), 
		  52, 0., 52., 80, 0., 80.); 

    h3 = new TH2D(Form("sig_%s_%s_C%d", name.c_str(), dac.c_str(), iroc), 
		  Form("sig_%s_%s_C%d", name.c_str(), dac.c_str(), iroc), 
		  52, 0., 52., 80, 0., 80.); 
    
    for (unsigned int i = 0; i < rmaps.size(); ++i) {
      threshold(rmaps[i]); 
      ic = i/80; 
      ir = i%80; 
      h2->SetBinContent(ic+1, ir+1, fThreshold); 
      h2->SetBinError(ic+1, ir+1, fThresholdE); 

      h3->SetBinContent(ic+1, ir+1, fSigma); 
      h3->SetBinError(ic+1, ir+1, fSigmaE); 
    }

    if (result & 0x1) {
      fHistList.push_back(h2); 
      fHistList.push_back(h3); 
    }

    if (result & 0x2) {
      TH1* d1 = distribution((TH2D*)h2, 100, 0., 100.); 
      resultMaps.push_back(d1); 
      TH1* d2 = distribution((TH2D*)h3, 100, 0., 100.); 
      resultMaps.push_back(d2); 
    }

    if (result & 0x4) {
      //      copy(emaps.begin(), emaps.end(), back_inserter(fHistList));
    }

    if (result & 0x8) {
      copy(rmaps.begin(), rmaps.end(), back_inserter(resultMaps));
    }

  }
  
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);

  if (h2) h2->Draw("colz");
  PixTest::update(); 

  return resultMaps; 
}

// ----------------------------------------------------------------------
vector<TH2D*> PixTest::efficiencyMaps(string name, int ntrig) {

  vector<pixel> results; 
  if (fApi) results = fApi->getEfficiencyMap(0, ntrig);

  fDirectory->cd(); 
  vector<TH2D*> maps;
  TH2D *h2(0); 
  for (int i = 0; i < fPixSetup->getConfigParameters()->getNrocs(); ++i){
    h2 = new TH2D(Form("%s_C%d", name.c_str(), i), Form("%s_C%d", name.c_str(), i), 52, 0., 52., 80, 0., 80.); 
    h2->SetMinimum(0.); 
    h2->SetDirectory(fDirectory); 
    setTitles(h2, "col", "row"); 
    maps.push_back(h2); 
  }
  
  for (int i = 0; i < results.size(); ++i) {
    cout << "results[i].roc_id = " << int(results[i].roc_id) 
	 << " col/row = " << int(results[i].column) << "/" << int(results[i].row)
	 << endl;
    h2 = maps[results[i].roc_id];
    if (h2) h2->SetBinContent(results[i].column +1, results[i].row + 1, static_cast<float>(results[i].value)/ntrig); 
  }

  return maps; 
}



// ----------------------------------------------------------------------
vector<TH1*> PixTest::mapsVsDac(string name, string dac, int ntrig) {

  int dacmin(0), dacmax(255); 

  fDirectory->cd(); 
  vector<TH1*> maps;
  vector<TH2D*> maps2d;
  vector<TH1D*> maps1d;  
  vector<TH1D*> mapspix;
  TH1D *h1(0); 
  TH2D *h2(0); 

  vector<pair<uint8_t, vector<pixel> > > results = fApi->getEfficiencyVsDAC(dac, dacmin, dacmax, 0, ntrig);

  // -- book histograms (the return vector is not over rocs, but rather DAC values
  for (int iroc = 0; iroc < fPixSetup->getConfigParameters()->getNrocs(); ++iroc){
    h2 = new TH2D(Form("%s_thr_%s_C%d", name.c_str(), dac.c_str(), iroc), Form("%s_C%d", name.c_str(), iroc), 52, 0., 52., 80, 0., 80.); 
    h2->SetMinimum(0.); 
    h2->SetDirectory(fDirectory); 
    setTitles(h2, "col", "row"); 
    maps.push_back(h2); 
    maps2d.push_back(h2); 


    h1 = new TH1D(Form("%s_hits_%s_C%d", name.c_str(), dac.c_str(), iroc), 
		  Form("%s_hits_%s_C%d", name.c_str(), dac.c_str(), iroc), 
		  256, 0., 256.); 
    h1->SetMinimum(0.); 
    setTitles(h1, "DAC", "# pixels"); 
    maps.push_back(h1); 
    maps1d.push_back(h1); 

    if (fPIX.size() > 0) {
      for (unsigned int ip = 0; ip < fPIX.size(); ++ip) {
	h1 = new TH1D(Form("%s_hits_%s_c%d_r%d_C%d", name.c_str(), dac.c_str(), fPIX[ip].first, fPIX[ip].second, iroc), 
		      Form("%s_hits_%s_c%d_r%d_C%d", name.c_str(), dac.c_str(), fPIX[ip].first, fPIX[ip].second, iroc), 
		      255, 0., 255.); 
	h1->SetMinimum(0.); 
	setTitles(h1, "DAC", "# pixels"); 
	maps.push_back(h1); 
	mapspix.push_back(h1); 
      }
    }

  }


  for (unsigned int id = 0; id < results.size(); ++id) {
    int idac = results[id].first;
    for (unsigned int i = 0; results[id].second.size(); ++i) {
      h2 = maps2d[results[id].second[i].roc_id];
      if (h2) h2->SetBinContent(results[id].second[i].column +1, results[id].second[i].row + 1, 
				static_cast<float>(results[id].second[i].value)/ntrig); 

    }
  }
  
  // -- FIXME: decode results and fill histograms
  // -- FIXME: add threshold finding

  return maps; 
}



// ----------------------------------------------------------------------
bool PixTest::setParameter(string parName, string value) {
  //  cout << " PixTest::setParameter wrong function" << endl;
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
  //  LOG(logINFO) << "PixTestBase dtor()";
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
    LOG(logINFO) << " Histogram not defined";
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
void PixTest::threshold(TH1 *h) {
  TF1 *f = fPIF->errScurve(h); 
  h->Fit(f, "qr", "", fPIF->fLo, fPIF->fHi); 
  // FIXME check for converged fit:
  //   if (strcmp(gMinuit->fCstatu.Data(), "CONVERGED ")) {
  //     LOG(logINFO) << "s-curve fit did not converge for histogram " << h->GetTitle(); 
  //   }

  fThreshold  = f->GetParameter(0); 
  fThresholdE = f->GetParError(0); 
  fSigma      = 1./(TMath::Sqrt(2.)*f->GetParameter(1)); 
  fSigmaE     = fSigma * f->GetParError(1) / f->GetParameter(1);
}


// ----------------------------------------------------------------------
TH1D *PixTest::distribution(TH2D* h2, int nbins, double xmin, double xmax) {
  TH1D *h1 = new TH1D(Form("dist_%s", h2->GetName()), Form("dist_%s", h2->GetName()), nbins, xmin, xmax); 
  for (int ix = 0; ix < h2->GetNbinsX(); ++ix) {
    for (int iy = 0; iy < h2->GetNbinsX(); ++iy) {
      h1->Fill(h2->GetBinContent(ix+1, iy+1)); 
    }
  }
  return h1; 
}
