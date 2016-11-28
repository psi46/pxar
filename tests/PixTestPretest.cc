#include <iostream>
#include <stdlib.h>
#include <algorithm>
#include <fstream>

#include <TColor.h>
#include <TStyle.h>
#include <TMarker.h>
#include <TStopwatch.h>
#include <bitset>

#include "PixTestPretest.hh"
#include "PixTestFactory.hh"
#include "timer.h"
#include "log.h"
#include "helper.h"
#include "PixUtil.hh"

using namespace std;
using namespace pxar;

ClassImp(PixTestPretest)

// ----------------------------------------------------------------------
PixTestPretest::PixTestPretest( PixSetup *a, std::string name) :
PixTest(a, name),
  fTargetIa(24), fNoiseWidth(22), fNoiseMargin(10),
  fParNtrig(1),
  fParVcal(200),
  fParDeltaVthrComp(-50),
  fParFracCalDel(0.5),
  fIgnoreProblems(0) {
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
      }

      if (!parName.compare("noisewidth")) {
	fNoiseWidth = atoi(sval.c_str());
      }

      if (!parName.compare("noisemargin")) {
	fNoiseMargin = atoi(sval.c_str());  // safety margin below noise
      }

      if (!parName.compare("ntrig") ) {
	fParNtrig = atoi(sval.c_str() );
      }

      if (!parName.compare("iterations") ) {
	fIterations = atoi(sval.c_str() );
      }

      if (!parName.compare("vcal") ) {
	fParVcal = atoi(sval.c_str() );
      }

      if (!parName.compare("deltavthrcomp") ) {
	fParDeltaVthrComp = atoi(sval.c_str() );
      }

      if (!parName.compare("fraccaldel") ) {
	fParFracCalDel = atof(sval.c_str() );
      }

      if (!parName.compare("ignoreproblems")) {
	PixUtil::replaceAll(sval, "checkbox(", "");
	PixUtil::replaceAll(sval, ")", "");
	fIgnoreProblems = atoi(sval.c_str());
      }

      if (!parName.compare("pix") || !parName.compare("pix1") ) {
	s1 = sval.find(",");
	if (string::npos != s1) {
	  str1 = sval.substr(0, s1);
	  pixc = atoi(str1.c_str());
	  str2 = sval.substr(s1+1);
	  pixr = atoi(str2.c_str());
	  clearSelectedPixels();
	  fPIX.push_back( make_pair(pixc, pixr) );
	  addSelectedPixels(sval);
	} else {
	  clearSelectedPixels();
	  addSelectedPixels("-1,-1");
	  LOG(logDEBUG) << "  clear fPIX: " << fPIX.size();
	}

      }
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

  TStopwatch t;

  fDirectory->cd();
  PixTest::update();
  bigBanner(Form("PixTestPretest::doTest()"));

  programROC();
  TH1 *h1 = (*fDisplayedHist);
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update();

  if (fProblem) {
    if (fIgnoreProblems) {
      bigBanner("ERROR: some ROCs are not programmable; NOT stopping because you chose not to");
      fProblem = false;
    } else {
      bigBanner("ERROR: some ROCs are not programmable; stop");
      return;
    }
  }

  setVana();
  if (fProblem) {
    if (fIgnoreProblems) {
      bigBanner("ERROR: turning off some ROCs lead to less I(ana) current drop than expected; NOT stopping because you chose not to");
      fProblem = false;
    } else {
      bigBanner("ERROR: turning off some ROCs lead to less I(ana) current drop than expected; stop");
      return;
    }
  }

  h1 = (*fDisplayedHist);
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update();

  string tbmtype = fApi->_dut->getTbmType(); //"tbm09c"
  if ((tbmtype == "tbm09c") || (tbmtype == "tbm08c") || (tbmtype == "tbm10c")) {
    findTiming();
  } else if (tbmtype == "tbm08b" || tbmtype == "tbm08a") {
    setTimings();
  } else if (tbmtype == "tbm08") {
    LOG(logWARNING) << "tbm08 does not have programable phase settings";
  } else {
    LOG(logWARNING) << "No timing test implemented for " <<  tbmtype << "! Do something on your own.";
  }

  if (fProblem) {
    if (fIgnoreProblems) {
      bigBanner("ERROR: No functional timings found;  NOT stopping because you chose not to");
      fProblem = false;
    } else {
      bigBanner("ERROR: No functional timings found;  stop");
      return;
    }
  }

  findWorkingPixel();
  h1 = (*fDisplayedHist);
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update();

  setVthrCompCalDel();
  h1 = (*fDisplayedHist);
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update();

  // -- save DACs and TBM parameters!
  saveDacs();
  if ((tbmtype == "tbm09c") || (tbmtype == "tbm08c") || (tbmtype == "tbm10c")) {
    saveTbmParameters();
  }

  int seconds = t.RealTime();
  LOG(logINFO) << "PixTestPretest::doTest() done, duration: " << seconds << " seconds";
}

// ----------------------------------------------------------------------
void PixTestPretest::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;
  if (!command.compare("programroc")) {
    programROC();
    return;
  }
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
  if (!command.compare("settimings")) {
    setTimings();
    return;
  }
  if (!command.compare("findtiming")) {
    findTiming();
    return;
  }
  if (!command.compare("findworkingpixel")) {
    findWorkingPixel();
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

  LOG(logDEBUG) << "did not find command ->" << command << "<-";
}


// ----------------------------------------------------------------------
void PixTestPretest::setVana() {
  fStopTest = false;
  cacheDacs();
  fDirectory->cd();
  PixTest::update();
  banner(Form("PixTestPretest::setVana() target Ia = %d mA/ROC", fTargetIa));

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

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

  // subtract one ROC to get the offset from the other Rocs (on average):
  double i015 = (nRocs-1) * i016 / nRocs; // = 0 for single chip tests
  LOG(logDEBUG) << "offset current from other " << nRocs-1 << " ROCs is " << i015 << " mA";

  // tune per ROC:

  const double extra = 0.1; // [mA] besser zu viel als zu wenig
  const double eps = 0.25; // [mA] convergence
  const double slope = 6; // 255 DACs / 40 mA

  for (int roc = 0; roc < nRocs; ++roc) {

    gSystem->ProcessEvents();
    if (fStopTest) break;

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

    double diff = fTargetIa + extra - (ia - i015);

    int iter = 0;
    LOG(logDEBUG) << "ROC " << roc << " iter " << iter
		 << " Vana " << vana
		 << " Ia " << ia-i015 << " mA";

    while (TMath::Abs(diff) > eps && iter < 11 && vana >= 0 && vana < 255) {

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

      LOG(logDEBUG) << "ROC " << setw(2) << roc
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
  setTitles(hcurr, "ROC", "Iana [mA]");
  hcurr->SetStats(0); // no stats
  hcurr->SetMinimum(0);
  hcurr->SetMaximum(30.0);
  fHistList.push_back(hcurr);


  restoreDacs();
  for (int roc = 0; roc < nRocs; ++roc) {
    // -- reset all ROCs to optimum or cached value
    fApi->setDAC( "vana", vanaStart[roc], roc );
    LOG(logDEBUG) << "ROC " << setw(2) << roc << " Vana " << setw(3) << int(vanaStart[roc]);
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


  // -- test that current drops when turning off single ROCs
  cacheDacs();
  double iAll = fApi->getTBia()*1E3;
  sw.Start(kTRUE);
  do {
    sw.Start(kFALSE);
    iAll = fApi->getTBia()*1E3;
  } while (sw.RealTime() < 0.1);

  double iMinus1(0), vanaOld(0);
  vector<double> iLoss;
  for (int iroc = 0; iroc < nRocs; ++iroc) {
    vanaOld = fApi->_dut->getDAC(iroc, "vana");
    fApi->setDAC("vana", 0, iroc);

    iMinus1 = fApi->getTBia()*1E3; // [mA], just to be sure to flush usb
    sw.Start(kTRUE); // reset
    do {
      sw.Start(kFALSE); // continue
      iMinus1 = fApi->getTBia()*1E3; // [mA]
    } while (sw.RealTime() < 0.1);
    iLoss.push_back(iAll-iMinus1);

    fApi->setDAC("vana", vanaOld, iroc);
  }

  string vanaString(""), vthrcompString("");
  for (int iroc = 0; iroc < nRocs; ++iroc){
    if (iLoss[iroc] < 15) {
      vanaString += Form("  ->%3.1f<-", iLoss[iroc]);
      fProblem = true;
    } else {
      vanaString += Form("  %3.1f", iLoss[iroc]);
    }
  }
  // -- summary printout
  LOG(logINFO) << "PixTestPretest::setVana() done, Module Ia " << ia16 << " mA = " << ia16/nRocs << " mA/ROC";
  LOG(logINFO) << "i(loss) [mA/ROC]:   " << vanaString;


  restoreDacs();


  dutCalibrateOff();
}

// ----------------------------------------------------------------------
// this is quite horrible, but a consequence of the parallel world in PixTestCmd which I do not intend to duplicate here
void PixTestPretest::findTiming() {

  banner(Form("PixTestPretest::findTiming() "));
  PixTestFactory *factory = PixTestFactory::instance();
  PixTest *t =  factory->createTest("cmd", fPixSetup);
  t->runCommand("timing");
  delete t;

  // -- parse output file
  ifstream INS;
  char buffer[1000];
  string sline, sparameters, ssuccess;
  string::size_type s1;
  vector<double> x;
  INS.open("pxar_timing.log");
  while (INS.getline(buffer, 1000, '\n')) {
    sline = buffer;
    s1 = sline.find("selecting");
    if (string::npos == s1) continue;
    sparameters = sline;
    INS.getline(buffer, 1000, '\n');
    ssuccess = buffer;
  }
  INS.close();

  // -- parse relevant lines
  int tries(-1), success(-1);
  istringstream istring(ssuccess);
  istring >> sline >> sline >> success >> sline >> tries;
  istring.clear();
  istring.str(sparameters);
  int i160(-1), i400(-1), iroc(-1), iht(-1), itoken(-1), iwidth(-1);
  istring >> sline >> i160 >> i400 >> iroc >> iht >> itoken >> sline >> sline >> iwidth;
  LOG(logINFO) << "TBM phases:  160MHz: " << i160 << ", 400MHz: " << i400
	       << ", TBM delays: ROC(0/1):" << iroc << ", header/trailer: " << iht << ", token: " << itoken;
  LOG(logINFO) << "(success/tries = " << success << "/" << tries << "), width = " << iwidth;

  uint8_t value= ((i160 & 0x7)<<5) + ((i400 & 0x7)<<2);
  int stat = tbmSet("basee", 0, value);
  if (stat > 0){
    LOG(logWARNING) << "error setting delay  base E " << hex << value << dec;
  }

  if (iroc >= 0){
    value = ((itoken & 0x1)<<7) + ((iht & 0x1)<<6) + ((iroc & 0x7)<<3) + (iroc & 0x7);
    stat = tbmSet("basea",2, value);
    if (stat > 0){
      LOG(logWARNING) << "error setting delay  base A " << hex << value << dec;
    }
  }
  tbmSet("base4", 2, 0x80); // reset once after changing phases

  if (success < 0) fProblem = true;

  //This resets the DTB after the findtiming test.
  fApi->Poff();
  TStopwatch sw;
  sw.Start(kTRUE); // reset
  do {
    sw.Start(kFALSE); // continue
  } while (sw.RealTime() < 0.5);
  fApi->Pon();

}


// ----------------------------------------------------------------------
void PixTestPretest::setTimings() {

  fStopTest = false;
  // Start test timer
  timer t;

  banner(Form("PixTestPreTest::setTimings()"));
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  TLogLevel UserReportingLevel = Log::ReportingLevel();
  size_t nTBMs = fApi->_dut->getNTbms();
  if (nTBMs==0) {
    LOG(logINFO) << "Timing test not needed for single ROC.";
    return;
  }
  int nTokenChains = 0;
  std::vector<tbmConfig> enabledTBMs = fApi->_dut->getEnabledTbms();
  for(std::vector<tbmConfig>::iterator enabledTBM = enabledTBMs.begin(); enabledTBM != enabledTBMs.end(); enabledTBM++) nTokenChains += enabledTBM->tokenchains.size();

  int NTrig = 10000;
  uint16_t period = 300;
  int TrigBuffer = 3;

  vector<rawEvent> daqRawEv;
  vector<Event> daqEv;

  bool GoodDelaySettings = false;
  for (int itry = 0; itry < 3 && !GoodDelaySettings; itry++) {
    LOG(logDEBUG) << "Testing Timing: Attempt #" << itry+1;
    fApi->daqStart();
    Log::ReportingLevel() = Log::FromString("QUIET");
    bool goodreadback = checkReadBackBits(period);
    if (goodreadback) {
      statistics results = getEvents(NTrig, period, TrigBuffer);
      int NEvents = (results.info_events_empty()+results.info_events_valid())/nTokenChains;
      int NErrors = results.errors_tbm_header() + results.errors_tbm_trailer() + results.errors_roc_missing();
      if (NEvents==NTrig && NErrors==0) GoodDelaySettings=true;
    }
    Log::ReportingLevel() = UserReportingLevel;
    fApi->daqStop();
  }

  if (GoodDelaySettings) banner("Current timings are good. No timing scan needed.");
  else  banner("Current timings are not Good. Starting timing scan.");

  // Loop through selected TBM Phases settings.
  int PLL160Phases[4] = {6,5,7,0};
  int PLL400Phases[7] = {2,6,1,3,5,7};
  int ROCDelays[6] = {4,3,5,2,6,1};

  for (int ipll160 = 0; ipll160 < 4 && !GoodDelaySettings; ipll160++) {
    for (int ipll400 = 0; ipll400 < 7 && !GoodDelaySettings; ipll400++) {
      //Apply TBM Phase Settings
      int pll160 = PLL160Phases[ipll160];
      int pll400 = PLL400Phases[ipll400];
      uint8_t TBMPhase = pll160<<5 | pll400<<2;
      fApi->setTbmReg("basee", TBMPhase, 0); //Set TBM PLL Phases
      for (int iROCDelay = 0; iROCDelay < 6 && !GoodDelaySettings; iROCDelay++) {
        uint8_t ROCPhase = (3<<6) | (ROCDelays[iROCDelay]<<3) | ROCDelays[iROCDelay]; //Enable token delay, enable header/trailer delay, and set the ROC delays to the same values
        LOG(logDEBUG) << "Testing TBM Phase: " << bitset<8>(TBMPhase).to_string() << " 160 MHz PLL: " << pll160 << " 400MHz PLL: " << pll400 << " ROC Phase: " << bitset<8>(ROCPhase).to_string();
        for (size_t itbm=0; itbm<nTBMs; itbm++) fApi->setTbmReg("basea", ROCPhase, itbm); //Set ROC Phases
        //Test Delay Settings
        fApi->daqStart();
        Log::ReportingLevel() = Log::FromString("QUIET");
        bool goodreadback = checkReadBackBits(period);
        if (goodreadback) {
          statistics results = getEvents(NTrig, period, TrigBuffer);
          int NEvents = (results.info_events_empty()+results.info_events_valid())/nTokenChains;
          int NErrors = results.errors_tbm_header() + results.errors_tbm_trailer() + results.errors_roc_missing();
          if (NEvents==NTrig && NErrors==0) GoodDelaySettings=true;
        }
        Log::ReportingLevel() = UserReportingLevel;
        fApi->daqStop();
        if (GoodDelaySettings) {
          banner("Good Timings Found!!!");
          LOG(logINFO) << "Setting TBM Phases to " << bitset<8>(TBMPhase).to_string() << " 160 MHz PLL: " << pll160 << " 400MHz PLL: " << pll400;
          LOG(logINFO) << "Setting ROC Phases to " << bitset<8>(ROCPhase).to_string();
          fPixSetup->getConfigParameters()->setTbmDac("basee", TBMPhase, 0);
          for (size_t itbm=0; itbm<nTBMs; itbm++) fPixSetup->getConfigParameters()->setTbmDac("basea", ROCPhase, itbm);
        }
      }
    }
  }

  if (!GoodDelaySettings) {
    LOG(logERROR) << "No good timings found! Try running the Phase Scan in the Timing tab.";
    fProblem = true;
  }

  // Print timer value:
  LOG(logINFO) << "Test took " << t << " ms.";
  LOG(logINFO) << "PixTestPretest::setTimings() done.";

  dutCalibrateOff();
}

// ----------------------------------------------------------------------
void PixTestPretest::setVthrCompCalDel() {
  uint16_t FLAGS = FLAG_FORCE_MASKED;

  fStopTest = false;
  gStyle->SetPalette(1);
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

  TH1D *h1(0);
  h1 = bookTH1D(Form("pretestCalDel"), Form("pretestCalDel"), rocIds.size(), 0., rocIds.size());
  h1->SetMinimum(0.);
  h1->SetDirectory(fDirectory);
  setTitles(h1, "ROC", "CalDel DAC");

  TH2D *h2(0);

  vector<int> calDel(rocIds.size(), -1);
  vector<int> vthrComp(rocIds.size(), -1);
  vector<int> calDelE(rocIds.size(), -1);

  int ip = 0;

  fApi->_dut->testPixel(fPIX[ip].first, fPIX[ip].second, true);
  fApi->_dut->maskPixel(fPIX[ip].first, fPIX[ip].second, false);

  map<int, TH2D*> maps;
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    h2 = bookTH2D(Form("%s_c%d_r%d_C%d", name.c_str(), fPIX[ip].first, fPIX[ip].second, rocIds[iroc]),
		  Form("%s_c%d_r%d_C%d", name.c_str(), fPIX[ip].first, fPIX[ip].second, rocIds[iroc]),
		  255, 0., 255., 255, 0., 255.);
    fHistOptions.insert(make_pair(h2, "colz"));
    maps.insert(make_pair(rocIds[iroc], h2));
    h2->SetMinimum(0.);
    h2->SetMaximum(fParNtrig);
    h2->SetDirectory(fDirectory);
    setTitles(h2, "CalDel", "VthrComp");
  }

  bool done = false;
  vector<pair<uint8_t, pair<uint8_t, vector<pixel> > > >  rresults;
  while (!done) {
    rresults.clear();
    int cnt(0);
    gSystem->ProcessEvents();
    if (fStopTest) break;

    try{
      rresults = fApi->getEfficiencyVsDACDAC("caldel", 0, 255, "vthrcomp", 0, 180, FLAGS, fParNtrig);
      done = true;
    } catch(DataMissingEvent &e){
      LOG(logCRITICAL) << "problem with readout: "<< e.what() << " missing " << e.numberMissing << " events";
      ++cnt;
      if (e.numberMissing > 10) done = true;
    } catch(pxarException &e) {
      LOG(logCRITICAL) << "pXar execption: "<< e.what();
      ++cnt;
    }
    done = (cnt>5) || done;
  }

  fApi->_dut->testPixel(fPIX[ip].first, fPIX[ip].second, false);
  fApi->_dut->maskPixel(fPIX[ip].first, fPIX[ip].second, true);

  for (unsigned i = 0; i < rresults.size(); ++i) {
    pair<uint8_t, pair<uint8_t, vector<pixel> > > v = rresults[i];
    int idac1 = v.first;
    pair<uint8_t, vector<pixel> > w = v.second;
    int idac2 = w.first;
    vector<pixel> wpix = w.second;
    for (unsigned ipix = 0; ipix < wpix.size(); ++ipix) {
      maps[wpix[ipix].roc()]->Fill(idac1, idac2, wpix[ipix].value());
    }
  }
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    h2 = maps[rocIds[iroc]];
    TH1D *hy = h2->ProjectionY("_py", 5, h2->GetNbinsX());
    double vcthrMax = hy->GetMaximum();
    double bottom   = hy->FindFirstBinAbove(0.5*vcthrMax);
    double top      = hy->FindLastBinAbove(0.5*vcthrMax);
    delete hy;

    if (fParDeltaVthrComp>0) {
      vthrComp[iroc] = bottom + fParDeltaVthrComp;
    } else {
      vthrComp[iroc] = top + fParDeltaVthrComp;
    }

    TH1D *h0 = h2->ProjectionX("_px", vthrComp[iroc], vthrComp[iroc]);
    double cdMax   = h0->GetMaximum();
    double cdFirst = h0->GetBinLowEdge(h0->FindFirstBinAbove(0.5*cdMax));
    double cdLast  = h0->GetBinLowEdge(h0->FindLastBinAbove(0.5*cdMax));
    calDelE[iroc] = static_cast<int>(cdLast - cdFirst);
    calDel[iroc] = static_cast<int>(cdFirst + fParFracCalDel*calDelE[iroc]);
    TMarker *pm = new TMarker(calDel[iroc], vthrComp[iroc], 21);
    pm->SetMarkerColor(kWhite);
    pm->SetMarkerSize(2);
    h2->GetListOfFunctions()->Add(pm);
    pm = new TMarker(calDel[iroc], vthrComp[iroc], 7);
    pm->SetMarkerColor(kBlack);
    pm->SetMarkerSize(0.2);
    h2->GetListOfFunctions()->Add(pm);
    delete h0;

    h1->SetBinContent(rocIds[iroc]+1, calDel[iroc]);
    h1->SetBinError(rocIds[iroc]+1, 0.5*calDelE[iroc]);
    LOG(logDEBUG) << "CalDel: " << calDel[iroc] << " +/- " << 0.5*calDelE[iroc];

    h2->Draw(getHistOption(h2).c_str());
    PixTest::update();

    fHistList.push_back(h2);
  }

  fHistList.push_back(h1);

  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);
  PixTest::update();

  restoreDacs();
  string caldelString(""), vthrcompString("");
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    if (calDel[iroc] > 0) {
      fApi->setDAC("CalDel", calDel[iroc], rocIds[iroc]);
      caldelString += Form("  %4d", calDel[iroc]);
    } else {
      caldelString += Form(" _%4d", fApi->_dut->getDAC(rocIds[iroc], "caldel"));
    }
    fApi->setDAC("VthrComp", vthrComp[iroc], rocIds[iroc]);
    vthrcompString += Form("  %4d", vthrComp[iroc]);
  }

  // -- summary printout
  LOG(logINFO) << "PixTestPretest::setVthrCompCalDel() done";
  LOG(logINFO) << "CalDel:   " << caldelString;
  LOG(logINFO) << "VthrComp: " << vthrcompString;

  dutCalibrateOff();
}


// ----------------------------------------------------------------------
void PixTestPretest::setVthrCompId() {

  fStopTest = false;
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
  fApi->_dut->maskAllPixels(false); // enable all pix: more noise
  maskPixels();

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

  dutCalibrateOff();
}


// ----------------------------------------------------------------------
void PixTestPretest::setCalDel() {
  uint16_t FLAGS = FLAG_FORCE_SERIAL | FLAG_FORCE_MASKED; // required for manual loop over ROCs

  fStopTest = false;
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
      LOG(logCRITICAL) << "problem with readout: "<< e.what() << " missing " << e.numberMissing << " events";
      ++cnt;
      if (e.numberMissing > 10) done = true;
    } catch(pxarException &e) {
      LOG(logCRITICAL) << "pXar execption: "<< e.what();
      ++cnt;
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
      uint32_t roc = vpix.at(ipx).roc();

      if (fId2Idx[roc] < nRocs
	  && vpix[ipx].column() == fPIX[0].first
	  && vpix[ipx].row() == fPIX[0].second
	  ) {

	int nn = (int)vpix.at(ipx).value();

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
  dutCalibrateOff();
}

// ----------------------------------------------------------------------
void PixTestPretest::programROC() {
  fStopTest = false;
  cacheDacs();
  fDirectory->cd();
  PixTest::update();
  banner(Form("PixTestPretest::programROC() "));

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  unsigned int nRocs = rocIds.size();
  TH1D *h1 = bookTH1D("programROC", "#Delta(Iana) vs ROC", nRocs, 0., nRocs);
  fHistList.push_back(h1);

  vector<int> vanaStart;
  for (unsigned int iroc = 0; iroc < nRocs; ++iroc) {
    vanaStart.push_back(fApi->_dut->getDAC(rocIds[iroc], "vana"));
    fApi->setDAC("vana", 0, rocIds[iroc]);
  }

  pxar::mDelay(2000);
  double iA0 = fApi->getTBia()*1E3;
  //  cout << "iA0 = " << iA0 << endl;

  double iA, dA;
  string result("ROCs");
  bool problem(false);
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    fApi->setDAC("vana", vanaStart[iroc], rocIds[iroc]);
    pxar::mDelay(1000);
    iA = fApi->getTBia()*1E3;
    dA = iA - iA0;
    if (dA < 5) {
      result += Form(" %d", rocIds[iroc]);
      problem = true;
    }
    h1->SetBinContent(iroc+1, dA);
    fApi->setDAC("vana", 0, rocIds[iroc]);

    gSystem->ProcessEvents();
    if (fStopTest) break;

  }

  if (problem) {
    result += " cannot be programmed! Error!";
    fProblem = true;
  } else {
    result += " are all programmable";
  }

  // -- summary printout
  string dIaString("");
  for (unsigned int i = 0; i < nRocs; ++i) {
    dIaString += Form(" %3.1f", h1->GetBinContent(i+1));
  }

  LOG(logINFO) << "PixTestPretest::programROC() done: " << result;
  LOG(logINFO) << "IA differences per ROC: " << dIaString;

  h1 = (TH1D*)(fHistList.back());
  h1->Draw(getHistOption(h1).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);
  PixTest::update();

  dutCalibrateOff();
  restoreDacs();
}


// ----------------------------------------------------------------------
void PixTestPretest::findWorkingPixel() {

  gStyle->SetPalette(1);
  cacheDacs();
  fDirectory->cd();
  PixTest::update();
  banner(Form("PixTestPretest::findWorkingPixel()"));


  vector<pair<int, int> > pixelList;
  pixelList.push_back(make_pair(12,22));
  pixelList.push_back(make_pair(5,5));
  pixelList.push_back(make_pair(15,26));
  pixelList.push_back(make_pair(20,32));
  pixelList.push_back(make_pair(25,36));
  pixelList.push_back(make_pair(30,42));
  pixelList.push_back(make_pair(35,50));
  pixelList.push_back(make_pair(40,60));
  pixelList.push_back(make_pair(45,70));
  pixelList.push_back(make_pair(50,75));

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  uint16_t FLAGS = FLAG_FORCE_MASKED;

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  TH2D *h2(0);
  map<string, TH2D*> maps;

  bool gofishing(false);
  vector<pair<uint8_t, pair<uint8_t, vector<pixel> > > >  rresults;
  int ic(-1), ir(-1);
  for (unsigned int ifwp = 0; ifwp < pixelList.size(); ++ifwp) {
    gofishing = false;
    ic = pixelList[ifwp].first;
    ir = pixelList[ifwp].second;
    fApi->_dut->testPixel(ic, ir, true);
    fApi->_dut->maskPixel(ic, ir, false);

    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
      h2 = bookTH2D(Form("fwp_c%d_r%d_C%d", ic, ir, rocIds[iroc]),
		    Form("fwp_c%d_r%d_C%d", ic, ir, rocIds[iroc]),
		    256, 0., 256., 256, 0., 256.);
      h2->SetMinimum(0.);
      h2->SetDirectory(fDirectory);
      fHistOptions.insert(make_pair(h2, "colz"));
      maps.insert(make_pair(Form("fwp_c%d_r%d_C%d", ic, ir, rocIds[iroc]), h2));
    }

    rresults.clear();
    try{
      rresults = fApi->getEfficiencyVsDACDAC("caldel", 0, 255, "vthrcomp", 0, 180, FLAGS, 5);
    } catch(pxarException &e) {
      LOG(logCRITICAL) << "pXar execption: "<< e.what();
      gofishing = true;
    }

    fApi->_dut->testPixel(ic, ir, false);
    fApi->_dut->maskPixel(ic, ir, true);
    if (gofishing) continue;

    string hname;
    for (unsigned i = 0; i < rresults.size(); ++i) {
      pair<uint8_t, pair<uint8_t, vector<pixel> > > v = rresults[i];
      int idac1 = v.first;
      pair<uint8_t, vector<pixel> > w = v.second;
      int idac2 = w.first;
      vector<pixel> wpix = w.second;
      for (unsigned ipix = 0; ipix < wpix.size(); ++ipix) {
          hname = Form("fwp_c%d_r%d_C%d", ic, ir, wpix[ipix].roc());
          if (maps.count(hname) > 0) {
	        maps[hname]->Fill(idac1, idac2, wpix[ipix].value());
	      } else {
	        LOG(logDEBUG) << "bad pixel address decoded: " << hname << ", skipping";
	      }
      }
    }


    bool okVthrComp(false), okCalDel(false);
    bool okAllRocs(true);
    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
      okVthrComp = okCalDel = false;
      hname = Form("fwp_c%d_r%d_C%d", ic, ir, rocIds[iroc]);
      h2 = maps[hname];

      h2->Draw("colz");
      PixTest::update();

      TH1D *hy = h2->ProjectionY("_py", 5, h2->GetNbinsX());
      double vcthrMax = hy->GetMaximum();
      double bottom   = hy->FindFirstBinAbove(0.5*vcthrMax);
      double top      = hy->FindLastBinAbove(0.5*vcthrMax);
      double vthrComp = top - 50;
      delete hy;
      if (vthrComp > bottom) {
	okVthrComp = true;
      }

      TH1D *hx = h2->ProjectionX("_px", vthrComp, vthrComp);
      double cdMax   = hx->GetMaximum();
      double cdFirst = hx->GetBinLowEdge(hx->FindFirstBinAbove(0.5*cdMax));
      double cdLast  = hx->GetBinLowEdge(hx->FindLastBinAbove(0.5*cdMax));
      delete hx;
      if (cdLast - cdFirst > 30) {
	okCalDel = true;
      }

      if (!okVthrComp || !okCalDel) {
	okAllRocs = false;
	LOG(logINFO) << hname << " does not pass: vthrComp = " << vthrComp
		     << " Delta(CalDel) = " << cdLast - cdFirst << ((ifwp != pixelList.size() - 1) ? ", trying another" : ".");
	break;
      } else{
	LOG(logDEBUG) << hname << " OK, with vthrComp = " << vthrComp << " and Delta(CalDel) = " << cdLast - cdFirst;
      }
    }
    if (okAllRocs) {
      for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
	string name = Form("fwp_c%d_r%d_C%d", ic, ir, rocIds[iroc]);
	TH2D *h = maps[name];
	fHistList.push_back(h);
	h->Draw(getHistOption(h).c_str());
	fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
	PixTest::update();
      }
      break;
    } else {
      for (map<string, TH2D*>::iterator il = maps.begin(); il != maps.end(); ++il) {
	delete (*il).second;
      }
      maps.clear();
    }
  }

  if (maps.size()) {
    LOG(logINFO) << "Found working pixel in all ROCs: col/row = " << ic << "/" << ir;
    clearSelectedPixels();
    fPIX.push_back(make_pair(ic, ir));
    addSelectedPixels(Form("%d,%d", ic, ir));
  } else {
    LOG(logINFO) << "Something went wrong...";
    LOG(logINFO) << "Didn't find a working pixel in all ROCs.";
    for (size_t iroc = 0; iroc < rocIds.size(); iroc++) {
      LOG(logINFO) << "our roc list from in the dut: " << static_cast<int>(rocIds[iroc]);
    }
  }

  dutCalibrateOff();
  restoreDacs();

}
