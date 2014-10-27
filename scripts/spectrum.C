void spectrum() {

  //  TFile *f = TFile::Open("/scratch/ursl/pxar/rongshyang-pxar_bb_Oct01.root"); 
  TFile *f = TFile::Open("M0001/pxar.root"); 

  f->cd("BumpBonding"); 

  TH1D *h; 
  TSpectrum s;
  int nPeaks(0); 
  zone(4,4);
  double cutDead;
  int bbprob(0); 
  for (int i = 0; i < 16; ++i) {
    bbprob = 0; 
    c0.cd(i+1);
    h = (TH1D*)gDirectory->Get(Form("dist_thr_calSMap_VthrComp_C%d_V0", i)); 
    nPeaks = s.Search(h, 5, "", 0.01); 
    cout << "found " << nPeaks << " peaks in " << h->GetName() << endl; 
    cutDead = fitPeaks(h, s, nPeaks); 
    bbprob = static_cast<int>(h->Integral(cutDead, h->FindBin(255)));
    cout << "dead bumps: " << bbprob << endl;
    h->Draw(); 
    pl->DrawLine(cutDead, 0, cutDead, h->GetMaximum());
  }



}


// ----------------------------------------------------------------------
int fitPeaks(TH1D *h, TSpectrum &s, int npeaks) {

  Float_t *xpeaks = s.GetPositionX();
  string name; 
  double lcuts[3]; lcuts[0] = lcuts[1] = lcuts[2] = 255.;
  TF1 *f(0); 
  double peak, sigma, rms;
  int fittedPeaks(0); 
  for (int p = 0; p < npeaks; ++p) {
    double xp = xpeaks[p];
    if (p > 1) continue;
    if (xp > 200) {
      cout << "do not fit peak at " << xp << endl;
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
  
  cout << "cut for dead bump bonds: " << minbin << " (obtained for minval = " << minval << ")" 
       << " start: " << startbin << " .. " << endbin 
       << " last peak: " << peak << " sigma: " << sigma
       << " lcuts[0] = " << lcuts[0] << " lcuts[1] = " << lcuts[1]
       << endl;
  return minbin+1; 
}
