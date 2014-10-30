#include <iostream>
#include <fstream>
#include <stdlib.h>     /* atof, atoi */

#include <TKey.h>
#include <TClass.h>
#include <TMinuit.h>
#include <TMath.h>
#include <TStyle.h>
#include <TGMsgBox.h>

#include "PixTest.hh"
#include "PixUtil.hh"
#include "timer.h"
#include "log.h"
#include "helper.h"
#include "rsstools.hh"

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
  fTimeStamp      = new TTimeStamp(); 

  fProblem        = false; 

  fName = name;
  setToolTips();
  fParameters = a->getPixTestParameters()->getTestParameters(name); 
  fTree = 0; 

  // -- provide default map when all ROCs are selected
  map<int, int> id2idx; 
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned i = 0; i < rocIds.size(); ++i) {
    id2idx.insert(make_pair(rocIds[i], i)); 
  }
  setId2Idx(id2idx);

}

// ----------------------------------------------------------------------
PixTest::PixTest() {
  //  LOG(logINFO) << "PixTest ctor()";
  fTree = 0; 
  
}


// ----------------------------------------------------------------------
void PixTest::init() {
  int verbose(0);
   
  if (verbose) {
    cout << "test: " << getName()  << endl;
  }
  
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (verbose) cout << "fParameters[" << i << "].first = " << fParameters[i].first 
		      << " fParameters[" << i << "].second = " << fParameters[i].second 
		      << endl;
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
void PixTest::bookTree() {
  for (int ipix = 0; ipix < 2000; ++ipix) {
    fTreeEvent.proc[ipix] = 0; 
    fTreeEvent.pcol[ipix] = 0; 
    fTreeEvent.prow[ipix] = 0; 
    fTreeEvent.pval[ipix] = 0.; 
    fTreeEvent.pq[ipix]   = 0.; 
  }

  if (0 == fTree) {
    fTree = new TTree("events", "events"); 
    fTree->SetDirectory(fDirectory);
    fTree->Branch("header", &fTreeEvent.header, "header/s"); 
    fTree->Branch("trailer", &fTreeEvent.trailer, "trailer/s"); 
    fTree->Branch("npix", &fTreeEvent.npix, "npix/s"); 
    fTree->Branch("proc", fTreeEvent.proc, "proc[npix]/b");
    fTree->Branch("pcol", fTreeEvent.pcol, "pcol[npix]/b");
    fTree->Branch("prow", fTreeEvent.prow, "prow[npix]/b");
    fTree->Branch("pval", fTreeEvent.pval, "pval[npix]/D");
    fTree->Branch("pq",   fTreeEvent.pq,   "pq[npix]/D");
  }
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
  //  uint16_t FLAGS = FLAG_FORCE_MASKED | FLAG_FORCE_SERIAL;
  uint16_t FLAGS = FLAG_FORCE_MASKED;
  TH1D *h = new TH1D("h1", "h1", 256, 0., 256.); 

  vector<pair<uint8_t, vector<pixel> > > results;
  int cnt(0); 
  bool done(false);
  while (!done) {
    LOG(logDEBUG) << "      attempt #" << cnt;
    try {
      results = fApi->getEfficiencyVsDAC(dac, dacmin, dacmax, FLAGS, ntrig);
      fNDaqErrors = fApi->daqGetNDecoderErrors();
      done = true;
    } catch(pxarException &e) {
      ++cnt;
    }
    done = (cnt>2) || done;
  }

  int val(0); 
  for (unsigned int idac = 0; idac < results.size(); ++idac) {
    int dacval = results[idac].first; 
    for (unsigned int ipix = 0; ipix < results[idac].second.size(); ++ipix) {
      val = results[idac].second[ipix].value();
      h->Fill(dacval, val);
    }
  }
  int thr = simpleThreshold(h); 
  delete h; 
  return thr; 

}

// ----------------------------------------------------------------------
vector<TH1*> PixTest::scurveMaps(string dac, string name, int ntrig, int dacmin, int dacmax, int result, int ihit, int flag) {

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  string type("hits"); 
  if (2 == ihit) type = string("pulseheight"); 
  print(Form("dac: %s name: %s ntrig: %d dacrange: %d .. %d %s flags = %d (plus default)",  
	     dac.c_str(), name.c_str(), ntrig, dacmin, dacmax, type.c_str(), flag)); 

  vector<shist256*>  maps; 
  vector<TH1*>       resultMaps; 
  
  shist256 *pshistBlock  = new (fPixSetup->fPxarMemory) shist256[16*52*80]; 
  shist256 *ph;
  rsstools rss;

  int idx(0);
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    for (unsigned int ic = 0; ic < 52; ++ic) {
      for (unsigned int ir = 0; ir < 80; ++ir) {
	idx = PixUtil::rcr2idx(iroc, ic, ir); 
	ph = pshistBlock + idx;
	maps.push_back(ph); 
      }
    }
  }

  dacScan(dac, ntrig, dacmin, dacmax, maps, ihit, flag); 
  if (fNDaqErrors > 0) return resultMaps;
  if (1 == ihit) {
    scurveAna(dac, name, maps, resultMaps, result); 
  } 

  LOG(logDEBUG) << "PixTest::scurveMaps end: getCurrentRSS() = " << rss.getCurrentRSS();

  return resultMaps; 
}



// ----------------------------------------------------------------------
vector<TH2D*> PixTest::phMaps(string name, uint16_t ntrig, uint16_t FLAGS) {

  vector<pixel> results;

  int cnt(0); 
  bool done = false;
  while (!done){
    LOG(logDEBUG) << "      attempt #" << cnt;
    try {
      results = fApi->getPulseheightMap(FLAGS, ntrig);
      fNDaqErrors = fApi->daqGetNDecoderErrors();
      done = true; 
    } catch(pxarException &e) {
      fNDaqErrors = 666667;
      ++cnt;
    }
    done = (cnt>2) || done;
  }
  LOG(logDEBUG) << " eff result size = " << results.size(); 

  fDirectory->cd(); 
  vector<TH2D*> maps;
  TH2D *h2(0); 

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    LOG(logDEBUG) << "Create hist " << Form("%s_C%d", name.c_str(), rocIds[iroc]);
    h2 = bookTH2D(Form("%s_C%d", name.c_str(), rocIds[iroc]), Form("%s_C%d", name.c_str(), rocIds[iroc]), 52, 0., 52., 80, 0., 80.);
    h2->SetMinimum(0.); 
    h2->SetDirectory(fDirectory); 
    fHistOptions.insert(make_pair(h2, "colz")); 
    setTitles(h2, "col", "row"); 
    maps.push_back(h2); 
  }
  
  int idx(-1); 
  for (unsigned int i = 0; i < results.size(); ++i) {
    idx = getIdxFromId(results[i].roc());
    if (rocIds.end() != find(rocIds.begin(), rocIds.end(), results[i].roc())) {
      h2 = maps[idx];
      if (h2->GetBinContent(results[i].column()+1, results[i].row()+1) > 0) {
	LOG(logDEBUG) << "ROC/col/row = " << int(results[i].roc()) << "/" << int(results[i].column()) << "/" << int(results[i].row())
		     << " with = " << h2->GetBinContent(results[i].column()+1, results[i].row()+1)
		     << " now adding " << static_cast<float>(results[i].value());
      }
      h2->Fill(results[i].column(), results[i].row(), static_cast<float>(results[i].value())); 
    } else {
      LOG(logDEBUG) << "histogram for ROC " << (int)results[i].roc() << " not found"; 
    }
  }

  return maps; 
}


// ----------------------------------------------------------------------
vector<TH2D*> PixTest::efficiencyMaps(string name, uint16_t ntrig, uint16_t FLAGS) {

  vector<pixel> results;

  int cnt(0); 
  bool done = false;
  while (!done){
    LOG(logDEBUG) << "      attempt #" << cnt;
    try {
      results = fApi->getEfficiencyMap(FLAGS, ntrig);
      fNDaqErrors = fApi->daqGetNDecoderErrors();
      done = true; 
    } catch(pxarException &e) {
      fNDaqErrors = 666667;
      ++cnt;
    }
    done = (cnt>5) || done;
  }
  LOG(logDEBUG) << " eff result size = " << results.size(); 

  fDirectory->cd(); 
  vector<TH2D*> maps;
  TH2D *h2(0); 

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    LOG(logDEBUG) << "Create hist " << Form("%s_C%d", name.c_str(), rocIds[iroc]);
    h2 = bookTH2D(Form("%s_C%d", name.c_str(), rocIds[iroc]), Form("%s_C%d", name.c_str(), rocIds[iroc]), 52, 0., 52., 80, 0., 80.);
    h2->SetMinimum(0.); 
    h2->SetDirectory(fDirectory); 
    fHistOptions.insert(make_pair(h2, "colz")); 
    setTitles(h2, "col", "row"); 
    maps.push_back(h2); 
  }
  
  int idx(-1); 
  for (unsigned int i = 0; i < results.size(); ++i) {
    idx = getIdxFromId(results[i].roc());
    // if (rocIds.end() != find(rocIds.begin(), rocIds.end(), idx)) {
    if (rocIds.end() != find(rocIds.begin(), rocIds.end(), results[i].roc())) {
      h2 = maps[idx];
      if (h2->GetBinContent(results[i].column()+1, results[i].row()+1) > 0) {
	LOG(logDEBUG) << "ROC/col/row = " << int(results[i].roc()) << "/" << int(results[i].column()) << "/" << int(results[i].row())
		     << " with = " << h2->GetBinContent(results[i].column()+1, results[i].row()+1)
		     << " now adding " << static_cast<float>(results[i].value());
      }
      h2->Fill(results[i].column(), results[i].row(), static_cast<float>(results[i].value())); 
    } else {
      LOG(logDEBUG) << "histogram for ROC " << (int)results[i].roc() << " not found"; 
    }
  }

  return maps; 
}



// ----------------------------------------------------------------------
vector<TH1*> PixTest::thrMaps(string dac, string name, int ntrig, uint16_t flag) {
  return thrMaps(dac, name, 1, 0, ntrig, flag); 
}



// ----------------------------------------------------------------------
vector<TH1*> PixTest::thrMaps(string dac, string name, uint8_t daclo, uint8_t dachi, int ntrig, uint16_t flag) {
  // use at your own risk; pxarCore::getThresholdMap may or may not work as intended
  vector<TH1*> resultMaps; 

  if (daclo > dachi) {
    LOG(logWARNING) << "thrMaps called with dacLo = " << daclo << " > dacHi = " << dachi; 
    return resultMaps;
  }

  uint16_t FLAGS = flag | FLAG_RISING_EDGE;    
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
  
  std::vector<pixel> results;
  
  int cnt(0); 
  bool done = false;
  while (!done){
    LOG(logDEBUG) << "      attempt #" << cnt;
    try {
      results = fApi->getThresholdMap(dac, 1, daclo, dachi, FLAGS, ntrig);
      fNDaqErrors = fApi->daqGetNDecoderErrors();
      done = true; 
    } catch(pxarException &/*e*/) {
      fNDaqErrors = 666667;
      ++cnt;
    }
    done = (cnt>2) || done;
  }

  LOG(logDEBUG) << "finished threshold map for dac = " << dac << " results size = " << results.size(); 
  for (unsigned int ipix = 0; ipix < results.size(); ++ipix) {
    ic =   results[ipix].column(); 
    ir =   results[ipix].row(); 
    iroc = getIdxFromId(results[ipix].roc()); 
    val =  results[ipix].value();
    if (rocIds.end() != find(rocIds.begin(), rocIds.end(), results[ipix].roc())) {
      ((TH2D*)resultMaps[iroc])->Fill(ic, ir, val); 
    } else {
      LOG(logDEBUG) << "histogram for ROC " << static_cast<int>(results[ipix].roc()) << " not found"; 
    }
  }

  for (unsigned int i = 0; i < rocIds.size(); ++i){
    TH2D *h2 = (TH2D*)resultMaps[i];
    TH1* d1 = distribution(h2, 256, 0., 256.); 
    resultMaps.push_back(d1); 
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
string PixTest::getParameter(std::string parName) {
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (0 == fParameters[i].first.compare(parName)) {
      return fParameters[i].second; 
    }
  }
  return string(Form("parameter %s not found", parName.c_str())); 
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
      fval = static_cast<float>(atof(fParameters[i].first.c_str())); 
      break;
    }
  }
  return found; 
}


// ----------------------------------------------------------------------
void PixTest::addSelectedPixels(string sval) {
  bool reset(false), alreadyIn(false);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (!fParameters[i].first.compare("pix") && !fParameters[i].second.compare("reset")) {
      fParameters[i].second = sval; 
      reset = true; 
      break;
    }
    if (!fParameters[i].first.compare("pix") && !fParameters[i].second.compare(sval)) {
      alreadyIn = true; 
      break;
    }
  }
  if (!reset && !alreadyIn) fParameters.push_back(make_pair("pix", sval)); 
}


// ----------------------------------------------------------------------
void PixTest::clearSelectedPixels() {
  fPIX.clear(); 
  std::vector<std::pair<std::string, std::string> > pnew;   
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (0 == fParameters[i].first.compare("pix")) {
      pnew.push_back(make_pair("pix", "reset")); 
    } else {
      pnew.push_back(make_pair(fParameters[i].first, fParameters[i].second));
    }
  }
  fParameters.clear();
  fParameters = pnew; 
}


// ----------------------------------------------------------------------
bool PixTest::setTestParameter(string parname, string value) {
  
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (!fParameters[i].first.compare(parname)) {
      fParameters[i].second = value; 
      LOG(logDEBUG) << " setting  " << fParameters[i].first << " to new value " << fParameters[i].second;
    }
    return true;
  }
  
  return false; 
}


// ----------------------------------------------------------------------
void PixTest::dumpParameters() {
  LOG(logINFO) << "Parameters for test " << getName() << ", number of parameters = " << fParameters.size();
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
void PixTest::hvOn() {
  //  cout << "PixTest::hvOn()" << endl;
  Emit("hvOn()"); 
}

// ----------------------------------------------------------------------
void PixTest::hvOff() {
  //  cout << "PixTest::hvOff()" << endl;
  Emit("hvOff()"); 
}

// ----------------------------------------------------------------------
void PixTest::powerOn() {
  //  cout << "PixTest::powerOn()" << endl;
  Emit("powerOn()"); 
}

// ----------------------------------------------------------------------
void PixTest::powerOff() {
  //  cout << "PixTest::powerOff()" << endl;
  Emit("powerOff()"); 
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
  
  double plaVal = h->GetMaximum();
  double thrVal = 0.5*plaVal;
  for (int ibin = 1; ibin < h->GetNbinsX(); ++ibin) {
    if (h->GetBinContent(ibin) >= thrVal) {
      if (h->GetBinContent(ibin+1) < thrVal) continue;
      return static_cast<int>(h->GetBinCenter(ibin)); 
    }
  }
  return -1;
}


// ----------------------------------------------------------------------
bool PixTest::threshold(TH1 *h) {

  TF1 *f = fPIF->errScurve(h); 

  double lo, hi; 
  f->GetRange(lo, hi); 

  if (fPIF->doNotFit()) {
    fThreshold  = f->GetParameter(0); 
    fThresholdE = 0.3;
    fSigma      = 0.;
    fSigmaE     = 0.;
  } else {
    h->Fit(f, "qr", "", lo, hi); 
    fThreshold  = f->GetParameter(0); 
    fThresholdE = f->GetParError(0); 
    fSigma      = 1./(TMath::Sqrt(2.)/f->GetParameter(1)); 
    fSigmaE     = fSigma * f->GetParError(1) / f->GetParameter(1);
  }

  fThresholdN = h->FindLastBinAbove(0.5*h->GetMaximum()); 
  
  if (fThreshold < h->GetBinLowEdge(1)) {
    fThreshold  = -2.; 
    fThresholdE = -2.; 
    fSigma  = -2.; 
    fSigmaE = -2.; 
    fThresholdN = -2.;
    return false;
  }

  if (fThreshold > h->GetBinLowEdge(h->GetNbinsX())) {
    fThreshold  = h->GetBinLowEdge(h->GetNbinsX()); 
    fThresholdE = -1.; 
    fSigma  = -1.; 
    fSigmaE = -1.; 
    fThresholdN = fThreshold;
    return false;
  }

  return true;
}


// ----------------------------------------------------------------------
TH1D* PixTest::distribution(TH2D* h2, int nbins, double xmin, double xmax) {
  TH1D *h1 = new TH1D(Form("dist_%s", h2->GetName()), Form("dist_%s", h2->GetName()), nbins, xmin, xmax); 
  for (int ix = 0; ix < h2->GetNbinsX(); ++ix) {
    for (int iy = 0; iy < h2->GetNbinsY(); ++iy) {
      h1->Fill(h2->GetBinContent(ix+1, iy+1)); 
    }
  }
  return h1; 
}


// ----------------------------------------------------------------------
void PixTest::cacheDacs(bool verbose) {
  fDacCache.clear();
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned i = 0; i < rocIds.size(); ++i) {
    fDacCache.push_back(fApi->_dut->getDACs(rocIds[i]));
    if (verbose) fApi->_dut->printDACs(i);
  }
}


// ----------------------------------------------------------------------
void PixTest::restoreDacs(bool verbose) {

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    vector<pair<string, uint8_t> >  rocDacs = fDacCache[iroc];
    for (unsigned int idac = 0; idac < rocDacs.size(); ++idac) {
      fApi->setDAC(rocDacs[idac].first, rocDacs[idac].second, rocIds[iroc]);
    }
    if (verbose) fApi->_dut->printDACs(rocIds[iroc]);
  }
  fDacCache.clear();
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
      fApi->_dut->testPixel(i, 5 + i/2, true);  
      fApi->_dut->maskPixel(i, 5 + i/2, false);  
      ++cnt;
      fApi->_dut->testPixel(i, 15 + i/2, true);  
      fApi->_dut->maskPixel(i, 15 + i/2, false);  
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
  //  LOG(logDEBUG) << "bookTH1D " << Form("%s_V%d", sname.c_str(), cnt);
  return new TH1D(Form("%s_V%d", sname.c_str(), cnt), Form("%s (V%d)", title.c_str(), cnt), nbins, xmin, xmax); 
} 


// ----------------------------------------------------------------------
TH2D* PixTest::bookTH2D(std::string sname, std::string title, int nbinsx, double xmin, double xmax, 
			int nbinsy, double ymin, double ymax) {
  int cnt = histCycle(sname); 
  //  LOG(logDEBUG) << "bookTH2D " << Form("%s_V%d", sname.c_str(), cnt);
  return new TH2D(Form("%s_V%d", sname.c_str(), cnt), Form("%s (V%d)", title.c_str(), cnt), nbinsx, xmin, xmax, nbinsy, ymin, ymax); 
}

// ----------------------------------------------------------------------
TProfile2D* PixTest::bookTProfile2D(std::string sname, std::string title, int nbinsx, double xmin, double xmax, 
				    int nbinsy, double ymin, double ymax, string option) {
  int cnt = histCycle(sname); 
  //  LOG(logDEBUG) << "bookTH2D " << Form("%s_V%d", sname.c_str(), cnt);
  return new TProfile2D(Form("%s_V%d", sname.c_str(), cnt), Form("%s (V%d)", title.c_str(), cnt), 
			nbinsx, xmin, xmax, nbinsy, ymin, ymax, option.c_str()); 
}


// ----------------------------------------------------------------------
int PixTest::histCycle(string hname) {
  TH1* h(0); 
  int cnt(0); 
  if (fPixSetup->doRootFileUpdate()) fDirectory->ReadAll();
  h = (TH1*)fDirectory->FindObject(Form("%s_V%d", hname.c_str(), cnt));
  //   TKey *k(0); 
  //   k = (TKey*)fDirectory->FindKey(Form("%s_V%d", hname.c_str(), cnt)); 
  //   cout << k << endl;
  //   if (k) h = (TH1*)k->ReadObj();
  while (h) {
    ++cnt;
    h = (TH1*)fDirectory->FindObject(Form("%s_V%d", hname.c_str(), cnt));
    //     k = (TKey*)fDirectory->FindKey(Form("%s_V%d", hname.c_str(), cnt)); 
    //     cout << k << endl;
    //     if (k) h = (TH1*)k->ReadObj();
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

  //  uint16_t FLAGS = FLAG_FORCE_MASKED | FLAG_FORCE_SERIAL;
  uint16_t FLAGS = FLAG_FORCE_MASKED;

  vector<pair<uint8_t, vector<pixel> > > scans;
  int cnt(0); 
  bool done = false;
  while (!done){
    LOG(logDEBUG) << "      attempt #" << cnt;
    try {
      scans = fApi->getEfficiencyVsDAC("vthrcomp", 0, 255, FLAGS, ntrig);
      fNDaqErrors = fApi->daqGetNDecoderErrors();
      done = true; 
    } catch(pxarException &e) {
      fNDaqErrors = 666667;
      ++cnt;
    }
    done = (cnt>2) || done;
  }


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
      idx = getIdxFromId(vpix[ipix].roc()); 
      if (scanHists[idx]) {
	scanHists[idx]->Fill(idac, vpix[ipix].value()); 
      } else {
	LOG(logDEBUG) << "histogram for ROC " << vpix[ipix].roc() << " not found" << endl;
      }
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
vector<int> PixTest::getMinimumVthrComp(vector<TH1*>maps, int reserve, double nsigma) {
  vector<int> results; 
 
  TH2D *h2(0), *hn(0); 
  string hname(""); 
  for (unsigned int i = 0; i < maps.size(); ++i) {
    double minThr(999.), thr(0.), minThrN(999.), result(-1.); 
    h2 = (TH2D*)maps[i];
    hname = h2->GetName();
    if (string::npos == hname.find("thr_")) continue;
    hn = (TH2D*)maps[i+2]; 
    PixUtil::replaceAll(hname, "thr_", "thn_"); 
    if (strcmp(hname.c_str(), hn->GetName())) {
      LOG(logDEBUG) << "XXX problem in the ordering of the scurveMaps results ThrN map has name " << hn->GetName(); 
      continue;
    }
    TH1* d1 = distribution(h2, 256, 0., 256.); 
    TH1* dn = distribution(hn, 256, 0., 256.); 
    double minThrLimit = TMath::Max(1., d1->GetMean() - nsigma*d1->GetRMS());
    double minThrNLimit = TMath::Max(1., dn->GetMean() - nsigma*dn->GetRMS());
    delete d1; 
    delete dn; 
    for (int ic = 0; ic < 52; ++ic) {
      for (int ir = 0; ir < 80; ++ir) {
	thr = h2->GetBinContent(ic+1, ir+1); 
	if (thr < minThr && thr > minThrLimit) {
	  minThr = thr; 
	}

	thr = hn->GetBinContent(ic+1, ir+1); 
	if (thr < minThrN && thr > minThrNLimit) {
	  minThrN = thr; 
	}
      }
    }
    if (minThrN - reserve < minThr) { 
      result = minThrN - 10; 
      LOG(logDEBUG) << "minThr = " << minThr << " minThrN = " << minThrN << " -> result = " << result;
    } else {
      result = minThr; 
      LOG(logDEBUG) << "minThr = " << minThr << " minThrLimit = " << minThrLimit  << " minThrNLimit = " << minThrNLimit 
		    << " -> result = " << result << " -> " << static_cast<int>(result);
    }
    results.push_back(static_cast<int>(result)); 
  }
  return results; 
}


// ----------------------------------------------------------------------
double PixTest::getMinimumThreshold(vector<TH1*>maps) {
  double val(0.), result(999.);
  TH2D *h2(0); 
  for (unsigned int i = 0; i < maps.size(); ++i) {
    h2 = (TH2D*)maps[i];
    for (int ic = 0; ic < h2->GetNbinsX(); ++ic) {
      for (int ir = 0; ir < h2->GetNbinsY(); ++ir) {
	val = h2->GetBinContent(ic+1, ir+1); 
	if (val > 0 && val < result) result = val; 
      }
    }
  }
  if (result < 0.) result = 0.;
  return result;
}

// ----------------------------------------------------------------------
double PixTest::getMaximumThreshold(vector<TH1*>maps) {
  double result(-999.);
  TH2D *h2(0); 
  for (unsigned int i = 0; i < maps.size(); ++i) {
    h2 = (TH2D*)maps[i];
    double maxi = h2->GetMaximum(); 
    if (maxi > result) result = maxi; 
  }
  if (result > 255.) result = 255.;
  return result;
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
int PixTest::getIdxFromId(int id) {
  if (fId2Idx.count(id) > 0) {
    return fId2Idx[id]; 
  }
  return -1;
}


// ----------------------------------------------------------------------
void PixTest::output4moreweb() {

}

// ----------------------------------------------------------------------
vector<TH1*> PixTest::mapsWithString(vector<TH1*>maps, string name) {
  vector<TH1*> results; 
  string hname(""); 
  for (unsigned i = 0; i <  maps.size(); ++i) {
    hname = maps[i]->GetName(); 
    if (string::npos != hname.find(name)) results.push_back(maps[i]); 
  }
  return results; 
}

// ----------------------------------------------------------------------
vector<TH2D*> PixTest::mapsWithString(vector<TH2D*>maps, string name) {
  vector<TH2D*> results; 
  string hname(""); 
  for (unsigned i = 0; i <  maps.size(); ++i) {
    hname = maps[i]->GetName(); 
    if (string::npos != hname.find(name)) results.push_back(maps[i]); 
  }
  return results; 
}

// ----------------------------------------------------------------------
void PixTest::fillDacHist(vector<pair<uint8_t, vector<pixel> > > &results, TH1D *h, int icol, int irow, int iroc) {
  h->Reset();
  int ri(-1), ic(-1), ir(-1); 
  for (unsigned int idac = 0; idac < results.size(); ++idac) {
    int dac = results[idac].first; 
    for (unsigned int ipix = 0; ipix < results[idac].second.size(); ++ipix) {
      ri = results[idac].second[ipix].roc(); 
      ic = results[idac].second[ipix].column(); 
      ir = results[idac].second[ipix].row(); 
      if (iroc > -1 && ri != iroc) continue;
      if (icol > -1 && ic != icol) continue;
      if (irow > -1 && ir != irow) continue;
      if (ic > 51 || ir > 79) continue;

      h->Fill(dac, results[idac].second[ipix].value()); 
    }
  }

}

// ----------------------------------------------------------------------
void PixTest::bigBanner(string what, TLogLevel log) {
  LOG(log) << "######################################################################";
  LOG(log) << what; 
  LOG(log) << "######################################################################";
}

// ----------------------------------------------------------------------
void PixTest::banner(string what, TLogLevel log) {
  LOG(log) << "   ----------------------------------------------------------------------";
  LOG(log) << "   " << what; 
  LOG(log) << "   ----------------------------------------------------------------------";
}

// ----------------------------------------------------------------------
void PixTest::print(string what, TLogLevel log) {
  LOG(log) << "---> " << what; 
}


// ----------------------------------------------------------------------
void PixTest::dacScan(string dac, int ntrig, int dacmin, int dacmax, std::vector<shist256*> maps, int ihit, int flag) {
  uint16_t FLAGS = flag | FLAG_FORCE_MASKED;

  fNtrig = ntrig; 

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 

  int ic, ir, iroc; 
  double val;
  bool done = false;
  int cnt(0); 
  vector<pair<uint8_t, vector<pixel> > > results;
  
  if (2 == ihit) {
    LOG(logDEBUG) << "determine PH error: " << dacmin << " .. " << dacmax; 
    getPhError(dac, dacmin, dacmax, FLAGS, fNtrig); 
  }

  while (!done){
    LOG(logDEBUG) << "      attempt #" << cnt;
    try{
      if (1 == ihit) {
	results = fApi->getEfficiencyVsDAC(dac, dacmin, dacmax, FLAGS, fNtrig); 
	fNDaqErrors = fApi->daqGetNDecoderErrors();
      } else {
	results = fApi->getPulseheightVsDAC(dac, dacmin, dacmax, FLAGS, fNtrig); 
	fNDaqErrors = fApi->daqGetNDecoderErrors();
      }
      done = true;
    } catch(pxarException &e) {
      fNDaqErrors = 666667;
      ++cnt;
    }
    done = (cnt>2) || done;
  }
  
  int idx(0); 
  for (unsigned int idac = 0; idac < results.size(); ++idac) {
    int dac = results[idac].first; 
    for (unsigned int ipix = 0; ipix < results[idac].second.size(); ++ipix) {
      ic =   results[idac].second[ipix].column(); 
      ir =   results[idac].second[ipix].row(); 
      iroc = results[idac].second[ipix].roc(); 
      if (ic > 51 || ir > 79) {
	LOG(logDEBUG) << "bad pixel address encountered: ROC/col/row = " << iroc << "/" << ic << "/" << ir;
	continue;
      }
      val =  results[idac].second[ipix].value();
      idx = PixUtil::rcr2idx(getIdxFromId(iroc), ic, ir);
      if (idx > -1) maps[idx]->fill(dac, val);
    }
  }
  
}


// ----------------------------------------------------------------------
void PixTest::scurveAna(string dac, string name, vector<shist256*> maps, vector<TH1*> &resultMaps, int result) {
  fDirectory->cd(); 
  TH1* h2(0), *h3(0), *h4(0); 
  string fname("SCurveData");
  ofstream OutputFile;
  string line; 
  string empty("32  93   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0 ");
  bool dumpFile(false); 
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  int roc(0), ic(0), ir(0); 
  TH1D *h1 = new TH1D("h1", "h1", 256, 0., 256.); h1->Sumw2(); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    LOG(logDEBUG) << "analyzing ROC " << static_cast<int>(rocIds[iroc]);
    h2 = bookTH2D(Form("thr_%s_%s_C%d", name.c_str(), dac.c_str(), rocIds[iroc]), 
		  Form("thr_%s_%s_C%d", name.c_str(), dac.c_str(), rocIds[iroc]), 
		  52, 0., 52., 80, 0., 80.); 
    fHistOptions.insert(make_pair(h2, "colz")); 

    h3 = bookTH2D(Form("sig_%s_%s_C%d", name.c_str(), dac.c_str(), rocIds[iroc]), 
		  Form("sig_%s_%s_C%d", name.c_str(), dac.c_str(), rocIds[iroc]), 
		  52, 0., 52., 80, 0., 80.); 
    fHistOptions.insert(make_pair(h3, "colz")); 

    h4 = bookTH2D(Form("thn_%s_%s_C%d", name.c_str(), dac.c_str(), rocIds[iroc]), 
		  Form("thn_%s_%s_C%d", name.c_str(), dac.c_str(), rocIds[iroc]), 
		  52, 0., 52., 80, 0., 80.); 
    fHistOptions.insert(make_pair(h4, "colz")); 

    string lname(name); 
    std::transform(lname.begin(), lname.end(), lname.begin(), ::tolower);
    if (!name.compare("scurveVcal") || !lname.compare("scurvevcal")) {
      dumpFile = true; 
      OutputFile.open(Form("%s/%s_C%d.dat", fPixSetup->getConfigParameters()->getDirectory().c_str(), fname.c_str(), iroc));
      OutputFile << "Mode 1 " << "Ntrig " << getParameter("ntrig") << endl;
    }


    for (unsigned int i = iroc*4160; i < (iroc+1)*4160; ++i) {
      if (maps[i]->getSumOfWeights() < 1) {
	if (dumpFile) OutputFile << empty << endl;
	continue;
      }
      
      // -- calculated "proper" errors
      for (int ib = 0; ib <= 256; ++ib) {
	h1->SetBinContent(ib, maps[i]->get(ib));
	h1->SetBinError(ib, fNtrig*PixUtil::dBinomial(static_cast<int>(maps[i]->get(ib)), fNtrig)); 
      }

      bool ok = threshold(h1); 
      if (result&0x10 && !ok) {
	TH1D *h1c = (TH1D*)h1->Clone(Form("c%d_r%d_C%d", ic, ir, rocIds[iroc])); 
	h1c->SetTitle(Form("problematic Scurve Thr (c%d_r%d_C%d), thr = %4.3f", ic, ir, rocIds[iroc], fThreshold));
	fHistList.push_back(h1c); 
      }
      PixUtil::idx2rcr(i, roc, ic, ir);
      h2->SetBinContent(ic+1, ir+1, fThreshold); 
      h2->SetBinError(ic+1, ir+1, fThresholdE); 

      h3->SetBinContent(ic+1, ir+1, fSigma); 
      h3->SetBinError(ic+1, ir+1, fSigmaE); 

      h4->SetBinContent(ic+1, ir+1, fThresholdN); 

      // -- write file
      if (dumpFile) {
	int NSAMPLES(32); 
	int ibin = h1->FindBin(fThreshold); 
	int bmin = ibin - 15;
	line = Form("%2d %3d", NSAMPLES, bmin); 
	for (int ix = bmin; ix < bmin + NSAMPLES; ++ix) {
	  line += string(Form(" %3d", static_cast<int>(h1->GetBinContent(ix+1)))); 
	}
	OutputFile << line << endl;
      }
    }
    if (dumpFile) OutputFile.close();

    if (result & 0x1) {
      resultMaps.push_back(h2); 
      fHistList.push_back(h2); 
    }
    if (result & 0x2) {
      resultMaps.push_back(h3); 
      fHistList.push_back(h3); 
    } 
    if (result & 0x4) {
      resultMaps.push_back(h4); 
      fHistList.push_back(h4); 
    }

    if (result & 0x8) {
      if (result & 0x1) {
	TH1* d1 = distribution((TH2D*)h2, 256, 0., 256.); 
	resultMaps.push_back(d1); 
	fHistList.push_back(d1); 
      }
      if (result & 0x2) {
	TH1* d2 = distribution((TH2D*)h3, 100, 0., 6.); 
	resultMaps.push_back(d2); 
	fHistList.push_back(d2); 
      }
      if (result & 0x4) {
	TH1* d3 = distribution((TH2D*)h4, 256, 0., 256.); 
	resultMaps.push_back(d3); 
	fHistList.push_back(d3); 
      }
    }

  }

  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);

  delete h1;

  if (h2) h2->Draw("colz");
  PixTest::update(); 
  
}

// ----------------------------------------------------------------------
void PixTest::getPhError(std::string /*dac*/, int /*dacmin*/, int /*dacmax*/, int /*FLAGS*/, int /*ntrig*/) {

  // -- initialize to 5% constant error
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    fPhErrP0.push_back(0.05);
    fPhErrP1.push_back(0.0);
    //    h0 = new TH1D(Form("phErr_C%d", rocIds[iroc]), Form("phErr_C%d", rocIds[iroc]), 256, 0., 256.); 
    //    maps.push_back(h0); 
  }

  return; 
}

// ----------------------------------------------------------------------
void PixTest::saveDacs() {

  vector<uint8_t> rocs = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocs.size(); ++iroc) {
    fPixSetup->getConfigParameters()->writeDacParameterFile(rocs[iroc], fApi->_dut->getDACs(iroc)); 
  }

}

// ----------------------------------------------------------------------
void PixTest::saveTrimBits() {
  vector<uint8_t> rocs = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocs.size(); ++iroc) {
    fPixSetup->getConfigParameters()->writeTrimFile(rocs[iroc], fApi->_dut->getEnabledPixels(rocs[iroc])); 
  }
  
}

// ----------------------------------------------------------------------
void PixTest::saveTbParameters() {
  LOG(logDEBUG) << "save Tb parameters"; 
  fPixSetup->getConfigParameters()->writeTbParameterFile();
}

// ----------------------------------------------------------------------
vector<vector<pair<int, int> > > PixTest::deadPixels(int ntrig) {
  vector<vector<pair<int, int> > > deadPixels;
  vector<pair<int, int> > deadPixelsRoc;
  
  vector<uint8_t> vVcal = getDacs("vcal"); 
  vector<uint8_t> vCreg = getDacs("ctrlreg"); 

  fApi->setDAC("vcal", 200); 
  fApi->setDAC("ctrlreg", 4); 
  
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  vector<TH2D*> testEff = efficiencyMaps("deadPixels", ntrig);
  std::pair<int, int> badPix;
  Double_t eff(0.);
  
  for (unsigned int i = 0; i < testEff.size(); ++i) {
    deadPixelsRoc.clear();
    for(int r=0; r<80; r++){
      for(int c=0; c<52; c++){
	eff = testEff[i]->GetBinContent( testEff[i]->FindFixBin((double)c + 0.5, (double)r+0.5) );
	if (eff<ntrig){
	  LOG(logDEBUG) << "ROC idx = " << i << " pixel "<<c<<", "<<r<<" has eff "<<eff<<"/"<<ntrig << ";  blacklisting";
	  badPix.first = c;
	  badPix.second = r;
	  deadPixelsRoc.push_back(badPix);
	}
      }
    }
    deadPixels.push_back(deadPixelsRoc);
  }

  setDacs("vcal", vVcal); 
  setDacs("ctrlreg", vCreg); 
  
  return deadPixels;
}


// ----------------------------------------------------------------------
vector<uint8_t> PixTest::getDacs(string dacName) {
  vector<uint8_t> result; 
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int i = 0; i < rocIds.size(); ++i) {
    result.push_back(fApi->_dut->getDAC(rocIds[i], dacName)); 
  }
  return result;
}

// ----------------------------------------------------------------------
string PixTest::getDacsString(std::string dacName) {
  vector<uint8_t> v = getDacs(dacName); 
  stringstream s; 
  unsigned int vsize = v.size();
  for (unsigned int i = 0; i < vsize; ++i) {
    s << static_cast<int>(v[i]); 
    if (i < vsize-1) s << " "; 
  }
  return s.str(); 
}


// ----------------------------------------------------------------------
void PixTest::setDacs(string dacName, vector<uint8_t> v) {
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int i = 0; i < rocIds.size(); ++i) {
    fApi->setDAC(dacName, v[i], i); 
  }
}


// ----------------------------------------------------------------------
vector<pair<int,int> > PixTest::checkHotPixels(TH2D* h) {
  vector<pair<int,int> > hotPixels(0);
  double mean(0.);
  for (int ix = 0; ix < h->GetNbinsX(); ++ix) {
    for (int iy = 0; iy < h->GetNbinsY(); ++iy) {
      mean += h->GetBinContent(ix+1, iy+1); 
    }
  }
  mean /= h->GetNbinsX()*h->GetNbinsY();

  double fos = 6.0;
  double bc(0.);
  double hitThr = (mean < 10.?10.:3.*mean);
  LOG(logDEBUG) << "average number of unexpected hits per pixel: " 
		<< mean
		<< " hit threshold: " << hitThr;
  for (int ix = 0; ix < h->GetNbinsX(); ++ix) {
    for (int iy = 0; iy < h->GetNbinsY(); ++iy) {
      bc = h->GetBinContent(ix+1, iy+1);
      if (bc > hitThr && bc > fos*mean) {
	hotPixels.push_back(make_pair(ix,iy));
	LOG(logDEBUG) << "Found a hot pixel with " << bc << " hits "
		      << " and masking Pixel (" << ix << ',' << iy << ')';
      }
    }
  }
  return hotPixels;
}


// ----------------------------------------------------------------------
pair<vector<TH2D*>,vector<TH2D*> > PixTest::xEfficiencyMaps(string name, uint16_t ntrig, uint16_t FLAGS) {

  vector<pixel> results;
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();   
  int cnt(0); 
  bool done = false;
  while (!done){
    LOG(logDEBUG) << "      attempt #" << cnt;
    try {
      results = fApi->getEfficiencyMap(FLAGS, ntrig);
      fNDaqErrors = fApi->daqGetNDecoderErrors();
      done = true; 
    } catch(pxarException &e) {
      fNDaqErrors = 666667;
      ++cnt;
    }
    done = (cnt>2) || done;
  }
  LOG(logDEBUG) << " eff result size = " << results.size() << " (should be 4160-#dead pixels + #unexpected hits)"; 

  fDirectory->cd(); 
  vector<TH2D*> maps;
  vector<TH2D*> xMaps;
  TH2D *h2(0),*h3(0); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    h2 = bookTH2D(Form("%s_calmap_C%d", name.c_str(), rocIds[iroc]), 
		  Form("%s_calmap_C%d", name.c_str(), rocIds[iroc]), 
		  52, 0., 52., 80, 0., 80.); 
    fHistOptions.insert(make_pair(h2,"colz"));
    h3 = bookTH2D(Form("%s_xraymap_C%d", name.c_str(), rocIds[iroc]), 
		  Form("%s_xraymap_C%d", name.c_str(), rocIds[iroc]), 
		  52, 0., 52., 80, 0., 80.); 
    fHistOptions.insert(make_pair(h3,"colz"));
    h2->SetMinimum(0.);
    h3->SetMinimum(0.);
    
    h2->SetDirectory(fDirectory); 
    h3->SetDirectory(fDirectory);
    
    setTitles(h2, "col", "row"); 
    setTitles(h3, "col", "row"); 
        
    maps.push_back(h2); 
    xMaps.push_back(h3);
  }

  int idx(-1);  
  for (unsigned int i = 0; i < results.size(); ++i) {
    idx = getIdxFromId(results[i].roc());
    if (rocIds.end() != find(rocIds.begin(), rocIds.end(), idx)) {
      h2 = maps[idx];
      h3 = xMaps[idx];
      if (FLAGS | FLAG_CHECK_ORDER) {
	if (results[i].value() > 0) {
	  h2->Fill(results[i].column(), results[i].row(), static_cast<float>(results[i].value())); 
	} 
	else { 
	  //add a hit to the X-ray counter if a hit comes in out of order
	  h3->Fill(results[i].column(), results[i].row(), 1);
        }
      } 
      else {
	h2->Fill(results[i].column(), results[i].row(), static_cast<float>(results[i].value())); 
      } 
    }
    else {
      LOG(logDEBUG) << "histogram for ROC " << (int)results[i].roc() << " not found"; 
    }
  }
  LOG(logDEBUG) << "Size of results from : PixTestHighRate::xEfficiencyMaps" << results.size();
  return make_pair(maps, xMaps); 
}


// ----------------------------------------------------------------------
void PixTest::maskPixels() {
  string mfile = fPixSetup->getConfigParameters()->getDirectory() + "/" + fPixSetup->getConfigParameters()->getMaskFileName();
  vector<vector<pair<int, int> > > vmask = fPixSetup->getConfigParameters()->readMaskFile(mfile); 

  for (unsigned int i = 0; i < vmask.size(); ++i) {
    vector<pair<int, int> > mask = vmask[i]; 
    for (unsigned int ipix = 0; ipix < mask.size(); ++ipix) {
      LOG(logINFO) << "ROC " << getIdFromIdx(i) << " masking pixel " << mask[ipix].first << "/" << mask[ipix].second; 
      fApi->_dut->maskPixel(mask[ipix].first, mask[ipix].second, true, getIdFromIdx(i)); 
    }
  }

}


// ----------------------------------------------------------------------
void PixTest::pgToDefault() {
  fPg_setup.clear();
  fPg_setup = fPixSetup->getConfigParameters()->getTbPgSettings();
  fApi->setPatternGenerator(fPg_setup);
  LOG(logINFO) << "PixTest::       pg_setup set to default.";
}

// ----------------------------------------------------------------------
void PixTest::finalCleanup() {
  pgToDefault();
  fPg_setup.clear();
}

// ----------------------------------------------------------------------
void PixTest::resetROC() {
  // -- setup DAQ for data taking
  fPg_setup.clear();
  fPg_setup.push_back(make_pair("resetroc", 0)); // PG_RESR b001000
  uint16_t period = 28;
  fApi->setPatternGenerator(fPg_setup);
  fApi->daqStart();
  fApi->daqTrigger(1, period);
  LOG(logDEBUG) << "PixTest: resetROC sent once.";
  fApi->daqStop();
  fPg_setup.clear();
}

// ----------------------------------------------------------------------
void PixTest::resetTBM() {
  // -- setup DAQ for data taking
  fPg_setup.clear();
  fPg_setup.push_back(make_pair("resettbm", 0)); // PG_RESR b001000
  uint16_t period = 28;
  fApi->setPatternGenerator(fPg_setup);
  fApi->daqStart();
  fApi->daqTrigger(1, period);
  LOG(logDEBUG) << "PixTest: resetTBM sent once.";
  fApi->daqStop();
  fPg_setup.clear();
}

// ----------------------------------------------------------------------
uint16_t PixTest::prepareDaq(int triggerFreq, uint8_t trgTkDel) {
  resetROC(); 
  uint16_t totalPeriod = setTriggerFrequency(triggerFreq, trgTkDel);
  fApi->setPatternGenerator(fPg_setup);
  return totalPeriod;
}

// ----------------------------------------------------------------------
uint16_t PixTest::setTriggerFrequency(int triggerFreq, uint8_t trgTkDel) {

  uint16_t nDel = 0;
  uint16_t totalPeriod = 0;
  
  double period_ns = 1 / (double)triggerFreq * 1000000; // trigger frequency in kHz.
  double clkDelays = period_ns / 25 - trgTkDel;
  uint16_t ClkDelays = (uint16_t)clkDelays; // aproximate to defect
  
  fPg_setup.clear();
  
  // -- add right delay between triggers:
  uint16_t i = ClkDelays;
  while (i>255){
    fPg_setup.push_back(make_pair("delay", 255));
    i = i - 255;
    nDel++;
  }
  fPg_setup.push_back(make_pair("delay", i));
  
  // -- then send trigger and token:
  fPg_setup.push_back(make_pair("trg", trgTkDel));    // PG_TRG b000010
  fPg_setup.push_back(make_pair("tok", 0));    // PG_TOK
  if (0) for (unsigned int i = 0; i < fPg_setup.size(); ++i) cout << fPg_setup[i].first << ": " << (int)fPg_setup[i].second << endl;
  
  totalPeriod = ((uint16_t)period_ns / 25) + 4 + nDel; //+4 to align to the new pg minimum (1 additional clk cycle per PG call);
  return totalPeriod;
}

// ----------------------------------------------------------------------
void PixTest::maskHotPixels(std::vector<TH2D*> v) {

  int NSECONDS(10); 
  int TRGFREQ(100); // in kiloHertz

  banner(Form("PixTest::maskHotPixels() running for %d seconds with %d kHz trigger rate", NSECONDS, TRGFREQ));

  fHotPixels.clear(); 

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(false);

  int totalPeriod = prepareDaq(TRGFREQ, (uint8_t)500);
  
  timer t;
  uint8_t perFull;
  bool daq_loop = true;
    
  fApi->daqStart();

  int finalPeriod = fApi->daqTriggerLoop(totalPeriod);
  LOG(logINFO) << "PixTestHighRate::maskHotPixels start TriggerLoop with period " << finalPeriod 
	       << " and duration " << NSECONDS << " seconds and trigger rate " << TRGFREQ << " kHz";
  
  while (fApi->daqStatus(perFull) && daq_loop) {
    if (perFull > 80) {
      LOG(logINFO) << "Buffer almost full, pausing triggers.";
      fApi->daqTriggerLoopHalt();

      // fillMap(v):
      vector<pxar::Event> daqdat = fApi->daqGetEventBuffer();
      for(std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
	for (unsigned int ipix = 0; ipix < it->pixels.size(); ++ipix) {
	  v[getIdxFromId(it->pixels[ipix].roc())]->Fill(it->pixels[ipix].column(), it->pixels[ipix].row());
	}
      }

      LOG(logINFO) << "Resuming triggers.";
	  fApi->daqTriggerLoop(finalPeriod);
    }
    
    if (static_cast<int>(t.get()/1000) >= NSECONDS)	{
      LOG(logINFO) << "Done with hot pixel readout";
      daq_loop = false;
      break;
    }
  }
    
  fApi->daqTriggerLoopHalt();
  fApi->daqStop();

  // fillMap(v):
  vector<pxar::Event> daqdat = fApi->daqGetEventBuffer();
  for(std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
    for (unsigned int ipix = 0; ipix < it->pixels.size(); ++ipix) {
      v[getIdxFromId(it->pixels[ipix].roc())]->Fill(it->pixels[ipix].column(), it->pixels[ipix].row());
    }
  }

  finalCleanup();


  // -- analysis of hit map
  double THR = 1.e-4*NSECONDS*TRGFREQ*1000; 
  LOG(logDEBUG) << "hot pixel determination with THR = " << THR; 
  int cntHot(0); 
  TH2D *h(0); 
  for (unsigned int i = 0; i < v.size(); ++i) {
    h = v[i]; 
    vector<pair<int, int> > hot; 
    for (int ix = 0; ix < h->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < h->GetNbinsY(); ++iy) {
	if (h->GetBinContent(ix+1, iy+1) > THR) {
	  ++cntHot; 
	  LOG(logDEBUG) << "ROC " << i << " with hot pixel " << ix << "/" << iy << ",  hits = " << h->GetBinContent(ix+1, iy+1);
	  hot.push_back(make_pair(ix, iy)); 
	}
      }
    }
    fHotPixels.push_back(hot); 
  }
  if (0 == cntHot) {
    LOG(logDEBUG) << "no hot pixel found!";
  }
  LOG(logINFO) << "PixTest::maskHotPixels() done";

}
