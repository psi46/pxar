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

int PixTestPretest::fPrerun = 0;
int PixTestPretest::fNtrigTimingTest  = 100;
int PixTestPretest::fIgnoreReadbackErrors = false;
int PixTestPretest::fGetBufMethod = 1; // 0:daqGetBuffer  1:daqGetRawEventBuffer


// ----------------------------------------------------------------------
PixTestPretest::PixTestPretest( PixSetup *a, std::string name) :
PixTest(a, name),
  fTargetIa(24), fNoiseWidth(22), fNoiseMargin(10),
  fParNtrig(1),
  fParVcal(200),
  fParDeltaVthrComp(-50),
  fParFracCalDel(0.5),
  fIgnoreProblems(0)
{
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
string PixTestPretest::toolTip(string what) {
  if (string::npos != what.find("programroc")) return string("Quick check that each ROC is programmable ");
  if (string::npos != what.find("setvana")) return string("Set VANA such that I(ROC) ~ 24mA ");
  if (string::npos != what.find("settimings")) return string("optimize timing settings for TBM08A/TBM08B");
  if (string::npos != what.find("findtiming")) return string("optimize timing settings for TBMs >= TBM08C");

  if (string::npos != what.find("findworkingpixel")) return string("try to find working pixels on all ROCs");
  if (string::npos != what.find("setvthrcompcaldel")) return string("adjust vthrcomp vs caldel (`tornado plot')");
  if (string::npos != what.find("savedacs")) return string("write DAC parameters to file");
  return string("nada");
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
  if ((tbmtype == "tbm09c") || (tbmtype == "tbm08c") || (tbmtype == "tbm10c")
       || (tbmtype == "tbm08d")  || (tbmtype == "tbm10d") ) {
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
void PixTestPretest::findTimingCmd() {

  banner(Form("PixTestPretest::findTimingCmd() "));
  PixTestFactory *factory = PixTestFactory::instance();
  PixTest *t =  factory->createTest("cmd", fPixSetup);
  t->runCommand("pretesttiming");
  delete t;

  // -- parse output file
  ifstream INS;
  char buffer[1000];
  string sline, sparameters, ssuccess;
  string::size_type s1;
  vector<double> x;
  string lcase = string(Form("pxar_timing_%s.log", fPixSetup->getConfigParameters()->getDirectory().c_str()));
  std::transform(lcase.begin(), lcase.end(), lcase.begin(), ::tolower);
  system(Form("/bin/mv %s %s/pxar_timing.log", lcase.c_str(), fPixSetup->getConfigParameters()->getDirectory().c_str()));
  INS.open(Form("%s/pxar_timing.log", fPixSetup->getConfigParameters()->getDirectory().c_str()));
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

}


// ----------------------------------------------------------------------
// this is a local replica of the above with some histogramming
void PixTestPretest::findTiming() {

  banner(Form("PixTestPretest::findTiming() "));

  string tbmtype = fApi->_dut->getTbmType();
  if (! ((tbmtype=="tbm09c")||(tbmtype=="tbm08c")||(tbmtype=="tbm10c")
	 || (tbmtype == "tbm08d")  || (tbmtype == "tbm10d")) ){
    LOG(logWARNING) << "This only works for TBM08c/08d/09c/10c/10d!";
  }

  uint8_t register_0=0;
  uint8_t register_e=0;
  uint8_t register_a=0;
  tbmget("base0", TBMA, register_0);
  tbmget("basee", TBMA, register_e);
  tbmget("basea", TBMA, register_a);
  uint8_t d400= (register_e >> 2) & 0x7;
  uint8_t d160= (register_e >> 5) & 0x7;
  int tokendelay =(register_a >> 7) & 0x1;
  int htdelay =   (register_a >> 6) & 0x1;
  int rocdelay =  (register_a)&7;


  int nloop=10;

  // disable token pass
  tbmsetbit("base0",ALLTBMS , 6, 1);
  // diagonal scan to find something that works
  int nmax=0;
  for(uint8_t m=0; m<8; m++){
    int nvalid = test_timing(nloop, m, m);
    LOG(logINFO) << "diag scan " << (int) m << "  valid = " << nvalid << "/ " << nloop;
    if (nvalid>nmax){
      d400 = m;
      d160 = m;
      nmax = nvalid;
    }
    flush(out);
  }
  if (nmax==0){
    out << " no working phases found ";
    tbmset("base0",ALLTBMS ,register_0);
    tbmset("basee",ALLTBMS ,register_e);
    return;
  }
  LOG(logINFO) << "diag scan result = " << (int) d400;

  return;

  // -- parse output file
  ifstream INS;
  char buffer[1000];
  string sline, sparameters, ssuccess;
  string::size_type s1;
  vector<double> x;
  string lcase = string(Form("pxar_timing_%s.log", fPixSetup->getConfigParameters()->getDirectory().c_str()));
  std::transform(lcase.begin(), lcase.end(), lcase.begin(), ::tolower);
  system(Form("/bin/mv %s %s/pxar_timing.log", lcase.c_str(), fPixSetup->getConfigParameters()->getDirectory().c_str()));
  INS.open(Form("%s/pxar_timing.log", fPixSetup->getConfigParameters()->getDirectory().c_str()));
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

}


// ----------------------------------------------------------------------
void PixTestPretest::setTimings() {

  string tbmtype = fApi->_dut->getTbmType(); //"tbm09c"
  bool OK(false);
  if (tbmtype == "tbm08b") OK = true;
  if (tbmtype == "tbm08a") OK = true;
  if (!OK) {
    LOG(logINFO) << "Wrong timing test for TBM type " << tbmtype;
    return;
  }
  size_t nTBMs = fApi->_dut->getNTbms();
  if (nTBMs==0) {
    LOG(logINFO) << "Timing test not needed for single ROC.";
    return;
  }

  fStopTest = false;
  // Start test timer
  timer t;

  banner(Form("PixTestPreTest::setTimings()"));
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  TLogLevel UserReportingLevel = Log::ReportingLevel();

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

  fApi->setVcalLowRange();
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
      rresults = fApi->getEfficiencyVsDACDAC("caldel", 0, 255, "vthrcomp", 0, 255, FLAGS, fParNtrig);
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
  fApi->setVcalHighRange();

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
      rresults = fApi->getEfficiencyVsDACDAC("caldel", 0, 255, "vthrcomp", 0, 255, FLAGS, 5);
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


// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%% functions for timing test (all from CmdProc)
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

int PixTestPretest::tbmset(int address, int  value){
  /* emulate direct access via register address
   * higher bits select the tbm 0 or 1 in case of layer 1 modules */
  uint8_t core=0;
  uint8_t base = (address & 0xF0);
  uint8_t tbm = (address & 0x100);
  if( base == 0xF0){ core = 1;}
  else if(base==0xE0){ core = 0;}
  else {out << "bad tbm register address "<< hex << address << dec << "\n"; return 1;};

  if (tbm==1){
    core +=2;
  }

  uint8_t idx = (address & 0x0F) >> 1;
  const char* apinames[] = {"base0", "base2", "base4","invalid","base8","basea","basec","basee"};
  fApi->setTbmReg( apinames[ idx], value, core );
  return 0; // nonzero values for errors
}


int PixTestPretest::tbmset(string name, uint8_t coreMask, int value, uint8_t valueMask){
  /* set a tbm register, allow setting a subset of bits (thoese where mask=1)
   * the default value of mask is 0xff, i.e. all bits are changed
   * the cores = 0:TBMA(0), 1:TBMB(0), 2:TBMA(1), 4:TBMB(1)
   * combinations are possible
   *
   */
  if ((value & (~valueMask) )>0) {
    out << "Warning! tbm set value " << hex << (int) value
	<< " has bits outside mask ("<<hex<< (int) valueMask << ")\n";
  }


  int err=1;
  //out << "tbmset: " << name << " " <<  (int) coreMask << " " << value << "  " << bitset<8>(valueMask) << "\n";
  for(size_t core=0; core < fApi->_dut->getNEnabledTbms(); core++){
    if ( ((coreMask >> core) & 1) == 1 ){
      std::vector< std::pair<std::string,uint8_t> > regs = fApi->_dut->getTbmDACs(core);
      if (regs.size()==0) {
	out << "TBM registers not set !?! This is not going to work.\n";
      }
      for(unsigned int i=0; i<regs.size(); i++){
	if (name==regs[i].first){
	  // found it , do something
	  uint8_t present = regs[i].second;
	  uint8_t update = value & valueMask;
	  update |= (present & (~valueMask) );
	  out << "changing tbm reg " <<  name << "["<<core<<"]";
	  out << " from 0x" << hex << (int) regs[i].second;
	  out << " to 0x" << hex << (int) update << "\n";
	  out << "fApi->setTbmReg  name=" << name << " update=" << (int) update << " core= "<< (int) core << "\n";
	  bool stat = fApi->setTbmReg( name, update, core );
	  if (stat){
	    err=0;
	  }else{
	    out << "api setTbmReg returns error status\n";
	    err=1;
	  }
	}
      }
    }
  }
  return err; // nonzero values for errors
}



int PixTestPretest::tbmsetbit(string name, uint8_t coreMask, int bit, int value){
  /* set individual bits */
  return tbmset(name, coreMask, (value & 1)<<bit, 1<<bit);
}


int PixTestPretest::tbmget(string name, const uint8_t coreMask, uint8_t & value){
  /* get a tbm register value as cached by the api
   * when multiple cores are present int the core mask, only the first value is returned
   */
  int error=1;
  for(uint8_t core=0; core<fApi->_dut->getNEnabledTbms(); core++){
    if (( (coreMask >> core) & 1 ) == 1){
      std::vector< std::pair<std::string,uint8_t> > regs = fApi->_dut->getTbmDACs(core);
      for(unsigned int i=0; i<regs.size(); i++){
	if (name==regs[i].first){ value = regs[i].second; error=0;}
      }
      return error;
    }
  }
  return error; // nonzero values for errors
}

int PixTestPretest::pg_restore(){
  // restore the default pattern generator so that it works for other tests
  std::vector<std::pair<std::string,uint8_t> > pg_setup
    =  fPixSetup->getConfigParameters()->getTbPgSettings();
  for(std::vector<std::pair<std::string,uint8_t> >::iterator it=pg_setup.begin(); it!=pg_setup.end(); it++){
    LOG(logINFO) << it->first << "  : " << (int) it->second;
  }
  fApi->setPatternGenerator(pg_setup);
  return 0;
}


int PixTestPretest::restoreDaq(int verbosity){
  bool stat = fApi->daqStop(false);
  if( (! stat ) && (verbosity>0) ){
    out  << "something wrong with daqstop !" <<endl;
  }
  pg_restore(); // undo any changes
  return 0;
}

int PixTestPretest::pg_sequence(int seq, int length){
  // configure the DTB pattern generator for a simple sequence
  // as other tests don't seem to take care of the pg configuration,
  // this must be undone afterwards (using pg_restore)
  // if length is 0 (default), the full buffersize is used for fPeriod
  // the calculated sequence length is returned as a return value
  vector< pair<string, uint8_t> > pgsetup;
  uint16_t delay = 15 + 7 + 8*3 + 7 + 8*6; // module minimal readout time, allow 8 hits
  if (seq & 0x20 ) { pgsetup.push_back( make_pair("resettbm", 10) ); delay+=11;}
  if (seq & 0x10 ) { pgsetup.push_back( make_pair("sync", 10) ); delay+=11;}
  if (seq & 0x40 ) { pgsetup.push_back( make_pair("resr", fTRC) ); delay+=fTRC+1; }
  if (seq & 0x08 ) { pgsetup.push_back( make_pair("resr", fTRC) ); delay+=fTRC+1; }
  if (seq & 0x04 ) { pgsetup.push_back( make_pair("cal",  fTCT )); delay+=fTCT+1; }
  if (seq & 0x02 ) { pgsetup.push_back( make_pair("trg",  fTTK ));  delay+=fTTK+1;}
  if (seq & 0x01 ) { pgsetup.push_back( make_pair("token", 1)); delay+=1; }
  pgsetup.push_back(make_pair("none", 0));


  if (length==0){
    fPeriod = fMaxPeriod;
  }else{
    fPeriod = length;
  }

  fApi->setPatternGenerator(pgsetup);
  return delay;
}

int PixTestPretest::setupDaq(int ntrig, int ftrigkhz, int verbosity){
  /* setup pattern generator and daq
   * always call restoreDaq before returning control to Pxar
   */

  // warn user when no data expected
  if( (ntrig>0) && (fApi->_dut->getNTbms()>0) && ((fSeq & 0x02 ) ==0 ) ){
    out << "The current sequence does not contain a trigger!\n";
  }else if( (fApi->_dut->getNTbms()==0) && ((fSeq & 0x01 ) ==0 ) ){
    out <<"The current sequence does not contain a token!\n";
  }

  int length=0;
  if((ntrig==0) || (ntrig==1) || (ftrigkhz==0)){
    length=fBufsize/8;
  }else{
    length = 40000 / ftrigkhz;
  }

  pg_sequence( fSeq, length ); // set up the pattern generator
  fApi->daqTriggerSource("pg_dir");
  bool stat = fApi->daqStart(fBufsize, fPixelConfigNeeded);

  for(unsigned int i=0; i<8; i++){ fDeser400XOR1sum[i]=fDeser400XOR2sum[i]=0;}

  fPixelConfigNeeded = false;
  if (! stat ){
    LOG(logINFO) << "something wrong with daqstart !!!";
    return 2;
  }

  LOG(logINFO) << "runDaq  " << dec << ftrigkhz  << "   " << length << "  " << fPeriod;
  LOG(logINFO) << "runDaq  " << dec << ftrigkhz  << "   " << length << "  " << fPeriod;
  int leff = 40* int(length / 40);  // emulate testboards cDelay
  if((leff<length)&&(ntrig>1)){
    out << "period will be truncated to " << dec<< leff << " BC  = " << int(40000/leff) << " kHz !!" << endl;
  }
  return 0;

}


int PixTestPretest::countErrors(unsigned int ntrig, int ftrigkhz, int nroc, bool setup){
    int stat = runDaqRaw(fBuf, ntrig, ftrigkhz, 0, setup);
    if (stat>0) return -1; // no data
    vector<DRecord > data;
    data.clear();
    int verbosity = 1;
    stat = getData(fBuf, data, verbosity, nroc, true);
    if (stat>0){
        return stat;
    }else{
        if  (fNumberOfEvents==ntrig) return 0;
        LOG(logINFO) << "number of events (" << fNumberOfEvents << ") does not match triggers "<< ntrig;
        return 999;
    }
}

int PixTestPretest::countGood(unsigned int nloop, unsigned int ntrig, int ftrigkhz, int nroc){
  /* run loops eve events with ntrig triggers each ,
   * returns the total number good loops
   * the result separated by channel is available in fGoodLoops */

  tbmset("base4", ALLTBMS, 0x80);// reset tbm event counter, both cores
  int good=0;
  clear_DaqChannelCounter( fGoodLoops );

  setupDaq(ntrig, ftrigkhz, 0);
  for(unsigned int k=0; k<nloop; k++){
    if(fPrerun>0) runDaqRaw(fPrerun, 10);
    int nerr = countErrors(ntrig, ftrigkhz, nroc, false);
    if(nerr==0){
      good++;
    }

    for( unsigned int i=0;i<8; i++){  //legacy code
      fDeser400XOR1sum[i] += ( ((fDeser400XOR[0]|fDeser400XOR[1]) >> i) & 1);
    }

    // channel-wise book-keeping
    for( unsigned int i=0; i<nDaqChannelMax; i++ ) {
      if ((fDaqErrorCount[i] == 0)  && (fNEvent[i]==ntrig) ){
	fGoodLoops[i]++;
      }
    }
  }
  restoreDaq(1);
  return good;
}



int PixTestPretest::test_timing(int nloop, int d160, int d400, int rocdelay, int htdelay, int tokdelay){
  int ftrigkhz = 100;
  int nroc=16;
  uint8_t value = ( (d160&0x7)<<5 ) + ( (d400&0x7 )<<2 );

  int stat = tbmset("basee", TBMA, value);
  if(stat>0){
    out << "error setting delay  base E " << hex << (int) value << dec << "\n";
  }

  if (rocdelay>=0){
    value = ( (tokdelay&0x1)<<7 ) + ( (htdelay&0x1)<<6 ) + ( (rocdelay&0x7)<<3 ) + (rocdelay&0x7);
    stat = tbmset("basea",ALLTBMS, value);
    if(stat>0){
      out << "error setting delay  base A " << hex << (int) value << dec << "\n";
    }
  }
  tbmset("base4", ALLTBMS, 0x10); // reset once after changing phases
  pxar::mDelay( 10 );
  return countGood(nloop, fNtrigTimingTest, ftrigkhz, nroc);
}


void PixTestPretest::flush(stringstream &st) {
  string s = st.str();
  if (s.size()>0){

    // break multiline output into lines
    std::stringstream ss( s );
    std::string line;
    while(std::getline(ss,line,'\n')){
      LOG(logINFO) << line;
    }
  }

  gSystem->ProcessEvents();
}


int PixTestPretest::runDaqRaw(vector<uint16_t> & buf, int ntrig, int ftrigkhz, int verbosity, bool setup){
    /* run ntrig sequences and get the raw data from the DTB */

    if(setup) setupDaq(ntrig, ftrigkhz, verbosity);

	if(ntrig>0){
	  fApi->daqTrigger(ntrig, fPeriod);
	}

    getBuffer( buf );
    if(buf.size()==0){
        if (verbosity>0){ out << "no data !" << endl;}
        if (setup) restoreDaq(verbosity);
        return 1;
    }

    if(setup) restoreDaq(verbosity);
    return 0;
}



int PixTestPretest::runDaqRaw(int ntrig, int ftrigkhz, int verbosity){
    /* run ntrig sequences but don't get the data */

    int length=0;
    if((ntrig==1) || (ftrigkhz==0)){
        length=0;
    }else{
        length = 40000 / ftrigkhz;
    }

    pg_sequence( fSeq, length ); // set up the pattern generator
    fApi->daqTriggerSource("pg_dir");

    bool stat = fApi->daqStart(fBufsize, fPixelConfigNeeded);
    if (! stat ){
        if(verbosity>0){ out << "something wrong with daqstart !!!" << endl;}
        return 2;
    }

    LOG(logINFO) << "runDaq  f=" << dec <<  ftrigkhz  << "   length=" << length << "  fPeriod=" << fPeriod;
    int leff = 40* int(length / 40);  // emulate testboards cDelay
    if((leff<length)&&(ntrig>1)){
        out << "period will be truncated to " << dec<< leff << " BC  = " << int(40000/leff) << " kHz !!" << endl;
    }
    if (length>0){
        fApi->daqTrigger(ntrig, length);
    }else{
        // take the default (maximum)
        fApi->daqTrigger(ntrig, fPeriod);
    }

    fApi->daqStop(false);
    fPixelConfigNeeded = false;
    pg_restore(); // undo any changes

    return 0;
}

int PixTestPretest::getData(vector<uint16_t> & buf, vector<DRecord > & data, int verbosity,
    int nroc_expected, bool resetStats){
    // pre-decoding and validity check of raw data in fBuf
    // returns the number of errors
    // record flags:
    //   0 (0x00) = hit         24 bits ("raw")
    //   4 (0x04) = roc header  12 bits
    //  10 (0x0a) = tbm header  16 bits
    //  14 (0x0e) = tbm trailer 16 bits
    //  16  = filler
    //
    // FIXME, for >1 event this code required fGetBufMethod=1

    if(verbosity>100){
	stringstream ss;
        for(unsigned int i=0; i< buf.size(); i++){
            ss << hex << setw(4) << setfill('0') << buf[i] << " ";
        }
        LOG(logINFO) << ss.str() << dec << setfill(' ');
    }

    unsigned int nerr=0;

    data.clear();
    fDeser400err  =  0;
    if (resetStats) resetDaqStatus();

    for(size_t i=0; i<17; i++){
        fRocHeaderData[i]=0;
    }
    vector<uint16_t> rocHeaderWord(17);
    vector<uint16_t> rocHeaderBits(17);

    fNumberOfEvents = 0;
    fHeaderCount=0;
    fHeadersWithErrors.clear();  // a list of headers with errors

    unsigned int i=0;
    if ( fApi->_dut->getNTbms()>0 ) {

        uint8_t daqChannel=0; // from tbm header qualifier
        uint8_t roc=0;
        uint8_t tbm=0;
        bool tbmHeaderSeen=false;
        unsigned int nevent=0;
        unsigned int lastEventStart=0;

        vector<unsigned int> rocCounter(nDaqChannelMax);


        unsigned int nloop=0;
        int fffCounter=0;


        while((i<buf.size())&&((nloop++)<buf.size())){

            uint16_t flag= ((buf[i]>>12)&0xE);

            if (buf[i]&0x1000){
                fDeser400err++;
                fDeser400SymbolErrors[daqChannel]++;
                fDaqErrorCount[daqChannel]++;
            }

            if ((buf[i]&0x0fff)==0xfff){
                fffCounter++;
                if (fffCounter>1000){
                    if(verbosity>0) out << "junk data, decoding aborted\n";
                    fDaqErrorCount[daqChannel]++;
                    nerr++;
                    return nerr + fDeser400err;
                 }
            }else{
                fffCounter=0;
            }


            if( flag == 0xa ){ // TBM header

                tbmHeaderSeen=true;
                fHeaderCount++;

                daqChannel = (buf[i]&0x700)>>8;
                fNTBMHeader[daqChannel]++;
                rocCounter[daqChannel]=0;

                if (buf[i]&0x0800) { // assuming the lower bits are used for the channel nr
                    nerr++; // should be 0
                    fDaqErrorCount[daqChannel]++;
                    if (verbosity>0) out << "illegal deser400 header record \n";
                }


                uint8_t h1=buf[i]&0xFF;
                i++;
                if(i>=buf.size()){
                    if(verbosity>0) out << " unexpected end of data\n";
                    nerr++;
                    fHeadersWithErrors.push_back(fHeaderCount);
                    return nerr + fDeser400err;
                }

                if (buf[i]&0x1000){
                    fDeser400SymbolErrors[daqChannel]++;
                    fDaqErrorCount[daqChannel]++;
					fDeser400err++;
				}

                if ((buf[i]&0xE000)==0x8000){
                    uint8_t h2 = buf[i]&0xFF;
                    data.push_back( DRecord( daqChannel, 0xa, (h1<<8 | h2), buf[i-1], buf[i]) );
                    i++;
                }else{
                    if(verbosity>0){
                        out << " incomplete header ";
                        out << hex << (int) buf[i-1] << " " << (int) buf[i]  << dec << "\n";
                    }
                    nerr ++;
                    fDaqErrorCount[daqChannel]++;
                }
                continue;
            }// TBM header


            if( flag == 0x4 ){ // ROC header

                if( !tbmHeaderSeen ){
                    nerr++;
                    fDaqErrorCount[daqChannel]++;
                    if(verbosity>0)  out << "ROC header outside TBM header/trailer ["<<(int)i<< "]\n";
                    cout << "ROC header outside TBM header/trailer\n";
                    for(unsigned int ii=lastEventStart; ii<i+1; ii++){
                        cout << hex << setw(4) <<  setfill('0') << buf[ii] << " ";
                    }
                    cout << endl;
                }

                if(nroc_expected==0){
                    nerr++;
                    fDaqErrorCount[daqChannel]++;
                    if(verbosity>0) out<< "no rocs expected\n";
                }else{
                    roc ++;
                    unsigned int rocId = rocIdFromReadoutPosition( daqChannel, rocCounter[daqChannel]);
                    if ( rocCounter[daqChannel] < fnRocPerChannel){
                        rocCounter[daqChannel]++;
                    }else{
                        rocId=16;// for unknown
                    }

                    if (rocId>16){
                        cout<< "roc counting error " << dec << (int) daqChannel << setw(4) << dec << (int) rocCounter[daqChannel]<<  setw(4) << dec << (int) fDaqChannelRocIdOffset[daqChannel]<< endl;
                        rocId=0;
                        nerr++;
                        fDaqErrorCount[daqChannel]++;
                    }

                    if( (buf[i]&0x0004)>0 ) {
                        nerr++;
                        fDaqErrorCount[daqChannel]++;
                        if(verbosity>0) out << "zero-bit in roc header not zero\n";
                    }
                    data.push_back( DRecord(daqChannel, 0x4, buf[i]&0x3, buf[i], rocId) );

                    // roc header data
                    rocHeaderWord[rocId] = (rocHeaderWord[rocId] << 1) + (buf[i]&0x0001);
                    rocHeaderBits[rocId]++;

                    if ((buf[i]&0x0002)==2){  // startbit seen
                        if (rocHeaderBits[rocId]==16){
                            fRocHeaderData[rocId] = rocHeaderWord[rocId]&0xffff;
                            uint8_t hrocid= (fRocHeaderData[rocId] & 0xF000) >> 12;

                            if ( !(hrocid == rocId) ){
                                fRocReadBackErrors[daqChannel]++;
                                if( !fIgnoreReadbackErrors ) {
									nerr++;
									fDaqErrorCount[daqChannel]++;
								}
                                if( !fIgnoreReadbackErrors ){
                                    cout << hex << setw(4) << rocHeaderWord[rocId];
                                    cout  << "    rocid=" << dec <<  setw(2) << (int) rocId;
                                    cout <<  "  <> " << dec <<  setw(2) << (int) hrocid;
                                    cout << "  daqChannel " << (int) daqChannel;
                                    cout << endl;
                                }
                            }else{
                                //cout << " readback ok " << dec<< (int) rocId << "  " << hex<< rocHeaderWord[rocId] << endl;
                            }
                        }
                        rocHeaderWord[rocId]=0;
                        rocHeaderBits[rocId]=0;
                    }else  if(rocHeaderBits[rocId]==16){
                        if( ! fIgnoreReadbackErrors ) {
							nerr++;
						}
                        if( !fIgnoreReadbackErrors ){
                                    cout << "start bit expected ";
                                    cout  << "    rocid=" << dec <<  setw(2) << (int) rocId;
                                    cout << "  daqChannel " << (int) daqChannel;
                                    cout << endl;
                                }

                    }



                    uint8_t xordata = (buf[i] & 0x0ff0)>>4;
                    if(xordata==0xff){
                        nerr++;
                        fDaqErrorCount[daqChannel]++;
                        if(verbosity>0) out << "Deser400 phase error\n";
                        fDeser400PhaseErrors[daqChannel]++;
                    }else{
                        if (fDeser400XOR[daqChannel]==xordata){
                            // ok
                        }else{
                            if(fDeser400XOR[daqChannel]==0x100){
                                // inital value, ok
                            }else{
                                fDeser400XORChanges[daqChannel]++;
                            }
                            fDeser400XOR[daqChannel]=xordata;
                        }


                    }
                }

                i++;
                continue;
            }


            if (flag == 0x0){  // hit
                if (roc==0) {
                    if(verbosity>0) out << "no hit expected here\n";
                    nerr++;
                    fDaqErrorCount[daqChannel]++;
                }
                int rocId = rocIdFromReadoutPosition( daqChannel, roc-1);
                int d1=buf[i++];
                if(i>=buf.size()){
                    if(verbosity>0) out << " unexpected end of data\n";
                    nerr++;
                    fDaqErrorCount[daqChannel]++;
                    fHeadersWithErrors.push_back(fHeaderCount);
                    return nerr + fDeser400err;
                }

                if (buf[i]&0x1000) {
                    fDaqErrorCount[daqChannel]++;
                    fDeser400err++;
                }

                flag = (buf[i]>>12)&0xe;
                int d2=buf[i++];
                if (flag == 0x2) {
                    uint32_t raw = ((d1 &0x0fff) << 12) + (d2 & 0x0fff);
                    if( tbmWithDummyHits() && (roc == fnRocPerChannel) && (raw==0xffffff)){
                        data.push_back( DRecord(daqChannel, 15, raw, buf[i-2], buf[i-1], rocId) );
                    }else{
                        data.push_back( DRecord(daqChannel, 0x0, raw, buf[i-2], buf[i-1], rocId) );
                    }

                }else{
                    if(verbosity>0) {
                        out << " unexpected qualifier in ROC hit:" << (int) flag << " "
                            << hex << setw(4) << setfill('0')  << buf[i-1]
                            << dec << setfill(' ') << " at position "<< i-1 << "\n";
                        }
                    nerr++;
                    fDaqErrorCount[daqChannel]++;
                 }
                continue;
            }

            if (flag  == 0xe){
                if (!tbmHeaderSeen){
                    cout << "tbm trailer without header [" << (int) i << "]" << endl;
                    for(unsigned int ii=lastEventStart; ii< i+2; ii++){
                        cout << hex << setw(4) <<  setfill('0') << buf[ii] << " ";
                    }
                    cout << endl;
                }
                fNEvent[daqChannel]++;

                tbmHeaderSeen=false;
                // TBM trailer
                if (buf[i]&0x0f00){
                    nerr++;
                    fDaqErrorCount[daqChannel]++;

                     if(verbosity>0) {
                        out << "deser400 error flags: ";
                        if (buf[i]&0x0800) out << "frame ";
                        if (buf[i]&0x0400) out << "code ";
                        if (buf[i]&0x0200) out << "idle ";
                        if (buf[i]&0x0100) out << "missing trailer ";
                        out << " channel " << (int) daqChannel << "\n";
                    }
                    if (buf[i]&0x0800) fDeser400_frame_error[daqChannel]++;
                    if (buf[i]&0x0400) fDeser400_code_error[daqChannel]++;
                    if (buf[i]&0x0200) fDeser400_idle_error[daqChannel]++;
                    if (buf[i]&0x0100) fDeser400_trailer_error[daqChannel]++;

                }
                int t1=buf[i++];
                if(i>=buf.size()){
                    nerr++;
                    fDaqErrorCount[daqChannel]++;

                    if(verbosity>0) out << "unexpected end of data\n";
                    return nerr + fDeser400err;
                }

                flag = (buf[i]>>12)&0xe;
                if (flag != 0xc ){
                    if(verbosity>0) out << "unexpected qualifier " << (int) flag <<"in TBM trailer \n";
                    continue;
                }
                /* in fw4.6 (and higher?) this qualifier just repeat the previous entry
                if (buf[i]&0x1000) fDeser400err++;
                if (buf[i]&0x0f00) {
                    nerr++;
                    if(verbosity>0) out << "illegal data in deser400 trailer record \n";
                }
                */
                int t2=buf[i++];
                data.push_back( DRecord(daqChannel, 0xe, (t1&0xFF)<<8 | (t2&(0xff)), t1, t2) );
                tbm++;

                if (tbm==fnDaqChannel){
                    tbm=0; // new event, this depends on the getBuffer method
                    nevent++;
                    lastEventStart=i;
                }

                roc=0;
                continue;
            }

            if (verbosity>0){
                out << "unexpected qualifier " << hex << (int) flag <<", skipped\n" << dec;
            }
            nerr++;
            fDaqErrorCount[daqChannel]++;
            i++;

        } // while i< buf.size()

        // debugging
        if ((nloop>=buf.size())&&(buf.size()>1000) ){
                cout << "stuck at i=" << dec << i << endl;
                cout << hex << (int) buf[i-1] << " " << (int) buf[i] << dec << endl;
        }

         fNumberOfEvents = nevent;

    }

    else if (fApi->_dut->getNTbms() == 0) {
        // single ROC
        while(i<buf.size() && (i<500) ){

            //if ( (buf[i] & 0x0ff8) == 0x07f8 ){
            if ( (buf[i] & 0x8000) == 0x8000 ){
                // roc header
                data.push_back(DRecord(0, 0x4, buf[i]&0x07, buf[i]));
                i++;
            }
            else if( (i+1)<buf.size() ){
                // hit
                 uint32_t raw = ((buf[i] & 0x0fff)  << 12) + (buf[i+1] & 0x0fff);
                 data.push_back(DRecord(0, 0, raw, buf[i], buf[i+1] ) );
                i+=2;
            }else{
                out << "unexpected end of data\n";
                nerr++;
                return nerr;
            }
        }
    }

    return nerr + fDeser400err;
}

int PixTestPretest::getBuffer(vector<uint16_t> & buf){
    for(size_t i=0; i<nDaqChannelMax; i++){
        fDeser400XOR[i]=0;
    }
    if (fGetBufMethod==1){
        buf.clear();
        vector<rawEvent> vre;
        try { vre = fApi->daqGetRawEventBuffer(); }
        catch(pxar::DataNoEvent &) {}

        for(unsigned int i=0; i<vre.size(); i++){
            for(unsigned int j=0; j<vre.at(i).GetSize(); j++){
                buf.push_back( vre.at(i)[j] );
            }
        }
    }else{
        buf.clear();
        try { buf  = fApi->daqGetBuffer(); }
        catch(pxar::DataNoEvent &) {}
    }
    return 0;
}


int PixTestPretest::resetDaqStatus(){
    for(size_t i=0; i<nDaqChannelMax; i++){
        fDeser400XOR[i]=0x100;
        fDeser400SymbolErrors[i]=0;
        fDeser400PhaseErrors[i]=0;
        fDeser400XORChanges[i]=0;
        fNTBMHeader[i]=0;
        fNEvent[i]=0;
        fRocReadBackErrors[i]=0;
        fDeser400_frame_error[i]=0;
        fDeser400_code_error[i]=0;
        fDeser400_idle_error[i]=0;
        fDeser400_trailer_error[i]=0;
        fDaqErrorCount[i]=0;
    }
    return 0;
}
