
// scan a DAC, measure currents

#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>

#include <TH1.h>

#include "PixTestCurrentVsDac.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestCurrentVsDac)

// ----------------------------------------------------------------------
PixTestCurrentVsDac::PixTestCurrentVsDac( PixSetup *a, std::string name )
: PixTest(a, name), fParDAC("nada")
{
  PixTest::init( a, name );
  init();
}

//----------------------------------------------------------
PixTestCurrentVsDac::PixTestCurrentVsDac() : PixTest()
{
  LOG(logINFO) << "PixTestCurrentVsDac ctor()";
}

// ----------------------------------------------------------------------
bool PixTestCurrentVsDac::setParameter( string parName, string sval )
{
  bool found(false);

  for( map<string,string>::iterator imap = fParameters.begin(); imap != fParameters.end(); ++imap ) {
    LOG(logINFO) << "---> " << imap->first;
    if( 0 == imap->first.compare(parName) ) {
      found = true;
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());
      fParameters[parName] = sval;

      if( !parName.compare( "DAC" ) ) {
	fParDAC = sval;
	LOG(logINFO) << "  setting fParDAC  ->" << fParDAC
		     << "<- from sval = " << sval;
      }

      break;
    }
  }
  
  return found;
}

// ----------------------------------------------------------------------
void PixTestCurrentVsDac::init()
{
  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory )
    fDirectory = gFile->mkdir( fName.c_str() );
  fDirectory->cd();
}

// ----------------------------------------------------------------------
void PixTestCurrentVsDac::bookHist(string name) // general booking routine
{
  fDirectory->cd();

  TH1D *h1(0);
  fHistList.clear();
  for( uint32_t i = 0; i < fPixSetup->getConfigParameters()->getNrocs(); ++i ) {

    h1 = new TH1D( Form( "Ia_vs_%s_C%d", name.c_str(), i),
		   Form( "Ia vs %s C%d", name.c_str(), i),
		   256, 0, 256 );
    h1->SetMinimum(0);
    h1->SetStats(0);
    setTitles( h1, Form( "%s [DAC]", name.c_str() ), "analog current [mA]" );
    fHistList.push_back(h1);

    h1 = new TH1D( Form( "Id_vs_%s_C%d", name.c_str(), i),
		   Form( "Id vs %s C%d", name.c_str(), i),
		   256, 0, 256 );
    h1->SetMinimum(0);
    h1->SetStats(0);
    setTitles( h1, Form( "%s [DAC]", name.c_str() ), "digital current [mA]" );
    fHistList.push_back(h1);
  }
}

//----------------------------------------------------------
PixTestCurrentVsDac::~PixTestCurrentVsDac()
{
  //  LOG(logINFO) << "PixTestCurrentVsDac dtor";
  std::list<TH1*>::iterator il;
  fDirectory->cd();
  for( il = fHistList.begin(); il != fHistList.end(); ++il) {
    LOG(logINFO) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory);
    (*il)->Write();
  }
}

// ----------------------------------------------------------------------
void PixTestCurrentVsDac::doTest() {
  fDirectory->cd();
  PixTest::update();

  LOG(logINFO) << "PixTestCurrentVsDac::doTest() DAC = " << fParDAC;

  fApi->_dut->testAllPixels(false);

  bookHist( fParDAC );

  TH1D *hia(0);
  TH1D *hid(0);
  
  for( uint32_t roc = 0; roc < fPixSetup->getConfigParameters()->getNrocs(); ++roc ) {
    
    hia = (TH1D*)fDirectory->Get( Form( "Ia_vs_%s_C%d", fParDAC.c_str(), roc ) );
    hid = (TH1D*)fDirectory->Get( Form( "Id_vs_%s_C%d", fParDAC.c_str(), roc ) );
    
    if( hia && hid ) {
      
      // remember DAC
      
      uint8_t dacval = fApi->_dut->getDAC( roc, fParDAC );
      
      // loop over DAC
      
      for( int idac = 0; idac < 256; ++idac ) {
	
	fApi->setDAC( fParDAC, idac, roc );
	// delay
	hia->SetBinContent( idac+1, fApi->getTBia()*1E3 );
	hid->SetBinContent( idac+1, fApi->getTBid()*1E3 );
      }
      
      fApi->setDAC( fParDAC, dacval, roc ); // restore
    }
    else {
      LOG(logINFO) << "XX did not find "
		   << Form( "Ia_vs_%s_C%d", fParDAC.c_str(), roc );
    }
    fDisplayedHist = find( fHistList.begin(), fHistList.end(), hid );
    if( hid ) hid->Draw();
    PixTest::update();

  } // rocs

}
