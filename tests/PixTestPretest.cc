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
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true;
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());

      if (!parName.compare("targetia")) {
	fTargetIa = atoi(sval.c_str());  // [mA/ROC]
	LOG(logDEBUG) << "setting fTargetIa    = " << fTargetIa << " mA/ROC";
      }

      if (!parName.compare("noisewidth")) {
	fNoiseWidth = atoi(sval.c_str());  
	LOG(logDEBUG) << "setting fNoiseWidth  = " << fNoiseWidth << " DAC units";
      }

      if (!parName.compare("noisemargin")) {
	fNoiseMargin = atoi(sval.c_str());  // safety margin below noise
	LOG(logDEBUG) << "setting fNoiseMargin = " << fNoiseMargin << " DAC units";
      }

      if (!parName.compare("ntrig") ) {
	fParNtrig = atoi(sval.c_str() );
	LOG(logDEBUG) << "setting fParNtrig    = " << fParNtrig; 
      }

      if (!parName.compare("vcal") ) {
	fParVcal = atoi(sval.c_str() );
	LOG(logDEBUG) << "setting fParVcal    = " << fParVcal; 
      }

      if (!parName.compare("deltavthrcomp") ) {
	fParDeltaVthrComp = atoi(sval.c_str() );
	LOG(logDEBUG) << "setting fParDeltaVthrComp    = " << fParDeltaVthrComp; 
      }

      if (!parName.compare("pix") || !parName.compare("pix1") ) {
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

  // -- take out until it performs a bit faster
  if (0){
  setPhRange();
  h1 = (*fDisplayedHist); 
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update(); 
  }

  // -- save DACs!
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

  vector<int> calDel(rocIds.size(), -1); 
  vector<int> vthrComp(rocIds.size(), -1); 
  vector<int> calDelE(rocIds.size(), -1);
  
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
  fApi->_dut->maskAllPixels(true);

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
  uint16_t FLAGS = FLAG_FORCE_MASKED | FLAG_FORCE_SERIAL;
  cacheDacs();
  fDirectory->cd();
  PixTest::update(); 
  banner(Form("PixTestPretest::setPhRange()")); 

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  fApi->setDAC("ctrlReg", 4); 

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  fApi->_dut->testPixel(fPIX[0].first, fPIX[0].second, true); 
  fApi->_dut->maskPixel(fPIX[0].first, fPIX[0].second, false); 

  TH1D* h1(0); 
  map<string, TH1D*> hmap; 
  int cycle(0); 
  string hname;
  for (unsigned int io = 0; io < 26; ++io) {
    for (unsigned int is = 0; is < 52; ++is) {
      LOG(logDEBUG) << Form("pulseheight for phoffset = %3d, phscale = %3d", io*10, is*5); 
      for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
	hname = Form("ph_Vcal_c%d_r%d_C%d", fPIX[0].first, fPIX[0].second, rocIds[iroc]);
	h1 = bookTH1D(hname.c_str(), hname.c_str(), 256, 0., 256.);
	h1->Sumw2();
	h1->SetTitle(Form("ph_Vcal_c%d_r%d_C%d_V%d phscale=%d phoffset=%d", 
			  fPIX[0].first, fPIX[0].second, rocIds[iroc], cycle, is*5, io*10));
	hmap[hname] = h1; 
	++cycle;
      }

      bool done = false;
      vector<pair<uint8_t, vector<pixel> > > results;
      fApi->setDAC("phscale", is*5); 
      fApi->setDAC("phoffset", io*10); 
      while (!done) {
	int cnt(0); 
	try{
	  results = fApi->getPulseheightVsDAC("vcal", 0, 255, FLAGS, fParNtrig);
	  done = true;
	} catch(DataMissingEvent &e){
	  LOG(logDEBUG) << "problem with readout: "<< e.what() << " missing " << e.numberMissing << " events"; 
	  ++cnt;
	  if (e.numberMissing > 10) done = true; 
	}
	done = (cnt>5) || done;
      }

      for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
	for (unsigned int i = 0; i < results.size(); ++i) {
	  pair<uint8_t, vector<pixel> > v = results[i];
	  int idac = v.first; 
	  
	  vector<pixel> vpix = v.second;
	  for (unsigned int ipix = 0; ipix < vpix.size(); ++ipix) {
	    if (vpix[ipix].roc_id == rocIds[iroc]) {
	      hname = Form("ph_Vcal_c%d_r%d_C%d", vpix[ipix].column, vpix[ipix].row, rocIds[iroc]);
	      h1 = hmap[hname];
	      if (h1) {
		h1->SetBinContent(idac+1, vpix[ipix].value); 
		h1->SetBinError(idac+1, 0.05*vpix[ipix].value); 
	      } else {
		LOG(logDEBUG) << "XX did not find "  << hname; 
	      }
	    }
	    
	  }
	}
      }
    }
  }


  // -- analysis
  vector<int> phscale, phoffset; 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    LOG(logDEBUG) << "analysis for ROC " << static_cast<int>(rocIds[iroc]); 
    TH1D *h;
    TH1D *hopt(0); 
    
    unsigned int io, is, is_opt=999;
    int safety_margin=0;
    int bin_min;
    double bestDist=255;
    double dist =255;
    double upEd_dist=255, lowEd_dist=255;
    unsigned int io_opt=999;
    
    //set midrange value for offset and scan scale, until PH curve is well inside the range
    io=20;
    io_opt=io;
    safety_margin=50;
    for (is=0; is<52; is++){
      h = (TH1D*)fDirectory->Get(Form("ph_Vcal_c%d_r%d_C%d_V%d", fPIX[0].first, fPIX[0].second, rocIds[iroc], 52*io+is));
      h->Draw();
      //erasing first garbage bin
      h->SetBinContent(1,0.);
      bin_min = h->FindFirstBinAbove(1.);
      upEd_dist = TMath::Abs(h->GetBinContent(h->FindFixBin(255)) - (255 - safety_margin));
      lowEd_dist = TMath::Abs(h->GetBinContent(bin_min) - safety_margin);
      dist = (upEd_dist > lowEd_dist ) ? (upEd_dist) : (lowEd_dist);
      if(dist < bestDist){
	is_opt = is;
	bestDist=dist;
      }
    }
    
    if (is_opt==999){
      LOG(logDEBUG) << "PH optimization failed";
      return;
    }
    
    
    //centring PH curve adjusting ph offset
    bestDist=255;
    io_opt=999;
    for(io=0; io<26; io++){
      h = (TH1D*)gDirectory->Get(Form("ph_Vcal_c%d_r%d_C%d_V%d", fPIX[0].first, fPIX[0].second, rocIds[iroc], 52*io+is_opt));
      //erasing first garbage bin
      h->SetBinContent(1,0.);
      double plateau = h->GetMaximum();
      bin_min = h->FindFirstBinAbove(1.);
      double minPH = h->GetBinContent(bin_min);
      
      dist = TMath::Abs(minPH - (255 - plateau));
      
      if (dist < bestDist){
	io_opt = io;
	bestDist = dist;
      }
    }
    
    //stretching curve to maximally exploit range
    is_opt=999;
    safety_margin=15;
    bestDist=255;
    dist =0;
    upEd_dist=255;
    lowEd_dist=255;
    for(is=0; is<52; is++){
      h = (TH1D*)gDirectory->Get(Form("ph_Vcal_c%d_r%d_C%d_V%d", fPIX[0].first, fPIX[0].second, rocIds[iroc], 52*io_opt+is));
      h->Draw();
      //erasing first garbage bin
      h->SetBinContent(1,0.);
      bin_min = h->FindFirstBinAbove(1.);
      upEd_dist = TMath::Abs(h->GetBinContent(h->FindFixBin(255)) - (255 - safety_margin));
      lowEd_dist = TMath::Abs(h->GetBinContent(bin_min) - safety_margin);
      dist = (upEd_dist < lowEd_dist ) ? (upEd_dist) : (lowEd_dist);
      if(dist<bestDist){
	is_opt = is;
	bestDist=dist;
	//break;
      }
    }
    
    if(is_opt==999){
      LOG(logDEBUG) <<"PH optimization failed (stretching)";
      return;
    }
    
    
    hopt = (TH1D*)gDirectory->Get(Form("ph_Vcal_c%d_r%d_C%d_V%d", fPIX[0].first, fPIX[0].second, rocIds[iroc], 52*io_opt+is_opt));
    LOG(logDEBUG) << "hopt: " << h->GetName()
		  << Form(" optimal PH parameters: PH scale = %d (%d) PH offset = %d (%d)", 
			  is_opt, is_opt*5, io_opt, io_opt*10); 

    fHistList.push_back(hopt); 

    phscale.push_back(is_opt*5); 
    phoffset.push_back(io_opt*10); 
  }



  restoreDacs();

  string phscaleString, phoffsetString; 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    fApi->setDAC("phscale", phscale[iroc], rocIds[iroc]); 
    phscaleString += Form("%3d ", phscale[iroc]); 
    fApi->setDAC("phoffset", phoffset[iroc], rocIds[iroc]); 
    phoffsetString += Form("%3d ", phoffset[iroc]); 
  }

  TH2D *h = (TH2D*)(fHistList.back());
  h->Draw(getHistOption(h).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 

  
  // -- summary printout
  LOG(logINFO) << "PixTestAlive::setPhRange() done";
  LOG(logINFO) << "phscale:  " << phscaleString; 
  LOG(logINFO) << "phoffset: " << phoffsetString; 

}



