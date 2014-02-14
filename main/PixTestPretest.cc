#include <stdlib.h>  
#include <algorithm> 

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
	LOG(logDEBUG) << "setting fTargetIa    = " << fTargetIa << " mA/ROC";
      }

      if (!parName.compare("noiseWidth")) {
	fNoiseWidth = atoi(sval.c_str());  
	LOG(logDEBUG) << "setting fNoiseWidth  = " << fNoiseWidth << " DAC units";
      }

      if (!parName.compare("noiseMargin")) {
	fNoiseMargin = atoi(sval.c_str());  // safety margin below noise
	LOG(logDEBUG) << "setting fNoiseMargin = " << fNoiseMargin << " DAC units";
      }

      if (!parName.compare("Ntrig") ) {
	fParNtrig = atoi(sval.c_str() );
	LOG(logDEBUG) << "setting fParNtrig    = " << fParNtrig; 
      }

      if (!parName.compare("PIX") ) {
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

  LOG(logINFO) << "PixTestPretest::setVana()";
  setVana();
  //  saveDacs();
  LOG(logINFO) << "PixTestPretest::setVthrCompId()";
  setVthrCompId(); // the order is important, VthrComp changes timing
  //  saveDacs();
  LOG(logINFO) << "PixTestPretest::setCalDel()";
  setCalDel();

  LOG(logINFO) << "PixTestPretest::saveDacs()";
  saveDacs();
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

}

// ----------------------------------------------------------------------
void PixTestPretest::setVana() {

  LOG(logINFO) << "PixTestPretest::setVana() start, target Ia " << fTargetIa << " mA/ROC";

  fApi->_dut->testAllPixels(false);

  vector<uint8_t> vanaStart;
  vector<double> rocIana;

  // -- cache setting and switch off all(!) ROCs
  int nRocs = fApi->_dut->getNRocs(); 
  for (size_t iroc = 0; iroc < nRocs; ++iroc) {
    vanaStart.push_back(fApi->_dut->getDAC(iroc, "vana"));
    rocIana.push_back(0.); 
    fApi->setDAC("vana", 0, iroc);
  }
  
  double i016 = fApi->getTBia()*1E3;

  // FIXME this should not be a stopwatch, but a delay
  TStopwatch sw;
  sw.Start(kTRUE); // reset
  do {
    sw.Start(kFALSE); // continue
    i016 = fApi->getTBia()*1E3;
  } while (sw.RealTime() < 0.1);

  LOG(logINFO) << "delay " << sw.RealTime() << " s"
	       << ", Stopwatch counter " << sw.Counter();

  // subtract one ROC to get the offset from the other Rocs (on average):
  double i015 = (nRocs-1) * i016 / nRocs; // = 0 for single chip tests

  LOG(logINFO) << "offset current from other " << nRocs-1 << " ROCs is "
	       << i015 << " mA";

  // tune per ROC:

  const double extra = 0.1; // [mA] besser zu viel als zu wenig 
  const double eps = 0.25; // [mA] convergence
  const double slope = 6; // 255 DACs / 40 mA

  for (uint32_t roc = 0; roc < nRocs; ++roc) {
    if (!selectedRoc(roc)) {
      LOG(logDEBUG) << "skipping ROC idx = " << roc << " (not selected) for Vana tuning"; 
      continue;
    }
    int vana = vanaStart[roc];
    fApi->setDAC("vana", vana, roc); // start value

    double ia = fApi->getTBia()*1E3; // [mA], just to be sure to flush usb
    sw.Start(kTRUE); // reset
    do {
      sw.Start(kFALSE); // continue
      ia = fApi->getTBia()*1E3; // [mA]
    } while (sw.RealTime() < 0.1);

    LOG(logINFO) << "delay " << sw.RealTime() << " s"
		 << ", Stopwatch counter " << sw.Counter();

    double diff = fTargetIa + extra - (ia - i015);

    int iter = 0;
    LOG(logINFO) << "ROC " << roc << " iter " << iter
		 << " Vana " << vana
		 << " Ia " << ia-i015 << " mA";

    while (TMath::Abs(diff) > eps && iter < 11 && vana > 0 && vana < 255) {

      int stp = static_cast<int>(TMath::Abs(slope*diff));
      if (stp == 0) stp = 1;
      if (diff < 0) stp = -stp;

      vana += stp;

      if (vana < 0) {
	vana = 0;
      } else {
	if (vana > 255) {
	  vana = 255;
	}
      }

      fApi->setDAC("vana", vana, roc);
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

    rocIana[roc] = ia-i015; // more or less identical for all ROCS?!
    vanaStart[roc] = vana; // remember best
    fApi->setDAC( "vana", 0, roc ); // switch off for next ROC

  } // rocs

  TH1D *hsum = new TH1D("VanaSettings", "Vana per ROC", nRocs, 0., nRocs);
  setTitles(hsum, "ROC", "Vana [DAC]"); 
  hsum->SetStats(0);
  hsum->SetMinimum(0);
  hsum->SetMaximum(256);
  fHistList.push_back(hsum);

  TH1D *hcurr = new TH1D("Iana", "Iana per ROC", nRocs, 0., nRocs);
  setTitles(hcurr, "ROC", "Vana [DAC]"); 
  hcurr->SetStats(0); // no stats
  hcurr->SetMinimum(0);
  hcurr->SetMaximum(30.0);
  fHistList.push_back(hcurr);

  for (size_t roc = 0; roc < nRocs; ++roc) {
    // -- reset all ROCs to optimum or cached value
    fApi->setDAC( "vana", vanaStart[roc], roc );
    LOG(logINFO) << "ROC " << setw(2) << roc << " Vana " << setw(3) << int(vanaStart[roc]);
    // -- histogramming only for those ROCs that were selected
    if (!selectedRoc(roc)) continue;
    hsum->Fill(roc, vanaStart[roc] );
    hcurr->Fill(roc, rocIana[roc]); 
  }
  
  hsum->Draw();
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), hsum);
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

  int nRocs = fApi->_dut->getNRocs(); 

  for (size_t roc = 0; roc < nRocs; ++roc) {
    if (!selectedRoc(roc)) continue;
    h1 = new TH1D(Form("Id_vs_VthrComp_C%d", int(roc)),	
		  Form("Id vs VthrComp C%d", int(roc)),
		   256, 0., 256.);
    h1->SetMinimum(0);
    h1->SetStats(0);
    setTitles( h1, "VthrComp [DAC]", "ROC digital current [mA]" );
    hsts.push_back(h1);
    fHistList.push_back(h1);
  }

  fApi->_dut->testAllPixels(true); // enable all pix: more noise

  vector<int> rocVsf, rocVthrComp; 
  for (size_t roc = 0; roc < nRocs; ++roc) {
    rocVsf.push_back(fApi->_dut->getDAC(roc, "Vsf" )); 
    rocVthrComp.push_back(fApi->_dut->getDAC(roc, "VthrComp")); 

    fApi->setDAC("Vsf", 33, roc); // small
    fApi->setDAC("VthrComp", 0, roc); // off
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

  for (uint32_t roc = 0; roc < nRocs; ++roc) {
    if (!selectedRoc(roc)) continue;

    LOG(logDEBUG) << "ROC " << setw(2) << roc;
      
    hid = hsts[roc];

    for (size_t idac = 0; idac < 256; ++idac) {
      fApi->setDAC("VthrComp", idac, roc);
      // delay?
      hid->Fill(idac, fApi->getTBid()*1E3 - i015);
    } 

    fApi->setDAC("VthrComp", 0, roc); // switch off

    if (hid) hid->Draw();
    PixTest::update();

    // analyze:

    LOG(logDEBUG) << "current peak " << hid->GetMaximum()
		 << " mA at DAC " << hid->GetMaximumBin();

    double maxd = 0;
    int maxi = 0;
    for (int i = 1; i <= 256-fNoiseWidth; ++i) { // histo bin counting starts at 1
      double ni = hid->GetBinContent(i);
      double d = hid->GetBinContent(i+fNoiseWidth) - ni;
      if (d > maxd) { 
	maxd = d; 
	maxi = i-1; 
      }
    }
    LOG(logDEBUG) << "[SetComp] max d" << fNoiseWidth
		  << maxd << " at " << maxi;

    int32_t val = maxi - fNoiseMargin; // safety
    if (val < 0) val = 0;
    rocVthrComp[roc] = val;
    LOG(logDEBUG) << "set VthrComp to " << val;

  } // rocs

  TH1D *hsum = new TH1D("VthrCompSettings", "VthrComp per ROC",  nRocs, 0., nRocs);
  setTitles(hsum, "ROC", "VthrComp [DAC]"); 
  hsum->SetStats(0); // no stats
  hsum->SetMinimum(0);
  hsum->SetMaximum(256);
  fHistList.push_back(hsum);

  for (size_t roc = 0; roc < nRocs; ++roc) {
    // -- (re)set all
    fApi->setDAC("Vsf", rocVsf[roc], roc);
    fApi->setDAC("VthrComp", rocVthrComp[roc], roc);
    // -- report on/histogram only selected ROCs
    LOG(logINFO) << "ROC " << setw(2) << roc
		 << " VthrComp " << setw(3) << rocVthrComp[roc];
    hsum->Fill(roc, rocVthrComp[roc]);
  }

  hsum->Draw();
  PixTest::update();
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), hsum);

  LOG(logINFO) << "PixTestPretest::setVthrCompId() done";

}


// ----------------------------------------------------------------------
void PixTestPretest::setCalDel() {
  LOG(logINFO) << "PixTestPretest::setCalDel() start";

  fApi->_dut->testAllPixels(false);

  if (fPIX[0].first > -1)  {
    fApi->_dut->testPixel(fPIX[0].first, fPIX[0].second, true);
    fApi->_dut->maskPixel(fPIX[0].first, fPIX[0].second, false);
  } else {
    LOG(logWARNING) << "PreTest: no pixel defined, return";
    return;
  }

  // set maximum pulse (minimal time walk):

  uint8_t cal = fApi->_dut->getDAC( 0, "Vcal" );
  uint8_t ctl = fApi->_dut->getDAC( 0, "CtrlReg" );

  fApi->setDAC("Vcal", 250);
  fApi->setDAC("CtrlReg", 0);

  string DacName = "caldel";

  // measure:
  vector<pair<uint8_t, vector<pixel> > > results =  fApi->getEfficiencyVsDAC(DacName, 0, 250, 0, fParNtrig);

  // histos:
  vector<TH1D*> hsts;
  TH1D *h1(0);
  int nRocs = fApi->_dut->getNEnabledRocs();
  for (unsigned int iroc = 0; iroc < nRocs; ++iroc) {
    if (!selectedRoc(iroc)) continue;
    h1 = new TH1D(Form("NhitsVs%s_c%d_r%d_C%d", DacName.c_str(), fPIX[0].first, fPIX[0].second, fId2Idx[iroc]),
		  Form("NhitsVs%s_c%d_r%d_C%d", DacName.c_str(), fPIX[0].first, fPIX[0].second, fId2Idx[iroc]),
		  256, 0., 256.);
    h1->SetMinimum(0);
    setTitles(h1, Form( "%s [DAC]", DacName.c_str() ), "readouts");
    fHistList.push_back(h1);
    hsts.push_back(h1);
  }

  TH1D *hsum = new TH1D( "CalDelSettings", "CalDel per ROC;ROC;CalDel [DAC]", 16, 0., 16.);
  hsum->SetStats(0); // no stats
  hsum->SetMinimum(0);
  hsum->SetMaximum(256);
  fHistList.push_back(hsum);

  // FIXME this is so bad
  int i0[35] = {0};
  int i9[35] = {0};
  int nm[35] = {0};

  for (size_t i = 0; i < results.size(); ++i) { 
    int caldel = results[i].first;
    vector<pixel> vpix = results[i].second; 

    for (size_t ipx = 0; ipx < vpix.size(); ++ipx)  {
      uint32_t roc = vpix.at(ipx).roc_id;

      if (static_cast<unsigned int>(fId2Idx[roc]) < nRocs
	  && vpix[ipx].column == fPIX[0].first 
	  && vpix[ipx].row == fPIX[0].second
	  ) {

	int nn = vpix.at(ipx).value;

	if (nn > nm[fId2Idx[roc]]) {
	  nm[fId2Idx[roc]] = nn;
	  i0[fId2Idx[roc]] = caldel; // begin of plateau
	}
	if (nn == nm[fId2Idx[roc]] )
	  i9[fId2Idx[roc]] = caldel; // end of plateau

	h1 = hsts.at(fId2Idx[roc]);
	h1->Fill(caldel, nn);
	
      } // valid
      
    } // pixels and rocs
    
  } // caldel vals

  for (size_t roc = 0; roc < nRocs; ++roc) {
    hsts[fId2Idx[roc]]->Draw();
    PixTest::update();
  }

  // set CalDel:
  for (uint32_t roc = 0; roc < nRocs; ++roc) {
    if (i9[fId2Idx[roc]] > 0 ) {

      int i2 = i0[fId2Idx[roc]] + (i9[fId2Idx[roc]]-i0[fId2Idx[roc]])/4;
      fApi->setDAC(DacName, i2, roc);
      
      LOG(logINFO) << "ROC " << setw(2) << roc
		   << ": eff plateau from " << setw(3) << i0[fId2Idx[roc]]
		   << " to " << setw(3) << i9[fId2Idx[roc]]
		   << ": set CalDel to " << i2;

      hsum->Fill(roc, i2);
    }
  } // rocs

  fApi->setDAC( "Vcal", cal ); // restore
  fApi->setDAC( "CtrlReg", ctl ); // restore

  hsum->Draw();
  PixTest::update();
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), hsum);
}
