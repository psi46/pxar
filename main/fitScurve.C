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
  } 

  f->SetRange(lo, hi); 

  cout << "initializing to " << 0.5*h1->GetMaximum() << endl;
  f->SetParameter(0, 0.5*h1->GetMaximum());
  f->SetParameter(1, 30.); 
  f->SetParameter(2, .2); 
  f->SetParameter(3, 0.5*h1->GetMaximum()); 

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

  //  f1->FixParameter(0, 0.5*h1->GetMaximum()); 
  f1->FixParameter(3, 0.5*h1->GetMaximum()); 
  
  h1->Fit(f1, "r"); 
  double sig = 1./(TMath::Sqrt(2.)*f1->GetParameter(2));
  cout << "==> " << sig << endl;
}


// ----------------------------------------------------------------------
void overlay() {

  TFile *f0 = TFile::Open("roc/pxar-ntrig5.root");
  TH1D *h0 = (TH1D*)f0->Get("Scurves/dist_sig_scurveVcal_Vcal_C0_V0");

  TFile *f1 = TFile::Open("roc/pxar-ntrig10.root");
  TH1D *h1 = (TH1D*)f1->Get("Scurves/dist_sig_scurveVcal_Vcal_C0_V0");

  TFile *f2 = TFile::Open("roc/pxar-ntrig50.root");
  TH1D *h2 = (TH1D*)f2->Get("Scurves/dist_sig_scurveVcal_Vcal_C0_V0");

  TFile *f3 = TFile::Open("roc/pxar-ntrig100.root");
  TH1D *h3 = (TH1D*)f3->Get("Scurves/dist_sig_scurveVcal_Vcal_C0_V0");

  TFile *f4 = TFile::Open("roc/pxar-ntrig200.root");
  TH1D *h4 = (TH1D*)f4->Get("Scurves/dist_sig_scurveVcal_Vcal_C0_V0");
  
  h4->Draw();

  h0->SetLineColor(kRed);
  h0->Draw("samehist");
  tl.SetTextColor(kRed);
  tl.DrawLatex(0.6, 0.75, "ntrig = 5"); 

  h1->SetLineColor(kCyan);
  h1->Draw("samehist");
  tl.SetTextColor(kCyan);
  tl.DrawLatex(0.6, 0.70, "ntrig = 10"); 

  h2->SetLineColor(kBlue);
  h2->Draw("samehist");
  tl.SetTextColor(kBlue);
  tl.DrawLatex(0.6, 0.65, "ntrig = 50"); 

  h3->SetLineColor(kGreen);
  h3->Draw("samehist");
  tl.SetTextColor(kGreen);
  tl.DrawLatex(0.6, 0.60, "ntrig = 100"); 

  tl.SetTextColor(kBlack); 
  tl.DrawLatex(0.6, 0.55, "ntrig = 200"); 

  
}
