double PIF_errOld(double *x, double *par) {
  return par[0]*TMath::Erf(par[2]*(x[0]-par[1]))+par[3];
} 

// ----------------------------------------------------------------------
TF1* errScurveOld(TH1 *h) {

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
  TF1* f = (TF1*)gROOT->FindObject("PIF_err_old");
  if (0 == f) {
    f = new TF1("PIF_err_old",  PIF_errOld, h->GetBinLowEdge(1), h->GetBinLowEdge(h->GetNbinsX()+1), 4);
    f->SetParNames("p0", "p1", "p2", "p3");                       
    f->SetNpx(1000);
    f->SetRange(lo, hi); 
  } 

  f->SetParameter(0, 2.5);
  f->SetParameter(1, 140.); 
  f->SetParameter(2, .2); 
  f->SetParameter(3, 2.5); 

  return f; 
}


// ----------------------------------------------------------------------
void fitScurve(int idx = 0, double p0 = -1., double p1 = -1., double p2 = -1., double p3 = -1) {
  TFile *file = TFile::Open("roc/pxar.root");

  TH1D *h(0), *h0(0), *h1(0); 
  int row(0), col(0); 

  PixInitFunc *fPIF = new PixInitFunc();
  TF1 *f(0), *f1(0); 

  col = idx/80;
  row = idx%80;
  h = (TH1D*)file->Get(Form("Scurves/scurveVcal_Vcal_c%d_r%d_C0_V0", col, row)); 

  h0 = (TH1D*)h->Clone("h0");
  h1 = (TH1D*)h->Clone("h1");

  zone(1,2);

  if (1) {
    f =  fPIF->errScurve(h0);
    if (p0 > 0.) f->SetParameter(0, p0); 
    if (p1 > 0.) f->SetParameter(1, p1); 
    if (p2 > 0.) f->SetParameter(2, p2); 
    if (p3 > 0.) f->SetParameter(3, p3); 
    
    
    f->SetLineColor(kRed); 
    
    h0->Fit(f); 

    double fSigma = 1./(TMath::Sqrt(2.)/f->GetParameter(1)); 
    cout << "==> " << fSigma << endl;
  }

  c0.cd(2);
  f1 = errScurveOld(h1);
  if (p0 > 0.) f1->SetParameter(0, p0); 
  if (p1 > 0.) f1->SetParameter(1, p1); 
  if (p2 > 0.) f1->SetParameter(2, p2); 
  if (p3 > 0.) f1->SetParameter(3, p3); 
  
  
  f1->SetLineColor(kBlue); 

  f1->FixParameter(0, 0.5*h1->GetMaximum()); 
  f1->FixParameter(3, 0.5*h1->GetMaximum()); 
  
  h1->Fit(f1); 
  double sig = 1./(TMath::Sqrt(2.)*f1->GetParameter(2));
  cout << "==> " << sig << endl;
}
