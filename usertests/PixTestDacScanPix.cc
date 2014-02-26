// -- author: Daniel Pitzl
// Readout count vs DAC for one pixel per ROC

#include <stdlib.h>   // atof, atoi
#include <algorithm>  // std::find

#include "PixTestDacScanPix.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestDacScanPix)

//------------------------------------------------------------------------------
PixTestDacScanPix::PixTestDacScanPix( PixSetup *a, std::string name )
: PixTest(a, name), fParNtrig(-1), fParDAC("nada"),
  fParLoDAC(-1), fParHiDAC(-1)
{
  PixTest::init();
  init();
}

//------------------------------------------------------------------------------
PixTestDacScanPix::PixTestDacScanPix() : PixTest()
{
}

//------------------------------------------------------------------------------
bool PixTestDacScanPix::setParameter( string parName, string sval )
{
  bool found(false);

  for( uint32_t i = 0; i < fParameters.size(); ++i ) {

    if( fParameters[i].first == parName ) {

      found = true;

      sval.erase( remove( sval.begin(), sval.end(), ' '), sval.end() );

      if( !parName.compare("Ntrig") ) {
	fParNtrig = atoi(sval.c_str() );
	LOG(logDEBUG) << "  setting fParNtrig  ->" << fParNtrig
		      << "<- from sval = " << sval;
      }

      if( !parName.compare("DAC") ) {
	fParDAC = sval;
	LOG(logDEBUG) << "  setting fParDAC  ->" << fParDAC
		      << "<- from sval = " << sval;
      }

      if( !parName.compare("DACLO") ) {
	fParLoDAC = atoi(sval.c_str() );
	LOG(logDEBUG) << "  setting fParLoDAC  ->" << fParLoDAC
		      << "<- from sval = " << sval;
      }

      if( !parName.compare("DACHI") ) {
	fParHiDAC = atoi(sval.c_str() );
	LOG(logDEBUG) << "  setting fParHiDAC  ->" << fParHiDAC
		      << "<- from sval = " << sval;
      }

      string::size_type s1;
      string str1, str2;
      int pixc, pixr;
      if( !parName.compare("PIX1") ) {
	s1 = sval.find(",");
	if( string::npos != s1) {
	  str1 = sval.substr(0, s1);
	  pixc = atoi(str1.c_str() );
	  str2 = sval.substr(s1+1);
	  pixr = atoi(str2.c_str() );
	  fPIX.push_back( make_pair(pixc, pixr) );
	}
	else {
	  fPIX.push_back(make_pair(-1, -1) );
	}
      }
      // FIXME: remove/update from fPIX if the user removes via the GUI!
      break;
    }
  }
  return found;
}

//------------------------------------------------------------------------------
void PixTestDacScanPix::init()
{
  setToolTips();
  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory )
    fDirectory = gFile->mkdir( fName.c_str() );
  fDirectory->cd();
}

//------------------------------------------------------------------------------
void PixTestDacScanPix::setToolTips() {
  fTestTip =
    string( Form( "scan DAC %s, count pixel responses\n", fParDAC.c_str() ) )
    ;
  fSummaryTip = string("summary plot to be implemented")
    ;
}

//------------------------------------------------------------------------------
void PixTestDacScanPix::bookHist(string name)
{
  LOG(logDEBUG) << "nothing done with " << name;
}

//------------------------------------------------------------------------------
PixTestDacScanPix::~PixTestDacScanPix()
{
}

//------------------------------------------------------------------------------
void PixTestDacScanPix::doTest()
{
  fDirectory->cd();
  fHistList.clear();
  PixTest::update();

  LOG(logINFO) << "PixTestDacScanPix::doTest() DAC = " << fParDAC;

  uint8_t maxDac = fApi->getDACRange( fParDAC ); // 15 or 255

  if( maxDac == 0 ) {
    LOG(logINFO) << "ERROR: " << fParDAC << " is not a ROC register";
    return;
  }

  if( fParHiDAC > maxDac ) {
    LOG(logINFO) << fParDAC << " range only " << maxDac;
    fParHiDAC = maxDac;
  }

  LOG(logINFO) << "PixTestDacScanPix::doTest() ntrig = " << fParNtrig;

  fApi->_dut->testAllPixels(false);

  if( fPIX[0].first > -1)
    fApi->_dut->testPixel( fPIX[0].first, fPIX[0].second, true ); // all ROCs

  // measure:

  vector< pair< uint8_t, vector<pixel> > >
    result = fApi->getEfficiencyVsDAC( fParDAC, fParLoDAC, fParHiDAC,
				       0, fParNtrig ); // all ROCs

  // book:

  vector<TH1D*> hsts;
  TH1D *h1(0);

  uint32_t nRocs = fPixSetup->getConfigParameters()->getNrocs();
  uint32_t col = fPIX[0].first;
  uint32_t row = fPIX[0].second;

  for( uint32_t roc = 0; roc < nRocs; ++roc ) {

    h1 = new TH1D( Form( "Readouts_vs_%s_c%d_r%d_C%d",
			 fParDAC.c_str(), col, row, roc ),
		   Form( "Readouts vs %s c%d r%d C%d",
			 fParDAC.c_str(), col, row, roc ),
		   256, -0.5, 255.5 );

    h1->SetMinimum(0);
    setTitles( h1, Form( "%s [DAC]", fParDAC.c_str() ), "readouts" );
    hsts.push_back(h1);
    fHistList.push_back(h1);

  }

  // plot data:

  for( unsigned int i = 0; i < result.size(); ++i ) {

    int idac = result[i].first;

    vector<pixel> vpix = result[i].second;

    for( size_t ipx = 0; ipx < vpix.size(); ++ipx ) {

      uint8_t roc = vpix[ipx].roc_id;

      if( roc < nRocs &&
	  vpix[ipx].column == fPIX[0].first &&
	  vpix[ipx].row == fPIX[0].second ) {
	h1 = hsts.at(roc);
	h1->Fill( idac, vpix[ipx].value ); // already averaged
      } // valid

    } // pix

  } // dacs

  for( size_t roc = 0; roc < nRocs; ++roc ) {
    hsts[roc]->Draw();
    PixTest::update();
  }
  fDisplayedHist = find( fHistList.begin(), fHistList.end(), hsts[nRocs-1] );

}
