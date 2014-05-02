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

  double PIF_weibullCdf(double *x, double *par) {
    if (par[1] < 0) return 0.;
    if (par[2] < 0) return 0.;
    return par[0]*(1 - TMath::Exp(-TMath::Power(x[0]/par[1], par[2]))); 
  } 



  const double xCut = TMath::Pi()/2. - 0.0005;
  const double tanXCut = TMath::Tan(xCut);

  double PIF_gpTanPol( Double_t *x, Double_t *par) {
    if (par[0]*x[0] - par[4] > xCut) return tanXCut + (x[0] - (xCut + par[4])/par[0])* 1e8;
    double result = TMath::Tan(par[0]*x[0] - par[4]) + par[1]*x[0]*x[0]*x[0] + par[5]*x[0]*x[0] + par[2]*x[0] + par[3];
    cout << result << endl;
    return result; 
  }


  double PIF_gpTanH( Double_t *x, Double_t *par) {
    return par[3] + par[2] * TMath::TanH(par[0]*x[0] - par[1]);
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
  for (int i = 0; i < 20; ++i) {
    fLimit[i] = false; 
    fLimitLo[i] = 0.; 
    fLimitHi[i] = 0.; 
  }
}

// ----------------------------------------------------------------------
void PixInitFunc::limitPar(int ipar, double lo, double hi) {
  fLimit[ipar] = true; 
  fLimitLo[ipar] = lo; 
  fLimitHi[ipar] = hi; 
}


// ----------------------------------------------------------------------
TF1* PixInitFunc::gpTanPol(TH1 *h) {

  double lo = h->GetBinLowEdge(1); 
  double hi = h->FindLastBinAbove(0.9*h->GetMaximum());
  hi = h->GetBinLowEdge(h->GetNbinsX()+1); 
  // -- setup function
  TF1* f = (TF1*)gROOT->FindObject("PIF_gpTanPol");
  if (0 == f) {
    f = new TF1("PIF_gpTanPol", PIF_gpTanPol, lo, hi, 6);
    f->SetNpx(1000);
    //    f->SetParNames("norm", "scale", "shape");                       
    f->SetRange(lo, hi); 
  } else {
    f->ReleaseParameter(0);     
    f->ReleaseParameter(1);     
    f->ReleaseParameter(2);     
    f->ReleaseParameter(3); 
    f->ReleaseParameter(4); 
    f->ReleaseParameter(5); 
    f->SetRange(lo, hi); 
  }

  int ilo = h->FindFirstBinAbove(0);
  double xlo = h->GetBinCenter(ilo); 
  double ylo = h->GetBinContent(ilo);

  int iup = h->FindFirstBinAbove(0.8*h->GetMaximum());
  double xup = h->GetBinCenter(iup); 
  double yup = h->GetBinContent(iup);

  double slope(0.5);
  if (ilo != iup && (TMath::Abs(xup - xlo) > 1e-5)) slope = (yup-ylo)/(xup-xlo); 
  
  f->SetParameter(2, slope);
  f->SetParameter(3, yup - slope*xup);
  
  double par0 = (TMath::Pi()/2. - 1.4) / h->GetBinCenter(h->FindLastBinAbove(0.));
  f->SetParameter(0, par0);
  f->FixParameter(1, 0.);
  f->SetParameter(4, -1.4);

  if (xup > 0.) f->SetParameter(5, (yup - (TMath::Tan(f->GetParameter(0)*xup - f->GetParameter(4))
					    + f->GetParameter(1)*xup*xup*xup + slope*xup + f->GetParameter(3)))/(xup*xup));
  else f->SetParameter(5, 0.);
  
  cout << f->GetParameter(0) << endl;
  cout << f->GetParameter(1) << endl;
  cout << f->GetParameter(2) << endl;
  cout << f->GetParameter(3) << endl;
  cout << f->GetParameter(4) << endl;
  cout << f->GetParameter(5) << endl;

  return f;
}


// ----------------------------------------------------------------------
TF1* PixInitFunc::weibullCdf(TH1 *h) {
  fDoNotFit = false;

  double hi = h->FindLastBinAbove(0.9*h->GetMaximum());
  double lo = h->GetBinLowEdge(1); 
  // require 3 consecutive bins at zero
  for (int i = 3; i < h->GetNbinsX(); ++i) {
    if (h->GetBinContent(i-2) < 1 && h->GetBinContent(i-1) < 1 && h->GetBinContent(i) < 1) {
      lo = h->GetBinLowEdge(i-2);
      break;
    }
  }

  // -- setup function
  TF1* f = (TF1*)gROOT->FindObject("PIF_weibullCdf");
  if (0 == f) {
    f = new TF1("PIF_weibullCdf", PIF_weibullCdf, h->GetBinLowEdge(1), h->GetBinLowEdge(h->GetNbinsX()+1), 3);
    f->SetParNames("norm", "scale", "shape");                       
    f->SetNpx(1000);
    f->SetRange(lo, hi); 
  } else {
    f->ReleaseParameter(0);     
    f->ReleaseParameter(1);     
    f->ReleaseParameter(2);     
    f->ReleaseParameter(3); 
    f->SetRange(lo, hi); 
  }
  
  f->SetParameter(0, h->GetMaximum()); 
  f->SetParameter(1, h->GetBinCenter(h->FindFirstBinAbove(0.5*h->GetMaximum()))); 
  f->SetParameter(2, 1.5); 
  
  return f; 
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
    f->SetNpx(1000);
    f->SetRange(lo, hi); 
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




// ----------------------------------------------------------------------
void PixInitFunc::initExpo(double &p0, double &p1, TH1 *h) {

  int EDG(4), NB(EDG+1); 
  int lbin(1), hbin(h->GetNbinsX()+1); 
  if (fLo < fHi) {
    lbin = h->FindBin(fLo); 
    hbin = h->FindBin(fHi); 
  }
  
  double dx = h->GetBinLowEdge(hbin) - h->GetBinLowEdge(lbin);
  double ylo = h->Integral(lbin, lbin+EDG)/NB; 
  double yhi = h->Integral(hbin-EDG, hbin)/NB;

  if (ylo > 0 && yhi > 0) {
    p1 = (TMath::Log(yhi) - TMath::Log(ylo))/dx; 
    p0 = ylo/TMath::Exp(p1*fLo); 
  } else {
    if (yhi > ylo) {
      p1 = 1.;
    } else {
      p1 = -1.;
    }
    p0 = 50.;
  }

  cout << "fLo: " << fLo << " fHi: " << fHi << endl;
  cout << "ylo: " << ylo << " yhi: " << yhi << endl;
  cout << "p0:  " << p0 <<  " p1:  " << p1 << endl;

}

// ----------------------------------------------------------------------
void PixInitFunc::initPol1(double &p0, double &p1, TH1 *h) {

  int EDG(4), NB(EDG+1); 
  int lbin(1), hbin(h->GetNbinsX()+1); 
  if (fLo < fHi) {
    lbin = h->FindBin(fLo); 
    hbin = h->FindBin(fHi); 
  }
  
  double dx = h->GetBinLowEdge(hbin) - h->GetBinLowEdge(lbin);
  double ylo = h->Integral(lbin, lbin+EDG)/NB; 
  double yhi = h->Integral(hbin-EDG, hbin)/NB;

  //  cout << "ylo: " << ylo << " yhi: " << yhi << endl;

  p1  = (yhi-ylo)/dx;
  p0  = ylo - p1*fLo;
}

