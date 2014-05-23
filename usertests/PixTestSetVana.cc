// -- author: Daniel Pitzl
// set Vana to get desired target Ia per ROC

#include <cmath>      // fabs
#include <stdlib.h>   // atof, atoi
#include <algorithm>  // std::find
#include <iostream>

#include <TH1.h>
#include <TStopwatch.h>

#include "PixTestSetVana.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestSetVana)

//------------------------------------------------------------------------------
PixTestSetVana::PixTestSetVana( PixSetup *a, std::string name )
: PixTest(a, name), fTargetIa(24)
{
  PixTest::init();
  init();
}

//------------------------------------------------------------------------------
PixTestSetVana::PixTestSetVana() : PixTest()
{
  //LOG(logDEBUG) << "PixTestSetVana ctor()";
}

//------------------------------------------------------------------------------
bool PixTestSetVana::setParameter( string parName, string sval ) {
  bool found(false);
  string stripParName; 
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 

      LOG(logDEBUG) << "  ==> parName: " << parName;
      LOG(logDEBUG) << "  ==> sval:    " << sval;
      if (!parName.compare("targetia")) {
	fTargetIa = atoi(sval.c_str()); 
	setToolTips();
      }
      break;
    }
  }
  return found; 
}

//------------------------------------------------------------------------------
void PixTestSetVana::init()
{
  fDirectory = gFile->GetDirectory( fName.c_str() ); // fName set in PixTest
  if( !fDirectory )
    fDirectory = gFile->mkdir( fName.c_str() );
  fDirectory->cd();
}

//------------------------------------------------------------------------------
void PixTestSetVana::bookHist(string name) // no histos
{
  LOG(logDEBUG) << "not using " << name; 
  fDirectory->cd();
  fHistList.clear();
}

//------------------------------------------------------------------------------
PixTestSetVana::~PixTestSetVana()
{
  LOG(logDEBUG) << "PixTestSetVana dtor";
  std::list<TH1*>::iterator il;
  fDirectory->cd();
  for( il = fHistList.begin(); il != fHistList.end(); ++il) {
    LOG(logDEBUG) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory);
    (*il)->Write();
  }
}

//------------------------------------------------------------------------------
void PixTestSetVana::doTest()
{
  fDirectory->cd();
  PixTest::update();

  LOG(logINFO) << "PixTestSetVana::doTest() target Ia " << fTargetIa << " mA/ROC";

  fApi->_dut->testAllPixels(false);

  bookHist( "empty" );

  int vana16[16] = {0};

  uint32_t nRocs = fPixSetup->getConfigParameters()->getNrocs();

  // set all ROCs to minimum:

  for( uint32_t roc = 0; roc < nRocs; ++roc ) {
    vana16[roc] = fApi->_dut->getDAC( roc, "vana" ); // remember as start value
    fApi->setDAC( "vana", 0, roc ); // off
  } // rocs

  double i016 = fApi->getTBia()*1E3;

  TStopwatch sw;

  sw.Start(kTRUE); // reset
  do {
    sw.Start(kFALSE); // continue
    i016 = fApi->getTBia()*1E3;
  }
  while( sw.RealTime() < 0.1 );

  LOG(logINFO) << "delay " << sw.RealTime() << " s"
	       << ", Stopwatch counter " << sw.Counter();

  // i016 = nRocs * current01
  // subtract one ROC to get the offset from the other Rocs (on average):

  double i015 = (nRocs-1) * i016 / nRocs; // = 0 for single chip tests

  LOG(logINFO) << "offset current from other " << nRocs-1 << " ROCs is "
	       << i015 << " mA";

  // tune per ROC:

  const double extra = 0.1; // [mA] besser zu viel als zu wenig 
  const double eps = 0.25; // [mA] convergence
  const double slope = 6; // 255 DACs / 40 mA

  for( uint32_t roc = 0; roc < nRocs; ++roc ) {

    int vana = vana16[roc];
    fApi->setDAC( "vana", vana, roc ); // start value
    // delay

    double ia = fApi->getTBia()*1E3; // [mA], just to be sure to flush usb

    sw.Start(kTRUE); // reset
    do {
      sw.Start(kFALSE); // continue
      ia = fApi->getTBia()*1E3; // [mA]
    }
    while( sw.RealTime() < 0.1 );

    LOG(logINFO) << "delay " << sw.RealTime() << " s"
		 << ", Stopwatch counter " << sw.Counter();

    double diff = fTargetIa + extra - (ia - i015);

    int iter = 0;
    LOG(logINFO) << "ROC " << roc << " iter " << iter
		 << " Vana " << vana
		 << " Ia " << ia-i015 << " mA";

    while( fabs(diff) > eps && iter < 11 && vana > 0 && vana < 255 ) {

      int stp = int( fabs(slope*diff) );
      if( stp == 0 ) stp = 1;
      if( diff < 0 ) stp = -stp;

      vana += stp;

      if( vana < 0 )
	vana = 0;
      else
	if( vana > 255 )
	  vana = 255;

      fApi->setDAC( "vana", vana, roc );
      iter++;

      sw.Start(kTRUE); // reset
      do {
	sw.Start(kFALSE); // continue
	ia = fApi->getTBia()*1E3; // [mA]
      }
      while( sw.RealTime() < 0.1 );

      diff = fTargetIa + extra - (ia - i015);

      LOG(logINFO) << "ROC " << roc << " iter " << iter
		   << " Vana " << vana
		   << " Ia " << ia-i015 << " mA";
    } // iter

    vana16[roc] = vana; // remember best
    fApi->setDAC( "vana", 0, roc ); // switch off for next ROC

  } // rocs

  // set all ROCs to optimum:

  for( uint32_t roc = 0; roc < nRocs; ++roc ) {
    fApi->setDAC( "vana", vana16[roc], roc );
    LOG(logINFO) << "ROC " << setw(2) << roc
		 << " Vana " << vana16[roc];
  }

  double ia16 = fApi->getTBia()*1E3; // [mA]

  sw.Start(kTRUE); // reset
  do {
    sw.Start(kFALSE); // continue
    ia16 = fApi->getTBia()*1E3; // [mA]
  }
  while( sw.RealTime() < 0.1 );

  LOG(logINFO) << "Module Ia " << ia16 << " mA = " << ia16/nRocs << " mA/ROC";
}
