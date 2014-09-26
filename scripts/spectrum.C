void spectrum() {

  TFile *f = TFile::Open("testModule/pxar.root"); 

  f->cd("BumpBonding"); 

  TH1D *h; 
  TSpectrum s;
  int nPeaks(0); 
  zone(4,4);
  double cutDead;
  for (int i = 0; i < 16; ++i) {
    c0.cd(i+1);
    h = (TH1D*)gDirectory->Get(Form("dist_thr_calSMap_VthrComp_C%d_V0", i)); 
    nPeaks = s.Search(h, 5, "", 0.01); 
    cout << "found " << nPeaks << " peaks in " << h->GetName() << endl; 
    fitPeaks(h, s, nPeaks, cutDead); 
    h->Draw(); 
  }



}


// ----------------------------------------------------------------------
void fitPeaks(TH1D *h, TSpectrum &s, int npeaks, double cutDead) {

  cout << "----------------------------------------------------------------------" << endl;
  Float_t *xpeaks = s.GetPositionX();
  string name; 
  double lcuts[2]; lcuts[0] = lcuts[1] = 255.;
  for (int p = 0; p < npeaks; ++p) {
    double xp = xpeaks[p];
    if (p > 1) continue;
    if (xp > 200) {
      cout << "do not fit peak at " << xp << endl;
      continue;
    }
    name = Form("gauss_%d", p); 
    TF1 *f = new TF1(name.c_str(), "gaus(0)", 0., 256.);
    int bin = h->GetXaxis()->FindBin(xp);
    double yp = h->GetBinContent(bin);
    f->SetParameters(yp, xp, 2.);
    h->Fit(f, "+", "same"); 
    double peak = h->GetFunction(name.c_str())->GetParameter(1); 
    double sigma = h->GetFunction(name.c_str())->GetParameter(2); 
    if (0 == p) {
      lcuts[0] = peak + 3*sigma; 
      lcuts[1] = peak + 5*sigma; // in case there is no second peak, put here 5 sigma position
    } else {
      lcuts[1] = peak - 3*sigma; 
    }
  }
  
  int startbin = (int)(0.5*(lcuts[0] + lcuts[1])); 
  int endbin = (int)(lcuts[1]); 
  int minbin(0), minval(999); 

  cout << "cuts: " << lcuts[0]  << " .. " << lcuts[1] 
       << " startbin = " << startbin << " endbin = " << endbin
       << endl;
  for (int i = startbin; i < endbin; ++i) {
    if (h->GetBinContent(i) < minval) {
      minval = h->GetBinContent(i); 
      minbin = i; 
    }
  }
  
  cout << "cut for dead bump bonds: " << minbin << " (obtained for minval = " << minval << ")" << endl;
  cutDead = minbin; 
}
