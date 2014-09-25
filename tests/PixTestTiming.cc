#include <iostream>
#include <bitset>
#include <stdlib.h>
#include <algorithm>

#include <TStopwatch.h>
#include <TMarker.h>
#include <TStyle.h>
#include <TBox.h>

#include "PixTestTiming.hh"
#include "PixUtil.hh"
#include "timer.h"
#include "log.h"
#include "helper.h"
#include "constants.h"
#include "math.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestTiming)

//------------------------------------------------------------------------------
PixTestTiming::PixTestTiming(PixSetup *a, std::string name) : PixTest(a, name)
{
  PixTest::init();
  init();
}

//------------------------------------------------------------------------------
PixTestTiming::PixTestTiming() : PixTest() {}

bool PixTestTiming::setParameter(string parName, string sval) {
  bool found(false);
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true;
      if (!parName.compare("fastscan")) {
        PixUtil::replaceAll(sval, "checkbox(", "");
        PixUtil::replaceAll(sval, ")", "");
        fFastScan = atoi(sval.c_str());
        LOG(logDEBUG) << "fFastScan: " << fFastScan;
        setToolTips();
      }
      if (!parName.compare("targetclk")) {
        fTargetClk = atoi(sval.c_str());
        LOG(logDEBUG) << "PixTestTiming::PixTest() targetclk = " << fTargetClk;
      }
      break;
    }
  }
  return found;
}

void PixTestTiming::init()
{
  LOG(logDEBUG) << "PixTestTiming::init()";

  fDirectory = gFile->GetDirectory(fName.c_str());
  if (!fDirectory)
    fDirectory = gFile->mkdir(fName.c_str());
  fDirectory->cd();
}

void PixTestTiming::setToolTips()
{
  fTestTip = string(Form("scan testboard parameter settings and check for valid readout\n")
                    + string("TO BE IMPLEMENTED!!"));  //FIXME
  fSummaryTip = string("summary plot to be implemented");  //FIXME
}

void PixTestTiming::bookHist(string name)
{
  fDirectory->cd();
  LOG(logDEBUG) << "nothing done with " << name;
}

PixTestTiming::~PixTestTiming()
{
  LOG(logDEBUG) << "PixTestTiming dtor";
  std::list<TH1*>::iterator il;
  fDirectory->cd();
  for (il = fHistList.begin(); il != fHistList.end(); ++il)
    {
      LOG(logINFO) << "Write out " << (*il)->GetName();
      (*il)->SetDirectory(fDirectory);
      (*il)->Write();
    }
}

// ----------------------------------------------------------------------
void PixTestTiming::doTest() {

  fDirectory->cd();
  PixTest::update();
  bigBanner(Form("PixTestTiming::doTest()"));

  ClkSdaScan();
  TH1 *h1 = (*fDisplayedHist);
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update();

  PhaseScan();
  h1 = (*fDisplayedHist);
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update();

  // -- save DACs!
  saveTbParameters();
  LOG(logINFO) << "PixTestTiming::doTest() done";
}

//------------------------------------------------------------------------------
void PixTestTiming::ClkSdaScan() {
  cacheDacs();
  fDirectory->cd();
  PixTest::update();
  banner(Form("PixTestTiming::ClkSdaScan()"));

  //Make a histogram
  TH2D *h2(0);
  h2 = bookTH2D("ClkSdaScan","ClkSdaScan", 20, -0.5, 19.5, 20, -0.5, 19.5);
  h2->SetDirectory(fDirectory);
  setTitles(h2, "Clk", "Sda");
  fHistOptions.insert(make_pair(h2, "colz"));

  //Turn of Vana
  fApi->setDAC("vana", 0);
  pxar::mDelay(2000);
  double IA = fApi->getTBia();

  // Start test timer
  timer t;

  //Scan the Clock and SDA to find the working values. iclk starts at fTargetClk and ends at fTargetClk-1. Both sda and clk ranges are limited to 0-19.
  int GoodSDA = -1;
  int GoodClk = -1;
  map<int, vector<int> > goodclksdalist;
  for (int i = 0; i < 20; i++) {
    int iclk = (i+fTargetClk) % 20;
    vector<int> goodsdalist;
    for (int j = 0; j < 20; j++) {
      int isda = (j+iclk+15) % 20;
      fApi->setTestboardDelays(getDelays(iclk,isda));
      LOG(logDEBUG) << "Checking Clk: " << iclk << " Sda: " << isda;
      fApi->setDAC("vana", 70);
      pxar::mDelay(10);
      double NewIA = fApi->getTBia();
      if (fabs(NewIA-IA) > 0.005) {
        h2->Fill(iclk,isda); //Need 5mA change in current to see if ROCs are programmable
        GoodClk=iclk;
        GoodSDA=isda;
        goodsdalist.push_back(isda);
        if (fFastScan) break;
      }
      fApi->setDAC("vana", 0);
      pxar::mDelay(10);
    }
    goodclksdalist[iclk]=goodsdalist;
    if (fFastScan && GoodClk != -1) break;
  }

  if (GoodClk == -1) {
    LOG(logERROR) << "No working SDA setting found!";
    return;
  }

  //Overly complicated algorithm to figure out the best SDA.
  //Normally there are 7 sda settings that work, and this selects the middle one.
  //It's completcated because the working SDA settings can be 0, 1, 2, 3, 4, 18, and 19. The center most value is 1.
  if (!fFastScan) {
    GoodClk = -1;
    for (int i = 0; i < 20; i++) {
      int iclk = (i+fTargetClk) % 20;
      if (goodclksdalist.count(iclk)) {
        GoodClk = iclk;
        vector<int> goodsdalist = goodclksdalist[iclk];
        if (goodsdalist.size() == 1) {
          GoodSDA=goodsdalist[0];
        } else {
          sort(goodsdalist.begin(),goodsdalist.end());
          for (size_t isda=1; isda<goodsdalist.size(); isda++) if (fabs(goodsdalist[isda]-goodsdalist[isda-1])>1) goodsdalist[isda] -= 20;
          sort(goodsdalist.begin(),goodsdalist.end());
          GoodSDA=goodsdalist[round(goodsdalist.size()/2)];
          if (GoodSDA<0) GoodSDA+=20;
          break;
        }
      }
    }
  }

  //Print out good values and set them on the test board.
  vector<pair<string,uint8_t> > GoodDelays = getDelays(GoodClk,GoodSDA);
  LOG(logINFO) << "SDA delay found at clock:" << GoodClk;
  for(vector<pair<string,uint8_t> >::iterator idelay = GoodDelays.begin(); idelay != GoodDelays.end(); ++idelay) {
    LOG(logINFO) << idelay->first << ":    " << int(idelay->second);
    fPixSetup->getConfigParameters()->setTbParameter(idelay->first, idelay->second);
  }
  fApi->setTestboardDelays(GoodDelays);

  //Draw the plot
  h2->Draw("colz");
  fHistList.push_back(h2);
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);
  PixTest::update();
  restoreDacs();

  // Print timer value:
  LOG(logINFO) << "Test took " << t << "ms.";
  LOG(logINFO) << "PixTestTiming::ClkSdaScan() done.";
}

//------------------------------------------------------------------------------
void PixTestTiming::PhaseScan() {

  banner(Form("PixTestTiming::PhaseScan()"));

  // Setup a new pattern with only res and token:
  vector<pair<string, uint8_t> > pg_setup;
  pg_setup.push_back(make_pair("resetroc", 25));
  pg_setup.push_back(make_pair("trigger", 0));
  fApi->setPatternGenerator(pg_setup);
  uint16_t period = 300;
  vector<rawEvent> daqRawEv;

  int nTBMs = fApi->_dut->getNTbms();
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  vector<int> nROCs;

  for (int itbm = 0; itbm < nTBMs; itbm++) {
    fApi->setTbmReg("basea", 0, itbm); //Reset the ROC delays
    nROCs.push_back(0);
  }
  //Count up the ROC on each TBM Core
  for (size_t iROC=0; iROC<rocIds.size(); iROC++) {
    if (rocIds[iROC]<8) nROCs[0]++;
    else nROCs[1]++;
  }

  bool goodtimings = false;
  fFastScan=true;

  // Start test timer
  timer t;

  fApi->daqStart();
  // Loop through all possible TBM phase settings.
  for (int iclk160 = 0; iclk160 < 8 && !goodtimings; iclk160++) {
    for (int clk400 = 0; clk400 < 4 && !goodtimings; clk400++) {
      int iclk400 = (iclk160+clk400) % 8;
      uint8_t delaysetting = iclk160<<5 | iclk400<<2;
      fApi->setTbmReg("basee", delaysetting, 0); //Set TBM 160-400 MHz Clock Phase
      LOG(logINFO) << "160MHz Phase: " << iclk160 << " 400MHz Phase: " << iclk400 << " Delay Setting: " << bitset<8>(delaysetting).to_string();;
      ClkSdaScan(); //Everytime you adjust the TBM clock phase, the DAQ phase must also be adjusted!
      fApi->daqTrigger(1,period);
      daqRawEv = fApi->daqGetRawEventBuffer();
      //if (daqRawEv.size() < 1) continue;
      vector<int> GoodROCDelays;
      //Loop through each TBM core and count the number of ROC headers on the core for all 256 delay settings
      for (int itbm = 0; itbm < nTBMs; itbm++) {
        for (int ROCDelay = 0; ROCDelay < 256; ROCDelay++) {
          fApi->setTbmReg("basea", ROCDelay, itbm);
          fApi->daqTrigger(1,period);
          daqRawEv = fApi->daqGetRawEventBuffer();
          if (daqRawEv.size() < 1) continue;
          rawEvent event = daqRawEv.at(0);
          LOG(logDEBUG) << "Event: " << event;
          int rocpos = (itbm==0) ? 2:6+nROCs[itbm-1];
          int rocheader_count = 0;
          if (int(event.data.size()) < rocpos) continue;
          for (int idata = rocpos; idata < rocpos+nROCs[itbm]; idata++) if (event.data.at(idata) >> 12 == 4) rocheader_count++; //Count the number of ROCs on TBM Core itbm
          LOG(logDEBUG) << rocheader_count << " ROC headers found for TBM Core " << itbm << ". Looking for " << nROCs[itbm] << ".";
          if (rocheader_count==nROCs[itbm]) {
            GoodROCDelays.push_back(ROCDelay);
            fPixSetup->getConfigParameters()->setTbmDac("basea", ROCDelay, itbm);
            if (int(GoodROCDelays.size())==nTBMs) goodtimings = true;
            break;
          }
        }
      }
      if (goodtimings){
        fPixSetup->getConfigParameters()->setTbmDac("basee", delaysetting, 0);
        LOG(logINFO) << "Good Timings Found!";
        LOG(logINFO) << "TBMPhase basee: " << bitset<8>(delaysetting).to_string();
        for (int itbm = 0; itbm < nTBMs; itbm++) LOG(logINFO) << "ROCPhase TBM Core " << itbm << " basea: " << bitset<8>(GoodROCDelays[itbm]).to_string();
      }
    }
  }
  fApi->daqStop();

  // Reset the pattern generator to the configured default:
  fApi->setPatternGenerator(fPixSetup->getConfigParameters()->getTbPgSettings());

  if (!goodtimings) {
    LOG(logERROR) << "No working TBM-ROC Phase found. Verify you have disabled bypassed ROCs in pXar on the h/w tab.";
    for (int itbm = 0; itbm < nTBMs; itbm++) LOG(logERROR) << "PhaseScan searched for " << nROCs[itbm] << " ROCs on TBM Core " << itbm << ".";
    return;
  }
  // Print timer value:
  LOG(logINFO) << "Test took " << t << "ms.";
  LOG(logINFO) << "PixTestTiming::PhaseScan() done.";
}

//------------------------------------------------------------------------------
void PixTestTiming::DummyTest() {

  banner(Form("PixTestTiming::DummyScan()"));

  // Setup a new pattern with only res and token:
  vector<pair<string, uint8_t> > pg_setup;
  pg_setup.push_back(make_pair("resetroc", 25));
  pg_setup.push_back(make_pair("trigger", 0));
  fApi->setPatternGenerator(pg_setup);
  vector<rawEvent> daqRawEv;
  fApi->daqStart();
  fApi->daqTrigger(1,300);
  daqRawEv = fApi->daqGetRawEventBuffer();
  fApi->daqStop();
  if (daqRawEv.size() > 0) { LOG(logINFO) << "Event: " << daqRawEv.at(0); }

  LOG(logINFO) << "PixTestTiming::DummyScan() done.";
}

//------------------------------------------------------------------------------
std::vector<std::pair<std::string,uint8_t> > PixTestTiming::getDelays(uint8_t clk, uint8_t sda) {
  std::vector<std::pair<std::string,uint8_t> > sigdelays;
  sigdelays.push_back(std::make_pair("clk", clk%20));
  sigdelays.push_back(std::make_pair("ctr", clk%20));
  sigdelays.push_back(std::make_pair("sda", sda%20));
  sigdelays.push_back(std::make_pair("tin", (clk+5)%20));
  return sigdelays;
}

// ----------------------------------------------------------------------
void PixTestTiming::saveTbParameters() {
  LOG(logINFO) << "PixTestTiming:: Write Tb parameters to file.";
  fPixSetup->getConfigParameters()->writeTbParameterFile();
}

// ----------------------------------------------------------------------
void PixTestTiming::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;
  if (!command.compare("clocksdascan")) {
    ClkSdaScan();
    return;
  }
  if (!command.compare("phasescan")) {
    PhaseScan();
    return;
  }
  if (!command.compare("savetbparameters")) {
    saveTbParameters();
    return;
  }
  if (!command.compare("dummytest")) {
    DummyTest();
    return;
  }
  LOG(logDEBUG) << "did not find command ->" << command << "<-";
}
