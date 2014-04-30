#include <stdlib.h>  
#include <algorithm> 

#include <TStopwatch.h>
#include <TMarker.h>
#include <TStyle.h>

#include "PixTestPretest.hh"
#include "log.h"
#include "helper.h"

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

      if (!parName.compare("Vcal") ) {
	fParVcal = atoi(sval.c_str() );
	LOG(logDEBUG) << "setting fParVcal    = " << fParVcal; 
      }

      if (!parName.compare("DeltaVthrComp") ) {
	fParDeltaVthrComp = atoi(sval.c_str() );
	LOG(logDEBUG) << "setting fParDeltaVthrComp    = " << fParDeltaVthrComp; 
      }

      if (!parName.compare("PIX") || !parName.compare("PIX1") ) {
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
  fDirectory = gFile->GetDirectory(fName.c_str());
  if( !fDirectory ) {
    fDirectory = gFile->mkdir(fName.c_str());
  }
  fDirectory->cd();
}


// ----------------------------------------------------------------------
void PixTestPretest::setToolTips() {
  fTestTip = string( "Pretest: set Vana, VthrComp, and CalDel")
    + string("\ntune PH into ADC range using VOffsetRO and VIref_ADC")
    ;
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
  PixTest::update(); 
  bigBanner(Form("PixTestTrim::doTest()"));

  setVana();
  TH1 *h1 = (*fDisplayedHist); 
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update(); 

  setVthrCompCalDel();
  h1 = (*fDisplayedHist); 
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update(); 

  setPhRange();
  h1 = (*fDisplayedHist); 
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update(); 

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
  if (!command.compare("setvthrcompcaldel")) {
    setVthrCompCalDel(); 
    return;
  }
  if (!command.compare("setvthrcompid")) {
    setVthrCompId(); 
    return;
  }
  if (!command.compare("setphrange")) {
    setPhRange(); 
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
  cacheDacs();
  fDirectory->cd();
  PixTest::update(); 
  banner(Form("PixTestPretest::setVana() target Ia = %d mA/ROC", fTargetIa)); 

  fApi->_dut->testAllPixels(false);

  vector<uint8_t> vanaStart;
  vector<double> rocIana;

  // -- cache setting and switch off all(!) ROCs
  int nRocs = fApi->_dut->getNRocs(); 
  for (int iroc = 0; iroc < nRocs; ++iroc) {
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

  for (int roc = 0; roc < nRocs; ++roc) {
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

  TH1D *hsum = bookTH1D("VanaSettings", "Vana per ROC", nRocs, 0., nRocs);
  setTitles(hsum, "ROC", "Vana [DAC]"); 
  hsum->SetStats(0);
  hsum->SetMinimum(0);
  hsum->SetMaximum(256);
  fHistList.push_back(hsum);

  TH1D *hcurr = bookTH1D("Iana", "Iana per ROC", nRocs, 0., nRocs);
  setTitles(hcurr, "ROC", "Vana [DAC]"); 
  hcurr->SetStats(0); // no stats
  hcurr->SetMinimum(0);
  hcurr->SetMaximum(30.0);
  fHistList.push_back(hcurr);


  restoreDacs();
  for (int roc = 0; roc < nRocs; ++roc) {
    // -- reset all ROCs to optimum or cached value
    fApi->setDAC( "vana", vanaStart[roc], roc );
    LOG(logINFO) << "ROC " << setw(2) << roc << " Vana " << setw(3) << int(vanaStart[roc]);
    // -- histogramming only for those ROCs that were selected
    if (!selectedRoc(roc)) continue;
    hsum->Fill(roc, vanaStart[roc] );
    hcurr->Fill(roc, rocIana[roc]); 
  }
  
  double ia16 = fApi->getTBia()*1E3; // [mA]

  sw.Start(kTRUE); // reset
  do {
    sw.Start(kFALSE); // continue
    ia16 = fApi->getTBia()*1E3; // [mA]
  }
  while( sw.RealTime() < 0.1 );


  hsum->Draw();
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), hsum);
  PixTest::update();


  LOG(logINFO) << "PixTestPretest::setVana() done, Module Ia " << ia16 << " mA = " << ia16/nRocs << " mA/ROC";

}


// ----------------------------------------------------------------------
void PixTestPretest::setVthrCompCalDel() {
  uint16_t FLAGS = FLAG_FORCE_SERIAL | FLAG_FORCE_MASKED; // required for manual loop over ROCs

  cacheDacs();
  fDirectory->cd();
  PixTest::update(); 
  banner(Form("PixTestPretest::setVthrCompCalDel()")); 

  string name("pretestVthrCompCalDel");

  fApi->setDAC("CtrlReg", 0); 
  fApi->setDAC("Vcal", 250); 

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  vector<pair<uint8_t, pair<uint8_t, vector<pixel> > > >  rresults;

  TH1D *h1(0); 
  h1 = bookTH1D(Form("pretestCalDel"), Form("pretestCalDel"), rocIds.size(), 0., rocIds.size()); 
  h1->SetMinimum(0.); 
  h1->SetDirectory(fDirectory); 
  setTitles(h1, "ROC", "CalDel DAC"); 

  TH2D *h2(0); 

  vector<int> calDel(rocIds.size(), -1.); 
  vector<int> vthrComp(rocIds.size(), -1.); 
  vector<int> calDelE(rocIds.size(), -1.);
  
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    
    int ip = 0; 

    fApi->_dut->testPixel(fPIX[ip].first, fPIX[ip].second, true, rocIds[iroc]);
    fApi->_dut->maskPixel(fPIX[ip].first, fPIX[ip].second, false, rocIds[iroc]);

    h2 = bookTH2D(Form("%s_c%d_r%d_C%d", name.c_str(), fPIX[ip].first, fPIX[ip].second, rocIds[iroc]), 
		  Form("%s_c%d_r%d_C%d", name.c_str(), fPIX[ip].first, fPIX[ip].second, rocIds[iroc]), 
		  255, 0., 255., 255, 0., 255.); 
    fHistOptions.insert(make_pair(h2, "colz")); 
    h2->SetMinimum(0.); 
    h2->SetDirectory(fDirectory); 
    setTitles(h2, "CalDel", "VthrComp"); 
    
    bool done = false;
    while (!done) {
      rresults.clear(); 
      
      LOG(logDEBUG) << " looking at roc = " << static_cast<unsigned int>(rocIds[iroc]) 
		    << " pixel col = " << fPIX[ip].first << ", row = " << fPIX[ip].second
		    << " getNEnabledRocs() = " << fApi->_dut->getNEnabledRocs()
		    << " getNEnabledPixels() = " << fApi->_dut->getNEnabledPixels(rocIds[iroc]); 
      int cnt(0); 
      try{
	rresults = fApi->getEfficiencyVsDACDAC("caldel", 0, 255, "vthrcomp", 0, 150, FLAGS, fParNtrig);
	done = true;
      } catch(DataMissingEvent &e){
	LOG(logDEBUG) << "problem with readout: "<< e.what() << " missing " << e.numberMissing << " events"; 
	++cnt;
	if (e.numberMissing > 10) done = true; 
      }
      done = (cnt>5) || done;
    }
    
    fApi->_dut->testPixel(fPIX[ip].first, fPIX[ip].second, false, rocIds[iroc]);
    fApi->_dut->maskPixel(fPIX[ip].first, fPIX[ip].second, true, rocIds[iroc]);
    
    for (unsigned i = 0; i < rresults.size(); ++i) {
      pair<uint8_t, pair<uint8_t, vector<pixel> > > v = rresults[i];
      int idac1 = v.first; 
      pair<uint8_t, vector<pixel> > w = v.second;      
      int idac2 = w.first;
      vector<pixel> wpix = w.second;
      for (unsigned ipix = 0; ipix < wpix.size(); ++ipix) {
	if (wpix[ipix].roc_id == rocIds[iroc]) {
	  h2->Fill(idac1, idac2, wpix[ipix].value); 
	} else {
	  LOG(logDEBUG) << "ghost ROC " << static_cast<unsigned int>(wpix[ipix].roc_id) << " seen with pixel "
			<< static_cast<unsigned int>(wpix[ipix].column) << " " << static_cast<unsigned int>(wpix[ipix].row); 
	}
      }
    }
    
    TH1D *h0 = h2->ProjectionX("_px", fParDeltaVthrComp+1, fParDeltaVthrComp+2); 
    h0->Draw(); 
    PixTest::update();
    
    pxar::mDelay(1000); 
    
    calDelE[iroc] = static_cast<int>(h0->GetRMS());
    calDel[iroc] = static_cast<int>(h0->GetMean()+0.25*calDelE[iroc]);
    delete h0;
    
    TH1D *hy = h2->ProjectionY("_py", 5, h2->GetNbinsX()); 
    int top = hy->FindLastBinAbove(1.); 
    hy->Draw(); 
    PixTest::update(); 
    pxar::mDelay(1000); 
    
    int itop(top); 
    for (itop = top; itop > 0; --itop) {
      h0 = h2->ProjectionX("_px", itop, itop); 
      int cnt(0); 
      for (int i = 0; i < h0->GetNbinsX(); ++i) {
	if (fParNtrig == h0->GetBinContent(i)) cnt++; 
      }
      if (cnt > 40) break;
    }
    
    vthrComp[iroc] = (fParDeltaVthrComp>0?fParDeltaVthrComp:itop+fParDeltaVthrComp);
    
    h1->SetBinContent(rocIds[iroc]+1, calDel[iroc]); 
    h1->SetBinError(rocIds[iroc]+1, calDelE[iroc]); 
    LOG(logDEBUG) << "CalDel: " << calDel[iroc] << " +/- " << calDelE[iroc];
    
    h2->Draw(getHistOption(h2).c_str());
    TMarker *tm = new TMarker(); 
    tm->SetMarkerSize(1);
    tm->SetMarkerStyle(21);
    tm->DrawMarker(calDel[iroc], vthrComp[iroc]); 
    PixTest::update(); 
    
    fHistList.push_back(h2); 
  }

  fHistList.push_back(h1); 

  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);
  PixTest::update(); 

  restoreDacs();
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    fApi->setDAC("CalDel", calDel[iroc], rocIds[iroc]);
    fApi->setDAC("VthrComp", vthrComp[iroc], rocIds[iroc]);
  }

  LOG(logINFO) << "PixTestPretest::setVthrCompCalDel() done";

}


// ----------------------------------------------------------------------
void PixTestPretest::setVthrCompId() {

  cacheDacs();
  fDirectory->cd();
  PixTest::update(); 
  banner(Form("PixTestPretest::setVthrCompId()")); 

  vector<TH1D*> hsts;
  TH1D *h1(0);

  int nRocs = fApi->_dut->getNRocs(); 

  for (int roc = 0; roc < nRocs; ++roc) {
    if (!selectedRoc(roc)) continue;
    h1 = bookTH1D(Form("Id_vs_VthrComp_C%d", int(roc)),	
		  Form("Id vs VthrComp C%d", int(roc)),
		   256, 0., 256.);
    h1->SetMinimum(0);
    h1->SetStats(0);
    setTitles( h1, "VthrComp [DAC]", "ROC digital current [mA]" );
    hsts.push_back(h1);
    fHistList.push_back(h1);
  }

  fApi->_dut->testAllPixels(true); // enable all pix: more noise

  for (int roc = 0; roc < nRocs; ++roc) {
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
  
  vector<int> rocVthrComp(nRocs, -1); 
  for (int roc = 0; roc < nRocs; ++roc) {
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

  TH1D *hsum = bookTH1D("VthrCompSettings", "VthrComp per ROC",  nRocs, 0., nRocs);
  setTitles(hsum, "ROC", "VthrComp [DAC]"); 
  hsum->SetStats(0); // no stats
  hsum->SetMinimum(0);
  hsum->SetMaximum(256);
  fHistList.push_back(hsum);


  restoreDacs();
  for (int roc = 0; roc < nRocs; ++roc) {
    // -- (re)set all
    fApi->setDAC("VthrComp", rocVthrComp[roc], roc);
    // -- report on/histogram only selected ROCs
    LOG(logINFO) << "ROC " << setw(2) << roc
		 << " VthrComp " << setw(3) << rocVthrComp[roc];
    hsum->Fill(roc, rocVthrComp[roc]);
  }

  hsum->Draw();
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), hsum);
  PixTest::update();

  LOG(logINFO) << "PixTestPretest::setVthrCompId() done";

}


// ----------------------------------------------------------------------
void PixTestPretest::setCalDel() {
  uint16_t FLAGS = FLAG_FORCE_SERIAL | FLAG_FORCE_MASKED; // required for manual loop over ROCs

  cacheDacs();
  fDirectory->cd();
  PixTest::update(); 
  banner(Form("PixTestPretest::setCalDel()")); 

  fApi->_dut->testAllPixels(false);

  if (fPIX[0].first > -1)  {
    fApi->_dut->testPixel(fPIX[0].first, fPIX[0].second, true);
    fApi->_dut->maskPixel(fPIX[0].first, fPIX[0].second, false);
  } else {
    LOG(logWARNING) << "PreTest: no pixel defined, return";
    return;
  }

  // set maximum pulse (minimal time walk):

  fApi->setDAC("Vcal", 250);
  fApi->setDAC("CtrlReg", 4);

  string DacName = "caldel";

  // measure:
  bool done = false;
  vector<pair<uint8_t, vector<pixel> > > results;
  int cnt(0); 
  while (!done) {
    try{
      results =  fApi->getEfficiencyVsDAC(DacName, 0, 250, FLAGS, fParNtrig);
      done = true;
    } catch(DataMissingEvent &e){
      LOG(logDEBUG) << "problem with readout: "<< e.what() << " missing " << e.numberMissing << " events"; 
      ++cnt;
      if (e.numberMissing > 10) done = true; 
    }
    done = (cnt>5) || done;
  }

  // histos:
  vector<TH1D*> hsts;
  TH1D *h1(0);
  int nRocs = fApi->_dut->getNEnabledRocs();
  for (int iroc = 0; iroc < nRocs; ++iroc) {
    if (!selectedRoc(iroc)) continue;
    h1 = bookTH1D(Form("NhitsVs%s_c%d_r%d_C%d", DacName.c_str(), fPIX[0].first, fPIX[0].second, fId2Idx[iroc]),
		  Form("NhitsVs%s_c%d_r%d_C%d", DacName.c_str(), fPIX[0].first, fPIX[0].second, fId2Idx[iroc]),
		  256, 0., 256.);
    h1->SetMinimum(0);
    setTitles(h1, Form( "%s [DAC]", DacName.c_str() ), "readouts");
    fHistList.push_back(h1);
    hsts.push_back(h1);
  }

  TH1D *hsum = bookTH1D( "CalDelSettings", "CalDel per ROC;ROC;CalDel [DAC]", 16, 0., 16.);
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

      if (fId2Idx[roc] < nRocs
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

  for (int roc = 0; roc < nRocs; ++roc) {
    hsts[fId2Idx[roc]]->Draw();
    PixTest::update();
  }

  // set CalDel:
  restoreDacs();
  for (int roc = 0; roc < nRocs; ++roc) {
    if (i9[fId2Idx[roc]] > 0 ) {

      int i2 = i0[fId2Idx[roc]] + (i9[fId2Idx[roc]]-i0[fId2Idx[roc]])/4;
      fApi->setDAC(DacName, i2, getIdFromIdx(roc));
      
      LOG(logINFO) << "ROC " << setw(2) << getIdFromIdx(roc)
		   << ": eff plateau from " << setw(3) << i0[fId2Idx[roc]]
		   << " to " << setw(3) << i9[fId2Idx[roc]]
		   << ": set CalDel to " << i2;

      hsum->Fill(roc, i2);
    }
  } // rocs

  hsum->Draw();
  PixTest::update();
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), hsum);
}


// ----------------------------------------------------------------------
void PixTestPretest::setPhRange() {
  cacheDacs();
  fDirectory->cd();
  PixTest::update(); 
  banner(Form("PixTestPretest::setPhRange()")); 

  if( fPIX.size() < 1 ) {
    LOG(logWARNING) << "PixTestSetCalDel: no pixel defined, return";
    return;
  }

  if( fPIX[0].first < 0 ) {
    LOG(logWARNING) << "PixTestSetCalDel: no valid pixel defined, return";
    return;
  }

  uint16_t flags = 0;

  fApi->setDAC("CtrlReg", 4); // all ROCs large Vcal

  // Minimize ADC gain for finding the midpoint
  // This avoids PH clipping

  fApi->setDAC( "VIref_ADC", 255 ); // 255 = minimal gain

  const int max_vcal = 255;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // search for min Vcal using one pixel:

  fApi->_dut->testAllPixels(false);

  fApi->_dut->testPixel( fPIX[0].first, fPIX[0].second, true );
  fApi->_dut->maskPixel( fPIX[0].first, fPIX[0].second, false );

  int nRocs = fPixSetup->getConfigParameters()->getNrocs();

  int pix_ph[16];
  int min_vcal[16]; // large Vcal, after trimming
  bool not_enough = 0;
  bool too_much = 0;

  for (size_t i = 0; i < 16; ++i ) {
    pix_ph[i] = -1;
    min_vcal[i] = 8;
  }

  // measure:

  do {

    for (int roc = 0; roc < nRocs; ++roc )
      fApi->setDAC( "Vcal", min_vcal[roc], roc );

    // measure:

    std::vector<pixel> vpix = fApi->getPulseheightMap( flags, fParNtrig );

    LOG(logINFO) << "vpix size " << vpix.size();

    // unpack:

    for( size_t ipx = 0; ipx < vpix.size(); ++ipx ) {
      uint8_t roc = vpix[ipx].roc_id;
      if( roc < 16 )
	pix_ph[roc] = vpix[ipx].value;
      LOG(logINFO) << "ipx " << setw(2) << ipx
		   << ": ROC " << setw(2) << (int) vpix[ipx].roc_id
		   << " col " << setw(2) << (int) vpix[ipx].column
		   << " row " << setw(2) << (int) vpix[ipx].row
		   << " val " << setw(3) << (int) vpix[ipx].value;
    }

    // check:
    not_enough = 0;
    too_much = 0;
    for (int roc = 0; roc < nRocs; ++roc ) {
      if( pix_ph[roc] <= 0 ) {
	min_vcal[roc] += 2;
	not_enough = 1;
      }
      if( min_vcal[roc] > 120 ) too_much = 1;
    }
  }
  while( not_enough && !too_much );

  if( too_much ) {
    LOG(logINFO)
      << "[SetPh] Cannot find working Vcal for pixel "
      << fPIX[0].first << "," << fPIX[0].second
      << ". Please use other pixel or re-trim to lower threshold";
    return;
  }

  for (int roc = 0; roc < nRocs; ++roc ) {
    min_vcal[roc] += 2; // safety
    fApi->setDAC( "Vcal", min_vcal[roc], roc );
    LOG(logINFO) << "ROC " << setw(2) << roc
		 << " min Vcal " << setw(2) << min_vcal[roc]
		 << " has Ph " << setw(3) << pix_ph[roc];
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // scan PH vs VOffsetRO for one pixel per ROC at min Vcal

  for (int roc = 0; roc < nRocs; ++roc )
    fApi->setDAC( "Vcal", min_vcal[roc], roc );

  string dacName = "VOffsetRO";
  vector<pair<uint8_t, vector<pixel> > > // dac and pix
    result = fApi->getPulseheightVsDAC( dacName, 0, 255, flags, fParNtrig );

  // plot:

  vector<TH1D*> hmin;
  TH1D *h1(0);

  for (int roc = 0; roc < nRocs; ++roc ) {

    h1 = new TH1D( Form( "PH_vs_%s_at_Vcal_%2d_c%02d_r%02d_C%02d",
			 dacName.c_str(), min_vcal[roc],
			 fPIX[0].first, fPIX[0].second, int(roc) ),
		   Form( "PH vs %s at Vcal %2d c%02d r%02d C%02d",
			 dacName.c_str(), min_vcal[roc],
			 fPIX[0].first, fPIX[0].second, int(roc) ),
		   256, -0.5, 255.5 );
    h1->SetMinimum(0);
    h1->SetMaximum(256);
    setTitles( h1, Form( "%s [DAC]", dacName.c_str() ),
	       Form( "Vcal %2d <PH> [ADC]", min_vcal[roc] ) );
    hmin.push_back(h1);
    fHistList.push_back(h1);

  } // rocs

  // data:

  for( size_t i = 0; i < result.size(); ++i ) {

    int idac = result[i].first;

    vector<pixel> vpix = result[i].second;

    for( size_t ipx = 0; ipx < vpix.size(); ++ipx ) {

      uint8_t roc = vpix[ipx].roc_id;

      if( roc < nRocs &&
	  vpix[ipx].column == fPIX[0].first &&
	  vpix[ipx].row == fPIX[0].second ) {
	h1 = hmin.at(roc);
	h1->Fill( idac, vpix[ipx].value ); // already averaged
      } // valid

    } // pix

  } // dac values

  for (int roc = 0; roc < nRocs; ++roc ) {
    hmin[roc]->Draw();
    PixTest::update();
  }
  fDisplayedHist = find( fHistList.begin(), fHistList.end(), hmin[nRocs-1] );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // scan PH vs VOffsetRO for one pixel per ROC at max Vcal

  fApi->setDAC( "Vcal", max_vcal );

  result = fApi->getPulseheightVsDAC( dacName, 0, 255, flags, fParNtrig );

  // plot:

  vector<TH1D*> hmax;

  for (int roc = 0; roc < nRocs; ++roc ) {

    h1 = new TH1D( Form( "PH_vs_%s_at_Vcal_%2d_c%02d_r%02d_C%02d",
			 dacName.c_str(), max_vcal,
			 fPIX[0].first, fPIX[0].second, int(roc) ),
		   Form( "PH vs %s at Vcal %2d c%02d r%02d C%02d",
			 dacName.c_str(), max_vcal,
			 fPIX[0].first, fPIX[0].second, int(roc) ),
		   256, -0.5, 255.5 );
    h1->SetMinimum(0);
    h1->SetMaximum(256);
    setTitles( h1, Form( "%s [DAC]", dacName.c_str() ),
	       Form( "Vcal %2d <PH> [ADC]", max_vcal ) );
    hmax.push_back(h1);
    fHistList.push_back(h1);

  } // rocs

  // data:

  for( size_t i = 0; i < result.size(); ++i ) {

    int idac = result[i].first;

    vector<pixel> vpix = result[i].second;

    for( size_t ipx = 0; ipx < vpix.size(); ++ipx ) {

      uint8_t roc = vpix[ipx].roc_id;

      if( roc < nRocs &&
	  vpix[ipx].column == fPIX[0].first &&
	  vpix[ipx].row == fPIX[0].second ) {
	h1 = hmax.at(roc);
	h1->Fill( idac, vpix[ipx].value ); // already averaged
      } // valid

    } // pix

  } // dac values

  for (int roc = 0; roc < nRocs; ++roc ) {
    hmax[roc]->Draw();
    PixTest::update();
  }
  fDisplayedHist = find( fHistList.begin(), fHistList.end(), hmax[nRocs-1] );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  /* Find VOffsetRO value that puts the midpoint between
     minimal and maximal Vcal to 133 */

  int voffset_opt[16];
  for( size_t i = 0; i < 16; ++i )
    voffset_opt[i] = -1;

  for (int roc = 0; roc < nRocs; ++roc ) {

    for( int i = 0; i < 256; ++i ) {
      int ph0 = static_cast<int>(hmin[roc]->GetBinContent(i+1));
      if( ph0 <= 0 ) continue;
      int ph4 = static_cast<int>(hmax[roc]->GetBinContent(i+1));
      if( ph4 <= 0 ) continue;
      if( roc == 0 ) {
	LOG(logINFO) << "VOffsetRO " << setw(3) << i
		     << ": min " << setw(3) << ph0
		     << ", max " << setw(3) << ph4
		     << ", mid " << setw(3) << (ph0+ph4)/2;
      }
      if( ( ph0 + ph4 ) / 2 < 133 ) {
	voffset_opt[roc] = i;
	break;
      }
    }
  } // rocs

  // Abort if the mid VoffsetOp could not be found

  for (int roc = 0; roc < nRocs; ++roc ) {

    if( voffset_opt[roc] == -1 ) {
      LOG(logINFO)
	<< "[SetPh] Warning: Cannot find VoffsetOp midpoint!";
      return;
    }

    LOG(logINFO)
      << "[SetPh] Found VOffsetRO value: "
      << voffset_opt[roc];

    fApi->setDAC( "VOffsetRO", voffset_opt[roc], roc );

  } // rocs

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Scan VIref_ADC to stretch the pulse height at max Vcal

  LOG(logINFO)
    << "[SetPh] Finding mid VIref_ADC value ...";

  // Find optimal VIref_ADC using a single pixel

  fApi->setDAC( "Vcal", max_vcal ); // all ROCs

  dacName = "VIref_ADC";

  result = fApi->getPulseheightVsDAC( dacName, 0, 255, flags, fParNtrig );

  // plot:

  vector<TH1D*> href;

  for (int roc = 0; roc < nRocs; ++roc ) {

    h1 = new TH1D( Form( "PH_vs_%s_at_Vcal_%2d_c%02d_r%02d_C%02d",
			 dacName.c_str(), max_vcal,
			 fPIX[0].first, fPIX[0].second, int(roc) ),
		   Form( "PH vs %s at Vcal %2d c%02d r%02d C%02d",
			 dacName.c_str(), max_vcal,
			 fPIX[0].first, fPIX[0].second, int(roc) ),
		   256, -0.5, 255.5 );
    h1->SetMinimum(0);
    h1->SetMaximum(256);
    setTitles( h1, Form( "%s [DAC]", dacName.c_str() ),
	       Form( "Vcal %2d <PH> [ADC]", max_vcal ) );
    href.push_back(h1);
    fHistList.push_back(h1);

  } // rocs

  // data:

  for( size_t i = 0; i < result.size(); ++i ) {

    int idac = result[i].first;

    vector<pixel> vpix = result[i].second;

    for( size_t ipx = 0; ipx < vpix.size(); ++ipx ) {

      uint8_t roc = vpix[ipx].roc_id;

      if( roc < nRocs &&
	  vpix[ipx].column == fPIX[0].first &&
	  vpix[ipx].row == fPIX[0].second ) {
	h1 = href.at(roc);
	h1->Fill( idac, vpix[ipx].value ); // already averaged
      } // valid

    } // pix

  } // dac values

  for (int roc = 0; roc < nRocs; ++roc ) {
    href[roc]->Draw();
    PixTest::update();
  }
  fDisplayedHist = find( fHistList.begin(), fHistList.end(), href[nRocs-1] );

  // Adjust VIref_ADC to have PH < 210 for this pixel

  int viref_adc_opt[16];
  for( size_t i = 0; i < 16; ++i )
    viref_adc_opt[i] = -1;

  for (int roc = 0; roc < nRocs; ++roc ) {
    for( int i = 0; i < 256; ++i ) {
      int ph = static_cast<int>(href[roc]->GetBinContent(i+1));
      if( ph <= 0 ) continue;
      if( ph < 210 ) {
	viref_adc_opt[roc] = i;
	break; // VIref_ADC has descending curve
      }
    }

    // Abort if optimal VIref_ADC value could not be found

    if( viref_adc_opt[roc] == -1 ) {
      LOG(logINFO)
	<< "[SetPh] Warning: Cannot adjust pulse height range!";
      return;
    }
    LOG(logINFO)
      << "[SetPh] Found VIref_ADC value: "
      << viref_adc_opt[roc];
    fApi->setDAC( dacName, viref_adc_opt[roc], roc ); // for one pixel

  } // rocs

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // check all pixels:

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  // book maps per ROC:

  vector<TH2D*> maps9;
  TH2D *h2(0);
  vector<TH1D*> hsts9;
  //TH1D *h1(0);

  for (int roc = 0; roc < nRocs; ++roc ) {

    h2 = new TH2D( Form( "maxPhMap_C%d", int(roc) ),
		   Form( "max PH map ROC %d", int(roc) ),
		   52, -0.5, 51.5, 80, -0.5, 79.5 );
    h2->SetMinimum(0);
    h2->SetMaximum(256);
    setTitles( h2, "col", "row" );
    h2->GetZaxis()->SetTitle( "<PH> [ADC]" );
    h2->SetStats(0);
    maps9.push_back(h2);
    fHistList.push_back(h2);

    h1 = new TH1D( Form( "maxPhDistribution_C%d", int(roc) ),
		   Form( "max Pulse height distribution ROC %d", int(roc) ),
		   256, -0.5, 255.5 );
    setTitles( h1, "<PH> [ADC]", "pixels" );
    h1->SetStats(1);
    hsts9.push_back(h1);
    fHistList.push_back(h1);
  }

  // measure:

  fApi->setDAC( "Vcal", max_vcal );

  bool again = 0;
  do {

    vector<pixel> vpix = fApi->getPulseheightMap( flags, fParNtrig ); // all pixels, all ROCs

    LOG(logINFO) << "vpix.size() " << vpix.size();

    // data:

    for( size_t ipx = 0; ipx < vpix.size(); ++ipx ) {
      h2 = maps9.at( vpix[ipx].roc_id );
      if( h2 ) h2->SetBinContent( vpix[ipx].column + 1, vpix[ipx].row + 1,
				  vpix[ipx].value );
      h1 = hsts9.at( vpix[ipx].roc_id );
      if( h1 ) h1->Fill( vpix[ipx].value );
    }

    for (int roc = 0; roc < nRocs; ++roc ) {
      h2 = maps9[roc];
      h2->Draw("colz");
      PixTest::update();
    }
    fDisplayedHist = find( fHistList.begin(), fHistList.end(), h2 );

    // check against clipping (ADC overflow):

    again = 0;

    for (int roc = 0; roc < nRocs; ++roc ) {

      LOG(logINFO) << "max Ph map for ROC " << roc;

      Int_t locmaxx,locmaxy,locmaxz;
      h2 = maps9[roc];
      h2->GetMaximumBin( locmaxx, locmaxy,locmaxz );
      double phmax = h2->GetBinContent( locmaxx, locmaxy );
      LOG(logINFO)
	<< "Ph max " << setw(3) << phmax
	<< " at " << setw(2) << locmaxx-1 // bin counting starts at 1
	<< "," << setw(2) << locmaxy-1; // pixel counting starts at 0
      if( phmax > 254.5 ) {
	voffset_opt[roc] += 5; // reduce offset
	viref_adc_opt[roc] += 3; // reduce gain
	fApi->setDAC( "VOffsetRO", voffset_opt[roc], roc );
	fApi->setDAC( "VIref_ADC", viref_adc_opt[roc], roc );
	LOG(logINFO)
	  << "Found overflow "
	  << " VoffsetOp " << setw(3) << voffset_opt[roc]
	  << " VIref_ADC " << setw(3) << viref_adc_opt[roc];
	if( voffset_opt[roc] < 251 && viref_adc_opt[roc] < 253 ) {
	  again = 1;
	  hsts9[roc]->Reset();
	}
      }
      else {
	fApi->_dut->testAllPixels( false, roc ); // remove ROC from list
	fApi->_dut->maskAllPixels( true, roc ); // remove ROC from list
      }
    } // rocs
  } // do
  while( again );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // check for underflow:

  vector<TH2D*> maps0;
  vector<TH1D*> hsts0;

  for (int roc = 0; roc < nRocs; ++roc ) {

    h2 = new TH2D( Form( "minPhMap_C%d", int(roc) ),
		   Form( "min PH map ROC %d", int(roc) ),
		   52, -0.5, 51.5, 80, -0.5, 79.5 );
    h2->SetMinimum(0);
    h2->SetMaximum(256);
    setTitles( h2, "col", "row" );
    h2->GetZaxis()->SetTitle( "<PH> [ADC]" );
    h2->SetStats(0);
    maps0.push_back(h2);
    fHistList.push_back(h2);

    h1 = new TH1D( Form( "minPhDistribution_C%d", int(roc) ),
		   Form( "min Pulse height distribution ROC %d", int(roc) ),
		   256, -0.5, 255.5 );
    setTitles( h1, "<PH> [ADC]", "pixels" );
    h1->SetStats(1);
    hsts0.push_back(h1);
    fHistList.push_back(h1);
  }

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  for (int roc = 0; roc < nRocs; ++roc )
    fApi->setDAC( "Vcal", min_vcal[roc], roc );

  LOG(logINFO) << "check against underflow...";

  do {

    vector<pixel> vpix = fApi->getPulseheightMap( flags, fParNtrig );

    LOG(logINFO) << "vpix.size() " << vpix.size();

    // protection against non-responding pixels
    // (distinguish PH zero from no entry)

    for (int roc = 0; roc < nRocs; ++roc ) {
      h2 = maps0[roc];
      for( int ix = 0; ix < h2->GetNbinsX(); ++ix )
	for( int iy = 0; iy < h2->GetNbinsY(); ++iy )
	  h2->SetBinContent( ix + 1, iy + 1, 256 ); // set to max
    }

    // data:

    for( size_t ipx = 0; ipx < vpix.size(); ++ipx ) {
      if( vpix[ipx].roc_id == 0 &&
	  vpix[ipx].column == 0 &&
	  vpix[ipx].row == 0 &&
	  vpix[ipx].value == 0 ) {
	LOG(logINFO) << "vpix[" << ipx << "] all zero, skipped";
	continue;
      }
      h2 = maps0.at( vpix[ipx].roc_id );
      if( h2 ) h2->SetBinContent( vpix[ipx].column + 1, vpix[ipx].row + 1,
				  vpix[ipx].value );
      h1 = hsts0.at( vpix[ipx].roc_id );
      if( h1 ) h1->Fill( vpix[ipx].value );
    }

    for (int roc = 0; roc < nRocs; ++roc ) {
      h2 = maps0[roc];
      h2->Draw("colz");
      PixTest::update();
    }
    fDisplayedHist = find( fHistList.begin(), fHistList.end(), h2 );

    // check against underflow:

    again = 0;

    for (int roc = 0; roc < nRocs; ++roc ) {

      LOG(logINFO) << "min Ph map for ROC " << roc;

      Int_t locminx,locminy,locminz;
      h2 = maps0[roc];
      h2->GetMinimumBin( locminx, locminy,locminz );
      double phmin = h2->GetBinContent( locminx, locminy );
      LOG(logINFO)
	<< "Ph min " << setw(3) << phmin
	<< " at " << setw(2) << locminx-1 // bin counting starts at 1
	<< "," << setw(2) << locminy-1; // pixel counting starts at 0
      if( phmin < 0.5 ) {
	voffset_opt[roc] -= 5; // increase offset
	viref_adc_opt[roc] += 3; // reduce gain
	fApi->setDAC( "VOffsetRO", voffset_opt[roc], roc );
	fApi->setDAC( "VIref_ADC", viref_adc_opt[roc], roc );
	LOG(logINFO)
	  << "Found underflow "
	  << " VoffsetOp " << setw(3) << voffset_opt[roc]
	  << " VIref_ADC " << setw(3) << viref_adc_opt[roc];
	if( voffset_opt[roc] > 4 && viref_adc_opt[roc] < 253 ) {
	  again = 1;
	  hsts0[roc]->Reset();
	}
      }
      else {
	fApi->_dut->testAllPixels( false, roc ); // remove ROC from list
	fApi->_dut->maskAllPixels( true, roc ); // remove ROC from list
      }
    } // rocs
  } // do
  while( again );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // We have two extreme points:
  //   the pixel with the highest pulse height
  //   the pixel with the lowest pulse height
  // Two quantities:
  //   mean and range
  // Two parameters:
  //   offset and gain
  // Find optimum:
  //   mean = 133
  //   range = 200

  fApi->_dut->testAllPixels(false); // mask and disable all
  fApi->_dut->maskAllPixels(true); // mask and disable all

  for (int roc = 0; roc < nRocs; ++roc ) {

    LOG(logINFO) << "Ph range fine tuning for ROC " << roc;

    Int_t locminx,locminy,locminz;
    h2 = maps0[roc];
    h2->GetMinimumBin( locminx, locminy,locminz );
    double min_ph = h2->GetBinContent( locminx, locminy );
    uint32_t min_col = locminx - 1; // bin counting starts at 1
    uint32_t min_row = locminy - 1; // pixel counting starts at 0
    LOG(logINFO)
      << "Ph min " << setw(3) << min_ph
      << " at " << setw(2) << min_col
      << "," << setw(2) << min_row;

    Int_t locmaxx,locmaxy,locmaxz;
    h2 = maps9[roc];
    h2->GetMaximumBin( locmaxx, locmaxy,locmaxz );
    double max_ph = h2->GetBinContent( locmaxx, locmaxy );
    uint32_t max_col = locmaxx - 1; // bin counting starts at 1
    uint32_t max_row = locmaxy - 1; // pixel counting starts at 0
    LOG(logINFO)
      << "Ph max " << setw(3) << max_ph
      << " at " << setw(2) << max_col
      << "," << setw(2) << max_row;

    int range = static_cast<int>(max_ph - min_ph);
    int mid = static_cast<int>(max_ph + min_ph)/2;
    int iter = 0;

    // iterate:

    while( ( abs(range-200) > 5 || abs(mid-133) > 5 ) &&
	   voffset_opt[roc] < 255 &&
	   voffset_opt[roc] >   0 &&
	   viref_adc_opt[roc] < 255 &&
	   viref_adc_opt[roc] >   0 ) {

    LOG(logINFO)
      << setw(2) << iter
      << ". gain_dac " << setw(3) << viref_adc_opt[roc]
      << ", offset_dac " << setw(3) << voffset_opt[roc]
      << ": min_ph " << setw(3) << min_ph
      << ", max_ph " << setw(3) << max_ph
      << ": range "  << setw(3) << range
      << ", mid " << setw(3) << mid;

      iter++;

      int dmid = 133-mid;
      if( abs(dmid) > 5 ) {
	voffset_opt[roc] -= dmid/2; // slope -1/2;
	if( voffset_opt[roc] < 0 )
	  voffset_opt[roc] = 0;
	else if( voffset_opt[roc] > 255 )
	  voffset_opt[roc] = 255;
	fApi->setDAC( "VOffsetRO", voffset_opt[roc] );
      }

      if(      range > 205 )
	fApi->setDAC( "VIref_ADC", ++viref_adc_opt[roc], roc ); // negative slope
      else if( range < 195 )
	fApi->setDAC( "VIref_ADC", --viref_adc_opt[roc], roc ); // negative slope

      fApi->setDAC( "Vcal", min_vcal[roc], roc );
      fApi->_dut->testPixel( min_col, min_row, true, roc );
      fApi->_dut->maskPixel( min_col, min_row, false, roc );
      vector<pixel> vpix0 = fApi->getPulseheightMap( flags, fParNtrig );
      
      for( size_t ipx = 0; ipx < vpix0.size(); ++ipx )
	if( vpix0[ipx].roc_id == roc &&
	    vpix0[ipx].column == min_col &&
	    vpix0[ipx].row == min_row )
	  min_ph = vpix0[ipx].value;
      fApi->_dut->testPixel( min_col, min_row, false, roc );
      fApi->_dut->maskPixel( min_col, min_row, true, roc );

      fApi->setDAC( "Vcal", max_vcal, roc );
      fApi->_dut->testPixel( max_col, max_row, true, roc );
      fApi->_dut->maskPixel( max_col, max_row, false, roc );
      vector<pixel> vpix9 = fApi->getPulseheightMap( flags, fParNtrig );
      for( size_t ipx = 0; ipx < vpix9.size(); ++ipx )
	if( vpix9[ipx].roc_id == roc &&
	    vpix9[ipx].column == max_col &&
	    vpix9[ipx].row == max_row )
	  max_ph = vpix9[ipx].value;
      fApi->_dut->testPixel( max_col, max_row, false, roc );
      fApi->_dut->maskPixel( max_col, max_row, true, roc );

      range = static_cast<int>(max_ph - min_ph);
      mid = static_cast<int>(max_ph + min_ph)/2;

    } // while

    LOG(logINFO)
      << setw(2) << iter
      << ". gain_dac " << setw(3) << viref_adc_opt[roc]
      << ", offset_dac " << setw(3) << voffset_opt[roc]
      << ", min_ph " << setw(3) << min_ph
      << ", max_ph " << setw(3) << max_ph
      << ", range "  << setw(3) << range
      << ", mid " << setw(3) << mid;

    LOG(logINFO) << "SetPh Final for ROC " << setw(2) << roc;
    LOG(logINFO) << " 17  VOffsetRO "
		 << setw(3) << (int) fApi->_dut->getDAC( roc, "VOffsetRO" );
    LOG(logINFO) << " 20  VIref_ADC "
		 << setw(3) << (int) fApi->_dut->getDAC( roc, "VIref_ADC" );

  } // rocs

  LOG(logINFO) << "PixTestSetPh::doTest() done for " << nRocs << " ROCs";
}

