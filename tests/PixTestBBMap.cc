// -- author: Wolfram Erdmann
// Bump Bonding tests, really just a threshold map using cals

#include <sstream>   // parsing
#include <algorithm>  // std::find

#include <TSpectrum.h>

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
  
  fDirectory = gFile->GetDirectory(fName.c_str());
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str());
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

  int result(1);

  fNDaqErrors = 0; 
  vector<TH1*>  thrmapsCals = scurveMaps("VthrComp", "calSMap", fParNtrig, 0, 170, result, 1, flag);

  // -- relabel negative thresholds as 255 and create distribution list
  vector<TH1D*> dlist; 
  TH1D *h(0);
  for (unsigned int i = 0; i < thrmapsCals.size(); ++i) {
    for (int ix = 0; ix < thrmapsCals[i]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < thrmapsCals[i]->GetNbinsY(); ++iy) {
	if (thrmapsCals[i]->GetBinContent(ix+1, iy+1) < 0) thrmapsCals[i]->SetBinContent(ix+1, iy+1, 255.);
      }
    }
    h = distribution((TH2D*)thrmapsCals[i], 256, 0., 256.);
    dlist.push_back(h); 
    fHistList.push_back(h); 
  }

  restoreDacs();

  // -- summary printout
  string bbString(""), bbCuts(""); 
  int bbprob(0); 
  int nPeaks(0), cutDead(0); 
  TSpectrum s; 
  for (unsigned int i = 0; i < dlist.size(); ++i) {
    h = (TH1D*)dlist[i];
    nPeaks = s.Search(h, 5, "", 0.01); 
    LOG(logDEBUG) << "found " << nPeaks << " peaks in " << h->GetName();
    cutDead = fitPeaks(h, s, nPeaks); 

    bbprob = static_cast<int>(h->Integral(cutDead, h->FindBin(255)));
    bbString += Form(" %4d", bbprob); 
    bbCuts   += Form(" %4d", cutDead); 
  }

  if (h) {
    h->Draw();
    fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  }
  PixTest::update(); 
  
  LOG(logINFO) << "PixTestBBMap::doTest() done"
	       << (fNDaqErrors>0? Form(" with %d decoding errors: ", static_cast<int>(fNDaqErrors)):"");
  LOG(logINFO) << "number of dead bumps (per ROC): " << bbString;
  LOG(logINFO) << "separation cut       (per ROC): " << bbCuts;

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



// ----------------------------------------------------------------------
int PixTestBBMap::fitPeaks(TH1D *h, TSpectrum &s, int npeaks) {

  Float_t *xpeaks = s.GetPositionX();
  string name; 
  double lcuts[3]; lcuts[0] = lcuts[1] = lcuts[2] = 255.;
  TF1 *f(0); 
  double peak, sigma;
  int fittedPeaks(0); 
  for (int p = 0; p < npeaks; ++p) {
    double xp = xpeaks[p];
    if (p > 1) continue;
    if (xp > 200) {
      continue;
    }
    name = Form("gauss_%d", p); 
    f = new TF1(name.c_str(), "gaus(0)", 0., 256.);
    int bin = h->GetXaxis()->FindBin(xp);
    double yp = h->GetBinContent(bin);
    f->SetParameters(yp, xp, 2.);
    h->Fit(f, "Q+"); 
    ++fittedPeaks;
    peak = h->GetFunction(name.c_str())->GetParameter(1); 
    sigma = h->GetFunction(name.c_str())->GetParameter(2); 
    if (0 == p) {
      lcuts[0] = peak + 3*sigma; 
      if (h->Integral(h->FindBin(peak + 10.*sigma), 250) > 10.) {
	lcuts[1] = peak + 5*sigma;
      } else {
	lcuts[1] = peak + 10*sigma;
      }
    } else {
      lcuts[1] = peak - 3*sigma; 
      lcuts[2] = peak - sigma; 
    }
    delete f;
  }
  
  int startbin = (int)(0.5*(lcuts[0] + lcuts[1])); 
  int endbin = (int)(lcuts[1]); 
  if (endbin <= startbin) {
    endbin = (int)(lcuts[2]); 
    if (endbin < startbin) {
      endbin = 255.;
    }
  }

  int minbin(0); 
  double minval(999.); 
  
  for (int i = startbin; i <= endbin; ++i) {
    if (h->GetBinContent(i) < minval) {
      if (1 == fittedPeaks) {
	if (0 == h->Integral(i, i+4)) {
	  minval = h->GetBinContent(i); 
	  minbin = i; 
	} else {
	  minbin = endbin;
	}
      } else {
	minval = h->GetBinContent(i); 
	minbin = i; 
      }
    }
  }
  
  LOG(logDEBUG) << "cut for dead bump bonds: " << minbin << " (obtained for minval = " << minval << ")" 
		<< " start: " << startbin << " .. " << endbin 
		<< " last peak: " << peak << " last sigma: " << sigma
		<< " lcuts[0] = " << lcuts[0] << " lcuts[1] = " << lcuts[1];
  return minbin+1; 
}
