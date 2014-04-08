#include "PixInitFunc.hh"

#include "TROOT.h"
#include "TMath.h"
#include "TCanvas.h"

#include <iostream>
#include <iomanip>


ClassImp(PixInitFunc)

using namespace std;

namespace {



  // ----------------------------------------------------------------------
  // par[0]: "step" 
  // par[1]: "slope"   the smaller the steeper
  // par[2]: "floor"   1 -> floor is at 0
  // par[3]: "plateau" y -> plateau is at 2*y                       

  double PIF_err(double *x, double *par) {
    return par[3]*(TMath::Erf((x[0]-par[0])/par[1])+par[2]); 
  } 


}


// ----------------------------------------------------------------------
PixInitFunc::PixInitFunc() {

}

// ---------------------------------------------------------------------- 
PixInitFunc::~PixInitFunc() {

}

// ----------------------------------------------------------------------
void PixInitFunc::resetLimits() {

}

// ----------------------------------------------------------------------
void PixInitFunc::limitPar(int ipar, double lo, double hi) {
  fLimit[ipar] = true; 
  fLimitLo[ipar] = lo; 
  fLimitHi[ipar] = hi; 
}


// ----------------------------------------------------------------------
TF1* PixInitFunc::errScurve(TH1 *h) {

  fDoNotFit = false;

  // -- determine step function and start of function range (to exclude spurious low-threshold readouts)
  int STARTBIN(2); 
  int ibin(-1), jbin(-1);
  double hmax(h->GetMaximum()); 
  for (int i = STARTBIN; i <= h->GetNbinsX(); ++i) {
    if (h->GetBinContent(i) > 0) {
      ibin = i; 
      break;
    }
  }

  // require 2 consecutive bins on plateau
  for (int i = STARTBIN; i < h->GetNbinsX(); ++i) {
    if (h->GetBinContent(i) > 0.9*hmax && h->GetBinContent(i+1) > 0.9*hmax) {
      jbin = i; 
      break;
    }
  }

  double lo = h->GetBinLowEdge(1); 
  int zcnt(0); 
  // require 3 consecutive bins at zero
  for (int i = 3; i < h->GetNbinsX(); ++i) {
    if (h->GetBinContent(i-2) < 1 && h->GetBinContent(i-1) < 1 && h->GetBinContent(i) < 1) {
      lo = h->GetBinLowEdge(i-2);
      break;
    }
  }

  double hi = h->FindLastBinAbove(0.9*h->GetMaximum());


  // -- setup function
  TF1* f = (TF1*)gROOT->FindObject("PIF_err");
  if (0 == f) {
    f = new TF1("PIF_err", PIF_err, h->GetBinLowEdge(1), h->GetBinLowEdge(h->GetNbinsX()+1), 4);
    f->SetParNames("step", "slope", "floor", "plateau");                       
  } else {
    f->ReleaseParameter(0);     
    f->ReleaseParameter(1);     
    f->ReleaseParameter(2);     
    f->ReleaseParameter(3); 
    f->SetRange(lo, hi); 
  }
  
  f->SetParameter(0, h->GetBinCenter((ibin+jbin)/2)); 
  f->SetParameter(1, 0.2); 
  if (jbin == ibin) {
    //    cout << "XXXXXXXXXXX PixInitFunc: STEP FUNCTION " << h->GetTitle() << " ibin = " << ibin << " jbin = " << jbin << endl;
    f->FixParameter(0, h->GetBinCenter(jbin)); 
    f->FixParameter(1, 1.e2); 
    fDoNotFit = true;
  }
  f->FixParameter(2, 1.); 
  f->FixParameter(3, 0.5*h->GetMaximum()); 
  return f; 
}




