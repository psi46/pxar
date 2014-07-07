void fitgp(int idx = 0, double p0 = -1., double p1 = -1., double p2 = -1., double p3 = -1) {
  TFile *file = TFile::Open("roc/pxar.root");

  TH1D *h1(0); 
  int row(0), col(0); 

  PixInitFunc *fPIF = new PixInitFunc();
  TF1 *f(0), *f1(0); 

  col = idx/80;
  row = idx%80;
  h1 = (TH1D*)file->Get(Form("GainPedestal/gainPedestal_c%d_r%d_C0_V0", col, row)); 
  f =  fPIF->gpTanH(h1); 
  if (p0 > 0.) f->SetParameter(0, p0); 
  if (p1 > 0.) f->SetParameter(1, p1); 
  if (p2 > 0.) f->SetParameter(2, p2); 
  if (p3 > 0.) f->SetParameter(3, p3); 

  
  f->SetLineColor(kBlue); 
  
  h1->Fit(f); 

}
