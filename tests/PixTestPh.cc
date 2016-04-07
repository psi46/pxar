#include <stdlib.h>  // atof, atoi
#include <algorithm> // std::find
#include <sstream>   // parsing

#include "PixTestPh.hh"
#include "log.h"

#include <TStyle.h>
#include <TH2.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestPh)

//------------------------------------------------------------------------------
PixTestPh::PixTestPh( PixSetup *a, std::string name ) :  PixTest(a, name),
  fParNtrig(-1), fParDAC("nada"), fParDacVal(100) {
  PixTest::init();
  init();
}


//------------------------------------------------------------------------------
PixTestPh::PixTestPh() : PixTest() {
  //  LOG(logDEBUG) << "PixTestPh ctor()";
}

//------------------------------------------------------------------------------
bool PixTestPh::setParameter(string parName, string sval) {
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
	fParNtrig = atoi( sval.c_str() );
	LOG(logDEBUG) << "  setting fParNtrig  ->" << fParNtrig
		      << "<- from sval = " << sval;
      }
      if (!parName.compare("dac")) {
	setTestParameter("dac", sval);
	fParDAC = sval;
	LOG(logDEBUG) << "  setting fParDAC  ->" << fParDAC
		      << "<- from sval = " << sval;
      }

      if (!parName.compare("dacval")) {
	setTestParameter("dacval", sval);
	fParDacVal = atoi(sval.c_str());
	LOG(logDEBUG) << "  setting fParDacVal  ->" << fParDacVal
		      << "<- from sval = " << sval;
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
	  LOG(logDEBUG) << "  adding to FPIX ->" << pixc << "/" << pixr << " fPIX.size() = " << fPIX.size() ;
	} else {
	  clearSelectedPixels();
	  addSelectedPixels("-1,-1");
	  LOG(logDEBUG) << "  clear fPIX: " << fPIX.size();
	}
      }
      break;
    }
  }
  return found;
}

//------------------------------------------------------------------------------
void PixTestPh::init() {
  fDirectory = gFile->GetDirectory(fName.c_str());
  if( !fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str());
  }
  fDirectory->cd();
}

//------------------------------------------------------------------------------
void PixTestPh::bookHist(string name) {
  LOG(logDEBUG) << "nothing done with " << name;
}

//------------------------------------------------------------------------------
PixTestPh::~PixTestPh() {
}

// ----------------------------------------------------------------------
void PixTestPh::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;

  if (!command.compare("phmap")) {
    phMap();
    return;
  }

  LOG(logDEBUG) << "did not find command ->" << command << "<-";
}


// ----------------------------------------------------------------------
void PixTestPh::phMap() {
  cacheDacs();
  fDirectory->cd();
  PixTest::update();
  banner(Form("PixTestPh::phMap() ntrig = %d, %s = %d",
	      static_cast<int>(fParNtrig), fParDAC.c_str(), static_cast<int>(fParDacVal)));

  fApi->setDAC(Form("%s", fParDAC.c_str()), fParDacVal);

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  maskPixels();

  fNDaqErrors = 0;
  vector<TH2D*> test2 = phMaps("PH", fParNtrig, FLAG_FORCE_MASKED);
  copy(test2.begin(), test2.end(), back_inserter(fHistList));

  TH2D *h = (TH2D*)(fHistList.back());

  if (h) {
    gStyle->SetPalette(1);
    h->Draw(getHistOption(h).c_str());
    fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
    PixTest::update();
  }


  restoreDacs();

  dutCalibrateOff();
}

//------------------------------------------------------------------------------
void PixTestPh::doTest() {

  cacheDacs();
  fDirectory->cd();
  PixTest::update();

  uint16_t FLAGS = FLAG_FORCE_MASKED | FLAG_FORCE_SERIAL;
  LOG(logDEBUG) << " using FLAGS = "  << (int)FLAGS;

  TH1D *h1(0);
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  map<string, TH1D*> hists;
  string name;
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    for (unsigned int i = 0; i < fPIX.size(); ++i) {
      if (fPIX[i].first > -1)  {
	name = Form("PH_c%d_r%d_C%d", fPIX[i].first, fPIX[i].second, rocIds[iroc]);
	h1 = bookTH1D(name, name, 256, 0., 256.);
	h1->SetMinimum(0);
	setTitles(h1, Form("PH [ADC] for %s = %d", fParDAC.c_str(), fParDacVal), "Entries/bin");
	hists.insert(make_pair(name, h1));
	fHistList.push_back(h1);
      }
    }
  }

  fApi->setDAC(fParDAC, fParDacVal);

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  vector<pair<uint8_t, vector<pixel> > > rresult, result;
  for (int ievt = 0; ievt < fParNtrig; ++ievt) {
    for (unsigned int i = 0; i < fPIX.size(); ++i) {
      if (fPIX[i].first > -1)  {
	fApi->_dut->testPixel(fPIX[i].first, fPIX[i].second, true);
	fApi->_dut->maskPixel(fPIX[i].first, fPIX[i].second, false);

	int cnt(0);
	bool done(false);
	while (!done) {
	  try {
	    rresult = fApi->getPulseheightVsDAC(fParDAC, fParDacVal, fParDacVal, FLAGS, 1);
	    done = true;
	  } catch(pxarException &e) {
	    LOG(logCRITICAL) << "pXar execption: "<< e.what();
	    ++cnt;
	  }
	  done = (cnt>5) || done;
	}

	copy(rresult.begin(), rresult.end(), back_inserter(result));
	fApi->_dut->testPixel(fPIX[i].first, fPIX[i].second, false);
	fApi->_dut->maskPixel(fPIX[i].first, fPIX[i].second, true);
      }
    }
  }

  for (unsigned int i = 0; i < result.size(); ++i) {
    vector<pixel> vpix = result[i].second;
    for (unsigned int ipx = 0; ipx < vpix.size(); ++ipx) {
      int roc = vpix[ipx].roc();
      int ic = vpix[ipx].column();
      int ir = vpix[ipx].row();
      name = Form("PH_c%d_r%d_C%d", ic, ir, roc);
      h1 = hists[name];
      if (h1) {
	h1->Fill(vpix[ipx].value());
      } else {
	LOG(logDEBUG) << " histogram " << Form("PH_c%d_r%d_C%d", ic, ir, roc) << " not found";
      }
    } // ipx
  } // dac values

  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    (*il)->Draw();
    PixTest::update();
  }
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);

  restoreDacs();
}
