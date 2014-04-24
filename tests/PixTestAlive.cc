#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include "PixUtil.hh"
#include "PixTestAlive.hh"
#include "log.h"

#include <TH2.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestAlive)

// ----------------------------------------------------------------------
PixTestAlive::PixTestAlive(PixSetup *a, std::string name) : PixTest(a, name), fParNtrig(0), fParVcal(-1) {
  PixTest::init();
  init(); 
  LOG(logDEBUG) << "PixTestAlive ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestAlive::PixTestAlive() : PixTest() {
  LOG(logDEBUG) << "PixTestAlive ctor()";
}

// ----------------------------------------------------------------------
bool PixTestAlive::setParameter(string parName, string sval) {
  bool found(false);
  string stripParName; 
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 

      LOG(logDEBUG) << "  ==> parName: " << parName;
      LOG(logDEBUG) << "  ==> sval:    " << sval;
      if (!parName.compare("Ntrig")) {
	fParNtrig = static_cast<uint16_t>(atoi(sval.c_str())); 
	setToolTips();
      }
      if (!parName.compare("Vcal")) {
	fParVcal = atoi(sval.c_str()); 
	setToolTips();
      }
      break;
    }
  }
  return found; 
}



// ----------------------------------------------------------------------
void PixTestAlive::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;
  if (!command.compare("masktest")) {
    maskTest(); 
    return;
  }

  if (!command.compare("addressdecodingtest")) {
    addressDecodingTest(); 
    return;
  }
  LOG(logDEBUG) << "did not find command ->" << command << "<-";
}


// ----------------------------------------------------------------------
void PixTestAlive::init() {
  LOG(logDEBUG) << "PixTestAlive::init()";

  setToolTips();
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}

// ----------------------------------------------------------------------
void PixTestAlive::setToolTips() {
  fTestTip    = string("send Ntrig \"calibrates\" and count how many hits were measured\n")
    + string("the result is a hitmap, not an efficiency map")
    ;
  fSummaryTip = string("all ROCs are displayed side-by-side. Note the orientation:\n")
    + string("the canvas bottom corresponds to the narrow module side with the cable")
    ;
}


// ----------------------------------------------------------------------
void PixTestAlive::bookHist(string name) {
  fDirectory->cd(); 
  LOG(logDEBUG) << "nothing done with " << name; 
}


//----------------------------------------------------------
PixTestAlive::~PixTestAlive() {
  LOG(logDEBUG) << "PixTestAlive dtor";
  if (fPixSetup->doMoreWebCloning()) output4moreweb();
}


// ----------------------------------------------------------------------
void PixTestAlive::doTest() {
  if (fPixSetup->isDummy()) {
    dummyAnalysis(); 
    return;
  }

  PixTest::update(); 
  fDirectory->cd();
  LOG(logINFO) << "PixTestAlive::doTest() ntrig = " << int(fParNtrig);
  PixTest::update(); 

  fApi->setDAC("ctrlreg", 4);
  fApi->setDAC("vcal", fParVcal);

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  vector<TH2D*> test2 = efficiencyMaps("PixelAlive", fParNtrig); 
  for (unsigned int i = 0; i < test2.size(); ++i) {
    fHistOptions.insert(make_pair(test2[i], "colz"));
  }

  copy(test2.begin(), test2.end(), back_inserter(fHistList));
  
  TH2D *h = (TH2D*)(*fHistList.begin());

  h->Draw(getHistOption(h).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 
  LOG(logINFO) << "PixTestAlive::doTest() done";
}


// ----------------------------------------------------------------------
void PixTestAlive::maskTest() {
  if (fPixSetup->isDummy()) {
    dummyAnalysis(); 
    return;
  }

  PixTest::update(); 
  fDirectory->cd();
  LOG(logINFO) << "PixTestAlive::maskTest() ntrig = " << int(fParNtrig);
  PixTest::update(); 

  fApi->setDAC("ctrlreg", 4);
  fApi->setDAC("vcal", fParVcal);

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(true);

  vector<TH2D*> test2 = efficiencyMaps("MaskTest", fParNtrig); 
  for (unsigned int i = 0; i < test2.size(); ++i) {
    fHistOptions.insert(make_pair(test2[i], "colz"));
  }

  copy(test2.begin(), test2.end(), back_inserter(fHistList));
  
  TH2D *h = (TH2D*)(*fHistList.begin());

  h->Draw(getHistOption(h).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 
  LOG(logINFO) << "PixTestAlive::maskTest() done";
}


// ----------------------------------------------------------------------
void PixTestAlive::addressDecodingTest() {
  if (fPixSetup->isDummy()) {
    dummyAnalysis(); 
    return;
  }

  PixTest::update(); 
  fDirectory->cd();
  LOG(logINFO) << "PixTestAlive::addressDecodingTest() ntrig = " << 1;
  PixTest::update(); 

  fApi->setDAC("ctrlreg", 4);
  fApi->setDAC("vcal", fParVcal);


  fDirectory->cd(); 
  vector<TH2D*> maps;
  TH2D *h2(0); 

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    LOG(logDEBUG) << "Create hist " << Form("addressDecoding_C%d", iroc); 
    h2 = bookTH2D(Form("addressDecoding_C%d", iroc), Form("addressDecoding_C%d", rocIds[iroc]), 52, 0., 52., 80, 0., 80.); 
    h2->SetMinimum(0.); 
    fHistOptions.insert(make_pair(h2, "colz"));
    h2->SetDirectory(fDirectory); 
    setTitles(h2, "col", "row"); 
    maps.push_back(h2); 
  }

  uint16_t FLAGS = FLAG_FORCE_MASKED;
  cout << "FLAGS = " << static_cast<unsigned int>(FLAGS) << endl;

  vector<pixel> results;
  
//   std::vector<std::pair<uint16_t, uint8_t> > pg_setup = fPixSetup->getConfigParameters()->getTbPgSettings();
//   fApi->initTestboard(fPixSetup->getConfigParameters()->getTbSigDelays(), 
// 		      fPixSetup->getConfigParameters()->getTbPowerSettings(), pg_setup);  //FIXME to be divided

//   for (int irow = 0; irow < 5; ++irow) {
//     LOG(logDEBUG) << "addressDecoding test, row " << irow; 
//     fApi->_dut->testAllPixels(false);
//     fApi->_dut->maskAllPixels(true);
//     for (int icol = 0; icol < 52; ++icol) {
//       fApi->_dut->testPixel(icol, irow, true); 
//       fApi->_dut->maskPixel(icol, irow, false); 
//     }

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

    int cnt(0); 
    bool done = false;
    results.clear();
    while (!done){
      try {
	LOG(logDEBUG) << "getEfficiencyMap() "; 
	results = fApi->getEfficiencyMap(FLAGS, 1);
// 	fApi->daqStart(pg_setup);
// 	fApi->daqTrigger(10);
// 	vector<Event> events = fApi->daqGetEventBuffer();
// 	cout << "daq status: " << fApi->daqStatus() << endl;
// 	fApi->daqStop();
// 	cout << "   events.size() = " << events.size() << endl;
// 	//	cout << event << endl;
// 	return;
	  
	done = true;
      }
      catch(pxar::DataMissingEvent &e){
	LOG(logDEBUG) << "problem with readout: "<< e.what() << " missing " << e.numberMissing << " events"; 
	++cnt;
	if (e.numberMissing > 10) done = true; 
      }
      done = (cnt>5) || done;
    }

    int idx(-1), oldIdx(-2); 
    int icol(0); 

    cout << "results.size() = " << results.size() << endl;
    for (unsigned int i = 0; i < results.size(); ++i) {
      idx = getIdxFromId(results[i].roc_id);
      // -- a new ROC is appearing in the readout, reset icol
      if (idx != oldIdx) {
	oldIdx = idx;
	icol = 0; 
	cout << endl;
      }
      if (rocIds.end() != find(rocIds.begin(), rocIds.end(), idx)) {
	h2 = maps[idx];
	int row = results[i].row; 
	int col = results[i].column; 
	int val = /* 100*row + */ col;
	if (icol == col) {
	  h2->Fill(icol, irow, val); 
	} else {
	}
	cout << val << " "; 
      } else {
	LOG(logDEBUG) << "histogram for ROC " << (int)results[i].roc_id << " not found"; 
      }
      ++icol;
    }
    cout << endl;
    
  }

  copy(maps.begin(), maps.end(), back_inserter(fHistList));
  
  TH2D *h = (TH2D*)(*fHistList.begin());

  h->Draw(getHistOption(h).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 
  LOG(logINFO) << "PixTestAlive::addDecodingTest() done";
}


// ----------------------------------------------------------------------
void PixTestAlive::dummyAnalysis() {
  string name("PixelAlive"); 
  TH2D *h2(0); 
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    fId2Idx.insert(make_pair(rocIds[iroc], iroc)); 
    h2 = bookTH2D(Form("%s_C%d", name.c_str(), iroc), Form("%s_C%d", name.c_str(), rocIds[iroc]), 52, 0., 52., 80, 0., 80.); 
    h2->SetMinimum(0.); 
    h2->SetDirectory(fDirectory); 
    setTitles(h2, "col", "row"); 
    fHistOptions.insert(make_pair(h2, "colz"));
    
    for (int ix = 0; ix < 52; ++ix) {
      for (int iy = 0; iy < 80; ++iy) {
	h2->SetBinContent(ix+1, iy+1, fParNtrig); 
      }
    }

    fHistList.push_back(h2); 
  }

  TH2D *h = (TH2D*)(*fHistList.begin());
  h->Draw(getHistOption(h).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 
  LOG(logINFO) << "PixTestAlive::dummyAnalysis() done";

}


// ----------------------------------------------------------------------
void PixTestAlive::output4moreweb() {
  list<TH1*>::iterator begin = fHistList.begin();
  list<TH1*>::iterator end = fHistList.end();
  
  TDirectory *pDir = gDirectory; 
  gFile->cd(); 
  for (list<TH1*>::iterator il = begin; il != end; ++il) {
    string name = (*il)->GetName(); 
    PixUtil::replaceAll(name, "PixelAlive", "PixelMap"); 
    PixUtil::replaceAll(name, "_V0", ""); 
    TH2D *h = (TH2D*)((*il)->Clone(name.c_str()));
    h->SetDirectory(gDirectory); 
    h->Write(); 

    name = (*il)->GetName(); 
    PixUtil::replaceAll(name, "PixelAlive", "AddressDecoding"); 
    PixUtil::replaceAll(name, "_V0", ""); 
    h = (TH2D*)((*il)->Clone(name.c_str()));
    h->SetDirectory(gDirectory); 
    h->Write(); 
  }
  pDir->cd(); 
}
