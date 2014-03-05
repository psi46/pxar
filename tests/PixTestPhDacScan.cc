#include <stdlib.h>  // atof, atoi
#include <algorithm> // std::find

#include "PixTestPhDacScan.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestPhDacScan)

//------------------------------------------------------------------------------
PixTestPhDacScan::PixTestPhDacScan( PixSetup *a, std::string name ) : PixTest(a, name), fParNtrig(-1), fParDAC("nada"), fParLoDAC(-1), fParHiDAC(-1) {
  PixTest::init();
  init();
  
  for (size_t i = 0; i < fPIX.size(); ++i)  {
    LOG(logDEBUG) << "  setting fPIX" << i
		  <<  " ->" << fPIX[i].first
		  << "/" << fPIX[i].second;
  }
}

//------------------------------------------------------------------------------
PixTestPhDacScan::PixTestPhDacScan() : PixTest() {
  //  LOG(logDEBUG) << "PixTestPhDacScan ctor()";
}

//------------------------------------------------------------------------------
bool PixTestPhDacScan::setParameter(string parName, string sval) {
  bool found(false);
  string str1, str2;
  string::size_type s1;
  int pixc, pixr;
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true;
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());
      if (!parName.compare("Ntrig")) {
	fParNtrig = atoi(sval.c_str());
	LOG(logDEBUG) << "  setting fParNtrig  ->" << fParNtrig
		     << "<- from sval = " << sval;
      }
      if (!parName.compare("DAC")) {
	fParDAC = sval;
	LOG(logDEBUG) << "  setting fParDAC  ->" << fParDAC
		     << "<- from sval = " << sval;
      }
      if (!parName.compare("DACLO")) {
	fParLoDAC = atoi(sval.c_str());
	LOG(logDEBUG) << "  setting fParLoDAC  ->" << fParLoDAC
		     << "<- from sval = " << sval;
      }
      if (!parName.compare( "DACHI" ) ) {
	fParHiDAC = atoi( sval.c_str() );
	LOG(logDEBUG) << "  setting fParHiDAC  ->" << fParHiDAC
		     << "<- from sval = " << sval;
      }
      if (!parName.compare("PIX1")) {
	s1 = sval.find(",");
	if (string::npos != s1) {
	  str1 = sval.substr(0, s1);
	  pixc = atoi(str1.c_str());
	  str2 = sval.substr(s1+1);
	  pixr = atoi(str2.c_str());
	  fPIX.push_back( make_pair(pixc, pixr));
	}
	else {
	  fPIX.push_back( make_pair( -1, -1 ) );
	}
      }
      if (!parName.compare("PIX")) {
	s1 = sval.find( "," );
	if (string::npos != s1) {
	  str1 = sval.substr(0, s1);
	  pixc = atoi(str1.c_str());
	  str2 = sval.substr(s1+1);
	  pixr = atoi(str2.c_str());
	  fPIX.push_back(make_pair(pixc, pixr));
	}
	else {
	  fPIX.push_back( make_pair(-1, -1));
	}
      }
      // FIXME: remove/update from fPIX if the user removes via the GUI!
      break;
    }
  }
  return found;
}

//------------------------------------------------------------------------------
void PixTestPhDacScan::init() {
  fDirectory = gFile->GetDirectory(fName.c_str());
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str());
  }
  fDirectory->cd();
}

//------------------------------------------------------------------------------
void PixTestPhDacScan::bookHist(string name) {
  LOG(logDEBUG) << "nothing done with " << name;
}

//------------------------------------------------------------------------------
PixTestPhDacScan::~PixTestPhDacScan() {
  std::list<TH1*>::iterator il;
  fDirectory->cd();
  for (il = fHistList.begin(); il != fHistList.end(); ++il) {
    LOG(logINFO) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory);
    (*il)->Write();
  }
}

//------------------------------------------------------------------------------
void PixTestPhDacScan::doTest() {
  fDirectory->cd();
  PixTest::update();

  LOG(logINFO) << "PixTestPhDacScan::doTest() ntrig = " << fParNtrig;

  vector<TH1D*> hsts;
  TH1D *h1(0);
  int cycle(-1); 

  size_t nRocs = fPixSetup->getConfigParameters()->getNrocs();
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    for (unsigned int i = 0; i < fPIX.size(); ++i) {
      if (fPIX[i].first > -1)  {
	h1 = bookTH1D(Form("PH_vs_%s_c%d_r%d_C%d", fParDAC.c_str(), fPIX[i].first, fPIX[i].second, rocIds[iroc]),
		      Form( "PH_vs_%s_c%d_r%d_C%d", fParDAC.c_str(), fPIX[i].first, fPIX[i].second, rocIds[iroc]),
		      256, -0.5, 255.5 );
	cycle = -1 + histCycle(Form("PH_vs_%s_c%d_r%d_C%d", fParDAC.c_str(), fPIX[i].first, fPIX[i].second, rocIds[iroc])); 
	h1->SetMinimum(0);
	setTitles(h1, Form( "%s [DAC]", fParDAC.c_str() ), "<PH> [ADC]");
	hsts.push_back(h1);
	fHistList.push_back(h1);
      } // rocs
    }
  }

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  vector<pair<uint8_t, pair<uint8_t, vector<pixel> > > >  rresults, results;
  vector<pair<uint8_t, vector<pixel> > > rresult, result; 
  for (unsigned int i = 0; i < fPIX.size(); ++i) {
    if (fPIX[i].first > -1)  {
      fApi->_dut->testPixel(fPIX[i].first, fPIX[i].second, true);
      fApi->_dut->maskPixel(fPIX[i].first, fPIX[i].second, false);
      rresult = fApi->getPulseheightVsDAC(fParDAC, fParLoDAC, fParHiDAC, 0, fParNtrig);
      copy(rresult.begin(), rresult.end(), back_inserter(result)); 
      fApi->_dut->testPixel(fPIX[i].first, fPIX[i].second, false);
      fApi->_dut->maskPixel(fPIX[i].first, fPIX[i].second, true);
    }
  }

  LOG(logINFO) << "dacscandata.size(): " << result.size();

  for (unsigned int i = 0; i < result.size(); ++i) {
    int idac = result[i].first;
    vector<pixel> vpix = result[i].second;
    for (unsigned int ipx = 0; ipx < vpix.size(); ++ipx) {
      int roc = vpix[ipx].roc_id;
      h1 = (TH1D*)fDirectory->Get(Form("PH_vs_%s_c%d_r%d_C%d_V%d", fParDAC.c_str(), vpix[ipx].column, vpix[ipx].row, rocIds[roc], cycle));
      if (h1) {
	h1->Fill(idac, vpix[ipx].value);
      } else {
	LOG(logDEBUG) << " histogram " << Form("PH_vs_%s_c%d_r%d_C%d_V%d", 
					       fParDAC.c_str(), vpix[ipx].column, vpix[ipx].row, rocIds[roc], cycle) << " not found";
      }
    } // ipx
  } // dac values

  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    (*il)->Draw();
    PixTest::update();
  }
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);
}
