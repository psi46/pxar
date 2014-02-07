
// set Vana to target Ia, set VthrComp below noise, set CalDel

#include <stdlib.h>  // atof, atoi
#include <algorithm> // std::find

#include <TStopwatch.h>

#include "PixTestPretest.hh"
#include "log.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestPretest)

// ----------------------------------------------------------------------
PixTestPretest::PixTestPretest( PixSetup *a, std::string name) : PixTest(a, name), fTargetIa(-1), fNoiseWidth(22), fNoiseMargin(10), fParNtrig(-1) {
  PixTest::init();
  init(); 
  //  LOG(logINFO) << "PixTestPretest ctor(PixSetup &a, string, TGTab *)";
  for( unsigned int i = 0; i < fPIX.size(); ++i) {
    LOG(logDEBUG) << " PixTestPretest setting fPIX" << i
		  <<  " ->" << fPIX[i].first
		  << "/" << fPIX[i].second;
  }
}

// ----------------------------------------------------------------------
PixTestPretest::PixTestPretest() : PixTest() {
  //  LOG(logINFO) << "PixTestPretest ctor()";
}


// ----------------------------------------------------------------------
bool PixTestPretest::setParameter(string parName, string sval) {
  bool found(false);
  string str1, str2; 
  string::size_type s1;
  int pixc, pixr; 
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true;
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());

      if (!parName.compare("targetIa")) {
	fTargetIa = atoi(sval.c_str());  // [mA/ROC]
	LOG(logDEBUG) << "target Ia " << fTargetIa << " mA/ROC";
      }

      if (!parName.compare("noiseWidth")) {
	fNoiseWidth = atoi(sval.c_str());  // half-width of Id noise peak
	LOG(logDEBUG) << "Id vs VthrComp noise peak width "
		      << fNoiseWidth << " DAC units";
      }

      if (!parName.compare("noiseMargin")) {
	fNoiseMargin = atoi(sval.c_str());  // safety margin below noise
	LOG(logDEBUG) << "safety margin for VthrComp below noise "
		      << fNoiseMargin << " DAC units";
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

// ----------------------------------------------------------------------
void PixTestPretest::init() {
  setToolTips();
  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory ) {
    fDirectory = gFile->mkdir( fName.c_str() );
  }
  fDirectory->cd();
}


// ----------------------------------------------------------------------
void PixTestPretest::setToolTips() {
  fTestTip = string( "Pretest: set Vana, VthrComp, and CalDel");
  fSummaryTip = string("summary plot to be implemented");
}

// ----------------------------------------------------------------------
void PixTestPretest::bookHist(string name) {
  fDirectory->cd(); 

  LOG(logDEBUG) << "nothing done with " << name;
}

// ----------------------------------------------------------------------
PixTestPretest::~PixTestPretest() {
  LOG(logDEBUG) << "PixTestPretest dtor";
}

// ----------------------------------------------------------------------
void PixTestPretest::doTest() {

  fDirectory->cd();
  fHistList.clear();
  PixTest::update();

  LOG(logINFO) << "PixTestPretest::doTest() ntrig = " << fParNtrig;

  setVana();
  //  saveDacs();
  setVthrCompId(); // the order is important, VthrComp changes timing
  //  saveDacs();
  setCalDel();
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

  vector<uint8_t> rocs = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocs.size(); ++iroc) {
    fPixSetup->getConfigParameters()->writeDacParameterFile(rocs[iroc], fApi->_dut->getDACs(iroc)); 
  }

  //  fPixSetup->getConfigParameters()->writeDacParameterFiles(fPixSetup->getConfigParameters()->getSelectedRocs());
}

// ----------------------------------------------------------------------
void PixTestPretest::setVana() {

  LOG(logINFO) << "PixTestPretest::setVana() start, target Ia " << fTargetIa << " mA/ROC";

  fApi->_dut->testAllPixels(false);

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

    if( hid ) hid->Draw();
    PixTest::update();

    // analyze:

    LOG(logDEBUG) << "current peak " << hid->GetMaximum()
		 << " mA at DAC " << hid->GetMaximumBin();

    double maxd = 0;
    int maxi = 0;
    for( int i = 1; i <= 256-fNoiseWidth; ++i ) { // histo bin counting starts at 1
      double ni = hid->GetBinContent(i);
      double d = hid->GetBinContent(i+fNoiseWidth) - ni;
      if( d > maxd ) { maxd = d; maxi = i-1; }
    }
    LOG(logDEBUG) << "[SetComp] max d" << fNoiseWidth
		  << maxd << " at " << maxi;

    int32_t val = maxi - fNoiseMargin; // safety
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

// ----------------------------------------------------------------------
void PixTestPretest::setCalDel() {
  LOG(logINFO) << "PixTestPretest::setCalDel() start";

  fApi->_dut->testAllPixels(false);

  // one pixel per ROC:

  if( fPIX[0].first > -1 )
    fApi->_dut->testPixel(fPIX[0].first, fPIX[0].second, true);
  else {
    LOG(logWARNING) << "PreTest: no pixel defined, return";
    return;
  }

  // set maximum pulse (minimal time walk):

  uint8_t cal = fApi->_dut->getDAC( 0, "Vcal" );
  uint8_t ctl = fApi->_dut->getDAC( 0, "CtrlReg" );

  fApi->setDAC( "Vcal", 255 ); // all ROCs large Vcal
  fApi->setDAC( "CtrlReg", 4 ); // all ROCs large Vcal

  string DacName = "caldel";

  // measure:
  
  vector <pair <uint8_t, vector<pixel> > > // dac, pixels
    results =  fApi->getEfficiencyVsDAC( DacName, 0, 255, 0, fParNtrig );

  // histos:

  vector<TH1D*> hsts;
  TH1D *h1(0);
  uint32_t nRocs = fPixSetup->getConfigParameters()->getNrocs();

  for( uint32_t i = 0; i < nRocs; ++i ) {

    h1 = new TH1D(Form("NhitsVs%s_c%d_r%d_C%d",
		       DacName.c_str(), fPIX[0].first, fPIX[0].second, i),
		  Form("NhitsVs%s_c%d_r%d_C%d",
		       DacName.c_str(), fPIX[0].first, fPIX[0].second, i),
		  256, -0.5, 255.5 );
    h1->SetMinimum(0);
    setTitles( h1, Form( "%s [DAC]", DacName.c_str() ), "readouts" );
    fHistList.push_back(h1);
    hsts.push_back(h1);
  }

  TH1D *hsum = new TH1D( "CalDelSettings", "CalDel per ROC;ROC;CalDel [DAC]",
			 16, -0.5, 15.5 );
  hsum->SetStats(0); // no stats
  hsum->SetMinimum(0);
  hsum->SetMaximum(256);
  fHistList.push_back(hsum);

  // analyze:

  int i0[16] = {0};
  int i9[16] = {0};
  int nm[16] = {0};

  for( size_t i = 0; i < results.size(); ++i ) { // dac values

    int caldel = results[i].first;

    vector<pixel> vpix = results[i].second; // pixels from all rocs

    for( size_t ipx = 0; ipx < vpix.size(); ++ipx ) {

      uint32_t roc = vpix.at(ipx).roc_id;

      if( roc < nRocs &&
	  vpix[ipx].column == fPIX[0].first &&
	  vpix[ipx].row == fPIX[0].second ) {

	int nn = vpix.at(ipx).value;

	if( nn > nm[roc] ) {
	  nm[roc] = nn;
	  i0[roc] = caldel; // begin of plateau
	}
	if( nn == nm[roc] )
	  i9[roc] = caldel; // end of plateau

	h1 = hsts.at(roc);
	h1->Fill( caldel, nn );

      } // valid

    } // pixels and rocs

  } // caldel vals

  for( size_t roc = 0; roc < nRocs; ++roc ) {
    hsts[roc]->Draw();
    PixTest::update();
  }

  // set CalDel:

  for( uint32_t roc = 0; roc < nRocs; ++roc ) {

    if( i9[roc] > 0 ) {

      int i2 = i0[roc] + (i9[roc]-i0[roc])/4;

      fApi->setDAC( DacName, i2, roc );

      LOG(logINFO) << "ROC " << setw(2) << roc
		   << ": eff plateau from " << setw(3) << i0[roc]
		   << " to " << setw(3) << i9[roc]
		   << ": set CalDel to " << i2;

      hsum->Fill( roc, i2 );
    }
  } // rocs

  fApi->setDAC( "Vcal", cal ); // restore
  fApi->setDAC( "CtrlReg", ctl ); // restore

  //flush?

  hsum->Draw();
  PixTest::update();
  fDisplayedHist = find( fHistList.begin(), fHistList.end(), hsum );

  // FIXME update configparameters with new caldel!
}
