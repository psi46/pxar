
// PH vs DAC

#include <stdlib.h>  // atof, atoi
#include <algorithm> // std::find

#include "PixTestDacScanPh.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestDacScanPh)

//------------------------------------------------------------------------------
PixTestDacScanPh::PixTestDacScanPh( PixSetup *a, std::string name )
: PixTest(a, name), fParNtrig(-1),
  fParDAC("nada"), fParLoDAC(-1), fParHiDAC(-1),
  fParCals(0)
{
  PixTest::init();
  init();
}

//------------------------------------------------------------------------------
PixTestDacScanPh::PixTestDacScanPh() : PixTest()
{
  //  LOG(logDEBUG) << "PixTestDacScanPh ctor()";
}

//------------------------------------------------------------------------------
bool PixTestDacScanPh::setParameter( string parName, string sval )
{
  bool found(false);

  for( uint32_t i = 0; i < fParameters.size(); ++i ) {

    if( fParameters[i].first == parName ) {

      found = true;

      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end() );

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
	fParLoDAC = atoi(sval.c_str() );
	LOG(logDEBUG) << "  setting fParLoDAC  ->" << fParLoDAC
		     << "<- from sval = " << sval;
      }
      if( !parName.compare( "DACHI" ) ) {
	fParHiDAC = atoi( sval.c_str() );
	LOG(logDEBUG) << "  setting fParHiDAC  ->" << fParHiDAC
		     << "<- from sval = " << sval;
      }
      if( !parName.compare( "CALS" ) ) {
	fParCals = atoi( sval.c_str() );
	LOG(logDEBUG) << "  setting fParCals  ->" << fParCals
		     << "<- from sval = " << sval;
      }

      if( !parName.compare( "PIX1" ) ) {
	string::size_type s1 = sval.find( "," );
	if( string::npos != s1 ) {
	  string str1 = sval.substr(0, s1);
	  int pixc = atoi( str1.c_str() );
	  string str2 = sval.substr(s1+1);
	  int pixr = atoi( str2.c_str() );
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
void PixTestDacScanPh::init()
{
  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory )
    fDirectory = gFile->mkdir( fName.c_str() );
  fDirectory->cd();
}

// ----------------------------------------------------------------------
void PixTestDacScanPh::setToolTips()
{
  fTestTip = string( "measure pixel pulse height vs DAC");
  fSummaryTip = string("summary plot to be implemented");
}

//------------------------------------------------------------------------------
void PixTestDacScanPh::bookHist(string name) // general booking routine
{
  LOG(logDEBUG) << "nothing done with " << name;
}

//------------------------------------------------------------------------------
PixTestDacScanPh::~PixTestDacScanPh()
{
  std::list<TH1*>::iterator il;
  fDirectory->cd();
  for( il = fHistList.begin(); il != fHistList.end(); ++il ) {
    LOG(logINFO) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory( fDirectory );
    (*il)->Write();
  }
}

//------------------------------------------------------------------------------
void PixTestDacScanPh::doTest()
{
  fDirectory->cd();
  fHistList.clear();
  PixTest::update();

  LOG(logINFO) << "PixTestCurrentVsDac::doTest() DAC = " << fParDAC;

  uint8_t maxDac = fApi->getDACRange( fParDAC ); // 15 or 255

  if( maxDac == 0 ) {
    LOG(logINFO) << "ERROR: " << fParDAC << " is not a ROC register";
    return;
  }

  if( fParHiDAC > maxDac ) {
    LOG(logINFO) << fParDAC << " range only " << maxDac;
    fParHiDAC = maxDac;
  }

  LOG(logINFO) << "PixTestDacScanPh::doTest() ntrig = " << fParNtrig;

  // activate one pixel per ROC:

  fApi->_dut->testAllPixels(false);

  if( fPIX[0].first > -1 )
    fApi->_dut->testPixel( fPIX[0].first, fPIX[0].second, true );

  // measure:

  uint16_t flags = 0;
  if( fParCals ) flags = FLAG_CALS;
  LOG(logINFO) << "flag " << flags;
  uint8_t ctl = fApi->_dut->getDAC( 0, "CtrlReg" );
  if( fParCals ) {
    fApi->setDAC( "CtrlReg", 4 ); // all ROCs large Vcal
    LOG(logINFO) << "CtrlReg 4 (large Vcal)";
  }

  vector<pair<uint8_t, vector<pixel> > > // dac and pix
    result = fApi->getPulseheightVsDAC( fParDAC, fParLoDAC, fParHiDAC, flags, fParNtrig );

  if( fParCals ) {
    fApi->setDAC( "CtrlReg", ctl ); // restore
    LOG(logINFO) << "back to CtrlReg " << ctl;
  }

  // book:

  vector<TH1D*> hsts;
  TH1D *h1(0);

  uint32_t nRocs = fPixSetup->getConfigParameters()->getNrocs();
  uint32_t col = fPIX[0].first;
  uint32_t row = fPIX[0].second;

  for( uint32_t roc = 0; roc < nRocs; ++roc ) {

    h1 = new TH1D( Form( "PH_vs_%s_c%02d_r%02d_C%02d",
			 fParDAC.c_str(), col, row, roc ),
		   Form( "PH vs %s c%02d r%02d C%02d",
			 fParDAC.c_str(), col, row, roc ),
		   256, -0.5, 255.5 );
    h1->SetMinimum(0);
    h1->SetMaximum(256);
    setTitles( h1, Form( "%s [DAC]", fParDAC.c_str() ), "<PH> [ADC]" );
    hsts.push_back(h1);
    fHistList.push_back(h1);

  } // rocs

  // plot data:

  for( size_t i = 0; i < result.size(); ++i ) {

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

  } // dac values

  for( size_t roc = 0; roc < nRocs; ++roc ) {
    hsts[roc]->Draw();
    PixTest::update();
  }
  fDisplayedHist = find( fHistList.begin(), fHistList.end(), hsts[nRocs-1] );

  // test:

  flags = FLAG_RISING_EDGE;
  vector<pixel> vpix9 = fApi->getThresholdMap( "Vcal", flags, fParNtrig );
  for( size_t ipx = 0; ipx < vpix9.size(); ++ipx )
    LOG(logINFO)
      << "roc " << setw(2) << (int) vpix9[ipx].roc_id
      << " pix " << setw(2) << (int) vpix9[ipx].column
      << " " << setw(2) << (int) vpix9[ipx].row
      << " thr " << setw(3) << vpix9[ipx].value;

  if( fPIX[0].first > -1 )
    fApi->_dut->testPixel( fPIX[0].first, fPIX[0].second, false );

}
