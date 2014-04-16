#include <stdlib.h>  // atof, atoi
#include <algorithm> // std::find
#include <sstream>   // parsing

#include "PixTestPh.hh"
#include "log.h"

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
  int imax(fParameters.size()); 
  for (unsigned int i = 0; i < imax; ++i) {
    if (!fParameters[i].first.compare(parName)) {
      found = true;
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());
      if (!parName.compare("Ntrig")) {
	setTestParameter("Ntrig", sval); 
	fParNtrig = atoi( sval.c_str() );
	LOG(logDEBUG) << "  setting fParNtrig  ->" << fParNtrig
		      << "<- from sval = " << sval;
      }
      if (!parName.compare("DAC")) {
	setTestParameter("DAC", sval); 
	fParDAC = sval;
	LOG(logDEBUG) << "  setting fParDAC  ->" << fParDAC
		      << "<- from sval = " << sval;
      }

      if (!parName.compare("DacVal")) {
	setTestParameter("DacVal", sval); 
	fParDacVal = atoi(sval.c_str());
	LOG(logDEBUG) << "  setting fParDacVal  ->" << fParDacVal
		      << "<- from sval = " << sval;
      }
      
      if (!parName.compare("PIX")) {
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

//------------------------------------------------------------------------------
void PixTestPh::doTest() {
  fDirectory->cd();
  PixTest::update();

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
	setTitles(h1, "PH [ADC]", "Entries/bin"); 
	hists.insert(make_pair(name, h1)); 
	fHistList.push_back(h1);
      }
    }
  }

  fApi->setDAC(fParDAC, fParDacVal); 

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  vector<pair<uint8_t, pair<uint8_t, vector<pixel> > > >  rresults, results;
  vector<pair<uint8_t, vector<pixel> > > rresult, result; 
  for (int ievt = 0; ievt < fParNtrig; ++ievt) {
    for (unsigned int i = 0; i < fPIX.size(); ++i) {
      if (fPIX[i].first > -1)  {
	fApi->_dut->testPixel(fPIX[i].first, fPIX[i].second, true);
	fApi->_dut->maskPixel(fPIX[i].first, fPIX[i].second, false);
	rresult = fApi->getPulseheightVsDAC(fParDAC, fParDacVal, fParDacVal, 0, 1);
	copy(rresult.begin(), rresult.end(), back_inserter(result)); 
	fApi->_dut->testPixel(fPIX[i].first, fPIX[i].second, false);
	fApi->_dut->maskPixel(fPIX[i].first, fPIX[i].second, true);
      }
    }
  }

  for (unsigned int i = 0; i < result.size(); ++i) {
    int idac = result[i].first;
    vector<pixel> vpix = result[i].second;
    for (unsigned int ipx = 0; ipx < vpix.size(); ++ipx) {
      int roc = vpix[ipx].roc_id;
      int ic = vpix[ipx].column;
      int ir = vpix[ipx].row;
      name = Form("PH_c%d_r%d_C%d", ic, ir, roc); 
      h1 = hists[name];
      if (h1) {
	h1->Fill(vpix[ipx].value);
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
  
}
