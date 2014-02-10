
// Thr vs DAC

#include <stdlib.h>  // atof, atoi
#include <algorithm> // std::find

#include "PixTestDacScanThr.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestDacScanThr)

//------------------------------------------------------------------------------
PixTestDacScanThr::PixTestDacScanThr( PixSetup *a, std::string name )
: PixTest(a, name), fParNtrig(-1),
  fParDAC("nada"), fParLoDAC(-1), fParHiDAC(-1),
  fParCals(0)
{
  PixTest::init();
  init();

  for( size_t i = 0; i < fPIX.size(); ++i )
    LOG(logDEBUG) << "  setting fPIX" << i
		 <<  " ->" << fPIX[i].first
		 << "/" << fPIX[i].second;
}

//------------------------------------------------------------------------------
PixTestDacScanThr::PixTestDacScanThr() : PixTest()
{
  //  LOG(logDEBUG) << "PixTestDacScanThr ctor()";
}

//------------------------------------------------------------------------------
bool PixTestDacScanThr::setParameter( string parName, string sval )
{
  bool found(false);
  string::size_type s1;

  for( uint32_t i = 0; i < fParameters.size(); ++i ) {

    if( fParameters[i].first == parName ) {

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
      if( !parName.compare( "CALS" ) ) {
	fParCals = atoi( sval.c_str() );
	LOG(logDEBUG) << "  setting fParCals  ->" << fParCals
		     << "<- from sval = " << sval;
      }

      if( !parName.compare( "PIX1" ) ) {
	s1 = sval.find( "," );
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
void PixTestDacScanThr::init()
{
  fDirectory = gFile->GetDirectory(fName.c_str());
  if( !fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str());
  }
  fDirectory->cd();
}

// ----------------------------------------------------------------------
void PixTestDacScanThr::setToolTips()
{
  fTestTip = string( "measure pixel threshold vs DAC");
  fSummaryTip = string("summary plot to be implemented");
}

//------------------------------------------------------------------------------
void PixTestDacScanThr::bookHist(string name) // general booking routine
{
  LOG(logDEBUG) << "nothing done with " << name;
}

//------------------------------------------------------------------------------
PixTestDacScanThr::~PixTestDacScanThr()
{
  //  LOG(logDEBUG) << "PixTestDacScanThr dtor";
  std::list<TH1*>::iterator il;
  fDirectory->cd();
  for( il = fHistList.begin(); il != fHistList.end(); ++il) {
    LOG(logINFO) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory);
    (*il)->Write();
  }
}

//------------------------------------------------------------------------------
void PixTestDacScanThr::doTest()
{
  fDirectory->cd();
  fHistList.clear();
  PixTest::update();

  LOG(logINFO) << "PixTestDacScanThr::doTest() DAC = " << fParDAC;

  uint8_t maxDac = fApi->getDACRange( fParDAC ); // 15 or 255

  if( maxDac == 0 ) {
    LOG(logINFO) << "ERROR: " << fParDAC << " is not a ROC register";
    return;
  }

  if( fParHiDAC > maxDac ) {
    LOG(logINFO) << fParDAC << " range only " << maxDac;
    fParHiDAC = maxDac;
  }

  LOG(logINFO) << "PixTestDacScanThr::doTest() ntrig = " << fParNtrig;

  // activate one pixel per ROC:

  size_t nRocs = fPixSetup->getConfigParameters()->getNrocs();

  fApi->_dut->testAllPixels(false);

  if( fPIX[0].first > -1 )
    fApi->_dut->testPixel( fPIX[0].first, fPIX[0].second, true );

  // DACs:

  uint8_t dacVal[16];
  for( uint32_t roc = 0; roc < nRocs; ++roc )
    dacVal[roc] = fApi->_dut->getDAC( roc, fParDAC );
  uint8_t cal = fApi->_dut->getDAC( 0, "Vcal" );
  uint8_t ctl = fApi->_dut->getDAC( 0, "CtrlReg" );
  if( fParCals ) {
    fApi->setDAC( "CtrlReg", 4 ); // all ROCs large Vcal
    LOG(logINFO) << "CtrlReg 4 (large Vcal)";
  }

  // measure:

  uint16_t flags = FLAG_RISING_EDGE; // thr in Vcal units
  if( fParCals ) flags |= FLAG_CALS;
  LOG(logINFO) << "flag " << flags;

  vector < pair < uint8_t, vector<pixel> > > result;

  for( uint8_t idac = fParLoDAC; idac < fParHiDAC; ++idac ) { // careful with uint8_t iterators: < 255

    fApi->setDAC( fParDAC, idac ); // all ROCs

    vector<pixel> vpix = fApi->getThresholdMap( "Vcal", flags, fParNtrig );

    pair < uint8_t, vector<pixel> > p;
    p.first = idac;
    p.second = vpix;
    result.push_back(p);

  } // dacs

  // restore:

  for( uint32_t roc = 0; roc < nRocs; ++roc )
    fApi->setDAC( fParDAC, dacVal[roc], roc );
  fApi->setDAC( "Vcal", cal ); // restore on all ROCs
  LOG(logINFO) << "back to Vcal " << int(cal);

  if( fParCals ) {
    fApi->setDAC( "CtrlReg", ctl ); // restore
    LOG(logINFO) << "back to CtrlReg " << ctl;
  }

  // plot:

  vector<TH1D*> hsts;
  TH1D *h1(0);

  for( size_t roc = 0; roc < nRocs; ++roc ) {

    h1 = new TH1D( Form( "PH_vs_%s_c%02d_r%02d_C%02d",
			 fParDAC.c_str(), fPIX[0].first, fPIX[0].second, int(roc) ),
		   Form( "PH vs %s c%02d r%02d C%02d",
			 fParDAC.c_str(), fPIX[0].first, fPIX[0].second, int(roc) ),
		   256, -0.5, 255.5 );
    h1->SetStats(0);
    h1->SetMinimum(0);
    h1->SetMaximum(256);
    setTitles( h1, Form( "%s [DAC]", fParDAC.c_str() ), "<PH> [ADC]" );
    hsts.push_back(h1);
    fHistList.push_back(h1);

  } // rocs

  // data:

  for( size_t i = 0; i < result.size(); ++i ) {

    int idac = result[i].first;

    vector<pixel> vpix = result[i].second;

    for( size_t ipx = 0; ipx < vpix.size(); ++ipx ) {

      uint8_t thr = vpix[ipx].value;
      if( thr == 255 ) continue; // invalid

      uint8_t roc = vpix[ipx].roc_id;

      if( roc < nRocs &&
	  vpix[ipx].column == fPIX[0].first &&
	  vpix[ipx].row == fPIX[0].second ) {
	h1 = hsts.at(roc);
	h1->Fill( idac, thr );
      } // valid

    } // pix

  } // dac values

  for( size_t roc = 0; roc < nRocs; ++roc ) {
    hsts[roc]->Draw();
    PixTest::update();
  }
  fDisplayedHist = find( fHistList.begin(), fHistList.end(), hsts[nRocs-1] );

}
