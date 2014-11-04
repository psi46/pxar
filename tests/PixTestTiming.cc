#include <iostream>
#include <bitset>
#include <stdlib.h>
#include <algorithm>

#include <TStopwatch.h>
#include <TMarker.h>
#include <TStyle.h>
#include <TBox.h>
#include <TMath.h>

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
      if (!parName.compare("ntrig")) {
        fNTrig = atoi(sval.c_str());
        LOG(logDEBUG) << "PixTestTiming::PixTest() ntrig = " << fNTrig;
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

  LevelScan();
  h1 = (*fDisplayedHist);
  h1->Draw(getHistOption(h1).c_str());
  PixTest::update();
 
  // -- save DACs!
  saveParameters();
  TimingTest();
  
  LOG(logINFO) << "PixTestTiming::doTest() done";
}

//------------------------------------------------------------------------------
void PixTestTiming::ClkSdaScan() {


  // Start test timer
  timer t;

  cacheDacs();
  fDirectory->cd();
  PixTest::update();
  banner(Form("PixTestTiming::ClkSdaScan()"));

  //Make a histogram
  TH2D *h1(0);
  h1 = bookTH2D("ClkSdaScan","ClkSdaScan", 20, -0.5, 19.5, 20, -0.5, 19.5);
  h1->SetDirectory(fDirectory);
  setTitles(h1, "Clk", "Sda");
  fHistOptions.insert(make_pair(h1, "colz"));

  //Turn of Vana
  fApi->setDAC("vana", 0);
  pxar::mDelay(2000);
  double IA = fApi->getTBia();

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
      if (TMath::Abs(NewIA-IA) > 0.005) {
        h1->Fill(iclk,isda); //Need 5mA change in current to see if ROCs are programmable
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

  //Overly complicated algorithm to figure out the best SDA.
  //Normally there are 7 sda settings that work, and this selects the middle one.
  //It's completcated because the working SDA settings can be 0, 1, 2, 3, 4, 18, and 19. The center most value is 1.
  if (GoodClk != -1) {
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
            for (size_t isda=1; isda<goodsdalist.size(); isda++) if (TMath::Abs(goodsdalist[isda]-goodsdalist[isda-1])>1) goodsdalist[isda] -= 20;
            sort(goodsdalist.begin(),goodsdalist.end());
            GoodSDA=goodsdalist[goodsdalist.size()/2];
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
  } else LOG(logERROR) << "No working SDA setting found!";

  //Draw the plot
  h1->Draw("colz");
  fHistList.push_back(h1);
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);
  PixTest::update();
  restoreDacs();

  // Print timer value:
  LOG(logINFO) << "Test took " << t << " ms.";
  LOG(logINFO) << "PixTestTiming::ClkSdaScan() done.";
}

//------------------------------------------------------------------------------
void PixTestTiming::PhaseScan() {

  // Start test timer
  timer t;

  banner(Form("PixTestTiming::PhaseScan()"));
  fDirectory->cd();
  PixTest::update();

  //Make histograms
  TH2D *h1(0);
  TH2D *h2(0);

  //Set Number of Port (Probably won't work for TBM09 and is untested)
  int nPorts = 2;
  if (fPixSetup->getConfigParameters()->getTbmType() == "tbm09") nPorts=1;

  // Setup a new pattern with only res and token:
  vector<pair<string, uint8_t> > pg_setup;
  //pg_setup.push_back(make_pair("resetroc", 25));
  pg_setup.push_back(make_pair("resettbm", 25));
  pg_setup.push_back(make_pair("trigger", 0));
  fApi->setPatternGenerator(pg_setup);
  fTrigBuffer = 3;
  uint16_t period = 300;
  vector<rawEvent> daqRawEv;

  //Get the number of TBMs, Total ROCs, and ROCs per TBM
  int nTBMs = fApi->_dut->getNTbms();
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  vector<int> nROCs;
  vector <vector<int> > nROCsPort;
  vector<TH2D*> phasehists;
  vector<map <int, int> > TBMROCPhases;
  for (int itbm = 0; itbm < nTBMs; itbm++) {
    fApi->setTbmReg("basea", 0, itbm); //Reset the ROC delays
    nROCs.push_back(0);
    vector<int> InitnROCPorts;
    InitnROCPorts.assign(nPorts, 0);
    nROCsPort.push_back(InitnROCPorts);
    map<int, int> TBMROCPhaseMap;
    TBMROCPhases.push_back(TBMROCPhaseMap);
    h1 = bookTH2D(Form("TBMPhaseScan%d",itbm),Form("TBM %d Phase Scan",itbm), 8, -0.5, 7.5, 8, -0.5, 7.5);
    h1->SetDirectory(fDirectory);
    setTitles(h1, "160MHz Phase", "400 MHz Phase");
    fHistOptions.insert(make_pair(h1, "colz"));
    h1->SetMinimum(0);
    phasehists.push_back(h1);
  }

  //Count up the ROCs on each TBM Core
  //8, 4 and 12 are all magic numbers! Number of expected ROCs per TBM, number of ROCs per port, and number of ports should be in the dut or ConfigParameters!
  for (size_t iROC=0; iROC<rocIds.size(); iROC++) {
    if (rocIds[iROC]<8) {
      nROCs[0]++;
      if (rocIds[iROC]<4) nROCsPort[0].at(0)++;
      else nROCsPort[0].at(1)++;
    } else {
      nROCs[1]++;
      if (rocIds[iROC]<12) nROCsPort[1].at(0)++;
      else nROCsPort[1].at(1)++;
    }
  }

  // Loop through all possible TBM phase settings.
  bool goodTBMPhase = false;
  for (int iclk160 = 0; iclk160 < 8 && !(goodTBMPhase && fFastScan); iclk160++) {
    for (int iclk400 = 0; iclk400 < 8 && !(goodTBMPhase && fFastScan); iclk400++) {
      uint8_t delaysetting = iclk160<<5 | iclk400<<2;
      fApi->setTbmReg("basee", delaysetting, 0); //Set TBM 160-400 MHz Clock Phase
      LOG(logINFO) << "160MHz Phase: " << iclk160 << " 400MHz Phase: " << iclk400 << " Delay Setting: " << bitset<8>(delaysetting).to_string();
      fApi->daqStart();
      fApi->daqTrigger(fTrigBuffer,period); //Read in fTrigBuffer events and throw them away, first event is generally bad.
      daqRawEv = fApi->daqGetRawEventBuffer();
      for (int itbm = 0; itbm < nTBMs; itbm++) fApi->setTbmReg("basea", 0, itbm); //Reset the ROC delays
      //Loop through each TBM core and count the number of ROC headers on the core for all 256 delay settings
      vector<int> GoodROCDelays;
      for (int itbm = 0; itbm < nTBMs; itbm++) {
        int MaxGoodROCSize=0;
        bool goodROCDelay = false;
        for (int delaytht = 0; delaytht < 4 && !goodROCDelay; delaytht++) {
          if (delaytht==2) continue;
          h2 = bookTH2D(Form("ROCPhaseScan_clk160_%d_clk400_%d_TBM_%d_delay_%d", iclk160, iclk400, itbm, delaytht),
                        Form("ROC Phase Scan: TBM %d Phase: %s THT Delay: %s", itbm, bitset<8>(delaysetting).to_string().c_str(), bitset<2>(delaytht).to_string().c_str()),
                        8, -0.5, 7.5, 8, -0.5, 7.5);
          setTitles(h2, "ROC Port 0", "ROC Port 1");
          h2->SetDirectory(fDirectory);
          h2->SetMinimum(0);
          h2->SetMaximum(nROCs[itbm]);
          fHistOptions.insert(make_pair(h2, "colz"));
          for (int iport = 0; iport < nPorts; iport++) {
            for (int idelay = 0; idelay < 8; idelay++) {
              int ROCDelay = (iport==0) ? (delaytht << 6) | idelay : (delaytht << 6) | (idelay << 3);
              LOG(logDEBUG) << "Testing ROC Delay: " << bitset<8>(ROCDelay).to_string() << " For TBM Core: " << itbm;
              fApi->setTbmReg("basea", ROCDelay, itbm);
              fApi->daqTrigger(fNTrig,period);
              daqRawEv = fApi->daqGetRawEventBuffer();
              LOG(logDEBUG) << "Events in Data Buffer: " << daqRawEv.size();
              if (int(daqRawEv.size()) < fNTrig) continue; //Grab fNTrig triggers
              for (int ievent = 0; ievent<fNTrig; ievent++) {
                rawEvent event = daqRawEv.at(ievent);
                LOG(logDEBUG) << "Event: " << event;
                vector<int> tbmheaders;
                vector<int> tbmtrailers;
                int header=0;
                for (int idata = 0; idata < int(event.data.size()); idata++) {
                  if (event.data.at(idata) >> 8 == 160) tbmheaders.push_back(idata); //Look for TBM Header a0
                  if (event.data.at(idata) >> 8 == 192) tbmtrailers.push_back(idata); //Look for TBM Trailer c0, maybe should be c000
                  if (header==0 && int(tbmheaders.size())==itbm+1 && int(tbmtrailers.size())==itbm && event.data.at(idata) >> 12 == 4) header = event.data.at(idata); //Grab the first object that looks like a header between the correct header and trailer
                }
                if (int(tbmheaders.size()) != nTBMs || int(tbmtrailers.size()) != nTBMs || header==0) continue; //Skip event if the correct number of TBM headers and trailer are not present or if a ROC header could not be found.
                int rocheader_count = 0;
                int StartData = tbmheaders[itbm]+2;
                int StopData = tbmtrailers[itbm]-2;
                for (int jport=0; jport<nPorts; jport++) {
                  if (jport < iport) StartData += nROCsPort[itbm][jport];
                  if (jport > iport) StopData -= nROCsPort[itbm][jport];
                }
                for (int idata = StartData; idata <= StopData; idata++) if (event.data.at(idata) >> 2  == header >> 2) rocheader_count++; //Count the number of ROCs on each TBM Core for each port
                LOG(logDEBUG) << rocheader_count << " ROC headers (" << hex << (header) << dec << ") found for TBM Core " << itbm << " for Port " << iport << ". Looking for " << nROCsPort[itbm][iport] << ".";
                for (int ibin=0; ibin<8; ibin++) {
                  if (iport==0) h2->Fill(idelay, ibin, rocheader_count);
                  if (iport==1) h2->Fill(ibin, idelay, rocheader_count);
                }
              }
            }
          }
          pair <int, int> GoodRegion = getGoodRegion(h2, nROCs[itbm]*fNTrig);
          LOG(logDEBUG) << h2->GetTitle() << " - Size: " << GoodRegion.first << " Delay: " << GoodRegion.second;
          h2->Scale(1/float(fNTrig));
          if (GoodRegion.first > MaxGoodROCSize) {
            MaxGoodROCSize = GoodRegion.first;
            int ROCDelay = (delaytht << 6) | GoodRegion.second;
            if (int(GoodROCDelays.size()) < itbm+1) GoodROCDelays.push_back(ROCDelay);
            else GoodROCDelays[itbm] = ROCDelay;
            if (fFastScan && GoodRegion.first > 0) {
              goodROCDelay = true;
              fPixSetup->getConfigParameters()->setTbmDac("basea", ROCDelay, itbm);
            }
          }
          //Draw the plot
          if (GoodRegion.first) {
            h2->Draw(getHistOption(h2).c_str());
            fHistList.push_back(h2);
            fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);
            PixTest::update();
          }
        }
        phasehists[itbm]->Fill(iclk160, iclk400, MaxGoodROCSize);
        if (int(GoodROCDelays.size())==itbm+1) { // Use the good ROC delays, or reset the ROCs to 0
          TBMROCPhases[itbm][delaysetting] = GoodROCDelays[itbm];
          fApi->setTbmReg("basea", GoodROCDelays[itbm], itbm);
          fPixSetup->getConfigParameters()->setTbmDac("basea", GoodROCDelays[itbm], itbm);
        } else fApi->setTbmReg("basea", 0, itbm);
      }
      fApi->daqStop();
      if (int(GoodROCDelays.size())==nTBMs){
        goodTBMPhase = true;
        if (fFastScan) fPixSetup->getConfigParameters()->setTbmDac("basee", delaysetting, 0);
        LOG(logINFO) << "Good timings found for TBMPhase basee: " << bitset<8>(delaysetting).to_string();
        for (int itbm = 0; itbm < nTBMs; itbm++) LOG(logINFO) << "ROCPhase TBM Core " << itbm << " basea: " << bitset<8>(GoodROCDelays[itbm]).to_string();
      }
    }
  }

  banner(Form("PixTestTiming::Phase Scan Completed"));

  if (!fFastScan) {
    int MaxBinSum = 0;
    pair<int, int> BestBin(0,0);
    for (int xbin=1; xbin<=phasehists[0]->GetNbinsX(); xbin++) {
      for (int ybin=1; ybin<=phasehists[0]->GetNbinsY(); ybin++) {
        double BinSum = 0;
        for (int itbm=0; itbm<nTBMs; itbm++) BinSum += phasehists[itbm]->GetBinContent(xbin,ybin);
        if (BinSum > MaxBinSum) {
          MaxBinSum = BinSum;
          BestBin = make_pair(xbin-1, ybin-1);
        }
      }
    }
    int delaysetting = BestBin.first<<5 | BestBin.second<<2;
    fApi->setTbmReg("basee", delaysetting, 0);
    fPixSetup->getConfigParameters()->setTbmDac("basee", delaysetting, 0);
    LOG(logINFO) << "Final timing for TBMPhase basee: " << bitset<8>(delaysetting).to_string();
    for (int itbm = 0; itbm < nTBMs; itbm++) {
      fApi->setTbmReg("basea", TBMROCPhases[itbm][delaysetting], itbm);
      fPixSetup->getConfigParameters()->setTbmDac("basea", TBMROCPhases[itbm][delaysetting], itbm);
      LOG(logINFO) << "ROCPhase TBM Core " << itbm << " basea: " << bitset<8>(TBMROCPhases[itbm][delaysetting]).to_string();
    }
  }

  // Reset the pattern generator to the configured default:
  fApi->setPatternGenerator(fPixSetup->getConfigParameters()->getTbPgSettings());

  if (!goodTBMPhase) {
    LOG(logERROR) << "No working TBM-ROC Phase found. Verify you have disabled bypassed ROCs in pXar on the h/w tab.";
    for (int itbm = 0; itbm < nTBMs; itbm++) LOG(logERROR) << "PhaseScan searched for " << nROCs[itbm] << " ROCs on TBM Core " << itbm << ".";
  }

  //Draw TBM Phase Map
  for (int itbm = 0; itbm < nTBMs && !fFastScan; itbm++) {
    phasehists[itbm]->Draw(getHistOption(phasehists[itbm]).c_str());
    fHistList.push_back(phasehists[itbm]);
    fDisplayedHist = find(fHistList.begin(), fHistList.end(), phasehists[itbm]);
    PixTest::update();
  }

  // Print timer value:
  LOG(logINFO) << "Test took " << t << " ms.";
  LOG(logINFO) << "PixTestTiming::PhaseScan() done.";
}

//------------------------------------------------------------------------------
void PixTestTiming::TimingTest() {

  // Start test timer
  timer t;

  banner(Form("PixTestTiming::TimingTest()"));

  size_t nTBMs = fApi->_dut->getNTbms();
  size_t nROCs = fApi->_dut->getEnabledRocIDs().size();

  // Setup a new pattern with only res and token:
  vector<pair<string, uint8_t> > pg_setup;
  pg_setup.push_back(make_pair("resetroc", 25));
  pg_setup.push_back(make_pair("trigger", 0));
  fApi->setPatternGenerator(pg_setup);
  fTrigBuffer = 3;
  uint16_t period = 300;
  vector<rawEvent> daqRawEv;
  fApi->daqStart();
  fApi->daqTrigger(fTrigBuffer,period); //Read in fTrigBuffer events and throw them away, first event is generally bad.
  daqRawEv = fApi->daqGetRawEventBuffer();
  fApi->daqTrigger(fNTrig,period);
  daqRawEv = fApi->daqGetRawEventBuffer();
  fApi->daqStop();
  LOG(logINFO) << daqRawEv.size() << " events found. " << fNTrig << " events expected.";
  int ngoodevents = 0;
  for (size_t ievent=0; ievent<daqRawEv.size(); ievent++) {
    banner(Form("Decoding Event Number %d", int(ievent)),logDEBUG);
    rawEvent event = daqRawEv.at(ievent);
    LOG(logDEBUG) << "Event: " << event;
    vector<int> tbmheaders;
    vector<int> tbmtrailers;
    vector<int> rocheaders;
    for (int idata=0; idata < int(event.data.size()); idata++) {
      if (event.data.at(idata) >> 8 == 160) tbmheaders.push_back(idata);
      if (event.data.at(idata) >> 8 == 192) tbmtrailers.push_back(idata); //192 is c0, maybe it shoud be 49152 (c0000)
      if (event.data.at(idata) >> 12 == 4) rocheaders.push_back(idata);
    }
    LOG(logDEBUG) << tbmheaders.size() << " TBM Headers found. " << nTBMs << " TBM Headers expected.";
    LOG(logDEBUG) << tbmtrailers.size() << " TBM Trailers found. " << nTBMs << " TBM Trailers expected.";
    LOG(logDEBUG) << rocheaders.size() << " ROC Headers found. " << nROCs << " ROC Headers expected." << endl;
    if (tbmheaders.size()==nTBMs && tbmtrailers.size()==nTBMs && rocheaders.size()==nROCs) ngoodevents++; //Number of ROC Headers and TBM Headers and Trailer must be correct
  }
  fApi->setPatternGenerator(fPixSetup->getConfigParameters()->getTbPgSettings());
  LOG(logINFO) << Form("The fraction of properly decoded events is %4.2f%%: ", float(ngoodevents)/fNTrig*100) << ngoodevents << "/" << fNTrig;
  LOG(logINFO) << "Test took " << t << " ms.";
  LOG(logINFO) << "PixTestTiming::TimingTest() done.";

}

//------------------------------------------------------------------------------
void PixTestTiming::LevelScan() {

  // Start test timer
  timer t;

  fDirectory->cd();
  PixTest::update();
  banner(Form("PixTestTiming::LevelScan()"));

  //The Buffer and Period
  fTrigBuffer = 3;
  uint16_t period = 300;

  //Make a histogram
  TH1D *h1(0);
  h1 = bookTH1D("LevelScan","Level Scan", 16, -0.5, 15.5);
  h1->SetDirectory(fDirectory);
  setTitles(h1, "DTB Level", "Good Events");

  //Get the normal info
  size_t nTBMs = fApi->_dut->getNTbms();
  size_t nROCs = fApi->_dut->getEnabledRocIDs().size();

  // Setup a new pattern with only res and token:
  vector<pair<string, uint8_t> > pg_setup;
  pg_setup.push_back(make_pair("resetroc", 25));
  pg_setup.push_back(make_pair("trigger", 0));
  fApi->setPatternGenerator(pg_setup);

  vector<uint8_t> GoodLevels;
  fApi->daqStart();
  fApi->daqTrigger(fTrigBuffer,period); //Read in fTrigBuffer events and throw them away, first event is generally bad.
  vector<rawEvent> daqRawEv;
  daqRawEv = fApi->daqGetRawEventBuffer();
  for (uint8_t ilevel=0; ilevel<16; ilevel++){
    LOG(logDEBUG) << "Testing Level: " << int(ilevel);
    fPixSetup->getConfigParameters()->setTbParameter("level", ilevel);
    fApi->setTestboardDelays(fPixSetup->getConfigParameters()->getTbParameters());
    fApi->daqTrigger(fNTrig,period);
    daqRawEv = fApi->daqGetRawEventBuffer();
    int ngoodevents = 0;
    for (size_t ievent=0; ievent<daqRawEv.size(); ievent++) {
      rawEvent event = daqRawEv.at(ievent);
      LOG(logDEBUG) << "Event: " << event;
      vector<int> tbmheaders;
      vector<int> tbmtrailers;
      vector<int> rocheaders;
      for (int idata=0; idata < int(event.data.size()); idata++) {
        if (event.data.at(idata) >> 8 == 160) tbmheaders.push_back(idata);
        if (event.data.at(idata) >> 8 == 192) tbmtrailers.push_back(idata); //192 is c0, maybe it shoud be 49152 (c0000)
        if (event.data.at(idata) >> 12 == 4) rocheaders.push_back(idata);
      }
      if (tbmheaders.size()==nTBMs && tbmtrailers.size()==nTBMs && rocheaders.size()==nROCs) ngoodevents++; //Number of ROC Headers and TBM Headers and Trailer must be correct
    }
    if (ngoodevents) h1->Fill(int(ilevel), ngoodevents);
    if (ngoodevents==fNTrig) GoodLevels.push_back(ilevel);
  }
  fApi->daqStop();

  if (GoodLevels.size()) {
    uint8_t MeanLevel = 0;
    if (GoodLevels.size()==1) MeanLevel = GoodLevels.front();
    else MeanLevel = GoodLevels[GoodLevels.size()/2]; //Pick the median functional level (hope there's no gaps)
    fPixSetup->getConfigParameters()->setTbParameter("level", MeanLevel);
    LOG(logINFO) << "DTB Level set to " << int(MeanLevel);
  } else {
    LOG(logERROR) << "No working level found! Verify you have disabled bypassed ROCs in pXar on the h/w tab.";
    LOG(logERROR) << "Level scan searched for " << nROCs << " total ROCs and " << nTBMs << " TBM Headers and Trailers.";
  }

  //Draw the plot
  h1->Draw();
  fHistList.push_back(h1);
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);
  PixTest::update();

  fApi->setPatternGenerator(fPixSetup->getConfigParameters()->getTbPgSettings());
  LOG(logINFO) << "Test took " << t << " ms.";
  LOG(logINFO) << "PixTestTiming::LevelScan() done.";

}

//------------------------------------------------------------------------------
vector<pair<string,uint8_t> > PixTestTiming::getDelays(uint8_t clk, uint8_t sda) {
  vector<pair<string,uint8_t> > sigdelays;
  sigdelays.push_back(make_pair("clk", clk%20));
  sigdelays.push_back(make_pair("ctr", clk%20));
  sigdelays.push_back(make_pair("sda", sda%20));
  sigdelays.push_back(make_pair("tin", (clk+5)%20));
  return sigdelays;
}

// ----------------------------------------------------------------------
pair <int, int> PixTestTiming::getGoodRegion(TH2D* hist, int hits) {

  if (hist->GetEntries()==0) return make_pair(0,0);

  int MaxGoodRegionSize=0;
  int GoodROCDelay=0;
  for (int startbinx=1; startbinx<=hist->GetNbinsX(); startbinx++) {
    for (int startbiny=1; startbiny<=hist->GetNbinsY(); startbiny++) {
      if (int(hist->GetBinContent(startbinx,startbiny))!=hits) continue;
      for (int regionsize=0; regionsize<8; regionsize++) {
        bool regiongood = true;
        for (int xoffset=0; xoffset<=regionsize && regiongood; xoffset++) {
          for (int yoffset=0; yoffset<=regionsize && regiongood; yoffset++) {
            int checkbinx = (startbinx+xoffset>8) ? startbinx+xoffset-8 : startbinx+xoffset;
            int checkbiny = (startbiny+yoffset>8) ? startbiny+yoffset-8 : startbiny+yoffset;
            if (int(hist->GetBinContent(checkbinx,checkbiny))!=hits) regiongood=false;
          }
        }
        if (regiongood && regionsize+1>MaxGoodRegionSize) {
          MaxGoodRegionSize=regionsize+1;
          GoodROCDelay = (startbinx-1+regionsize/2)%8 | (startbiny-1+regionsize/2)%8<<3;
        }
      }
    }
  }

  return make_pair(MaxGoodRegionSize, GoodROCDelay);

}

// ----------------------------------------------------------------------
void PixTestTiming::saveParameters() {
  LOG(logINFO) << "PixTestTiming:: Write Tbm parameters to file.";
  fPixSetup->getConfigParameters()->writeTbParameterFile();
  for (unsigned int itbm = 0; itbm < fApi->_dut->getNTbms(); itbm += 2) {
    fPixSetup->getConfigParameters()->writeTbmParameterFile(itbm, fApi->_dut->getTbmDACs(itbm), fApi->_dut->getTbmDACs(itbm+1));
  }
}

// ----------------------------------------------------------------------
void PixTestTiming::runCommand(string command) {
  transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;
  if (!command.compare("clocksdascan")) {
    ClkSdaScan();
    return;
  }
  if (!command.compare("phasescan")) {
    PhaseScan();
    return;
  }
  if (!command.compare("saveparameters")) {
    saveParameters();
    return;
  }
  if (!command.compare("timingtest")) {
    TimingTest();
    return;
  }
  if (!command.compare("levelscan")) {
    LevelScan();
    return;
  }
  LOG(logDEBUG) << "did not find command ->" << command << "<-";
}
