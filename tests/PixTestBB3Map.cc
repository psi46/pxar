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

double NSIGMA = 5;

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
  maskPixels();

  int flag(FLAG_CALS);
  fApi->setDAC("ctrlreg", 4);     // high range
  fApi->setDAC("vcal", fParVcalS);   

  int result(1);
  if (fDumpAll) result |= 0x20;
  if (fDumpProblematic) result |= 0x10;

  fNDaqErrors = 0;
  // scurveMaps function is located in pxar/tests/PixTest.cc
  // generate a TH1 s-curve wrt VthrComp for each pixel
  vector<TH1*>  thrmapsCals = scurveMaps("VthrComp", "calSMap", fParNtrig, 0, 149, 30, fParNtrig, result, 1, flag);
 
  // create vectors of plots to hold the rescaled thresholds
  vector<TH2D*>  rescaledThrmaps;
  vector<TH1D*>  rescaledThrdists;

  // create vector of pairs of 1D distributions (for odd/even columns)
  vector<pair<TH1D*,TH1D*> > dlist;


  for (unsigned int i = 0; i < thrmapsCals.size(); ++i) {

    // initialize 1D plots
    TH1D* hEven = bookTH1D(Form("dist_thr_calSMap_VthrComp_EvenCol_C%d", i), 
			   Form("VthrComp threshold distribution (even col.) (C%d)", i),
			   256, 0., 256.);
    TH1D* hOdd = bookTH1D(Form("dist_thr_calSMap_VthrComp_OddCol_C%d", i), 
			  Form("VthrComp threshold distribution (odd col.) (C%d)", i),
			  256, 0., 256.);
    TH1D* hRescaled = bookTH1D(Form("dist_rescaledThr_C%d", i), 
			   Form("(thr-mean)/sigma (C%d)", i),
			   100, -10., 10.);
    rescaledThrdists.push_back(hRescaled);

    // initialize 2D rescaled threshold plots
    TH2D* rescaledPlot = bookTH2D(Form("rescaledThr_C%d", i),
				  Form("(thr-mean)/sigma (C%d)", i),
				  52, 0., 52., 80, 0., 80.);
    rescaledThrmaps.push_back(rescaledPlot);

    // -- relabel negative thresholds as 255 and create distribution list
    // a negative threshold implies that the fit failed to converge
    for (int ix = 0; ix < thrmapsCals[i]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < thrmapsCals[i]->GetNbinsY(); ++iy) {
	if (thrmapsCals[i]->GetBinContent(ix+1, iy+1) < 0) thrmapsCals[i]->SetBinContent(ix+1, iy+1, 255.);
      }
    }
    // make a 1D distributions of the VthrComp threshold for each ROC
    // (one for odd columns, one for even columns)
    for (int ix = 0; ix < thrmapsCals[i]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < thrmapsCals[i]->GetNbinsY(); ++iy) {
	if (ix % 2 == 0) // even column
	  hEven->Fill(thrmapsCals[i]->GetBinContent(ix+1, iy+1));
	else // odd column
	  hOdd->Fill(thrmapsCals[i]->GetBinContent(ix+1, iy+1));
      }
    }

    dlist.push_back(std::make_pair(hEven,hOdd));
    fHistList.push_back(hEven);
    fHistList.push_back(hOdd);
  }

  restoreDacs();

  string bbString("");
  int nBadBumps(0);
  int nPeaksEven(0), nPeaksOdd(0);
  TSpectrum s;
  TH1D* distEven(0);
  TH1D* distOdd(0);
  TF1* fitEven(0);
  TF1* fitOdd(0);

  // loop over the 1D distribution pairs (one for each ROC)
  for (unsigned int i = 0; i < dlist.size(); ++i) {

    distEven = (TH1D*)dlist[i].first;
    distOdd  = (TH1D*)dlist[i].second;
    distEven->GetXaxis()->SetRangeUser(10.,256.);
    distOdd->GetXaxis()->SetRangeUser(10.,256.);

    // search for peaks in the distribution
    // sigma = 5, threshold = 50%
    // peaks below threshold*max_peak_height are discarded
    // "nobackground" means it doesn't try to subtract a background
    //   from the distribution
    nPeaksEven = s.Search(distEven, 5, "nobackground", 0.5);
    nPeaksOdd = s.Search(distOdd, 5, "nobackground", 0.5);
    distEven->GetXaxis()->UnZoom();
    distOdd->GetXaxis()->UnZoom();

    // use fitPeaks algorithm to get the fitted gaussian of good bumps
    fitEven = fitPeaks(distEven, s, nPeaksEven);
    double meanEven = fitEven->GetParameter(1);
    double sigmaEven = fabs(fitEven->GetParameter(2));
    fitOdd = fitPeaks(distOdd, s, nPeaksOdd);
    double meanOdd = fitOdd->GetParameter(1);
    double sigmaOdd = fabs(fitOdd->GetParameter(2));

    int cutEven = static_cast<int>(meanEven + NSIGMA*sigmaEven) + 1;
    int cutOdd = static_cast<int>(meanOdd + NSIGMA*sigmaOdd) + 1;

    LOG(logDEBUG) << "found " << nPeaksEven << " peaks in " << distEven->GetName();
    LOG(logDEBUG) << "  best peak: mean = " << meanEven << ", RMS = " << sigmaEven;
    LOG(logDEBUG) << "    cut value = " << cutEven;

    LOG(logDEBUG) << "found " << nPeaksOdd << " peaks in " << distOdd->GetName();
    LOG(logDEBUG) << "  best peak: mean = " << meanOdd << ", RMS = " << sigmaOdd;
    LOG(logDEBUG) << "    cut value = " << cutOdd;

    // draw an arrow on the plot to denote cutEven
    TArrow *paEven = new TArrow(cutEven, 0.5*distEven->GetMaximum(), cutEven, 0., 0.06, "|>");
    paEven->SetArrowSize(0.1);
    paEven->SetAngle(40);
    paEven->SetLineWidth(2);
    distEven->GetListOfFunctions()->Add(paEven);

    // draw an arrow on the plot to denote cutOdd
    TArrow *paOdd = new TArrow(cutOdd, 0.5*distOdd->GetMaximum(), cutOdd, 0., 0.06, "|>");
    paOdd->SetArrowSize(0.1);
    paOdd->SetAngle(40);
    paOdd->SetLineWidth(2);
    distOdd->GetListOfFunctions()->Add(paOdd);

    // fill plots with (thr-mean)/sigma (things above NSIGMA are called bad)
    for (int ix = 1; ix <= thrmapsCals[i]->GetNbinsX(); ++ix) {
      for (int iy = 1; iy <= thrmapsCals[i]->GetNbinsY(); ++iy) {
	double content = thrmapsCals[i]->GetBinContent(ix, iy);
	double rescaledThr = 0;
	if (ix % 2 == 1) // even column
	  rescaledThr = (content-meanEven)/sigmaEven;
	else // odd column
	  rescaledThr = (content-meanOdd)/sigmaOdd;

	// fill 1D plot, put under/overflow entries into first/last bin
	if (rescaledThr < rescaledThrdists[i]->GetXaxis()->GetXmax()-0.01){
	  if (rescaledThr > rescaledThrdists[i]->GetXaxis()->GetXmin()){
	    rescaledThrdists[i]->Fill(rescaledThr);
	  }
	  else {
	    rescaledThrdists[i]->Fill(rescaledThrdists[i]->GetXaxis()->GetXmin());
	  }
	}
	else {
	    rescaledThrdists[i]->Fill(rescaledThrdists[i]->GetXaxis()->GetXmax()-0.01);
	}
	  

	rescaledThrmaps[i]->Fill(ix-1,iy-1,rescaledThr);
      }
    }
    rescaledThrmaps[i]->SetMinimum(-5.);
    rescaledThrmaps[i]->SetMaximum(5);
    fHistList.push_back(rescaledThrmaps[i]);
    fHistOptions.insert(make_pair(rescaledThrmaps[i], "colz"));


    // draw an arrow on the plot to denote cutDead
    TArrow *pa = new TArrow(NSIGMA, 0.5*rescaledThrdists[i]->GetMaximum(), NSIGMA, 0., 0.06, "|>");
    pa->SetArrowSize(0.1);
    pa->SetAngle(40);
    pa->SetLineWidth(2);
    rescaledThrdists[i]->GetListOfFunctions()->Add(pa);
    fHistList.push_back(rescaledThrdists[i]);

    // bad bumps are those above NSIGMA*sigma above the mean
    nBadBumps = static_cast<int>(rescaledThrdists[i]->Integral(rescaledThrdists[i]->FindBin(NSIGMA), 
							       rescaledThrdists[i]->GetNbinsX()+1));
    bbString += Form(" %4d", nBadBumps);

  }
  if (rescaledThrdists[15]) {
    rescaledThrdists[15]->Draw();
    fDisplayedHist = find(fHistList.begin(), fHistList.end(), rescaledThrdists[15]);
  }
  PixTest::update();

  int seconds = t.RealTime();
  LOG(logINFO) << "PixTestBB3Map::doTest() done"
	       << (fNDaqErrors>0? Form(" with %d decoding errors: ", static_cast<int>(fNDaqErrors)):"")
	       << ", duration: " << seconds << " seconds";
  LOG(logINFO) << "number of dead bumps (per ROC): " << bbString;
  dutCalibrateOff();

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
    // (using pixels within +-25 DAC of peak)
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
