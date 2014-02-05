#include <stdlib.h>  // atof, atoi
#include <algorithm> // std::find
#include <iostream>  // cout
#include <iomanip>   // setw

#include <TH1.h>
#include <TStopwatch.h>

#include "PixTestPretest.hh"
#include "log.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestPretest)

//------------------------------------------------------------------------------
PixTestPretest::PixTestPretest( PixSetup *a, std::string name) :
PixTest(a, name), fParNtrig(-1), fTargetIa(-1)
{
  PixTest::init(a, name);
  init(); 
  //  LOG(logINFO) << "PixTestPretest ctor(PixSetup &a, string, TGTab *)";
  for( unsigned int i = 0; i < fPIX.size(); ++i) {
    LOG(logDEBUG) << " PixTestPretest setting fPIX" << i
		  <<  " ->" << fPIX[i].first
		  << "/" << fPIX[i].second;
  }
}

//------------------------------------------------------------------------------
PixTestPretest::PixTestPretest() : PixTest() {
  //  LOG(logINFO) << "PixTestPretest ctor()";
}

//------------------------------------------------------------------------------
bool PixTestPretest::setParameter(string parName, string sval) {
  bool found(false);
  string str1, str2; 
  string::size_type s1;
  int pixc, pixr; 
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true;
      sval.erase( remove( sval.begin(), sval.end(), ' '), sval.end() );
      if( !parName.compare("targetIa")) {
	fTargetIa = atoi(sval.c_str());  // [mA/ROC]
	LOG(logDEBUG) << "target Ia " << fTargetIa << " mA/ROC";
      }

      if (!parName.compare("Ntrig") ) {
	fParNtrig = atoi(sval.c_str() );
	LOG(logDEBUG) << " PixTestPretest setting fParNtrig  ->" << fParNtrig
		      << "<- from sval = " << sval;
      }

      if (!parName.compare("PIX1") ) {
	s1 = sval.find(",");
	if (string::npos != s1) {
	  str1 = sval.substr(0, s1);
	  pixc = atoi(str1.c_str());
	  str2 = sval.substr(s1+1);
	  pixr = atoi(str2.c_str());
	  fPIX.push_back( make_pair(pixc, pixr) );
	}
      }
      // FIXME: remove/update from fPIX if the user removes via the GUI!
      break;
    }
  }
  return found;
}

//------------------------------------------------------------------------------
void PixTestPretest::init()
{
  setToolTips();
  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory ) {
    fDirectory = gFile->mkdir( fName.c_str() );
  }
  fDirectory->cd();
}

//------------------------------------------------------------------------------
void PixTestPretest::setToolTips()
{
  fTestTip = string( "scan and set CalDel using one pixel");
  fSummaryTip = string("summary plot to be implemented");
}

//------------------------------------------------------------------------------
void PixTestPretest::bookHist(string name) {
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
PixTestPretest::~PixTestPretest() {
  LOG(logDEBUG) << "PixTestPretest dtor";
}

//------------------------------------------------------------------------------
void PixTestPretest::doTest() {
  fDirectory->cd();
  PixTest::update(); 
  LOG(logINFO) << "PixTestPretest::doTest() ntrig = " << fParNtrig;

  setVana();
  //  saveDacs();
  setCalDel(); 
  //  saveDacs();
  setVthrCompId();
  //  saveDacs();
}



// ----------------------------------------------------------------------
void PixTestPretest::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;
  if (!command.compare("setcaldel")) {
    setCalDel(); 
    return;
  }
  if (!command.compare("savedacs")) {
    saveDacs(); 
    return;
  }
  if (!command.compare("setvana")) {
    setVana(); 
    return;
  }
  if (!command.compare("setvthrcompid")) {
    setVthrCompId(); 
    return;
  }
  LOG(logDEBUG) << "did not find command ->" << command << "<-";
}


// ----------------------------------------------------------------------
void PixTestPretest::saveDacs() {
  LOG(logINFO) << "Write DAC parameters to file";
  fPixSetup->getConfigParameters()->writeDacParameterFiles(fPixSetup->getConfigParameters()->getSelectedRocs());
}


// ----------------------------------------------------------------------
void PixTestPretest::setCalDel() {
  LOG(logINFO) << "PixTestPretest::setCalDel() start";
  fDirectory->cd();
  PixTest::update(); 

  fApi->_dut->testAllPixels(false);
  LOG(logINFO) << "PixTestPretest::setCalDel() fApi->_dut->testAllPixels(false)" << " fPIX.size() = " << fPIX.size(); 
  for( unsigned int i = 0; i < fPIX.size(); ++i) {
    if( fPIX[i].first > -1)  fApi->_dut->testPixel(fPIX[i].first, fPIX[i].second, true);
  }
  
  bookHist("caldel");
  
  string DacName = "caldel";
  
  vector <pair <uint8_t, vector<pixel> > > results =  fApi->getEfficiencyVsDAC( DacName, 0, 255, 0, fParNtrig ); // dac, pixels

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

  // FIXME update configparameters with new caldel!
}


// ----------------------------------------------------------------------
void PixTestPretest::setVana() {

  fDirectory->cd();
  PixTest::update();
  
  LOG(logINFO) << "PixTestPretest::setVana() start, target Ia " << fTargetIa << " mA/ROC";
  
  fApi->_dut->testAllPixels(false);
  
  bookHist("empty");
  
  int vana16[16] = {0};

  size_t nRocs = fPixSetup->getConfigParameters()->getNrocs();

  // set all ROCs to minimum:

  for( size_t roc = 0; roc < nRocs; ++roc ) {
    vana16[roc] = fApi->_dut->getDAC( roc, "vana" ); // remember as start value
    fApi->setDAC( "vana", 0, roc ); // off
  }

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

    while( TMath::Abs(diff) > eps && iter < 11 && vana > 0 && vana < 255 ) {

      int stp = int( TMath::Abs(slope*diff) );
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

      LOG(logINFO) << "ROC " << setw(2) << roc
		   << " iter " << setw(2) << iter
		   << " Vana " << setw(3) << vana
		   << " Ia " << ia-i015 << " mA";
    } // iter

    vana16[roc] = vana; // remember best
    fApi->setDAC( "vana", 0, roc ); // switch off for next ROC

  } // rocs

  TH1D *hsum = new TH1D( "VanaSettings", "Vana per ROC;ROC;Vana [DAC]", 16, -0.5, 15.5 );
  hsum->SetStats(0); // no stats
  hsum->SetMinimum(0);
  hsum->SetMaximum(256);
  fHistList.push_back(hsum);

  // set all ROCs to optimum:

  for( size_t roc = 0; roc < nRocs; ++roc ) {
    fApi->setDAC( "vana", vana16[roc], roc );
    LOG(logINFO) << "ROC " << setw(2) << roc
		 << " Vana " << setw(3) << vana16[roc];
    hsum->Fill( roc, vana16[roc] );
  }

  hsum->Draw();
  PixTest::update();
  
  double ia16 = fApi->getTBia()*1E3; // [mA]

  sw.Start(kTRUE); // reset
  do {
    sw.Start(kFALSE); // continue
    ia16 = fApi->getTBia()*1E3; // [mA]
  }
  while( sw.RealTime() < 0.1 );

  LOG(logINFO) << "PixTestPretest::setVana() done, Module Ia " << ia16 << " mA = " << ia16/nRocs << " mA/ROC";

}


// ----------------------------------------------------------------------
void PixTestPretest::setVthrCompId() {

  LOG(logINFO) << "PixTestPretest::setVthrCompId() start";

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

    LOG(logDEBUG) << "ROC " << setw(2) << roc;
      
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

    LOG(logDEBUG) << "current peak " << hid->GetMaximum()
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
    LOG(logDEBUG) << "[SetComp] max d1  " << maxd1  << " at " << maxi1;
    LOG(logDEBUG) << "[SetComp] max d2  " << maxd2  << " at " << maxi2;
    LOG(logDEBUG) << "[SetComp] max d4  " << maxd4  << " at " << maxi4;
    LOG(logDEBUG) << "[SetComp] max d7  " << maxd7  << " at " << maxi7;
    LOG(logDEBUG) << "[SetComp] max d11 " << maxd11 << " at " << maxi11;
    LOG(logDEBUG) << "[SetComp] max d16 " << maxd16 << " at " << maxi16;
    LOG(logDEBUG) << "[SetComp] max d22 " << maxd22 << " at " << maxi22;
    LOG(logDEBUG) << "[SetComp] max d29 " << maxd29 << " at " << maxi29;

    //int32_t val = maxi22 - 8; // safety
    int32_t val = maxi29 - 8; // safety
    if( val < 0 ) val = 0;
    vthr16[roc] = val;

    LOG(logDEBUG) << "set VthrComp to " << val;

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
  LOG(logINFO) << "PixTestPretest::setVthrCompId() done";

}
