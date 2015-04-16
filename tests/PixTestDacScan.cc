#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>

#include <TH1.h>

#include "PixTestDacScan.hh"
#include "PixUtil.hh"
#include "log.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestDacScan)

// ----------------------------------------------------------------------
PixTestDacScan::PixTestDacScan(PixSetup *a, std::string name) : PixTest(a, name), fParPHmap(-1), fParAllPixels(-1), fParUnmasked(-1), 
  fParNtrig(-1), fParDAC("nada"), fParLoDAC(-1), fParHiDAC(-1) {
  PixTest::init();
  init(); 
}


//----------------------------------------------------------
PixTestDacScan::PixTestDacScan() : PixTest() {
  //  LOG(logINFO) << "PixTestDacScan ctor()";
}

// ----------------------------------------------------------------------
bool PixTestDacScan::setParameter(string parName, string sval) {
  bool found(false);
  string str1, str2; 
  string::size_type s1;
  int pixc, pixr; 
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());
      if (!parName.compare("phmap")) {
	PixUtil::replaceAll(sval, "checkbox(", ""); 
	PixUtil::replaceAll(sval, ")", ""); 
	fParPHmap = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("allpixels")) {
	PixUtil::replaceAll(sval, "checkbox(", ""); 
	PixUtil::replaceAll(sval, ")", ""); 
	fParAllPixels = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("unmasked")) {
	PixUtil::replaceAll(sval, "checkbox(", ""); 
	PixUtil::replaceAll(sval, ")", ""); 
	fParUnmasked = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("dac")) {
	fParDAC = sval; 
	setToolTips();
      }
      if (!parName.compare("daclo")) {
	fParLoDAC = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("dachi")) {
	fParHiDAC = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("pix")) {
	s1 = sval.find(","); 
	if (string::npos != s1) {
	  str1 = sval.substr(0, s1); 
	  pixc = atoi(str1.c_str()); 
	  str2 = sval.substr(s1+1); 
	  pixr = atoi(str2.c_str()); 
	  clearSelectedPixels();
	  fPIX.push_back(make_pair(pixc, pixr)); 
	  addSelectedPixels(sval); 
	} else {
	  clearSelectedPixels();
	  fPIX.push_back(make_pair(-1, -1)); 
	  addSelectedPixels("-1,-1"); 
	}
      }
      break;
    }
  }
  
  return found; 
}


// ----------------------------------------------------------------------
void PixTestDacScan::init() {
  setToolTips(); 
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}


// ----------------------------------------------------------------------
void PixTestDacScan::setToolTips() {
  fTestTip    = string(Form("scan the DAC %s and determine the number of hits vs DAC value\n", fParDAC.c_str()))
    ;
  fSummaryTip = string("summary plot to be implemented")
    ;

}

// ----------------------------------------------------------------------
void PixTestDacScan::bookHist(string /*name*/) {
  fDirectory->cd(); 
}


//----------------------------------------------------------
PixTestDacScan::~PixTestDacScan() {
  LOG(logDEBUG) << "PixTestDacScan dtor";
}


// ----------------------------------------------------------------------
void PixTestDacScan::doTest() {
  uint16_t FLAGS = FLAG_FORCE_MASKED; 
  if (fParUnmasked) {
    LOG(logINFO) << "unmasking the detector"; 
    FLAGS = FLAG_CHECK_ORDER | FLAG_FORCE_UNMASKED; 
  }

  fDirectory->cd();
  TH1D *h1(0);
  TH2D *h3(0);
  vector<TH1D*> vhist;
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  map<string, TH1D*> hmap; 
  map<string, TH2D*> xmap; 
  string name(fParPHmap?"ph":"nhits"); 
  string hname;
  if (fParAllPixels) {
    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
      for (unsigned int ic = 0; ic < 52; ++ic) {
	for (unsigned int ir = 0; ir < 80; ++ir) {
	  hname = Form("%s_%s_c%d_r%d_C%d", name.c_str(), fParDAC.c_str(), ic, ir, rocIds[iroc]);
	  h1 = bookTH1D(hname.c_str(), hname.c_str(), 256, 0., 256.); 
	  h1->SetMinimum(0.); 
	  setTitles(h1, Form("%s [DAC]", fParDAC.c_str()), (fParPHmap?"average PH":"readouts"));
	  hmap[hname] = h1;
	  fHistList.push_back(h1); 
	}
      }
    }
  } else {
    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
      for (unsigned int ip = 0; ip < fPIX.size(); ++ip) {
	hname = Form("%s_%s_c%d_r%d_C%d", name.c_str(), fParDAC.c_str(), fPIX[ip].first, fPIX[ip].second, rocIds[iroc]);
	h1 = bookTH1D(hname.c_str(), hname.c_str(), 256, 0., 256.); 
	h1->SetMinimum(0.); 
	setTitles(h1, Form("%s [DAC]", fParDAC.c_str()), (fParPHmap?"average PH":"readouts"));
	hmap[hname] = h1;
	fHistList.push_back(h1); 
      }
    }
  }

  if (fParUnmasked) {
    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
      hname = Form("%s_xraymap_C%d", name.c_str(), rocIds[iroc]);
      h3 = bookTH2D(hname, hname, 52, 0., 52., 80, 0., 80.); 
      xmap[hname] = h3;
      fHistOptions.insert(make_pair(h3,"colz"));
      h3->SetMinimum(0.);
      fHistList.push_back(h3); 
    }
  }

  PixTest::update(); 
  LOG(logINFO) << "PixTestDacScan: " << fParDAC << "[" << fParLoDAC << ", " << fParHiDAC << "]"
	       << (fParPHmap?" average PH":" readouts")
	       << ", ntrig = " << fParNtrig 
	       << (fParAllPixels>0? ", running on all pixels": Form("npixels = %d", fPIX.size())); 

  vector<pair<uint8_t, vector<pixel> > > rresults, results;
  int problems(0); 
  fNDaqErrors = 0; 

  if (fParAllPixels) {
    fApi->_dut->testAllPixels(true);
    fApi->_dut->maskAllPixels(false);

    bool done = false;
    int cnt(0); 
    while (!done) {
      try{
	if (0 == fParPHmap) {
	  results = fApi->getEfficiencyVsDAC(fParDAC, fParLoDAC, fParHiDAC, FLAGS, fParNtrig);
	  fNDaqErrors = fApi->getStatistics().errors_pixel();
	  done = true;
	} else {
	  results = fApi->getPulseheightVsDAC(fParDAC, fParLoDAC, fParHiDAC, FLAGS, fParNtrig);
	  fNDaqErrors = fApi->getStatistics().errors_pixel();
	  done = true;
	}
      } catch(DataMissingEvent &e){
	LOG(logCRITICAL) << "problem with readout: "<< e.what() << " missing " << e.numberMissing << " events"; 
	fNDaqErrors = 666666;
	++cnt;
	if (e.numberMissing > 10) done = true; 
      } catch(pxarException &e) {
	fNDaqErrors = 666667;
	LOG(logCRITICAL) << "pXar execption: "<< e.what(); 
	++cnt;
      }
      done = (cnt>5) || done;
    }
    if (fNDaqErrors > 0) problems = fNDaqErrors; 
  } else {
    fApi->_dut->testAllPixels(false);
    fApi->_dut->maskAllPixels(true);
    for (unsigned int i = 0; i < fPIX.size(); ++i) {
      if (fPIX[i].first > -1)  {
	fApi->_dut->testPixel(fPIX[i].first, fPIX[i].second, true);
	fApi->_dut->maskPixel(fPIX[i].first, fPIX[i].second, false);
	
	bool done = false;
	int cnt(0); 
	while (!done) {
	  try{
	    if (0 == fParPHmap) {
	      rresults = fApi->getEfficiencyVsDAC(fParDAC, fParLoDAC, fParHiDAC, FLAGS, fParNtrig);
	      fNDaqErrors = fApi->getStatistics().errors_pixel();
	      done = true;
	    } else {
	      rresults = fApi->getPulseheightVsDAC(fParDAC, fParLoDAC, fParHiDAC, FLAGS, fParNtrig);
	      fNDaqErrors = fApi->getStatistics().errors_pixel();
	      done = true;
	    }
	  } catch(DataMissingEvent &e){
	    LOG(logCRITICAL) << "problem with readout: "<< e.what() << " missing " << e.numberMissing << " events"; 
	    fNDaqErrors = 666666;
	    ++cnt;
	    if (e.numberMissing > 10) done = true; 
	  } catch(pxarException &e) {
	    fNDaqErrors = 666667;
	    LOG(logCRITICAL) << "pXar execption: "<< e.what(); 
	    ++cnt;
	  }
	  done = (cnt>5) || done;
	}
	
	if (fNDaqErrors > 0) problems = fNDaqErrors; 
	copy(rresults.begin(), rresults.end(), back_inserter(results)); 
	fApi->_dut->testPixel(fPIX[i].first, fPIX[i].second, false);
	fApi->_dut->maskPixel(fPIX[i].first, fPIX[i].second, true);
      }
    }
  }


  LOG(logDEBUG) << " dacscandata.size(): " << results.size();
  TH1D *h(0); 
  for (unsigned int i = 0; i < results.size(); ++i) {
    pair<uint8_t, vector<pixel> > v = results[i];
    int idac = v.first; 
    
    vector<pixel> vpix = v.second;
    for (unsigned int ipix = 0; ipix < vpix.size(); ++ipix) {
      hname = Form("%s_%s_c%d_r%d_C%d", name.c_str(), fParDAC.c_str(), vpix[ipix].column(), vpix[ipix].row(), vpix[ipix].roc());
      h = hmap[hname];
      if (h) {
	cout  << "roc = " << static_cast<int>(vpix[ipix].roc()) 
	      << " pixel c/r = " << static_cast<int>(vpix[ipix].column()) << "/" << static_cast<int>(vpix[ipix].row()) 
	      << " value = " << static_cast<float>(vpix[ipix].value())
	      << endl;
	if (vpix[ipix].value() > 0) {
	  h->Fill(idac, static_cast<float>(vpix[ipix].value())); 
	} else {
	  hname = Form("%s_xraymap_C%d", name.c_str(), vpix[ipix].roc()); 
	  h3 = xmap[hname];
	  if (h3) {
	    h3->Fill(vpix[ipix].column(), vpix[ipix].row(), 1);
	  } else {
	    LOG(logDEBUG) << "found stray (xray) hit for ROC " << static_cast<int>(vpix[ipix].roc()) 
			  << " pixel c/r = " << static_cast<int>(vpix[ipix].column()) << "/" << static_cast<int>(vpix[ipix].row()) 
			  << " value = " << static_cast<float>(vpix[ipix].value())
			  << ", but no histogram " << hname;
	  }
	  
	}
      } else {
	LOG(logDEBUG) << "XX did not find "  << hname; 
      }
    }
  }

  LOG(logINFO) << "dac scan done" << (problems > 0? Form(" problems observed: %d", problems): " no problems seen"); 

  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    (*il)->Draw((getHistOption(*il)).c_str()); 
    fDisplayedHist = (il);
  }
  PixTest::update(); 

}

// ----------------------------------------------------------------------
TH1* PixTestDacScan::moduleMap(string histname) {
  LOG(logDEBUG) << "PixTestDacScan::moduleMap histname: " << histname; 
  TH1* h0 = (*fDisplayedHist);
  if (!h0->InheritsFrom(TH1::Class())) {
    return 0; 
  }

  TH1D *h1 = (TH1D*)h0; 
  string h1name = h1->GetName();
  string::size_type s1 = h1name.rfind("_c"); 
  string barename = h1name.substr(0, s1);
  LOG(logDEBUG) << "h1->GetName() = " << h1name << " -> " << barename; 

  TH1 *h(0);
  string hname;
  int cycle(-1); 
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){

    h1  = bookTH1D(Form("%s_C%d", barename.c_str(), rocIds[iroc]), Form("%s_C%d", barename.c_str(), rocIds[iroc]), 
		   h0->GetNbinsX(), h0->GetBinLowEdge(1), h0->GetBinLowEdge(h0->GetNbinsX()+1)); 
    if (0 == iroc) cycle = -1 + histCycle(Form("%s_C%d", barename.c_str(), rocIds[iroc])); 

    list<TH1*>::iterator hbeg = fHistList.begin();
    list<TH1*>::iterator hend = fHistList.end();
    for (list<TH1*>::iterator il = hbeg; il != hend; ++il) {
      h = (*il);
      hname = h->GetName();
      if (string::npos != hname.find(Form("%s", barename.c_str()))) {
	if (string::npos != hname.find(Form("_C%d", rocIds[iroc]))) {
	  if (string::npos != hname.find(Form("_V%d", cycle))) {
	    for (int i = 1; i < h->GetNbinsX(); ++i) {
	      h1->Fill(h->GetBinCenter(i), h->GetBinContent(i)); 
	    }
	  }
	}
      }
    }
    fHistList.push_back(h1); 
  }

  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);

  if (h1) h1->Draw();
  update(); 
  return h1; 
}
