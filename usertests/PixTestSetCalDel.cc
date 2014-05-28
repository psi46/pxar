// -- author: Daniel Pitzl
// set good CalDel for each ROC

#include <stdlib.h>  // atof, atoi
#include <algorithm> // std::find
#include <iostream>  // cout
#include <iomanip>   // setw

#include <TH1.h>

#include "PixTestSetCalDel.hh"
#include "log.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestSetCalDel)

//------------------------------------------------------------------------------
PixTestSetCalDel::PixTestSetCalDel( PixSetup *a, std::string name) :
PixTest(a, name), fParNtrig(-1)
{
  PixTest::init();
  init(); 
  //  LOG(logINFO) << "PixTestSetCalDel ctor(PixSetup &a, string, TGTab *)";
  for( unsigned int i = 0; i < fPIX.size(); ++i) {
    LOG(logDEBUG) << " PixTestSetCalDel setting fPIX" << i
		  <<  " ->" << fPIX[i].first
		  << "/" << fPIX[i].second;
  }
}

//------------------------------------------------------------------------------
PixTestSetCalDel::PixTestSetCalDel() : PixTest() {
  //  LOG(logINFO) << "PixTestSetCalDel ctor()";
}

//------------------------------------------------------------------------------
bool PixTestSetCalDel::setParameter(string parName, string sval) {
  bool found(false);
  string str1, str2; 
  string::size_type s1;
  int pixc, pixr; 
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true;
      sval.erase( remove( sval.begin(), sval.end(), ' '), sval.end() );
      if( !parName.compare("ntrig") ) {
	fParNtrig = atoi( sval.c_str() );
	LOG(logDEBUG) << " PixTestSetCalDel setting fParNtrig  ->" << fParNtrig
		      << "<- from sval = " << sval;
      }
      if( !parName.compare("pix1") ) {
	s1 = sval.find(",");
	if( string::npos != s1) {
	  str1 = sval.substr(0, s1);
	  pixc = atoi(str1.c_str());
	  str2 = sval.substr(s1+1);
	  pixr = atoi(str2.c_str());
	  fPIX.push_back( make_pair(pixc, pixr) );
	}
	else {
	  fPIX.push_back( make_pair(-1, -1) );
	}
      }
      // FIXME: remove/update from fPIX if the user removes via the GUI!
      break;
    }
  }
  return found;
}

//------------------------------------------------------------------------------
void PixTestSetCalDel::init()
{
  setToolTips();
  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory ) {
    fDirectory = gFile->mkdir( fName.c_str() );
  }
  fDirectory->cd();
}

//------------------------------------------------------------------------------
void PixTestSetCalDel::setToolTips()
{
  fTestTip = string( "scan and set CalDel using one pixel");
  fSummaryTip = string("summary plot to be implemented");
}

//------------------------------------------------------------------------------
void PixTestSetCalDel::bookHist(string name) {
  fDirectory->cd();

  TH1D *h1(0);
  fHistList.clear();

  for( unsigned int i = 0; i < fPixSetup->getConfigParameters()->getNrocs(); ++i ) {

    for( unsigned int ip = 0; ip < fPIX.size(); ++ip ) {
      h1 = new TH1D(Form("NhitsVs%s_c%d_r%d_C%d", name.c_str(), fPIX[ip].first, fPIX[ip].second, i),
		    Form("NhitsVs%s_c%d_r%d_C%d", name.c_str(), fPIX[ip].first, fPIX[ip].second, i),
		    256, -0.5, 255.5 );
      h1->SetMinimum(0.);
      setTitles( h1, Form( "%s [DAC]", name.c_str() ), "readouts" );
      fHistList.push_back(h1);
    }
  }
}

//------------------------------------------------------------------------------
PixTestSetCalDel::~PixTestSetCalDel() {
  LOG(logDEBUG) << "PixTestSetCalDel dtor";
}

//------------------------------------------------------------------------------
void PixTestSetCalDel::doTest() {
  fDirectory->cd();
  PixTest::update(); 
  LOG(logINFO) << "PixTestSetCalDel::doTest() ntrig = " << fParNtrig;
  //FIXME  clearHist();
  // -- FIXME: Should/could separate better test from display?

  fApi->_dut->testAllPixels(false);
  for( unsigned int i = 0; i < fPIX.size(); ++i) {
    if( fPIX[i].first > -1)  fApi->_dut->testPixel(fPIX[i].first, fPIX[i].second, true);
  }

  bookHist("caldel");

  string DacName = "caldel";

  vector <pair <uint8_t, vector<pixel> > > results =
    fApi->getEfficiencyVsDAC( DacName, 0, 255, 0, fParNtrig ); // dac, pixels

  LOG(logDEBUG) << " dacscandata.size(): " << results.size();

  TH1D *h(0);

  int i0[16] = {0};
  int i9[16] = {0};
  int nm[16] = {0};

  for( unsigned int i = 0; i < results.size(); ++i ) { // dac values

    pair<uint8_t, vector<pixel> > v = results[i];
    int caldel = v.first;

    vector<pixel> vpix = v.second; // pixels from all rocs

    for( unsigned int ipx = 0; ipx < vpix.size(); ++ipx ) {

      int roc = vpix.at(ipx).roc_id;

      int nn = vpix.at(ipx).value;

      if( nn > nm[roc] ) {
	nm[roc] = nn;
	i0[roc] = caldel; // begin of plateau
      }
      if( nn == nm[roc] )
	i9[roc] = caldel; // end of plateau

      h = (TH1D*)fDirectory->Get( Form( "NhitsVs%s_c%d_r%d_C%d",
					DacName.c_str(), vpix.at(ipx).column, vpix.at(ipx).row, roc ) );

      if( h ) {
	h->Fill( caldel, nn );
      }
      else {
	LOG(logDEBUG) << "XX did not find "
		      << Form( "NhitsVs%s_c%d_r%d_C%d",
			       DacName.c_str(), vpix.at(ipx).column, vpix.at(ipx).row, roc );
      }
    } // pixels and rocs
  } // caldel vals

  for( list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il ) {
    (*il)->Draw();
    PixTest::update();
  }
  fDisplayedHist = fHistList.end();

  //h = (TH1D*)(*fHistList.begin() );
  //h->Draw();

  for( unsigned int roc = 0; roc < fPixSetup->getConfigParameters()->getNrocs(); ++roc ) {

    LOG(logINFO) << "ROC " << setw(2) << roc
		 << ": eff plateau from " << setw(3) << i0[roc]
		 << " to " << setw(3) << i9[roc];
    if( i9[roc] > 0 ) {
      int i2 = i0[roc] + (i9[roc]-i0[roc])/4;
      fApi->setDAC( DacName, i2, roc );
      LOG(logINFO) << "ROC " << setw(2) << roc
		   << ": set CalDel to " << i2;
    }
  } // rocs
}
