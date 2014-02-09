
// ROC readout count vs DAC

#include <stdlib.h>   // atof, atoi
#include <algorithm>  // std::find

#include <TStopwatch.h>

#include "PixTestDacScanRoc.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestDacScanRoc)

//------------------------------------------------------------------------------
PixTestDacScanRoc::PixTestDacScanRoc( PixSetup *a, std::string name )
: PixTest(a, name), fParNtrig(-1), fParDAC("nada"),
  fParLoDAC(-1), fParHiDAC(-1)
{
  PixTest::init(a, name);
  init();
}

//------------------------------------------------------------------------------
PixTestDacScanRoc::PixTestDacScanRoc() : PixTest()
{
}

//------------------------------------------------------------------------------
bool PixTestDacScanRoc::setParameter( string parName, string sval )
{
  bool found(false);

  for( map<string,string>::iterator imap = fParameters.begin();
       imap != fParameters.end(); ++imap ) {

    LOG(logDEBUG) << "---> " << imap->first;

    if( 0 == imap->first.compare(parName) ) {

      found = true;

      sval.erase( remove( sval.begin(), sval.end(), ' '), sval.end() );
      fParameters[parName] = sval;

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

      break;
    }
  }
  return found;
}

//------------------------------------------------------------------------------
void PixTestDacScanRoc::init()
{
  setToolTips();
  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory )
    fDirectory = gFile->mkdir( fName.c_str() );
  fDirectory->cd();
}

//------------------------------------------------------------------------------
void PixTestDacScanRoc::setToolTips() {
  fTestTip =
    string( Form( "scan DAC %s, count pixel responses\n", fParDAC.c_str() ) )
    ;
  fSummaryTip = string("summary plot to be implemented")
    ;
}

//------------------------------------------------------------------------------
void PixTestDacScanRoc::bookHist(string name)
{
  LOG(logDEBUG) << "nothing done with " << name;
}

//------------------------------------------------------------------------------
PixTestDacScanRoc::~PixTestDacScanRoc()
{
}

//------------------------------------------------------------------------------
void PixTestDacScanRoc::doTest()
{
  fDirectory->cd();
  fHistList.clear();
  PixTest::update();

  LOG(logINFO) << "PixTestDacScanRoc::doTest() DAC = " << fParDAC;

  uint8_t maxDac = fApi->getDACRange( fParDAC ); // 15 or 255

  if( maxDac == 0 ) {
    LOG(logINFO) << "ERROR: " << fParDAC << " is not a ROC register";
    return;
  }

  if( fParHiDAC > maxDac ) {
    LOG(logINFO) << fParDAC << " range only " << maxDac;
    fParHiDAC = maxDac;
  }

  uint32_t nRocs = fPixSetup->getConfigParameters()->getNrocs();

  uint8_t dacVal[16];
  for( uint32_t roc = 0; roc < nRocs; ++roc )
    dacVal[roc] = fApi->_dut->getDAC( roc, fParDAC );

  LOG(logINFO) << "PixTestDacScanRoc::doTest() ntrig = " << fParNtrig;

  fApi->_dut->testAllPixels(true);

  // book:

  vector<TH1D*> hAlive;
  vector<TH1D*> hPerfect;
  TH1D *h1(0);

  for( uint32_t roc = 0; roc < nRocs; ++roc ) {

    h1 = new TH1D( Form( "Alive_pixels_vs_%s_C%d",
			 fParDAC.c_str(), roc ),
		   Form( "Alive pixels vs %s C%d",
			 fParDAC.c_str(), roc ),
		   256, -0.5, 255.5 );

    h1->SetMinimum(0);
    h1->SetMaximum(4200);
    setTitles( h1, Form( "%s [DAC]", fParDAC.c_str() ), "alive pixels" );
    hAlive.push_back(h1);
    fHistList.push_back(h1);

    h1 = new TH1D( Form( "Perfect_pixels_vs_%s_C%d",
			 fParDAC.c_str(), roc ),
		   Form( "Perfect pixels vs %s C%d",
			 fParDAC.c_str(), roc ),
		   256, -0.5, 255.5 );

    h1->SetMinimum(0);
    h1->SetMaximum(4200);
    setTitles( h1, Form( "%s [DAC]", fParDAC.c_str() ), "perfect pixels" );
    hPerfect.push_back(h1);
    fHistList.push_back(h1);

  }

  TStopwatch sw;

  // loop over DAC values:

  for( int32_t idac = fParLoDAC; idac < fParHiDAC; ++idac ) { // careful with uint8_t iterators: < 255

    fApi->setDAC( fParDAC, idac ); // all ROCs

    // delay:

    sw.Start(kTRUE); // reset stopwatch
    double id = 0;
    do {
      sw.Start(kFALSE); // continue
      id = fApi->getTBid()*1E3; // [mA]
    }
    while( sw.RealTime() < 0.1 ); // [s]

    // measure:

    vector<pixel> vpix = fApi->getEfficiencyMap( 0, fParNtrig ); // all pix, ROCs

    LOG(logINFO) << fParDAC << setw(4) << idac << " " << id << " mA"
		 << setw(7) << vpix.size();

    // plot data:

    for( size_t ipx = 0; ipx < vpix.size(); ++ipx ) {

      uint8_t roc = vpix[ipx].roc_id;

      if( roc < nRocs ) {
	int32_t readouts = vpix[ipx].value;
	if( readouts == fParNtrig )
	  hPerfect[roc]->Fill( idac );
	if( readouts >= fParNtrig/2 )
	  hAlive[roc]->Fill( idac );
      } // valid

    } // pix

  } // dacs

  // restore:

  for( uint32_t roc = 0; roc < nRocs; ++roc )
    fApi->setDAC( fParDAC, dacVal[roc], roc );

  // draw:

  for( size_t roc = 0; roc < nRocs; ++roc ) {
    hPerfect[roc]->Draw();
    PixTest::update();
  }
  fDisplayedHist = find( fHistList.begin(), fHistList.end(), hPerfect[nRocs-1] );

}
