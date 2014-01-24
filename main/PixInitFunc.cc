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

}


// ----------------------------------------------------------------------
TF1* PixInitFunc::errScurve(TH1 *h) {

  TF1* f = (TF1*)gROOT->FindObject("PIF_err");
  if (0 == f) {
    f = new TF1("PIF_err", PIF_err, h->GetBinLowEdge(1), h->GetBinLowEdge(h->GetNbinsX()+1), 4);
    f->SetParNames("step", "slope", "floor", "plateau");                       
  } else {
    f->ReleaseParameter(0);     
    f->ReleaseParameter(1);     
    f->ReleaseParameter(2);     
    f->ReleaseParameter(3); 
    f->SetRange(h->GetBinLowEdge(1), h->GetBinLowEdge(h->GetNbinsX()+1)); 
  }
  int ibin = h->FindFirstBinAbove(0.); 
  int jbin = h->FindFirstBinAbove(0.9*h->GetMaximum());
  
  f->SetParameter(0, h->GetBinCenter(0.5*(ibin+jbin))); 
  f->SetParameter(1, 1.); 
  if (jbin == ibin) {
    cout << "XXXXXXXXXXX PixInitFunc: STEP FUNCTION " << endl;
    f->FixParameter(0, h->GetBinCenter(jbin)); 
    f->SetParameter(1, 0.); 
  }
  f->FixParameter(2, 1.); 
  f->SetParameter(3, 0.5*h->GetMaximum()); 
  return f; 
}




