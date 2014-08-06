// -- author: Wolfram Erdmann
// Bump Bonding tests, really just a threshold map using cals

#include <sstream>   // parsing
#include <algorithm>  // std::find

#include "PixTestBBMap.hh"
#include "PixUtil.hh"
#include "log.h"
#include "constants.h"   // roctypes

using namespace std;
using namespace pxar;

ClassImp(PixTestBBMap)

//------------------------------------------------------------------------------
PixTestBBMap::PixTestBBMap(PixSetup *a, std::string name): PixTest(a, name), fParNtrig(-1), fParVcalS(200) {
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
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());

      stringstream s(sval);
      if (!parName.compare( "ntrig")) { 
	s >> fParNtrig; 
	setToolTips();
	return true;
      }
      if (!parName.compare( "vcals")) { 
	s >> fParVcalS; 
	setToolTips();
	return true;
      }
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
  if (fPixSetup->doMoreWebCloning()) output4moreweb();
}

//------------------------------------------------------------------------------
void PixTestBBMap::doTest() {

  cacheDacs();
  PixTest::update();
  bigBanner(Form("PixTestBBMap::doTest() Ntrig = %d, VcalS = %d (high range)", fParNtrig, fParVcalS));
 
  fDirectory->cd();

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  int flag(FLAG_CALS);
  fApi->setDAC("ctrlreg", 4);     // high range
  fApi->setDAC("vcal", fParVcalS);    

  int result(7);

  fNDaqErrors = 0; 
  vector<TH1*>  thrmapsCals = scurveMaps("VthrComp", "calSMap", fParNtrig, 0, 170, result, 1, flag);

  TH1D *h(0);
  
  restoreDacs();

  // -- summary printout
  string bbString(""), hname(""); 
  int bbprob(0); 
  for (unsigned int i = 0; i < thrmapsCals.size(); ++i) {
    hname = thrmapsCals[i]->GetName();
    if (string::npos == hname.find("dist_thr_")) continue;
    h = (TH1D*)thrmapsCals[i];
    int imax = h->GetMaximumBin(); 
    LOG(logDEBUG) << "setting imax to bin with maximum: " << imax; 
    if (imax > 0.9*h->GetNbinsX()) {
      imax = h->FindBin(h->GetMean()); 
      LOG(logDEBUG) << "resetting to imax to bin with mean: " << imax; 
    }
    int firstZero(-1), lastZero(-1); 
    for (int ibin = imax; ibin < h->GetNbinsX(); ++ibin) {
      if (h->GetBinContent(ibin) < 1 && firstZero < 0) {
	firstZero = ibin;
      }
      if (firstZero > 0 && h->GetBinContent(ibin) > 0) {
	lastZero = ibin-1; 
	break;
      }
    }

    int bmin(0); 
    if (lastZero - firstZero > 10) {
      bmin = lastZero - 1;
    } else {
      bmin = firstZero + 5;
    }

    LOG(logDEBUG)  << "firstZero: " << firstZero 
		   << " lastZero: " << lastZero
		   << " -> bmin: " << bmin;

	bbprob = static_cast<int>(h->Integral(bmin, h->FindBin(255)));
    bbString += Form(" %d", bbprob); 
  }

  h->Draw();
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 
  
  LOG(logINFO) << "PixTestBBMap::doTest() done"
	       << (fNDaqErrors>0? Form(" with %d decoding errors", static_cast<int>(fNDaqErrors)):"");
  LOG(logINFO) << "number of dead bumps (per ROC): " << bbString;

}


// ----------------------------------------------------------------------
void PixTestBBMap::output4moreweb() {
  print("PixTestBBMap::output4moreweb()"); 

  list<TH1*>::iterator begin = fHistList.begin();
  list<TH1*>::iterator end = fHistList.end();

  TDirectory *pDir = gDirectory; 
  gFile->cd(); 
  for (list<TH1*>::iterator il = begin; il != end; ++il) {
    string name = (*il)->GetName(); 
    if (string::npos == name.find("_V0"))  continue;
    if (string::npos != name.find("dist_"))  continue;
    if (string::npos == name.find("thr_calSMap_VthrComp")) continue;
    if (string::npos != name.find("calSMap")) {
      PixUtil::replaceAll(name, "thr_calSMap_VthrComp", "BumpBondMap"); 
    }
    PixUtil::replaceAll(name, "_V0", ""); 
    TH2D *h = (TH2D*)((*il)->Clone(name.c_str()));
    h->SetDirectory(gDirectory); 
    h->Write(); 
  }
  pDir->cd(); 

}
