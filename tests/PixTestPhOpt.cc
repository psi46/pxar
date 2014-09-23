#include <stdlib.h>  // atof, atoi
#include <algorithm> // std::find
#include <sstream>   // parsing

#include "PixTestPhOpt.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestPhOpt)

// ----------------------------------------------------------------------
PixTestPhOpt::PixTestPhOpt() {}

// ----------------------------------------------------------------------
PixTestPhOpt::PixTestPhOpt(PixSetup *a, std::string name ) :  
  PixTest(a, name), 
  fParNtrig(-1), 
  fVcalLow(-1), 
  fVcalHigh(-1), 
  fPhMin(-1), 
  fPhMax(-1) {
  PixTest::init();
  init();
}

// ----------------------------------------------------------------------
bool PixTestPhOpt::setParameter(string parName, string sval) {
  bool found(false);
  string str1, str2;
  string::size_type s1;
  int pixc, pixr;
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (!fParameters[i].first.compare(parName)) {
      found = true;
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());
      if (!parName.compare("ntrig")) {
	setTestParameter("ntrig", sval); 
	fParNtrig = atoi(sval.c_str());
	LOG(logDEBUG) << "  setting fNtrig  ->" << fParNtrig
		      << "<- from sval = " << sval;
      }
      if (!parName.compare("vcallow")) {
	fVcalLow = atoi(sval.c_str());
	LOG(logDEBUG) << "  setting fVcalLow  ->" << fVcalLow
		      << "<- from sval = " << sval;
      }
      if (!parName.compare("vcalhigh")) {
	fVcalHigh = atoi(sval.c_str());
	LOG(logDEBUG) << "  setting fVcalHigh  ->" << fVcalHigh
		      << "<- from sval = " << sval;
      }

      if (!parName.compare("phmin")) {
	fPhMin = atoi(sval.c_str());
	LOG(logDEBUG) << "  setting fPhMin  ->" << fPhMin
		      << "<- from sval = " << sval;
      }

      if (!parName.compare("phmax")) {
	fPhMax = atoi(sval.c_str());
	LOG(logDEBUG) << "  setting fPhMax  ->" << fPhMax
		      << "<- from sval = " << sval;
      }
      
      if (!parName.compare("pix")) {
        s1 = sval.find(",");
        if (string::npos != s1) {
	  str1 = sval.substr(0, s1);
	  pixc = atoi(str1.c_str());
	  str2 = sval.substr(s1+1);
	  pixr = atoi(str2.c_str());
	  fPIX.push_back(make_pair(pixc, pixr));
	  addSelectedPixels(sval); 
	  LOG(logDEBUG) << "  adding to FPIX ->" << pixc << "/" << pixr << " fPIX.size() = " << fPIX.size() ;
	} else {
	  clearSelectedPixels();
	  LOG(logDEBUG) << "  clear fPIX: " << fPIX.size(); 
	}
      }
      break;
    }
  }
  return found;
}

// ----------------------------------------------------------------------
void PixTestPhOpt::init() {
  fDirectory = gFile->GetDirectory(fName.c_str());
  if(!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str());
  }
  fDirectory->cd();
}

// ----------------------------------------------------------------------
void PixTestPhOpt::bookHist(string /*name*/) {}

// ----------------------------------------------------------------------
PixTestPhOpt::~PixTestPhOpt() {}

// ----------------------------------------------------------------------
void PixTestPhOpt::doTest() {

  if (0 == fPIX.size()) {
    fPIX.push_back(make_pair(11, 12));
  }

  cacheDacs();
  bigBanner(Form("PixTestPhOpt::doTest() Ntrig = %d, vcal Low/High = %d/%d, fPIX[0,0] = %d/%d"
		 , fParNtrig, fVcalLow, fVcalHigh, fPIX[0].first, fPIX[0].second));
  fDirectory->cd();
  PixTest::update();

  fApi->setDAC("ctrlreg", 0);
  fApi->setDAC("vcal", fVcalLow);
  scan("phLo");

  fApi->setDAC("ctrlreg", 4);
  fApi->setDAC("vcal", fVcalHigh);
  scan("phHi");

  TH1 *hLo(0), *hHi(0);
  TH2D *h2[16]; 
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    hLo = fMaps[Form("phLo_phoffset_phscale_c%d_r%d_C%d", fPIX[0].first, fPIX[0].second, rocIds[iroc])];
    hHi = fMaps[Form("phHi_phoffset_phscale_c%d_r%d_C%d", fPIX[0].first, fPIX[0].second, rocIds[iroc])];
    h2[iroc] = bookTH2D(Form("phoffset_phscale_C%d", rocIds[iroc]), Form("phoffset_phscale_C%d", rocIds[iroc]), 
			256, 0., 256., 256, 0., 256.);
    setTitles(h2[iroc], "phoffset", "phscale"); 
    fHistList.push_back(h2[iroc]);
    fHistOptions.insert(make_pair(h2[iroc], "colz")); 
    
    for (int io = 0; io < 255; ++io) {
      for (int is = 0; is < 255; ++is) {
	if ((hLo->GetBinContent(io+1, is+1) >= fPhMin) 
	    && (hHi->GetBinContent(io+1, is+1) <= fPhMax)) {
	  h2[iroc]->SetBinContent(io+1, is+1, hHi->GetBinContent(io+1, is+1) - hLo->GetBinContent(io+1, is+1)); 
	}
      }
    }
    
  }

  restoreDacs(); 

  // -- set phscale and phoffset to values corresponding to maximum range
  int io(-1), is(-1), ibla(-1), ibin(-1); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    ibin = h2[iroc]->GetMaximumBin(io, is, ibla); 
    LOG(logDEBUG) << " roc " << static_cast<int>(rocIds[iroc]) << " ibin = " << ibin << " io = " << io << " is = " << is 
		  << " ibla = " << ibla;
    if (io > 10 && is > 10) {
      fApi->setDAC("phscale", is, rocIds[iroc]); 
      fApi->setDAC("phoffset", io, rocIds[iroc]); 
    } else {
      LOG(logWARNING) << " PH optimization did not converge for ROC " << static_cast<int>(rocIds[iroc]) << ", setting to default values";
      fApi->setDAC("phscale", fApi->_dut->getDAC(rocIds[iroc], "phscale"), rocIds[iroc]); 
      fApi->setDAC("phoffset", fApi->_dut->getDAC(rocIds[iroc], "phoffset"), rocIds[iroc]); 
    }
  }
  saveDacs();

  // -- validation
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  cacheDacs();
  fApi->setDAC("ctrlreg", 0);
  fApi->setDAC("vcal", fVcalLow);
  vector<TH2D*> loAlive = phMaps("phOptValLo", 10, FLAG_FORCE_MASKED); 
  for (unsigned int i = 0; i < loAlive.size(); ++i) {
    TH1* d1 = distribution(loAlive[i], 256, 0., 256.); 
    fHistList.push_back(loAlive[i]); 
    fHistList.push_back(d1); 
  }
  fApi->setDAC("ctrlreg", 4);
  fApi->setDAC("vcal", fVcalHigh);
  vector<TH2D*> hiAlive = phMaps("phOptValHi", 10, FLAG_FORCE_MASKED); 
  for (unsigned int i = 0; i < hiAlive.size(); ++i) {
    TH1* d1 = distribution(hiAlive[i], 256, 0., 256.); 
    fHistList.push_back(hiAlive[i]); 
    fHistList.push_back(d1); 
  }

  restoreDacs();


  TH1 *h1(0); 
  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    (*il)->Draw("colz");
    PixTest::update();
  }
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);


  // -- print summary information
  string psString(""), poString(""); 
  for (unsigned int i = 0; i < rocIds.size(); ++i) {
    psString += Form(" %3d", fApi->_dut->getDAC(rocIds[i], "phscale"));
    poString += Form(" %3d", fApi->_dut->getDAC(rocIds[i], "phoffset"));
  }
  LOG(logINFO) << "PixTestPhOpt::doTest() done";
  LOG(logINFO) << "PH scale (per ROC):  " << psString;
  LOG(logINFO) << "PH offset (per ROC): " << poString;

}



// ----------------------------------------------------------------------
void PixTestPhOpt::scan(string name) {

  uint16_t FLAGS = FLAG_FORCE_MASKED;

  TH2D *h2(0), *hd(0);
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  int nx = 256;
  int ny = 256;
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    for (unsigned int ip = 0; ip < fPIX.size(); ++ip) {
      h2 = bookTH2D(Form("%s_phoffset_phscale_c%d_r%d_C%d", name.c_str(), fPIX[ip].first, fPIX[ip].second, rocIds[iroc]), 
		    Form("%s_phoffset_phscale_c%d_r%d_C%d", name.c_str(), fPIX[ip].first, fPIX[ip].second, rocIds[iroc]), 
		    nx, 0., static_cast<double>(nx), ny, 0., static_cast<double>(ny)); 
      hd = h2; 
      h2->SetMinimum(0.); 
      setTitles(h2, "phoffset", "phscale"); 
      fHistList.push_back(h2);
      fHistOptions.insert(make_pair(h2, "colz")); 
      fMaps.insert(make_pair(Form("%s_phoffset_phscale_c%d_r%d_C%d", 
				 name.c_str(), fPIX[ip].first, fPIX[ip].second, rocIds[iroc]), h2)); 
    }
    
  }

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  vector<pair<uint8_t, pair<uint8_t, vector<pixel> > > >  rresults, results;
  int problems(0); 
  fNDaqErrors = 0; 
  for (unsigned int i = 0; i < fPIX.size(); ++i) {
    if (fPIX[i].first > -1)  {
      fApi->_dut->testPixel(fPIX[i].first, fPIX[i].second, true);
      fApi->_dut->maskPixel(fPIX[i].first, fPIX[i].second, false);
      bool done = false;
      int cnt(0); 
      while (!done) {
	try{
	  rresults = fApi->getPulseheightVsDACDAC("phoffset", 0, 255, "phscale", 0, 255, FLAGS, fParNtrig);
	  fNDaqErrors = fApi->daqGetNDecoderErrors();
	  done = true;
	} catch(DataMissingEvent &e){
	  LOG(logCRITICAL) << "problem with readout: "<< e.what() << " missing " << e.numberMissing << " events"; 
	  fNDaqErrors = 666666;
	  ++cnt;
	  if (e.numberMissing > 10) done = true; 
	} catch(pxarException &e) {
	  LOG(logCRITICAL) << "pXar exception: "<< e.what(); 
	  fNDaqErrors = 666667;
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

  TH2D *h(0); 
  int iroc(-1); 
  for (unsigned int i = 0; i < results.size(); ++i) {
    pair<uint8_t, pair<uint8_t, vector<pixel> > > v = results[i];
    int idac1 = v.first; 
    pair<uint8_t, vector<pixel> > w = v.second;      
    int idac2 = w.first;
    vector<pixel> wpix = w.second;
    
    for (unsigned ipix = 0; ipix < wpix.size(); ++ipix) {
      iroc = wpix[ipix].roc();
      h = fMaps[Form("%s_phoffset_phscale_c%d_r%d_C%d", name.c_str(), wpix[ipix].column(), wpix[ipix].row(), iroc)];
      if (h) {
	h->Fill(idac1, idac2, wpix[ipix].value()); 
      } else {
	LOG(logDEBUG) << "XX did not find " 
		      << Form("%s_phoffset_phscale_c%d_r%d_C%d", name.c_str(), wpix[ipix].column(), wpix[ipix].row(), rocIds[iroc]);
      }
    }
  }

  if (hd) hd->Draw("colz");
  PixTest::update();

}
