
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
  PixTest::init(a, name);
  init(); 
  //  LOG(logDEBUG) << "PixTestSetCalDel ctor(PixSetup &a, string, TGTab *)";
  for( unsigned int i = 0; i < fPIX.size(); ++i) {
    LOG(logDEBUG) << " PixTestSetCalDel setting fPIX" << i
		  <<  " ->" << fPIX[i].first
		  << "/" << fPIX[i].second;
  }
}

//------------------------------------------------------------------------------
PixTestSetCalDel::PixTestSetCalDel() : PixTest() {
  //  LOG(logDEBUG) << "PixTestSetCalDel ctor()";
}

//------------------------------------------------------------------------------
bool PixTestSetCalDel::setParameter(string parName, string sval) {
  bool found(false);
  string str1, str2; 
  string::size_type s1;
  int pixc, pixr; 
  for( map<string,string>::iterator imap = fParameters.begin();
       imap != fParameters.end(); ++imap ) {
    LOG(logDEBUG) << "---> " << imap->first;
    if( 0 == imap->first.compare(parName) ) {
      found = true;
      sval.erase( remove( sval.begin(), sval.end(), ' '), sval.end() );
      fParameters[parName] = sval;
      if( !parName.compare("Ntrig") ) {
	fParNtrig = atoi( sval.c_str() );
	LOG(logDEBUG) << " PixTestSetCalDel setting fParNtrig  ->" << fParNtrig
		      << "<- from sval = " << sval;
      }
      if( !parName.compare("PIX1") ) {
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
void PixTestSetCalDel::bookHist(string name)
{
}

//------------------------------------------------------------------------------
PixTestSetCalDel::~PixTestSetCalDel()
{
  LOG(logDEBUG) << "PixTestSetCalDel dtor";
}

//------------------------------------------------------------------------------
void PixTestSetCalDel::doTest()
{
  fDirectory->cd();
  PixTest::update();
  LOG(logINFO) << "PixTestSetCalDel::doTest() ntrig = " << fParNtrig;

  fHistList.clear();

  if( fPIX.size() < 1 ) {
    LOG(logWARNING) << "PixTestSetCalDel: no pixel defined, return";
    return;
  }

  if( fPIX[0].first < 0 ) {
    LOG(logWARNING) << "PixTestSetCalDel: no valid pixel defined, return";
    return;
  }

  vector<TH1D*> hsts;
  TH1D *h1(0);

  for( size_t roc = 0; roc < fPixSetup->getConfigParameters()->getNrocs(); ++roc ) {

    h1 = new TH1D( Form( "NhitsVsCalDel_c%d_r%d_C%d",
			 fPIX[0].first, fPIX[0].second, int(roc) ),
		   Form( "readouts vs CalDel c%d r%d C%d",
			 fPIX[0].first, fPIX[0].second, int(roc) ),
		   256, -0.5, 255.5 );
    h1->SetStats(0); // no stats
    h1->SetMinimum(0);
    setTitles( h1, "CalDel [DAC]", "readouts" );
    hsts.push_back(h1);
    fHistList.push_back(h1);
  }

  TH1D *hsum = new TH1D( "CalDelSettings", "CalDel per ROC;ROC;CalDel [DAC]",
			 16, -0.5, 15.5 );
  hsum->SetStats(0); // no stats
  hsum->SetMinimum(0);
  hsum->SetMaximum(256);
  fHistList.push_back(hsum);

  // activate one pixel per ROC:

  fApi->_dut->testAllPixels(false);
    
  fApi->_dut->testPixel( fPIX[0].first, fPIX[0].second, true );

  // set maximum pulse (minimal time walk):

  uint8_t cal = fApi->_dut->getDAC( 0, "Vcal" );
  uint8_t ctl = fApi->_dut->getDAC( 0, "CtrlReg" );

  fApi->setDAC( "Vcal", 255 ); // all ROCs large Vcal
  fApi->setDAC( "CtrlReg", 4 ); // all ROCs large Vcal

  string DacName = "caldel";

  vector <pair <uint8_t, vector<pixel> > > result =
    fApi->getEfficiencyVsDAC( DacName, 0, 255, 0, fParNtrig ); // dac, pixels

  LOG(logDEBUG) << "result size " << result.size();

  int i0[16] = {0};
  int i9[16] = {0};
  int nm[16] = {0};

  for( uint32_t i = 0; i < result.size(); ++i ) { // dac values

    pair<uint8_t, vector<pixel> > v = result[i];
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

      h1 = hsts.at(roc);
      h1->Fill( caldel, nn );

    } // pixels and rocs

  } // caldel vals

  for( list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il ) {
    (*il)->Draw();
    PixTest::update();
  }
  fDisplayedHist = fHistList.begin();

  //h = (TH1D*)(*fHistList.begin() );
  //h->Draw();

  for( uint32_t roc = 0; roc < fPixSetup->getConfigParameters()->getNrocs(); ++roc ) {

    if( i9[roc] > 0 ) {

      int i2 = i0[roc] + (i9[roc]-i0[roc])/4;

      fApi->setDAC( DacName, i2, roc );

      LOG(logINFO) << "ROC " << setw(2) << roc
		   << ": eff plateau from " << setw(3) << i0[roc]
		   << " to " << setw(3) << i9[roc]
		   << ": set CalDel to " << setw(3) << i2;

      hsum->Fill( roc, i2 );
    }

  } // rocs

  fApi->setDAC( "Vcal", cal ); // restore
  fApi->setDAC( "CtrlReg", ctl ); // restore

  //flush?

  hsum->Draw();
  PixTest::update();
}
