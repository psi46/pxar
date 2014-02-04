
// set VthrComp safely away from noise, using Id vs VthrComp

#include <stdlib.h>  // atof, atoi
#include <algorithm> // std::find
#include <iostream>

#include <TH1.h>
#include <TStopwatch.h>

#include "PixTestSetVthrComp.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestSetVthrComp)

//------------------------------------------------------------------------------
PixTestSetVthrComp::PixTestSetVthrComp( PixSetup *a, std::string name )
: PixTest(a, name)
{
  PixTest::init( a, name );
  init();
}

//------------------------------------------------------------------------------
PixTestSetVthrComp::PixTestSetVthrComp() : PixTest()
{
  //LOG(logDEBUG) << "PixTestSetVthrComp ctor()";
}

//------------------------------------------------------------------------------
bool PixTestSetVthrComp::setParameter( string parName, string sval )
{
  bool found(false);

  for( map<string,string>::iterator imap = fParameters.begin(); imap != fParameters.end(); ++imap ) {
    LOG(logDEBUG) << "---> " << imap->first;
    if( 0 == imap->first.compare(parName) ) {
      found = true;
      sval.erase( remove( sval.begin(), sval.end(), ' ' ), sval.end() );

      fParameters[parName] = sval;

      break;
    }
  }
  return found;
}

//------------------------------------------------------------------------------
void PixTestSetVthrComp::init()
{
  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory )
    fDirectory = gFile->mkdir( fName.c_str() );
  fDirectory->cd();
}

//------------------------------------------------------------------------------
void PixTestSetVthrComp::bookHist(string name) // general booking routine
{
}

//------------------------------------------------------------------------------
PixTestSetVthrComp::~PixTestSetVthrComp()
{
  LOG(logDEBUG) << "PixTestSetVthrComp dtor";
  std::list<TH1*>::iterator il;
  fDirectory->cd();
  for( il = fHistList.begin(); il != fHistList.end(); ++il) {
    LOG(logDEBUG) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory);
    (*il)->Write();
  }
}

//------------------------------------------------------------------------------
void PixTestSetVthrComp::doTest()
{
  LOG(logINFO) << "PixTestSetVthrComp::doTest()";

  fDirectory->cd();
  PixTest::update();

  fHistList.clear();
  vector<TH1D*> hsts;
  TH1D *h1(0);

  size_t nRocs = fPixSetup->getConfigParameters()->getNrocs();

  for( size_t roc = 0; roc < nRocs; ++roc ) {

    h1 = new TH1D( Form( "Id_vs_VthrComp_C%d", int(roc) ),
		   Form( "Id vs VthrComp C%d", int(roc) ),
		   256, -0.5, 255.5 );
    h1->SetMinimum(0);
    h1->SetStats(0);
    setTitles( h1, "VthrComp [DAC]", "ROC digital current [mA]" );
    hsts.push_back(h1);
    fHistList.push_back(h1);
  }

  fApi->_dut->testAllPixels(true); // enable all pix: more noise

  // set all ROCs to minimum Id:

  int vsf16[16] = {0};
  int vthr16[16] = {0};

  for( size_t roc = 0; roc < nRocs; ++roc ) {
    vsf16[roc] = fApi->_dut->getDAC( roc, "Vsf" ); // remember
    vthr16[roc] = fApi->_dut->getDAC( roc, "VthrComp" ); // remember
    fApi->setDAC( "Vsf", 33, roc ); // small
    fApi->setDAC( "VthrComp", 0, roc ); // off
  }

  double i016 = fApi->getTBid()*1E3;
  TStopwatch sw;
  sw.Start(kTRUE); // reset
  do {
    sw.Start(kFALSE); // continue
    i016 = fApi->getTBid()*1E3;
  }
  while( sw.RealTime() < 0.1 ); // discharge time

  double i015 = (nRocs-1) * i016 / nRocs; // = 0 for single chip tests

  LOG(logINFO) << "offset current from other " << nRocs-1 << " ROCs is "
	       << i015 << " mA";

  TH1D *hid(0);

  // loope over ROCs:

  for( uint32_t roc = 0; roc < nRocs; ++roc ) {

    LOG(logINFO) << "ROC " << setw(2) << roc;
      
    hid = hsts[roc];

    // loop over DAC:

    for( size_t idac = 0; idac < 256; ++idac ) {

      fApi->setDAC( "VthrComp", idac, roc );
      // delay?
      hid->Fill( idac, fApi->getTBid()*1E3 - i015 );

    } // dac

    fApi->setDAC( "VthrComp", 0, roc ); // switch off

    fDisplayedHist = find( fHistList.begin(), fHistList.end(), hid );
    if( hid ) hid->Draw();
    PixTest::update();

    // analyze:

    LOG(logINFO) << "current peak " << hid->GetMaximum()
		 << " mA at DAC " << hid->GetMaximumBin();

    double maxd1 = 0;
    double maxd2 = 0;
    double maxd4 = 0;
    double maxd7 = 0;
    double maxd11 = 0;
    double maxd16 = 0;
    double maxd22 = 0;
    double maxd29 = 0;
    int maxi1 = 0;
    int maxi2 = 0;
    int maxi4 = 0;
    int maxi7 = 0;
    int maxi11 = 0;
    int maxi16 = 0;
    int maxi22 = 0;
    int maxi29 = 0;
    for( int i = 1; i <= 256-29; ++i ) { // histo bin counting starts at 1
      double ni = hid->GetBinContent(i);
      double d1 = hid->GetBinContent(i+1)-ni;
      double d2 = hid->GetBinContent(i+2)-ni;
      double d4 = hid->GetBinContent(i+4)-ni;
      double d7 = hid->GetBinContent(i+7)-ni;
      double d11 = hid->GetBinContent(i+11)-ni;
      double d16 = hid->GetBinContent(i+16)-ni;
      double d22 = hid->GetBinContent(i+22)-ni;
      double d29 = hid->GetBinContent(i+29)-ni;
      if( d1 > maxd1 ) { maxd1 = d1; maxi1 = i-1; }
      if( d2 > maxd2 ) { maxd2 = d2; maxi2 = i-1; }
      if( d4 > maxd4 ) { maxd4 = d4; maxi4 = i-1; }
      if( d7 > maxd7 ) { maxd7 = d7; maxi7 = i-1; }
      if( d11 > maxd11 ) { maxd11 = d11; maxi11 = i-1; }
      if( d16 > maxd16 ) { maxd16 = d16; maxi16 = i-1; }
      if( d22 > maxd22 ) { maxd22 = d22; maxi22 = i-1; }
      if( d29 > maxd29 ) { maxd29 = d29; maxi29 = i-1; }
    }
    LOG(logINFO) << "[SetComp] max d1  " << maxd1  << " at " << maxi1;
    LOG(logINFO) << "[SetComp] max d2  " << maxd2  << " at " << maxi2;
    LOG(logINFO) << "[SetComp] max d4  " << maxd4  << " at " << maxi4;
    LOG(logINFO) << "[SetComp] max d7  " << maxd7  << " at " << maxi7;
    LOG(logINFO) << "[SetComp] max d11 " << maxd11 << " at " << maxi11;
    LOG(logINFO) << "[SetComp] max d16 " << maxd16 << " at " << maxi16;
    LOG(logINFO) << "[SetComp] max d22 " << maxd22 << " at " << maxi22;
    LOG(logINFO) << "[SetComp] max d29 " << maxd29 << " at " << maxi29;

    //int32_t val = maxi22 - 8; // safety
    int32_t val = maxi29 - 8; // safety
    if( val < 0 ) val = 0;
    vthr16[roc] = val;

    LOG(logINFO) << "set VthrComp to " << val;

  } // rocs

  TH1D *hsum = new TH1D( "VthrCompSettings", "VthrComp per ROC;ROC;VthrComp [DAC]",
			 16, -0.5, 15.5 );
  hsum->SetStats(0); // no stats
  hsum->SetMinimum(0);
  hsum->SetMaximum(256);
  fHistList.push_back(hsum);

  // restore:

  for( size_t roc = 0; roc < nRocs; ++roc ) {
    fApi->setDAC( "Vsf", vsf16[roc], roc );
    fApi->setDAC( "VthrComp", vthr16[roc], roc );
    LOG(logINFO) << "ROC " << setw(2) << roc
		 << " VthrComp " << setw(3) << vthr16[roc];
    hsum->Fill( roc, vthr16[roc] );
  }

  hsum->Draw();
  PixTest::update();

  // flush?
}
