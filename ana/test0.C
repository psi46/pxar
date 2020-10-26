void test0(string filename, string mod = "test1") {
  TFile *f = TFile::Open(filename.c_str());

  gStyle->SetOptStat(0);

  c0.Clear();
  c0.Divide(4,5);

  c0.cd(1);
  TH1D *h1 = (TH1D*)f->Get("Pretest/VanaSettings_V0");
  if (h1) {
    h1->Draw("hist");
  }

  TH2D *h2(0);
  TH1D *h1x = new TH1D("h1x", "caldel", 16, 0., 16.); h1x->Sumw2(); h1x->SetMinimum(0.); h1x->SetMaximum(200.);
  TH1D *h1y = new TH1D("h1y", "vthrcomp", 16, 0., 16.); h1y->Sumw2();h1y->SetMinimum(0.); h1y->SetMaximum(200.);
  for (int i = 0; i < 16; ++i) {
    h2 = (TH2D*)f->Get(Form("Pretest/pretestVthrCompCalDel_c12_r22_C%d_V0", i));
    if (h2) {
      h1x->SetBinContent(i+1, h2->GetMean(1)); h1x->SetBinError(i+1, h2->GetRMS(1));
      h1y->SetBinContent(i+1, h2->GetMean(2)); h1y->SetBinError(i+1, h2->GetRMS(2));
    } else {
      h1x->SetBinContent(i+1, 0.);
      h1y->SetBinContent(i+1, 0.);
    }
  }

  c0.cd(2);
  h1x->Draw();
  c0.cd(3);
  h1y->Draw();

  int c(5);
  for (int i = 0; i < 16; ++i) {
    c0.cd(c);
    h2 = (TH2D*)f->Get(Form("PixelAlive/PixelAlive_C%d_V0;1", i));
    if (h2) h2->Draw("col");
    ++c;
  }

  string pdfname = filename;
  pdfname.replace(pdfname.find(".root"), 5, Form("-%s.pdf", mod.c_str()));
  cout << "pdfname = " << pdfname << endl;
  c0.SaveAs(pdfname.c_str());
}
