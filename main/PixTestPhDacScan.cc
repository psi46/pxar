
// PH vs DAC

#include <stdlib.h>  // atof, atoi
#include <algorithm> // std::find

#include "PixTestPhDacScan.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestPhDacScan)

//------------------------------------------------------------------------------
PixTestPhDacScan::PixTestPhDacScan( PixSetup *a, std::string name )
: PixTest(a, name), fParNtrig(-1), fParDAC("nada"), fParLoDAC(-1), fParHiDAC(-1)
{
  PixTest::init( a, name );
  init();

  for( size_t i = 0; i < fPIX.size(); ++i )
    LOG(logDEBUG) << "  setting fPIX" << i
		 <<  " ->" << fPIX[i].first
		 << "/" << fPIX[i].second;
}

//------------------------------------------------------------------------------
PixTestPhDacScan::PixTestPhDacScan() : PixTest()
{
  //  LOG(logDEBUG) << "PixTestPhDacScan ctor()";
}

//------------------------------------------------------------------------------
bool PixTestPhDacScan::setParameter(string parName, string sval)
{
  bool found(false);
  string str1, str2;
  string::size_type s1;
  int pixc, pixr;
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true;
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());
      if( !parName.compare( "Ntrig" ) ) {
	fParNtrig = atoi( sval.c_str() );
	LOG(logDEBUG) << "  setting fParNtrig  ->" << fParNtrig
		     << "<- from sval = " << sval;
      }
      if( !parName.compare( "DAC" ) ) {
	fParDAC = sval;
	LOG(logDEBUG) << "  setting fParDAC  ->" << fParDAC
		     << "<- from sval = " << sval;
      }
      if( !parName.compare( "DACLO" ) ) {
	fParLoDAC = atoi(sval.c_str());
	LOG(logDEBUG) << "  setting fParLoDAC  ->" << fParLoDAC
		     << "<- from sval = " << sval;
      }
      if( !parName.compare( "DACHI" ) ) {
	fParHiDAC = atoi( sval.c_str() );
	LOG(logDEBUG) << "  setting fParHiDAC  ->" << fParHiDAC
		     << "<- from sval = " << sval;
      }
      if( !parName.compare( "PIX1" ) ) {
	s1 = sval.find( "," );
	if( string::npos != s1 ) {
	  str1 = sval.substr(0, s1);
	  pixc = atoi( str1.c_str() );
	  str2 = sval.substr(s1+1);
	  pixr = atoi( str2.c_str() );
	  fPIX.push_back( make_pair( pixc, pixr ) );
	}
	else {
	  fPIX.push_back( make_pair( -1, -1 ) );
	}
      }
      // FIXME: remove/update from fPIX if the user removes via the GUI!
      break;
    }
  }
  return found;
}

//------------------------------------------------------------------------------
void PixTestPhDacScan::init()
{
  fDirectory = gFile->GetDirectory(fName.c_str());
  if( !fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str());
  }
  fDirectory->cd();
}

//------------------------------------------------------------------------------
void PixTestPhDacScan::bookHist(string name) // general booking routine
{
  LOG(logDEBUG) << "nothing done with " << name;
}

//------------------------------------------------------------------------------
PixTestPhDacScan::~PixTestPhDacScan()
{
  //  LOG(logDEBUG) << "PixTestPhDacScan dtor";
  std::list<TH1*>::iterator il;
  fDirectory->cd();
  for( il = fHistList.begin(); il != fHistList.end(); ++il) {
    LOG(logINFO) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory);
    (*il)->Write();
  }
}

//------------------------------------------------------------------------------
void PixTestPhDacScan::doTest()
{
  fDirectory->cd();
  fHistList.clear();
  PixTest::update();

  LOG(logINFO) << "PixTestPhDacScan::doTest() ntrig = " << fParNtrig;

  // activate one pixel per ROC:

  fApi->_dut->testAllPixels(false);

  if( fPIX[0].first > -1 )
    fApi->_dut->testPixel( fPIX[0].first, fPIX[0].second, true );

  // book histos:

  vector<TH1D*> hsts;
  TH1D *h1(0);

  size_t nRocs = fPixSetup->getConfigParameters()->getNrocs();

  for( size_t roc = 0; roc < nRocs; ++roc ) {

    h1 = new TH1D( Form( "PH_vs_%s_c%02d_r%02d_C%02d",
			 fParDAC.c_str(), fPIX[0].first, fPIX[0].second, int(roc) ),
		   Form( "PH vs %s c%02d r%02d C%02d",
			 fParDAC.c_str(), fPIX[0].first, fPIX[0].second, int(roc) ),
		   256, -0.5, 255.5 );
    h1->SetMinimum(0);
    setTitles( h1, Form( "%s [DAC]", fParDAC.c_str() ), "<PH> [ADC]" );
    hsts.push_back(h1);
    fHistList.push_back(h1);

  } // rocs

  // measure:

  vector<pair<uint8_t, vector<pixel> > > // dac and pix
    result = fApi->getPulseheightVsDAC( fParDAC, fParLoDAC, fParHiDAC, 0, fParNtrig );

  LOG(logINFO) << "dacscandata.size(): " << result.size();

  // plot:

  for( unsigned int i = 0; i < result.size(); ++i ) {

    int idac = result[i].first;

    vector<pixel> vpix = result[i].second;

    for( unsigned int ipx = 0; ipx < vpix.size(); ++ipx ) {

      int roc = vpix[ipx].roc_id;
      h1 = hsts.at(roc);

      if( vpix[ipx].column == fPIX[0].first &&
	  vpix[ipx].row == fPIX[0].second )
	h1->Fill( idac, vpix[ipx].value ); // already averaged

    } // pix

  } // dac values

  for( list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il ) {
    (*il)->Draw();
    PixTest::update();
  }
  fDisplayedHist = fHistList.begin();

}
