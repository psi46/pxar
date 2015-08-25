#include <iostream>
#include <bitset>
#include <fstream>
#include <stdlib.h>     /* atof, atoi */

#include <TKey.h>
#include <TColor.h>
#include <TClass.h>
#include <TMinuit.h>
#include <TMath.h>
#include <TStyle.h>
#include <TGMsgBox.h>
#include "TVirtualFitter.h"

#include "PixTest.hh"
#include "PixUtil.hh"
#include "timer.h"
#include "log.h"
#include "helper.h"
#include "rsstools.hh"
#include "TStopwatch.h"

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
  fStopTest       = false;
  fOutputFilename = string(""); 

  fName = name;
  setToolTips();
  fParameters = a->getPixTestParameters()->getTestParameters(name); 
  fTree = 0; 

  fTriStateColors[0] = kRed;
  fTriStateColors[1] = 0;
  fTriStateColors[2] = kGreen;

//   TColor::InitializeColors();
//   Double_t stops[9] = { 0.0000, 0.1250, 0.2500, 0.3750, 0.5000, 0.6250, 0.7500, 0.8750, 1.0000};  
//   Double_t red[9]   = { 0.2082, 0.0592, 0.0780, 0.0232, 0.1802, 0.5301, 0.8186, 0.9956, 0.9764};
//   Double_t green[9] = { 0.1664, 0.3599, 0.5041, 0.6419, 0.7178, 0.7492, 0.7328, 0.7862, 0.9832};
//   Double_t blue[9]  = { 0.5293, 0.8684, 0.8385, 0.7914, 0.6425, 0.4662, 0.3499, 0.1968, 0.0539};
//   float alpha(1.); 
//   TColor::CreateGradientColorTable(9, stops, red, green, blue, 255, alpha);
  //  paletteType = ncolors;
      

  //  TVirtualFitter::SetDefaultFitter("Minuit2");

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
  fTreeEvent.npix = 0; 
  fTreeEvent.header = 0; 
  fTreeEvent.trailer = 0; 
  fTreeEvent.numDecoderErrors = 0; 
  fTreeEvent.dac = 0; 
  for (int ipix = 0; ipix < 20000; ++ipix) {
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
      fNDaqErrors = fApi->getStatistics().errors_pixel();
      done = true;
    } catch(pxarException &/*e*/) {
      ++cnt;
    }
    done = (cnt>5) || done;
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
vector<TH1*> PixTest::scurveMaps(string dac, string name, int ntrig, int dacmin, int dacmax, int dacsperstep, int ntrigperstep, 
				 int result, int ihit, int flag) {

  fNtrig = ntrig;

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  string type("hits"); 
  if (2 == ihit) type = string("pulseheight"); 
  print(Form("dac: %s name: %s ntrig: %d dacrange: %d .. %d (%d/%d) %s flags = %d (plus default)",  
	     dac.c_str(), name.c_str(), ntrig, dacmin, dacmax, dacsperstep, ntrigperstep, type.c_str(), flag)); 

  vector<shist256*>  maps; 
  vector<TH1*>       resultMaps; 
  resultMaps.clear();
  
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


  int ntrigMax(ntrig);
  if (ntrigperstep > 0) ntrigMax = ntrigperstep; 

  if (dacsperstep > 0) {
    int stepsize(dacsperstep); 
    int dacminAdj = dacmin; 
    int dacmaxAdj = dacmin + stepsize - 1;
    bool finalRun(false);
    while (dacmaxAdj <= dacmax) {
      LOG(logINFO) << "  dacScan step  from " << dacminAdj << " .. " << dacmaxAdj; 
      if (ntrig < ntrigMax) {
	LOG(logINFO) << "  dacScan in one run with ntrig =  " << ntrig; 
	gSystem->ProcessEvents();
	if (fStopTest) break;
	dacScan(dac, ntrig, dacminAdj, dacmaxAdj, maps, ihit, flag); 
      } else {
	LOG(logINFO) << "  dacScan split into " << ntrig/ntrigMax << " runs with ntrig = " << ntrigMax << (ntrig%ntrigMax > 0? " plus remainder": "");
	for (int i = 0; i < ntrig/ntrigMax; ++i) {
	  gSystem->ProcessEvents();
	  if (fStopTest) break;
	  LOG(logINFO) << "    run " << i+1 << " of " << ntrig/ntrigMax;
	  dacScan(dac, ntrigMax, dacminAdj, dacmaxAdj, maps, ihit, flag);
	}
	if (ntrig%ntrigMax > 0 && !fStopTest) {
	  LOG(logINFO) << "    remainder ";
	  dacScan(dac, ntrig%ntrigMax, dacminAdj, dacmaxAdj, maps, ihit, flag);
	}
	
      }
      if (fStopTest) break;
      if (finalRun) break;
      dacminAdj = dacminAdj + stepsize; 
      dacmaxAdj = dacminAdj + stepsize - 1;
      if (dacmaxAdj >= dacmax) {
	dacmaxAdj = dacmax; 
	finalRun = true;
      }
    }
  } else {
      if (ntrig < ntrigMax) {
	LOG(logINFO) << "  dacScan in one run with ntrig =  " << ntrig; 
	dacScan(dac, ntrig, dacmin, dacmax, maps, ihit, flag); 
      } else {
	LOG(logINFO) << "  dacScan split into " << ntrig/ntrigMax << " runs with ntrig = " << ntrigMax << (ntrig%ntrigMax > 0? " plus remainder": "");
	for (int i = 0; i < ntrig/ntrigMax; ++i) {
	  gSystem->ProcessEvents();
	  if (fStopTest) break;
	  LOG(logINFO) << "    run " << i+1 << " of " << ntrig/ntrigMax;
	  dacScan(dac, ntrigMax, dacmin, dacmax, maps, ihit, flag);
	}
	if (ntrig%ntrigMax > 0 && !fStopTest) {
	  LOG(logINFO) << "    remainder ";
	  dacScan(dac, ntrig%ntrigMax, dacmin, dacmax, maps, ihit, flag);
	}
	
      }
  }


  if (fNDaqErrors > 666000) {
    LOG(logDEBUG) << "PixTest::scurveMaps errors encountered, returning empty results";
    return resultMaps;
  }
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
      fNDaqErrors = fApi->getStatistics().errors_pixel();
      done = true; 
    } catch(pxarException &/*e*/) {
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
  LOG(logDEBUG) << "Create hists " 
		<< Form("%s_C%d", name.c_str(), rocIds[0]) 
		<< " .. " 
		<<  Form("%s_C%d", name.c_str(), rocIds[rocIds.size()-1]);
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
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
  vector<TH2D*> maps;
  TH2D *h2(0), *h3(0); 


  bool unmasked = (0 != (FLAGS & FLAG_CHECK_ORDER))  &&  (0 != (FLAGS & FLAG_FORCE_UNMASKED));

  int cnt(0); 
  bool done = false;
  while (!done){
    LOG(logDEBUG) << "      attempt #" << cnt;
    try {
      results = fApi->getEfficiencyMap(FLAGS, ntrig);
      fNDaqErrors = fApi->getStatistics().errors_pixel();
      done = true; 
    } catch(pxarException &/*e*/) {
      fNDaqErrors = 666667;
      ++cnt;
    }
    done = (cnt>5) || done;
  }
  LOG(logDEBUG) << " eff result size = " << results.size(); 

  fDirectory->cd(); 

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  LOG(logDEBUG) << "Create hists " 
		<< Form("%s_C%d", name.c_str(), rocIds[0]) 
		<< " .. " 
		<<  Form("%s_C%d", name.c_str(), rocIds[rocIds.size()-1]);
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    h2 = bookTH2D(Form("%s_C%d", name.c_str(), rocIds[iroc]), Form("%s_C%d", name.c_str(), rocIds[iroc]), 52, 0., 52., 80, 0., 80.);
    h2->SetMinimum(0.); 
    h2->SetDirectory(fDirectory); 
    fHistOptions.insert(make_pair(h2, "colz")); 
    setTitles(h2, "col", "row"); 
    maps.push_back(h2); 
  }
  
  if (unmasked) {
    LOG(logDEBUG) << "booking xray maps for unmasked detector"; 
    fXrayMaps.clear();
    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
      h3 = bookTH2D(Form("%s_xraymap_C%d", name.c_str(), rocIds[iroc]), 
		    Form("%s_xraymap_C%d", name.c_str(), rocIds[iroc]), 
		    52, 0., 52., 80, 0., 80.); 
      fHistOptions.insert(make_pair(h3, "colz"));
      h3->SetMinimum(0.);
      h3->SetDirectory(fDirectory);
      setTitles(h3, "col", "row"); 
      fXrayMaps.push_back(h3);
    }
  }
  
  int idx(-1); 
  for (unsigned int i = 0; i < results.size(); ++i) {
    idx = getIdxFromId(results[i].roc());
    if (rocIds.end() != find(rocIds.begin(), rocIds.end(), results[i].roc())) {
      h2 = maps[idx];
      if (unmasked) {
	h3 = fXrayMaps[idx];
	if (results[i].value() > 0) {
	  h2->Fill(results[i].column(), results[i].row(), static_cast<float>(results[i].value())); 
	} else { 
	  h3->Fill(results[i].column(), results[i].row(), 1);
        }
      } else {
	if (h2->GetBinContent(results[i].column()+1, results[i].row()+1) > 0) {
	  LOG(logDEBUG) << "ROC/col/row = " << int(results[i].roc()) << "/" << int(results[i].column()) << "/" << int(results[i].row())
			<< " with = " << h2->GetBinContent(results[i].column()+1, results[i].row()+1)
			<< " now adding " << static_cast<float>(results[i].value());
	}
	h2->Fill(results[i].column(), results[i].row(), static_cast<float>(results[i].value())); 
      }
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
      fNDaqErrors = fApi->getStatistics().errors_pixel();
      done = true; 
    } catch(pxarException &/*e*/) {
      fNDaqErrors = 666667;
      ++cnt;
    }
    done = (cnt>5) || done;
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
  //  LOG(logDEBUG) << "PixTestBase dtor(), writing out histograms";
  std::list<TH1*>::iterator il; 
  fDirectory->cd(); 
  for (il = fHistList.begin(); il != fHistList.end(); ++il) {
    //    LOG(logINFO) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory); 
    (*il)->Write(); 
  }

  TH1D *h = (TH1D*)gDirectory->Get("ha"); 
  if (h) {
    h->SetDirectory(fDirectory); 
    h->Write();
  }

  h = (TH1D*)gDirectory->Get("hd"); 
  if (h) {
    h->SetDirectory(fDirectory); 
    h->Write();
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
  fPixSetup->getPixMonitor()->update();
}

// ----------------------------------------------------------------------
void PixTest::hvOn() {
  if (fPixSetup->guiActive()) {
    LOG(logDEBUG) << "PixTest::hvOn() emit hvOn()";
    Emit("hvOn()"); 
  } else {
    LOG(logDEBUG) << "PixTest::hvOn() api::HVon()";
    fApi->HVon(); 
  }
}

// ----------------------------------------------------------------------
void PixTest::hvOff() {
  if (fPixSetup->guiActive()) {
    LOG(logDEBUG) << "PixTest::hvOff() emit hvOff()";
    Emit("hvOff()"); 
  } else {
    LOG(logDEBUG) << "PixTest::hvOff() api::HVoff()";
    fApi->HVoff(); 
  }
}

// ----------------------------------------------------------------------
void PixTest::powerOn() {
  if (fPixSetup->guiActive()) {
    LOG(logDEBUG) << "PixTest::powerOn() emit powerOn()";
    Emit("powerOn()");
  } else {
    LOG(logDEBUG) << "PixTest::hvOff() api::Pon()";
    fApi->Pon(); 
  }
}

// ----------------------------------------------------------------------
void PixTest::powerOff() {
  if (fPixSetup->guiActive()) {
    LOG(logDEBUG) << "PixTest::powerOff() emit powerOff()";
    Emit("powerOff()"); 
  } else {
    LOG(logDEBUG) << "PixTest::hvOff() api::Poff()";
    fApi->Poff(); 
  }
}


// ----------------------------------------------------------------------
void PixTest::doTest() {
  //  LOG(logINFO) << "PixTest::doTest()";
}

// ----------------------------------------------------------------------
void PixTest::fullTest() {
  doTest();
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
TH1* PixTest::nextHistV() {
  if (fHistList.size() == 0) return 0; 
  TH1* h0 = (*fDisplayedHist); 
  std::string histName(h0->GetName());
  size_t pos = histName.rfind("_V");
  if (pos != std::string::npos) {
    int currentV = atoi(histName.substr(pos+2).c_str());
    std::string histBaseName = histName.substr(0, pos);

    int maxV = 0;
    for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
      TH1* h1 = (*il); 
      std::string histName2 = h1->GetName();
      size_t pos2 = histName2.rfind("_V");
      if (pos2 != std::string::npos) {
        int checkV = atoi(histName2.substr(pos2+2).c_str());
        if (checkV > maxV) maxV = checkV;
      }
    }

    for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
      TH1* h1 = (*il); 
      std::string histName2 = h1->GetName();
      size_t pos2 = histName2.rfind("_V");
      if (pos2 != std::string::npos) {
        std::string histBaseName2 = histName2.substr(0, pos2);
        int checkV = atoi(histName2.substr(pos2+2).c_str());
        if (histBaseName == histBaseName2 && (checkV == currentV+1 || (currentV == maxV && checkV == 0))) {
          fDisplayedHist = il;
          return *il;
        }
      }  
    }
  }
  return 0;
}

// ----------------------------------------------------------------------
TH1* PixTest::previousHistV() {
  if (fHistList.size() == 0) return 0; 
  TH1* h0 = (*fDisplayedHist); 
  std::string histName(h0->GetName());
  size_t pos = histName.rfind("_V");
  if (pos != std::string::npos) {
    int currentV = atoi(histName.substr(pos+2).c_str());
    std::string histBaseName = histName.substr(0, pos);

    int maxV = 0;
    for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
      TH1* h1 = (*il); 
      std::string histName2 = h1->GetName();
      size_t pos2 = histName2.rfind("_V");
      if (pos2 != std::string::npos) {
        int checkV = atoi(histName2.substr(pos2+2).c_str());
        if (checkV > maxV) maxV = checkV;
      }
    }

    for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
      TH1* h1 = (*il); 
      std::string histName2 = h1->GetName();
      size_t pos2 = histName2.rfind("_V");
      if (pos2 != std::string::npos) {
        std::string histBaseName2 = histName2.substr(0, pos2);
        int checkV = atoi(histName2.substr(pos2+2).c_str());
        if (histBaseName == histBaseName2 && (checkV == currentV-1 || (currentV == 0 && checkV == maxV))) {
          fDisplayedHist = il;
          return *il;
        }
      }
    }
  }
  return 0;
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

  fThresholdN = h->FindLastBinAbove(0.5*h->GetMaximum()); 

  if (fPIF->doNotFit()) {
    fThreshold  = f->GetParameter(0); 
    fThresholdE = 0.3;
    fSigma      = 0.;
    fSigmaE     = 0.;
    return false;
  } else {
    h->Fit(f, "qr", "", lo, hi); 
    fThreshold  = f->GetParameter(0); 
    fThresholdE = f->GetParError(0); 
    fSigma      = 1./(TMath::Sqrt(2.)/f->GetParameter(1)); 
    fSigmaE     = fSigma * f->GetParError(1) / f->GetParameter(1);
  }

  //  cout << "fit status: " << gMinuit->GetStatus()  << endl;

  
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
void PixTest::cacheTBMDacs(bool verbose) {
  fDacTBMCache.clear();
  for (size_t itbm=0; itbm<fApi->_dut->getNTbms(); itbm++) {
    fDacTBMCache.push_back(fApi->_dut->getTbmDACs(itbm));
    if (verbose) {
      LOG(logINFO) << "Printing current DAC settings for TBM " << itbm << ":";
      for (vector<pair<string,uint8_t> >::iterator itbmdac = fDacTBMCache[itbm].begin(); itbmdac != fDacTBMCache[itbm].end(); ++itbmdac) {
        LOG(logINFO) << "TBM DAC: " << itbmdac->first << " = " << bitset<8>(itbmdac->second).to_string();
      }
    }
  }
}

// ----------------------------------------------------------------------
void PixTest::restoreTBMDacs(bool verbose) {
  for (size_t itbm=0; itbm<fApi->_dut->getNTbms(); itbm++) {
    if (verbose) { LOG(logINFO) << "Printing current DAC settings for TBM " << itbm << ":"; }
    for ( vector<pair<string,uint8_t> >::iterator itbmdac = fDacTBMCache[itbm].begin(); itbmdac != fDacTBMCache[itbm].end(); ++itbmdac) {
      fApi->setTbmReg(itbmdac->first, itbmdac->second, itbm);
      if (verbose) {
        LOG(logINFO) << "TBM DAC: " << itbmdac->first << " = " << bitset<8>(itbmdac->second).to_string();
      }
    }
  }
  fDacTBMCache.clear();
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

  vector<int> results; 
  results.clear();

  //  uint16_t FLAGS = FLAG_FORCE_MASKED | FLAG_FORCE_SERIAL;
  uint16_t FLAGS = FLAG_FORCE_MASKED;

  vector<pair<uint8_t, vector<pixel> > > scans;
  int cnt(0); 
  bool done = false;
  while (!done){
    LOG(logDEBUG) << "      attempt #" << cnt;
    try {
      scans = fApi->getEfficiencyVsDAC("vthrcomp", 0, 255, FLAGS, ntrig);
      fNDaqErrors = fApi->getStatistics().errors_pixel();
      done = true; 
    } catch(pxarException &/*e*/) {
      fNDaqErrors = 666667;
      ++cnt;
    }
    done = (cnt>5) || done;
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
  results.clear();

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
    // -- empty bins are ignored (Jamie)
    TH1* d1 = distribution(h2, 255, 1., 256.); 
    TH1* dn = distribution(hn, 255, 1., 256.); 
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
vector<TH2D*> PixTest::mapsWithString(string name) {
  vector<TH2D*> results; 
  string hname(""); 

  int oldCnt(-1), cnt(-1);
  // get them
  std::list<TH1*>::iterator il; 
  for (il = fHistList.begin(); il != fHistList.end(); ++il) {
    if (!(*il)->InheritsFrom(TH2::Class())) continue;
    hname = (*il)->GetName(); 
    if (string::npos == hname.find(name)) continue;
    
    size_t posV = hname.rfind("_V");
    string scnt = hname.substr(posV+2);
    cnt = atoi(scnt.c_str());
    // -- if a new cycle is found, restart from scratch
    if (cnt != oldCnt) {
      results.clear();
      oldCnt = cnt; 
    }
    results.push_back(static_cast<TH2D*>(*il)); 
  }    

  // sort them
  sort(results.begin(), results.end(), sortRocHist); 

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
void PixTest::preScan(string dac, std::vector<shist256*> maps, int &dacmin, int &dacmax) {
  PixTest::update(); 
  uint16_t FLAGS = FLAG_FORCE_MASKED;

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 

  bool done(false);
  int ntrig(3), cnt(0); 
  vector<pair<uint8_t, vector<pixel> > > results;

  while (!done){
    LOG(logDEBUG) << "      attempt #" << cnt;
    try{
      results = fApi->getEfficiencyVsDAC(dac, 1, 200, FLAGS, ntrig); 
      fNDaqErrors = fApi->getStatistics().errors_pixel();
      done = true;
    } catch(pxarException &/*e*/) {
      fNDaqErrors = 666667;
      ++cnt;
    }
    done = (cnt>5) || done;
  }

  if (666667 == fNDaqErrors) {
    LOG(logDEBUG) << "fNDaqErrors = " << fNDaqErrors; 
    return;
  }

  int idx(0), roc(0), ic(0), ir(0); 
  double val(0.);
  for (unsigned int idac = 0; idac < results.size(); ++idac) {
    int dac = results[idac].first; 
    for (unsigned int ipix = 0; ipix < results[idac].second.size(); ++ipix) {
      ic =   results[idac].second[ipix].column(); 
      ir =   results[idac].second[ipix].row(); 
      roc = results[idac].second[ipix].roc(); 
      if (ic > 51 || ir > 79) {
	LOG(logDEBUG) << "bad pixel address encountered: ROC/col/row = " << roc << "/" << ic << "/" << ir;
	continue;
      }
      val =  results[idac].second[ipix].value();
      idx = PixUtil::rcr2idx(getIdxFromId(roc), ic, ir);
      if (idx > -1) maps[idx]->fill(dac, val);
    }
  }

  
  // -- analyze results
  bool ok(false);
  TH1D *hT = new TH1D("hT", "hT", 256, 0., 256.); hT->Sumw2(); 
  TH1D *h1 = new TH1D("h1", "h1", 256, 0., 256.); h1->Sumw2(); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    LOG(logDEBUG) << "analyzing ROC " << static_cast<int>(rocIds[iroc]);
    for (unsigned int i = iroc*4160; i < (iroc+1)*4160; ++i) {
      if (maps[i]->getSumOfWeights() < 1) continue;
      
      h1->Reset();
      for (int ib = 1; ib <= 256; ++ib) {
	h1->SetBinContent(ib, maps[i]->get(ib));
	h1->SetBinError(ib, ntrig*PixUtil::dBinomial(static_cast<int>(maps[i]->get(ib)), ntrig)); 
      }
      
      ok = threshold(h1); 
      if (!ok) {
	LOG(logDEBUG) << "problem with threshold determination"; 
      }
      if (fThreshold > 0) {
	hT->Fill(fThreshold);
// 	TH1D *h = (TH1D*)h1->Clone(Form("h1_%d", i));
// 	h->SetTitle(Form("OK i = %d, thr = %4.3f, thrn = %4.3f", i, fThreshold, fThresholdN)); 
// 	fHistList.push_back(h); 
      } else {
	hT->Fill(0.);
// 	TH1D *h = (TH1D*)h1->Clone(Form("h1_%d", i));
// 	h->SetTitle(Form("i = %d, thr = %4.3f, thrn = %4.3f", i, fThreshold, fThresholdN)); 
// 	fHistList.push_back(h); 
      }

//       if (fThreshold > 80.) {
// 	TH1D *h = (TH1D*)h1->Clone(Form("h1_%d", i));
// 	h->SetTitle(Form("i = %d, thr = %4.3f, thrn = %4.3f", i, fThreshold, fThresholdN)); 
// 	fHistList.push_back(h); 
//       }
    }
  }

  int lo(-1), hi(-1); 
  for (int i = 1; i < 256; ++i) {
    if (hT->Integral(i, i+5) > 5) {
      lo = i; 
      break;
    }
  }

  for (int i = lo; i < 255; ++i) {
    if (0 == hT->Integral(i, i+5)) {
      hi = i; 
      break;
    }
  }

  if (lo > hi) {
    lo = 1; 
    hi = 255; 
  }
  
  dacmin = lo; 
  dacmax = hi;

  fHistList.push_back(hT); 
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), hT);
  hT->Draw();
  PixTest::update(); 
  
}


// ----------------------------------------------------------------------
void PixTest::dacScan(string dac, int ntrig, int dacmin, int dacmax, std::vector<shist256*> maps, int ihit, int FLAGS) {
  //  uint16_t FLAGS = flag | FLAG_FORCE_MASKED;

  bool unmasked = (0 != (FLAGS & FLAG_CHECK_ORDER))  &&  (0 != (FLAGS & FLAG_FORCE_UNMASKED));

  //  fNtrig = ntrig; 

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 

  TH2D *h3(0); 
  if (unmasked) {
    LOG(logDEBUG) << "booking xray maps for unmasked detector"; 
    fXrayMaps.clear();
    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
      h3 = bookTH2D(Form("%s_xraymap_C%d", dac.c_str(), rocIds[iroc]), 
		    Form("%s_xraymap_C%d", dac.c_str(), rocIds[iroc]), 
		    52, 0., 52., 80, 0., 80.); 
      fHistOptions.insert(make_pair(h3, "colz"));
      h3->SetMinimum(0.);
      h3->SetDirectory(fDirectory);
      setTitles(h3, "col", "row"); 
      fXrayMaps.push_back(h3);
    }
  }

  int ic, ir, iroc; 
  double val;
  bool done = false;
  int cnt(0); 
  vector<pair<uint8_t, vector<pixel> > > results;
  
  if (2 == ihit) {
    LOG(logDEBUG) << "determine PH error: " << dacmin << " .. " << dacmax; 
    getPhError(dac, dacmin, dacmax, FLAGS, ntrig); 
  }

  while (!done){
    LOG(logDEBUG) << "      attempt #" << cnt;
    try{
      gSystem->ProcessEvents();
      if (fStopTest) done = true;
      if (1 == ihit) {
	results = fApi->getEfficiencyVsDAC(dac, dacmin, dacmax, FLAGS, ntrig); 
	fNDaqErrors = fApi->getStatistics().errors_pixel();
      } else {
	results = fApi->getPulseheightVsDAC(dac, dacmin, dacmax, FLAGS, ntrig); 
	fNDaqErrors = fApi->getStatistics().errors_pixel();
      }
      done = true;
    } catch(pxarException &/*e*/) {
      fNDaqErrors = 666667;
      ++cnt;
    }
    done = (cnt>5) || done;
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
      if (unmasked) {
	h3 = fXrayMaps[getIdxFromId(iroc)];
	if (results[idac].second[ipix].value() > 0) {
	  if (idx > -1) maps[idx]->fill(dac, val);
	} else { 
	  h3->Fill(results[idac].second[ipix].column(), results[idac].second[ipix].row(), 1);
        }
      } else {
	if (idx > -1) maps[idx]->fill(dac, val);
      }

    }
  }
  
}


// ----------------------------------------------------------------------
void PixTest::scurveAna(string dac, string name, vector<shist256*> maps, vector<TH1*> &resultMaps, int result) {
  fDirectory->cd(); 
  TH1* h2(0), *h3(0), *h4(0); 
  //  string fname("SCurveData");
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
    //    if (!name.compare("scurveVcal") || !lname.compare("scurvevcal")) {
    if (fOutputFilename != "") {
      dumpFile = true; 
      LOG(logINFO) << "dumping ASCII scurve output file: " << fOutputFilename; 
      OutputFile.open(Form("%s/%s_C%d.dat", fPixSetup->getConfigParameters()->getDirectory().c_str(), fOutputFilename.c_str(), iroc));
      OutputFile << "Mode 1 " << "Ntrig " << fNtrig << endl;
    }

    for (unsigned int i = iroc*4160; i < (iroc+1)*4160; ++i) {
      PixUtil::idx2rcr(i, roc, ic, ir);
      if (maps[i]->getSumOfWeights() < 1) {
	if (dumpFile) OutputFile << empty << endl;
	continue;
      }
      // -- calculated "proper" errors
      h1->Reset();
      for (int ib = 1; ib <= 256; ++ib) {
	h1->SetBinContent(ib, maps[i]->get(ib));
	h1->SetBinError(ib, fNtrig*PixUtil::dBinomial(static_cast<int>(maps[i]->get(ib)), fNtrig)); 
      }

      bool ok = threshold(h1); 
      if (((result & 0x10) && !ok) || (result & 0x20)) {
	TH1D *h1c = (TH1D*)h1->Clone(Form("scurve_%s_c%d_r%d_C%d", dac.c_str(), ic, ir, rocIds[iroc])); 
	if (!ok) {
	  h1c->SetTitle(Form("problematic %s scurve (c%d_r%d_C%d), thr = %4.3f", dac.c_str(), ic, ir, rocIds[iroc], fThreshold));
	} else {
	  h1c->SetTitle(Form("%s scurve (c%d_r%d_C%d), thr = %4.3f", dac.c_str(), ic, ir, rocIds[iroc], fThreshold));
	}
	fHistList.push_back(h1c); 
      }
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
  fPixSetup->writeDacParameterFiles();
}

// ----------------------------------------------------------------------
void PixTest::saveTrimBits() {
  fPixSetup->writeTrimFiles();
  
}

// ----------------------------------------------------------------------
void PixTest::saveTbmParameters() {
  fPixSetup->writeTbmParameterFiles();
}

// ----------------------------------------------------------------------
void PixTest::saveTbParameters() {
  LOG(logDEBUG) << "save Tb parameters"; 
  fPixSetup->getConfigParameters()->writeTbParameterFile();
}

// ----------------------------------------------------------------------
vector<vector<pair<int, int> > > PixTest::deadPixels(int ntrig, bool scanCalDel) {
  vector<vector<pair<int, int> > > deadPixels;
  vector<pair<int, int> > deadPixelsRoc;
  
  // -- local DAC caches
  vector<uint8_t> vVcal     = getDacs("vcal"); 
  vector<uint8_t> vVthrComp = getDacs("vthrcomp"); 
  vector<uint8_t> vCreg     = getDacs("ctrlreg"); 
  vector<uint8_t> vCalDel   = getDacs("caldel"); 

  fApi->setDAC("vcal", 200); 
  fApi->setDAC("ctrlreg", 4); 
  fApi->setDAC("vthrcomp", 50); 
  
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  vector<TH2D*> testEff;

  if (scanCalDel) {
    // -- initialize testEff with zero-contents TH2D
    vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
      TH2D *h2 = bookTH2D(Form("dp0_C%d", rocIds[iroc]), Form("dp0_C%d", rocIds[iroc]), 52, 0., 52., 80, 0., 80.);
      testEff.push_back(h2);
    }
    
    // -- scan CalDel
    vector<TH2D*> tEff;
    for (int icaldel = 10; icaldel < 250; icaldel += 30) {
      fApi->setDAC("caldel", icaldel);
      tEff = efficiencyMaps(Form("dp_caldel%d", icaldel), ntrig);
      for (int i = 0; i < static_cast<int>(tEff.size()); ++i) {
	if (tEff[i]->Integral() > testEff[i]->Integral()) {
	  // cout << " tEff->Integral: " << tEff[i]->Integral() << " >  testEff[i]->Integral(): " << testEff[i]->Integral() << endl;
	  delete testEff[i];
	  testEff[i] = tEff[i]; 
	} else {
	  // cout << " tEff->Integral: " << tEff[i]->Integral() << " <  testEff[i]->Integral(): " << testEff[i]->Integral() << endl;
	  delete tEff[i]; 
	}
      }
      tEff.clear();
    }
  } else {
    testEff = efficiencyMaps("deadPixels", ntrig);
  }

  std::pair<int, int> badPix;
  Double_t eff(0.);
  
  for (unsigned int i = 0; i < testEff.size(); ++i) {
    deadPixelsRoc.clear();
    for(int r=0; r<80; r++){
      for(int c=0; c<52; c++){
	eff = testEff[i]->GetBinContent( testEff[i]->FindFixBin((double)c + 0.5, (double)r+0.5) );
	if (eff<ntrig){
	  LOG(logDEBUG) << Form("ROC %2d", i) << " col/row = " << c << "/" << r << " with eff " << eff << "/" << ntrig << ";  blacklisting";
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
  setDacs("caldel", vCalDel); 
  setDacs("vthrcomp", vVthrComp); 
  
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
    fApi->setDAC(dacName, v[i], rocIds[i]); 
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
void PixTest::dutCalibrateOn() {
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  maskPixels();
}

// ----------------------------------------------------------------------
void PixTest::dutCalibrateOff() {
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(false);
  if (fPixSetup->getConfigParameters()->nMaskedPixels() > 0) maskPixels();
}


// ----------------------------------------------------------------------
void PixTest::maskPixels() {
  if (0 == fPixSetup->getConfigParameters()->nMaskedPixels()) return;
  vector<vector<pair<int, int> > > vmask =  fPixSetup->getConfigParameters()->getMaskedPixels();
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
  // FIXME - issuing a ROC reset should not be necessary anymore since
  // pxarCore automatically resets the ROC when WBC is changed.
  fApi->daqSingleSignal("resetroc");
}

// ----------------------------------------------------------------------
void PixTest::resetTBM() {
  // -- setup DAQ for data taking
  // Issue a TBM reset:
  fApi->daqSingleSignal("resettbm");
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
void PixTest::trimHotPixels(int hitThr, int runSeconds, bool maskuntrimmable) {

  int NSECONDS(runSeconds); 
  int TRGFREQ(100); // in kiloHertz

  double THR = 1.e-5*NSECONDS*TRGFREQ*1000;

  if (hitThr > 0) {
    THR = (double)hitThr;
  }

  banner(Form("PixTest::trimHotPixels() running for %d seconds with %d kHz trigger rate", NSECONDS, TRGFREQ));

  LOG(logINFO) << "THR = " << THR << ", corresponding to ~ " << THR/(NSECONDS*TRGFREQ*1000*2.5e-8*150*100*1.e-8)*1.e-6 << " MHz/cm2";
  LOG(logINFO) << "edge/corner pixel THR is adjusted";

  bool finished = false;
  int step = 0;
  int nHotPixels = 0;
  LOG(logINFO) << "PixTestHighRate::trimHotPixels: step " << step << "...";

  // get enabled rocs
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 

  // get trim bits
  ConfigParameters* cp = fPixSetup->getConfigParameters();
  vector<vector<pxar::pixelConfig> > rocPixelConfig = cp->getRocPixelConfig();

  std::vector<TH2D*> hotpixel_map(rocIds.size());
  std::vector<std::pair<int, std::pair<int,int> > > hotPixelList;

  for (size_t i = 0; i < rocIds.size(); ++i) { 
    hotpixel_map[i] = bookTH2D(Form("hitMap_hotpixels_C%d", rocIds[i]), Form("hits_C%d", rocIds[i]), 52, 0., 52., 80, 0., 80.);
  }

  std::vector<TH2D*> diff_map(rocIds.size());
  for (size_t i = 0; i < rocIds.size(); ++i) { 
    diff_map[i] = bookTH2D(Form("trimbitdiff_C%d", rocIds[i]), Form("trimbitdiff_C%d", rocIds[i]), 52, 0., 52., 80, 0., 80.);
  }

  while (!finished) {
    for (size_t i = 0; i < rocIds.size(); ++i) { 
      hotpixel_map[i]->Reset();
    }

    nHotPixels = 0;
    finished = true;
    fApi->_dut->testAllPixels(false);
    fApi->_dut->maskAllPixels(false);

    int totalPeriod = prepareDaq(TRGFREQ, (uint8_t)500);
    
    TStopwatch t;
    int seconds(0);
    uint8_t perFull;
    bool daq_loop = true;
      
    fApi->daqStart();

    int finalPeriod = fApi->daqTriggerLoop(totalPeriod);
    LOG(logINFO) << "Collecting data for " << NSECONDS << " seconds...";
    
    t.Start(kTRUE);
    while (fApi->daqStatus(perFull) && daq_loop) {
      if (perFull > 80) {
        LOG(logINFO) << "Buffer almost full, pausing triggers.";
        fApi->daqTriggerLoopHalt();
        t.Stop();
        vector<pxar::Event> daqdat;
        try { daqdat = fApi->daqGetEventBuffer(); }
        catch(pxar::DataNoEvent &) {}
        for(std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
          for (unsigned int ipix = 0; ipix < it->pixels.size(); ++ipix) {
            hotpixel_map[getIdxFromId(it->pixels[ipix].roc())]->Fill(it->pixels[ipix].column(), it->pixels[ipix].row());
          }
        }

        LOG(logINFO) << "Resuming triggers.";
        t.Start(kFALSE);
      fApi->daqTriggerLoop(finalPeriod);
      }
     
      seconds = t.RealTime(); 
      t.Start(kFALSE); 
      if (static_cast<int>(seconds >= NSECONDS)) {
        LOG(logINFO) << "Done with hot pixel readout";
        daq_loop = false;
        break;
      }
    }
      
    fApi->daqTriggerLoopHalt();
    fApi->daqStop();

    vector<pxar::Event> daqdat;
    try { daqdat = fApi->daqGetEventBuffer(); }
    catch(pxar::DataNoEvent &) {}
    for(std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
      for (unsigned int ipix = 0; ipix < it->pixels.size(); ++ipix) {
        int rocIdx = getIdxFromId(it->pixels[ipix].roc());
        if (rocIdx >= 0 && rocIdx < static_cast<int>(hotpixel_map.size())) {
          hotpixel_map[rocIdx]->Fill(it->pixels[ipix].column(), it->pixels[ipix].row());
        } else {
          LOG(logERROR) << "found hit from disabled ROC " << (int)it->pixels[ipix].roc() 
			<< ", col " << (int)it->pixels[ipix].column() << " row " << (int)it->pixels[ipix].row(); 
          break;
        }
      }
    }
    finalCleanup();

    // -- analysis of hit map
    LOG(logDEBUG) << "hot pixel determination with THR = " << THR; 
    TH2D *h(0);
    float pixelAreaFactor = 1.0;
    for (unsigned int i = 0; i < hotpixel_map.size(); ++i) {
      h = hotpixel_map[i];
      for (int ix = 0; ix < h->GetNbinsX(); ++ix) {
        for (int iy = 0; iy < h->GetNbinsY(); ++iy) {
          pixelAreaFactor = 1.0;
          if (ix == 0 || ix == 51)
            pixelAreaFactor*=2;
          if (iy == 0 || iy == 79)
            pixelAreaFactor*=2;
          if (h->GetBinContent(ix+1, iy+1) > THR * pixelAreaFactor) {
            nHotPixels++;
            if (step == 0) {
              hotPixelList.push_back(std::make_pair(rocIds[i], std::make_pair(ix, iy)));
            }
            LOG(logDEBUG) << "ROC " << (int)rocIds[i] << " with hot pixel " << ix << "/" << iy << ",  hits = " << h->GetBinContent(ix+1, iy+1);
            
            // find pixel 
            int foundPixel = -1;
            for(size_t k=0;k<rocPixelConfig[i].size();k++) {
              if (rocPixelConfig[i][k].column() == ix && rocPixelConfig[i][k].row() == iy) {
                foundPixel = k;
                break;
              }
            }

            if(foundPixel > -1){
              int trimBits = (int)(rocPixelConfig[i][foundPixel].trim());              
              if (trimBits < 15) {
                LOG(logDEBUG) << " => trim bit: " << trimBits << " => " << (trimBits+1);
                trimBits++;
                int col = rocPixelConfig[i][foundPixel].column();
                int row = rocPixelConfig[i][foundPixel].row();
                diff_map[i]->SetBinContent(1 + col, 1 + row, diff_map[i]->GetBinContent(1 + col, 1 + row) + 1);
                rocPixelConfig[i][foundPixel].setTrim(trimBits);
                bool result = fApi->_dut->updateTrimBits(rocPixelConfig[i][foundPixel].column(), rocPixelConfig[i][foundPixel].row(), trimBits, rocIds[i]);
                if (!result) {
                  LOG(logERROR) << "could not update trim bit.";
                } else {
                  // check again if increasing trim bits by one was enough, otherwise repeat
                  finished = false;
                }
              } else {
                LOG(logWARNING) << "  => trimBits already at highest possible threshold, 'real' hot pixel found";
              }
            }
          }
        }
      }
    }
    LOG(logINFO) << nHotPixels << " hot pixels found in step " << step;
    step++;
    if (step > 14) {
      finished = true;
    }
  }

  int trimBitMargin = 1;
  LOG(logDEBUG) << "list of re-trimmed hot pixels:";
  for (size_t i = 0; i< hotPixelList.size(); ++i) {
    int rocId = hotPixelList[i].first;
    int col = hotPixelList[i].second.first;
    int row = hotPixelList[i].second.second;

    // find pixel config
    int foundPixel = -1;
    for(size_t k=0;k<rocPixelConfig[rocId].size();k++) {
      if (rocPixelConfig[rocId][k].column() == col && rocPixelConfig[rocId][k].row() == row) {
        foundPixel = k;
        break;
      }
    }

    // increase trim bits by trimBitMargin
    if(foundPixel > -1){
      int trimBits = (int)(rocPixelConfig[rocId][foundPixel].trim());
      int trimBitsOld = trimBits;
      trimBits+=trimBitMargin;
      if (trimBits > 15) trimBits = 15;
      diff_map[rocId]->SetBinContent(1 + col, 1 + row, diff_map[rocId]->GetBinContent(1 + col, 1 + row) + (trimBits - trimBitsOld));
      rocPixelConfig[rocId][foundPixel].setTrim(trimBits);
      fApi->_dut->updateTrimBits(col, row, trimBits, rocId);
      
    }
    LOG(logDEBUG) << "ROC " << rocId << " pix " << col << "/" << row;
  }


  // now mask all remaining pixels
  if (maskuntrimmable) {

    fHotPixels.clear();
    int numMaskedHotPixels = 0;
    TH2D *h(0);

    for (unsigned int i = 0; i < hotpixel_map.size(); ++i) {
      h = hotpixel_map[i];
      vector<pair<int, int> > hot; 
      for (int ix = 0; ix < h->GetNbinsX(); ++ix) {

        
        for (int iy = 0; iy < h->GetNbinsY(); ++iy) {
          float pixelAreaFactor = 1.0;
          if (ix == 0 || ix == 51)
            pixelAreaFactor*=2;
          if (iy == 0 || iy == 79)
            pixelAreaFactor*=2;
          if (h->GetBinContent(ix+1, iy+1) > THR * pixelAreaFactor) {
            hot.push_back(make_pair(ix, iy));
            numMaskedHotPixels++;
          }
        }
      }
      fHotPixels.push_back(hot); 
    }
    LOG(logINFO) << numMaskedHotPixels << " hot pixels could not be trimmed and have been masked.";
  }

  for (size_t i = 0; i < diff_map.size(); ++i) {
    fHistList.push_back(hotpixel_map[i]);
    fHistOptions.insert(make_pair(hotpixel_map[i], "colz"));
  }
  for (size_t i = 0; i < diff_map.size(); ++i) {
    fHistList.push_back(diff_map[i]);
    fHistOptions.insert(make_pair(diff_map[i], "colz"));
  }
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), diff_map[diff_map.size()-1]);
  diff_map[diff_map.size()-1]->Draw("colz");
  PixTest::update();

  LOG(logINFO) << "PixTest::trimHotPixels() done";
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
      vector<pxar::Event> daqdat;
      try { daqdat = fApi->daqGetEventBuffer(); }
      catch(pxar::DataNoEvent &) {}
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
  vector<pxar::Event> daqdat;
  try { daqdat = fApi->daqGetEventBuffer(); }
  catch(pxar::DataNoEvent &) {}
  for(std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
    for (unsigned int ipix = 0; ipix < it->pixels.size(); ++ipix) {
      v[getIdxFromId(it->pixels[ipix].roc())]->Fill(it->pixels[ipix].column(), it->pixels[ipix].row());
    }
  }

  finalCleanup();


  // -- analysis of hit map
  double THR = 1.e-5*NSECONDS*TRGFREQ*1000; 
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
    LOG(logINFO) << "no hot pixel found!";
  } else {
    LOG(logINFO) << cntHot << " hot pixels found!";
  }
  LOG(logINFO) << "PixTest::maskHotPixels() done";

}


// ----------------------------------------------------------------------
bool sortRocHist(const TH1* h1, const TH1* h2) {
  string hname1 = h1->GetName(); 
  string hname2 = h2->GetName(); 

  size_t pos1 = hname1.rfind("_C");
  size_t pos2 = hname1.rfind("_V");
  string sroc1 = hname1.substr(pos1 + 2, pos2 - pos1 - 2); 
  int roc1 = atoi(sroc1.c_str()); 

  pos1 = hname2.rfind("_C");
  pos2 = hname2.rfind("_V");
  string sroc2 = hname2.substr(pos1 + 2, pos2 - pos1 - 2); 
  int roc2 = atoi(sroc2.c_str()); 
  
  return roc1 < roc2;
}


// ----------------------------------------------------------------------
// -- from PixTestCmd
int PixTest::tbmSet(string name, uint8_t cores, int value, uint8_t valueMask){
  /* set a tbm register, allow setting a subset of bits (thoese where mask=1)
   * the default value of mask is 0xff, i.e. all bits are changed
   * the cores = 0:TBMA, 1:TBMB, >1 TBMA+TBMB
   * 
   */
  if ((value & (~valueMask) )>0) {
    LOG(logDEBUG) << "Warning! tbm set value " << hex << (int) value 
		  << " has bits outside mask ("<<hex<< (int) valueMask << ")";
  }
  
  uint8_t coreMask = 3;
  if (cores==0){coreMask=1;}  else if(cores==1){ coreMask=2;}
  
  int err=1;
  //out << "tbmset" << name << " " <<  (int) coreMask << " " << value << "  " << bitset<8>(valueMask) << "\n";
  for(size_t core=0; core<2; core++){
    if ( ((coreMask >> core) & 1) == 1 ){
      std::vector< std::pair<std::string,uint8_t> > regs = fApi->_dut->getTbmDACs(core);
      if (regs.size()==0) {
	LOG(logWARNING) << "TBM registers not set !?! This is not going to work.";
      }
      for(unsigned int i=0; i<regs.size(); i++){
	if (name==regs[i].first){
	  // found it , do something
	  uint8_t present = regs[i].second;
	  uint8_t update = value & valueMask;
	  update |= (present & (~valueMask) );
	  LOG(logDEBUG) << "changing tbm reg " <<  name << "["<<core<<"]"
			<< " from 0x" << hex << (int) regs[i].second
			<< " to 0x" << hex << (int) update << "";
	  fApi->setTbmReg( name, update, core );
	  err=0;
	}
      }
    }
  }
  return err; // nonzero values for errors
}
