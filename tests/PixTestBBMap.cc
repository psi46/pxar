// -- author: Wolfram Erdmann
// Bump Bonding tests, really just a threshold map using cals

#include <sstream>   // parsing
#include <algorithm>  // std::find

#include "PixTestBBMap.hh"
#include "log.h"
#include "constants.h"   // roctypes

using namespace std;
using namespace pxar;

ClassImp(PixTestBBMap)

//------------------------------------------------------------------------------
PixTestBBMap::PixTestBBMap(PixSetup *a, std::string name): PixTest(a, name), fParNtrig(-1), fParVcalS(200), fParXtalk(0) {
  PixTest::init();
  init();
  LOG(logDEBUG) << "PixTestBBMap ctor(PixSetup &a, string, TGTab *)";
}


//------------------------------------------------------------------------------
PixTestBBMap::PixTestBBMap(): PixTest() {
  LOG(logDEBUG) << "PixTestBBMap ctor()";
}



//------------------------------------------------------------------------------
bool PixTestBBMap::setParameter(string parName, string sval) {

  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (uint32_t i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      stringstream s(sval);
      if (!parName.compare( "ntrig")) { s >> fParNtrig; return true;}
      if (!parName.compare( "vcals")) { s >> fParVcalS; return true;}
      if (!parName.compare( "xtalk"))  { s >> fParXtalk;  return true;}
    }
  }
  return false;
}

//------------------------------------------------------------------------------
void PixTestBBMap::init() {
  LOG(logDEBUG) << "PixTestBBMap::init()";
  
  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory ) {
    fDirectory = gFile->mkdir( fName.c_str() );
  }
  fDirectory->cd();
}

// ----------------------------------------------------------------------
void PixTestBBMap::setToolTips() {
  fTestTip = string( "Bump Bonding Test = threshold map for CalS");
  fSummaryTip = string("module summary");
}


//------------------------------------------------------------------------------
PixTestBBMap::~PixTestBBMap() {
  LOG(logDEBUG) << "PixTestBBMap dtor";
}

//------------------------------------------------------------------------------
void PixTestBBMap::doTest() {

  cacheDacs();
  PixTest::update();
  bigBanner(Form("PixTestBBMap::doTest() Ntrig = %d, VcalS = %d, xtalk = %d", fParNtrig, fParVcalS, fParXtalk));
 
  fDirectory->cd();

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  int flag(FLAG_CALS);
  fApi->setDAC("ctrlreg", 4);     // high range
  fApi->setDAC("vcal", fParVcalS);    

  int result(7);

  LOG(logDEBUG) << "taking CalS threshold maps";
  vector<TH1*>  thrmapsCals = scurveMaps("VthrComp", "calSMap", fParNtrig, 0, 170, result, 1, flag);
  copy(thrmapsCals.begin(), thrmapsCals.end(), back_inserter(fHistList));

  if (fParXtalk) {
    LOG(logDEBUG) << "taking Xtalk maps";
    vector<TH1*> thrmapsXtalk = scurveMaps("VthrComp", "calSMapXtalk", fParNtrig, 0, 170, result, 1, flag | FLAG_XTALK); 

    LOG(logDEBUG) << "map analysis";
    vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
    for (unsigned int idx = 0; idx < rocIds.size(); ++idx){
      unsigned int rocId = getIdFromIdx(idx);
      TH2D* rocmapRaw = (TH2D*)thrmapsCals[idx];
      TH2D* rocmapBB(0); 
      TH2D* rocmapXtalk(0);
      
      rocmapBB  = (TH2D*)rocmapRaw->Clone(Form("BB-%2d",rocId));
      rocmapBB->SetTitle(Form("CalS - Xtalk %2d",rocId));
      rocmapXtalk = (TH2D*)thrmapsXtalk[idx];  
      rocmapBB->Add(rocmapXtalk, -1.);
      fHistOptions.insert(make_pair(rocmapBB, "colz"));
      fHistList.push_back(rocmapBB);
      
      TH1D* hdistBB = bookTH1D(Form("dist_CalS-Xtalksubtracted_C%d", rocId), 
			       Form("CalS-Xtalksubtracted C%d", rocId), 
			       514, -257., 257.);
      
      for (int col = 0; col < ROC_NUMCOLS; col++) {
	for (int row = 0; row < ROC_NUMROWS; row++) {
	  hdistBB->Fill(rocmapBB->GetBinContent(col+1, row+1));
	}
      }
      fHistList.push_back(hdistBB);
    }

  }
  
  TH1D *h(0);
  
  restoreDacs();

  // -- summary printout
  string bbString(""), hname(""); 
  int bbprob(0); 
  for (unsigned int i = 0; i < thrmapsCals.size(); ++i) {
    hname = thrmapsCals[i]->GetName();
    if (string::npos == hname.find("dist_thr_")) continue;
    h = (TH1D*)thrmapsCals[i];
    bbprob = h->Integral(1, 10); 
    bbString += Form(" %4d", bbprob); 
  }

  h->Draw();
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 
  
  LOG(logINFO) << "PixTestBBMap::doTest() done";
  LOG(logINFO) << "number of dead bumps (per ROC): " << bbString;

}


// ----------------------------------------------------------------------
void PixTestBBMap::output4moreweb() {

}
