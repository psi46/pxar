// -- author: Jamie Antonelli
// simplified Bump Bonding test
// based on the BBMap test written by Wolfram and Urs

#include <sstream>   // parsing
#include <algorithm>  // std::find

#include <TArrow.h>
#include <TSpectrum.h>
#include "TStopwatch.h"

#include "PixTestBB3Map.hh"
#include "PixUtil.hh"
#include "log.h"
#include "constants.h"   // roctypes

using namespace std;
using namespace pxar;

double NSIGMA = 4;

ClassImp(PixTestBB3Map)

//------------------------------------------------------------------------------
PixTestBB3Map::PixTestBB3Map(PixSetup *a, std::string name): PixTest(a, name), 
  fParNtrig(-1), fParVcalS(200), fDumpAll(-1), fDumpProblematic(-1) {
  PixTest::init();
  init();
  LOG(logDEBUG) << "PixTestBB3Map ctor(PixSetup &a, string, TGTab *)";
}


//------------------------------------------------------------------------------
PixTestBB3Map::PixTestBB3Map(): PixTest() {
  LOG(logDEBUG) << "PixTestBB3Map ctor()";
}



//------------------------------------------------------------------------------
bool PixTestBB3Map::setParameter(string parName, string sval) {

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

      if (!parName.compare("dumpall")) {
	PixUtil::replaceAll(sval, "checkbox(", ""); 
	PixUtil::replaceAll(sval, ")", ""); 
	fDumpAll = atoi(sval.c_str()); 
	setToolTips();
      }

      if (!parName.compare("dumpallproblematic")) {
	PixUtil::replaceAll(sval, "checkbox(", ""); 
	PixUtil::replaceAll(sval, ")", ""); 
	fDumpProblematic = atoi(sval.c_str()); 
	setToolTips();
      }

    }
  }
  return false;
}

//------------------------------------------------------------------------------
void PixTestBB3Map::init() {
  LOG(logDEBUG) << "PixTestBB3Map::init()";
  
  fDirectory = gFile->GetDirectory("BB3");
  if (!fDirectory) {
    fDirectory = gFile->mkdir("BB3");
  }
  fDirectory->cd();
}

// ----------------------------------------------------------------------
void PixTestBB3Map::setToolTips() {
  fTestTip = string( "Bump Bonding Test = threshold map for CalS");
  fSummaryTip = string("module summary");
}


//------------------------------------------------------------------------------
PixTestBB3Map::~PixTestBB3Map() {
  LOG(logDEBUG) << "PixTestBB3Map dtor";
}

//------------------------------------------------------------------------------
void PixTestBB3Map::doTest() {

  TStopwatch t;

  cacheDacs();
  PixTest::update();
  bigBanner(Form("PixTestBB3Map::doTest() Ntrig = %d, VcalS = %d (high range)", fParNtrig, fParVcalS));
 
  fDirectory->cd();

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  int flag(FLAG_CALS);
  fApi->setDAC("ctrlreg", 4);     // high range
  fApi->setDAC("vcal", fParVcalS);    

  int result(1);
  if (fDumpAll) result |= 0x20;
  if (fDumpProblematic) result |= 0x10;

  fNDaqErrors = 0; 
  // scurveMaps function is located in pxar/tests/PixTest.cc
  // generate a TH1 s-curve wrt VthrComp for each pixel
  vector<TH1*>  thrmapsCals = scurveMaps("VthrComp", "calSMap", fParNtrig, 0, 149, 30, result, 1, flag);
  
  // create vector of 2D plots to hold rescaled threshold maps
  vector<TH2D*>  rescaledThrmaps;

  // -- relabel negative thresholds as 255 and create distribution list
  // a negative threshold implies that the fit failed to converge
  vector<TH1D*> dlist; 
  TH1D *h(0);
  for (unsigned int i = 0; i < thrmapsCals.size(); ++i) {
    // initialize 2D rescaled threshold plots
    TH2D* rescaledPlot = bookTH2D(Form("rescaledThr_C%d", i), 
				  Form("(thr-mean)/sigma (C%d)", i), 
				  52, 0., 52., 80, 0., 80.); 
    rescaledThrmaps.push_back(rescaledPlot);
    for (int ix = 0; ix < thrmapsCals[i]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < thrmapsCals[i]->GetNbinsY(); ++iy) {
	if (thrmapsCals[i]->GetBinContent(ix+1, iy+1) < 0) thrmapsCals[i]->SetBinContent(ix+1, iy+1, 255.);
      }
    }
    // make a 1D histogram using the VthrComp for each ROC
    // (one entry per pixel)
    h = distribution((TH2D*)thrmapsCals[i], 256, 0., 256.);

    dlist.push_back(h); 
    fHistList.push_back(h); 
  }

  restoreDacs();

  // -- summary printout
  string bbString(""), bbCuts(""); 
  int nBadBumps(0); 
  int nPeaks(0), cutDead(0); 
  TSpectrum s; 
  TF1* fit(0);

  // loop over the 1D distributions (one for each ROC)
  for (unsigned int i = 0; i < dlist.size(); ++i) {
    h = (TH1D*)dlist[i];
    // search for peaks in the distribution
    // sigma = 5, threshold = 1%
    // peaks below threshold*max_peak_height are discarded
    // "nobackground" means it doesn't try to subtract a background
    //   from the distribution
    nPeaks = s.Search(h, 5, "nobackground", 0.01); 
    LOG(logDEBUG) << "found " << nPeaks << " peaks in " << h->GetName();
    // use fitPeaks algorithm to get the fitted gaussian of good bumps
    //    cutDead = fitPeaks(h, s, nPeaks); 
    fit = fitPeaks(h, s, nPeaks); 
    double mean = fit->GetParameter(1); 
    double sigma = fit->GetParameter(2); 

    // place cut at NSIGMA*sigma above mean of gaussian
    // +1 places cut to the right of the arrow
    cutDead = h->GetXaxis()->FindBin(mean + NSIGMA*sigma) + 1; 

    // bad bumps are those above cutDead
    nBadBumps = static_cast<int>(h->Integral(cutDead+1, h->FindBin(255)));
    bbString += Form(" %4d", nBadBumps); 
    bbCuts   += Form(" %4d", cutDead); 

    // draw an arrow on the plot to denote cutDead
    TArrow *pa = new TArrow(cutDead, 0.5*h->GetMaximum(), cutDead, 0., 0.06, "|>"); 
    pa->SetArrowSize(0.1);
    pa->SetAngle(40);
    pa->SetLineWidth(2);
    h->GetListOfFunctions()->Add(pa); 

    // fill 2D plots with (thr-mean)/sigma (things above NSIGMA are called bad)
    for (int ix = 1; ix <= thrmapsCals[i]->GetNbinsX(); ++ix) {
      for (int iy = 1; iy <= thrmapsCals[i]->GetNbinsY(); ++iy) {
	double content = thrmapsCals[i]->GetBinContent(ix, iy);
	double rescaledThr = (content-mean)/sigma;
	rescaledThrmaps[i]->Fill(ix-1,iy-1,rescaledThr);
	rescaledThrmaps[i]->SetMinimum(-5);
	rescaledThrmaps[i]->SetMaximum(5);
      }
    }
    fHistList.push_back(rescaledThrmaps[i]);
    fHistOptions.insert(make_pair(rescaledThrmaps[i], "colz")); 
  }



  if (h) {
    h->Draw();
    fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  }
  PixTest::update(); 
  
  int seconds = t.RealTime();
  LOG(logINFO) << "PixTestBB3Map::doTest() done"
	       << (fNDaqErrors>0? Form(" with %d decoding errors: ", static_cast<int>(fNDaqErrors)):"") 
	       << ", duration: " << seconds << " seconds";
  LOG(logINFO) << "number of dead bumps (per ROC): " << bbString;
  LOG(logINFO) << "separation cut       (per ROC): " << bbCuts;

}



// ----------------------------------------------------------------------
TF1* PixTestBB3Map::fitPeaks(TH1D *h, TSpectrum &s, int npeaks) {

#if defined ROOT_MAJOR_VER && ROOT_MAJOR_VER > 5
  Double_t *xpeaks = s.GetPositionX();
#else
  Float_t *xpeaks = s.GetPositionX();
#endif

  // this function has been simplified wrt the BB test
  // we now only use the highest peak to fit
  // bumps more than NSIGMA sigma above the peak are called bad
  double bestHeight = 0;
  TF1* f(0);
  TF1* bestFit(0);

  // loop over the found peaks, fit each with gaussian
  // save position, width of the chosen peak
  for (int p = 0; p < npeaks; ++p) {
    double xp = xpeaks[p];
    // don't consider failed s-curve fits (peak at 255)
    if (xp > 250) {
      continue;
    }
    // don't consider dead pixels (peak at 0)
    if (xp < 10) {
      continue;
    }
    string name = Form("gauss_%d", p); 
    // fit a gaussian to the peak
    // (using pixels within +-50 DAC of peak)
    f = new TF1(name.c_str(), "gaus(0)", xp-25., xp+25.);
    int bin = h->GetXaxis()->FindBin(xp);
    double yp = h->GetBinContent(bin);
    // use yp at peak position as guess for height of fit
    // use xp as guess of mean of fit
    // use 2 as guess for width of fit
    f->SetParameters(yp, xp, 2.);
    h->Fit(f, "Q+"); 
    double height = h->GetFunction(name.c_str())->GetParameter(0); 
    // save the parameters of the highest peak
    if (height > bestHeight) {
      bestHeight = height;
      bestFit = f;
    }
  }

  return bestFit;
}
