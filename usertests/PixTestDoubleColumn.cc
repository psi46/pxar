#include <stdlib.h>     
#include <algorithm>    
#include <iostream>
#include <fstream>
#include <numeric>

#include <TH1.h>
#include <TRandom.h>
#include <TStopwatch.h>
#include <TStyle.h>

#include "PixTestDoubleColumn.hh"
#include "PixUtil.hh"
#include "log.h"
#include "helper.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestDoubleColumn)

/*

-- DoubleColumn
min                 0
max                 40
npix                4
delay               65
data                button
*/

// ----------------------------------------------------------------------
PixTestDoubleColumn::PixTestDoubleColumn(PixSetup *a, std::string name) : PixTest(a, name), fParNtrig(1), fParNpix(4), fParDelay(65),fParTsMin(0),fParTsMax(40),fParDaqDatRead(false),fParRowOffset(10) {
  PixTest::init();
  init(); 
  //  LOG(logINFO) << "PixTestDoubleColumn ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestDoubleColumn::PixTestDoubleColumn() : PixTest() {
  //  LOG(logINFO) << "PixTestDoubleColumn ctor()";
}

// ----------------------------------------------------------------------
bool PixTestDoubleColumn::setParameter(string parName, string sval) {
  bool found(false);
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      if (!parName.compare("ntrig")) {
  fParNtrig = atoi(sval.c_str()); 
      }
      if (!parName.compare("min")) {
  fParTsMin = atoi(sval.c_str()); 
      }
      if (!parName.compare("max")) {
  fParTsMax = atoi(sval.c_str()); 
      }
      if (!parName.compare("npix")) {
  fParNpix = atoi(sval.c_str()); 
  if (fParNpix > 4) {
    fParNpix = 4;
  }
      }
      if (!parName.compare("delay")) {
  fParDelay = atoi(sval.c_str()); 
      }
      break;
    }
  }
  
  return found; 
}


// ----------------------------------------------------------------------
void PixTestDoubleColumn::init() {
  setToolTips(); 
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}


// ----------------------------------------------------------------------
void PixTestDoubleColumn::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;
  if (!command.compare("data")) {
    testData();
    return;
  }

  LOG(logDEBUG) << "did not find command ->" << command << "<-";
}


// ----------------------------------------------------------------------
void PixTestDoubleColumn::setToolTips() {
  fTestTip    = string("")
    ;
  fSummaryTip = string("summary plot to be implemented")
    ;
}

// ----------------------------------------------------------------------
void PixTestDoubleColumn::bookHist(string name) {
  fDirectory->cd(); 

  LOG(logDEBUG) << "nothing done with " << name;
}


//----------------------------------------------------------
PixTestDoubleColumn::~PixTestDoubleColumn() {
  LOG(logDEBUG) << "PixTestDoubleColumn dtor";
}


void PixTestDoubleColumn::resetDaq() {
fParDaqDatRead = false;
}

std::vector< std::vector<int> > PixTestDoubleColumn::readPulseheights(int nEv) {

  if (!fParDaqDatRead) {
    try { 
      fParDaqDat = fApi->daqGetEventBuffer(); 
      fParDaqDatRead = true;
    }
    catch(pxar::DataNoEvent &) {}
  }
  
  int nRocs = fApi->_dut->getNEnabledRocs();
  std::vector<int> doubleColumnHitsRoc(52,0);
  std::vector< std::vector<int> > doubleColumnHits(nRocs, doubleColumnHitsRoc);
  std::vector< std::vector<int> > doubleColumnPH(nRocs, doubleColumnHitsRoc);

  //for (std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
  if (nEv < fParDaqDat.size()) {
    pxar::Event it = fParDaqDat.at(nEv);
    int idx(0); 
    int meanPH = 0;
    int doubleColumn = -1;
    for (unsigned int ipix = 0; ipix < it.pixels.size(); ++ipix) {   
      idx = getIdxFromId(it.pixels[ipix].roc());
      doubleColumn = it.pixels[ipix].column() / 2;
      if (it.pixels[ipix].value() > 0) {
        doubleColumnHits[idx][doubleColumn]++;
        doubleColumnPH[idx][doubleColumn] += it.pixels[ipix].value();
      }
    }
  }

  for (int idx=0;idx<nRocs;idx++) {
    for (int doubleColumn=0;doubleColumn<26;doubleColumn++) {
      if (doubleColumnHits[idx][doubleColumn] > 0) {
        doubleColumnPH[idx][doubleColumn] /= doubleColumnHits[idx][doubleColumn];
      } else {
        doubleColumnPH[idx][doubleColumn] = 0;
      }
    }
  }
  //}
  return doubleColumnPH;
}

std::vector< std::vector<int> > PixTestDoubleColumn::readData(int nEv) {

  
  int nRocs = fApi->_dut->getNEnabledRocs();
  std::vector<int> doubleColumnHitsRoc(52,0);
  std::vector< std::vector<int> > doubleColumnHits(nRocs, doubleColumnHitsRoc);

  //for (std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
  if (nEv < fParDaqDat.size()) {
    pxar::Event it = fParDaqDat.at(nEv);
    int idx(0); 
    for (unsigned int ipix = 0; ipix < it.pixels.size(); ++ipix) {   
      idx = getIdxFromId(it.pixels[ipix].roc());
      int doubleColumn = it.pixels[ipix].column() / 2;
      //LOG(logINFO) << "col " << (int)it.pixels[ipix].column();
      doubleColumnHits[idx][doubleColumn]++;
    }
  }
  //}
  return doubleColumnHits;
}


void PixTestDoubleColumn::testBuffers(std::vector<TH2D*> hX, std::vector<TH2D*> hXPH, int tsMin, int tsMax) {

  int nRocs = fApi->_dut->getNEnabledRocs();
  bool unmaskChip = false;

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  // get calibrate signal delay
  uint8_t wbc = 100;
  uint8_t delay = 6;
  vector<pair<string, uint8_t> > pgtmp = fPixSetup->getConfigParameters()->getTbPgSettings();
  int calibrateDelayTotal = wbc + delay; //default
  for (unsigned i = 0; i < pgtmp.size(); ++i) {
    if (string::npos != pgtmp[i].first.find("calibrate")) {
      calibrateDelayTotal = pgtmp[i].second;
    };
  }

  //fApi->setDAC("wbc", wbc);
  resetROC();

  int tsStepSize = 6; // number of timestamps to test at once
  for (int tsOffset=tsMin;tsOffset<tsMax+1;tsOffset+=tsStepSize) {
    LOG(logINFO) << "timestamps: " << (int)tsOffset;

    fPg_setup.clear();
    resetROC();

    int timestampsTestedPerLoop = 0;
    for (int ts=0;ts<tsStepSize;ts++) {
      int nTestTimestamps = tsOffset + ts;

      if (nTestTimestamps <= tsMax) {
        // -- pattern generator setup
        fPg_setup.push_back(std::make_pair("resetroc", 50));    // PG_REST
        for (int i=0;i<nTestTimestamps;i++) {
        fPg_setup.push_back(std::make_pair("calibrate", fParDelay)); // PG_CAL
        }
        fPg_setup.push_back(std::make_pair("calibrate", calibrateDelayTotal)); // PG_CAL

        fPg_setup.push_back(std::make_pair("trigger;sync", 250));   
        if (nTestTimestamps < tsMax) {
          fPg_setup.push_back(std::make_pair("delay", 250)); 
        } 
        timestampsTestedPerLoop++;
      }

    }
    fPg_setup.push_back(std::make_pair("delay", 0)); 

    int period = 22000;
    LOG(logDEBUG) << "set pattern generator to:";
    for (unsigned int i = 0; i < fPg_setup.size(); ++i) LOG(logDEBUG) << fPg_setup[i].first << ": " << (int)fPg_setup[i].second;

    fApi->setPatternGenerator(fPg_setup);

    // check if enough good rows are available
    if (fGoodRows.size() < fParNpix) {
      LOG(logINFO) << "not enough good rows found!!! accepting rows with dead pixels now";
      fGoodRows.clear();
      for (int j=0;j<fParNpix;j++) {
        fGoodRows.push_back(j);
      }
    }

    fApi->_dut->maskAllPixels(false);
    fApi->daqStart();

    // test all columns
    for (int iCol=0;iCol<52;iCol++) {
      //LOG(logINFO) << "testing column " << (int)iCol;

      // enable pixels in the column
      for (int j=0;j<fParNpix;j++) {
        fApi->_dut->testPixel(iCol,fGoodRows[j],true);
        //LOG(logINFO) << "testing pix " << (int)fGoodRows[j];
      }
     
      fApi->SetCalibrateBits(true);
      fApi->daqTrigger(1, period);
      fApi->SetCalibrateBits(false);

      // disable pixels in the column
      for (int j=0;j<fParNpix;j++) {
        fApi->_dut->testPixel(iCol,fGoodRows[j],false);
      }

    }

    fApi->_dut->maskAllPixels(true);
    fApi->daqStop();
    std::vector< std::vector<int> > doubleColumnHits;
    std::vector< std::vector<int> > doubleColumnPH;

    fParDaqDatRead = false;
    try { 
      fParDaqDat = fApi->daqGetEventBuffer(); 
      fParDaqDatRead = true;
    }
    catch(pxar::DataNoEvent &) {}


    // fill histograms
    if (fParDaqDatRead) {

      for (int ts=0;ts<timestampsTestedPerLoop;ts++) {

        for (int column=0;column<52;column++) {
          int eventId = column * timestampsTestedPerLoop + ts;
          pxar::Event it;
          try { 
            it = fParDaqDat.at(eventId);
          }
          catch(...) {
            LOG(logINFO) << "no DATA!";
          }
          
          std::vector< int> goodHits(nRocs,0);

          //LOG(logINFO) << " column " << column << ": " << it.pixels.size();
          int pixSize = it.pixels.size();
          for (unsigned int ipix = 0; ipix < pixSize; ++ipix) {
            //LOG(logINFO) << "ipix " << ipix;
            bool rowGood = false;
            bool columnGood = true;
            if (it.pixels[ipix].column() != column) {
              columnGood = false;
             // LOG(logINFO) << "wrong column " << (int)it.pixels[ipix].column() << " instead of " << column;
            }
            for (int j=0;j<fParNpix;j++) {
              if (it.pixels[ipix].row() == fGoodRows[j]) {
                rowGood = true;
              }
            }

            if (rowGood && columnGood) {
              goodHits[getIdxFromId(it.pixels.at(ipix).roc())]++;
            }
          }
          for (int iRocIdx=0;iRocIdx<nRocs;iRocIdx++) {
            int nTestTimestamps = tsOffset + ts;
            hX[iRocIdx]->Fill(column+0.1, nTestTimestamps+0.1, goodHits[iRocIdx]);
          }
        }
      }
      
    }

  }


}

void PixTestDoubleColumn::testData() {
  cacheDacs();

  std::vector<TH2D*> hX;
  std::vector<TH2D*> hXPH;

  int nRocs = fApi->_dut->getNEnabledRocs();

  PixTest::update(); 
  for (int iRocIdx=0;iRocIdx<nRocs;iRocIdx++) {
    TH2D* rocHist  = bookTH2D(Form("npix%d_C%d", fParNpix, getIdFromIdx(iRocIdx)),  Form("npix%d_C%d", fParNpix, getIdFromIdx(iRocIdx)),  52, 0, 52.0, fParTsMax-fParTsMin+1, fParTsMin, fParTsMax+1);
    TH2D* rocHistPH  = bookTH2D(Form("npix%d_PH_C%d", fParNpix, getIdFromIdx(iRocIdx)),  Form("npix%d_PH_C%d", fParNpix, getIdFromIdx(iRocIdx)),  52, 0, 52.0, fParTsMax-fParTsMin+1, fParTsMin, fParTsMax+1);
    hX.push_back(rocHist);    
    hXPH.push_back(rocHistPH);
  }

  testBuffers(hX, hXPH, fParTsMin, fParTsMax);

  //for (int iRocIdx=0;iRocIdx<nRocs;iRocIdx++) {
  //  fHistList.push_back(hXPH[iRocIdx]);
  //  fHistOptions.insert( make_pair(hXPH[iRocIdx], "colz")  ); 
  //}

  for (int iRocIdx=0;iRocIdx<nRocs;iRocIdx++) {
    fHistList.push_back(hX[iRocIdx]);
    fHistOptions.insert( make_pair(hX[iRocIdx], "colz")  ); 
  }

  PixTest::update(); 
  fDisplayedHist = find( fHistList.begin(), fHistList.end(), hX[0] );
  (*fDisplayedHist)->Draw("colz");

  fPg_setup.clear();
  LOG(logDEBUG) << "PixTestDoubleColumn::PG_Setup clean";
  fPg_setup = fPixSetup->getConfigParameters()->getTbPgSettings();
  fApi->setPatternGenerator(fPg_setup);

  restoreDacs();
  PixTest::update(); 
  dutCalibrateOff();

}

void PixTestDoubleColumn::findWorkingRows() {

  cacheDacs();
  fDirectory->cd();
  PixTest::update(); 

  fApi->setDAC("vcal", 250);

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  maskPixels();

  fNDaqErrors = 0; 
  int nTrig=10;
  vector<TH2D*> test2 = efficiencyMaps("PixelAlive", nTrig, FLAG_FORCE_MASKED); 
  vector<int> deadPixel(test2.size(), 0); 
  vector<int> probPixel(test2.size(), 0); 

  bool rowOk = false;
  fGoodRows.clear();

  int rowOrdering[80] = {63, 0, 61, 72, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 65, 66, 67, 68, 69, 70, 71, 73, 74, 75, 76, 77, 78, 79};

  for (int iRowIndex=0;iRowIndex<79;iRowIndex++) {
    int iRow = rowOrdering[iRowIndex];
    rowOk = true;
    for (unsigned int i = 0; i < test2.size(); ++i) {
      // -- count dead pixels
      for (int ix = 0; ix < test2[i]->GetNbinsX(); ++ix) {
        for (int iy = iRow; iy < iRow+2; ++iy) {
          if (test2[i]->GetBinContent(ix+1, iy+1) < nTrig) {
            rowOk = false;
          }
        }
      }
    }
    if (rowOk) {
      fGoodRows.push_back(iRow);
    } else {
      LOG(logINFO) << "can't use row " << iRow << " because of inefficienct pixels, test next row...";
      iRow++;
    }
  }

  if (fGoodRows.size() > 0) {
    LOG(logINFO) << "use row" << fGoodRows[0] << "...";
  }

  restoreDacs(); 
}

// ----------------------------------------------------------------------
void PixTestDoubleColumn::doTest() {

  TStopwatch t;

  fDirectory->cd();
  PixTest::update(); 
  bigBanner(Form("PixTestDoubleColumn::doTest()"));

  findWorkingRows();
  testData();

  int seconds = t.RealTime(); 
  LOG(logINFO) << "PixTestDoubleColumn::doTest() done, duration: " << seconds << " seconds";
}