void test3(string filename, string mod = "") {
  TFile *f = TFile::Open(filename.c_str());

  gStyle->SetOptStat(0);

  TCanvas c0("c0","--c0--",2303,0,656,700);
  TLatex *tl = new TLatex();
  tl->SetNDC(kTRUE);
  tl->SetTextFont(42);

  c0.Clear();
  //  c0.Divide(4,5);
  c0.Divide(2,3);

  // -- VANA
  c0.cd(1);
  TH1D *h1 = (TH1D*)f->Get("Pretest/VanaSettings_V0");
  if (h1) {
    h1->Draw("hist");
  }

  TH2D *h2(0);
  TH1D *h1x = new TH1D("h1x", "caldel", 16, 0., 16.); h1x->Sumw2(); h1x->SetMinimum(0.); h1x->SetMaximum(200.);
  TH1D *h1y = new TH1D("h1y", "vthrcomp", 16, 0., 16.); h1y->Sumw2();h1y->SetMinimum(0.); h1y->SetMaximum(200.);

  for (int i = 0; i < 16; ++i) {
    h2 = (TH2D*)f->Get(Form("Pretest/pretestVthrCompCalDel_c11_r20_C%d_V0", i));
    if (h2) {
      h1x->SetBinContent(i+1, h2->GetMean(1)); h1x->SetBinError(i+1, h2->GetRMS(1));
      h1y->SetBinContent(i+1, h2->GetMean(2)); h1y->SetBinError(i+1, h2->GetRMS(2));
    } else {
      h1x->SetBinContent(i+1, 0.);
      h1y->SetBinContent(i+1, 0.);
    }
  }

  // -- Pojections of CALDEL and VTHRCOMP
  c0.cd(2);
  h1x->Draw();
  c0.cd(3);
  h1y->Draw();

  // // -- All the pixelalive maps
  // int c(5);
  // for (int i = 0; i < 16; ++i) {
  //   c0.cd(c);
  //   h2 = (TH2D*)f->Get(Form("PixelAlive/PixelAlive_C%d_V0;1", i));
  //   if (h2) h2->Draw("col");
  //   ++c;
  // }

  // -- dump all zero-entries of the BB2 test into one TH2D
  TH2D *hall = new TH2D("hall", "Missing BB (complete module)", 52, 0., 52., 80, 0., 80.);
  bool OK(false);
  int nbump(0);
  for (int i = 0; i < 16; ++i) {
    h2 = (TH2D*)f->Get(Form("BB2/BBtestMap_C%d_V0;1", i));
    if (h2) {
      OK = true;
      int nDead(0);
      for (int ix = 1; ix < 53; ++ix) {
	for (int iy = 1; iy < 81; ++iy) {
	  if (h2->GetBinContent(ix, iy) > 1) {
	    ++nbump;
	    ++nDead;
	    cout << Form("%03d ", nDead) << "missing BB in ROC " << i << ": " << ix << "/" << iy << endl;
	    hall->Fill(ix-1, iy-1);
	  }
	}
      }
    }
  }
  hall->SetTitle(Form("Missing BB (complete module: %d)", nbump));
  c0.cd(6);
  if (OK) hall->Draw("colz");

  // -- dump all zero-entries of the alive test into one TH2D
  OK = false;
  TH2D *hall2 = new TH2D("hall2", "Dead pixels (complete module)", 52, 0., 52., 80, 0., 80.);
  int ndead(0);
  for (int i = 0; i < 16; ++i) {
    h2 = (TH2D*)f->Get(Form("PixelAlive/PixelAlive_C%d_V0;1", i));
    if (h2) {
      OK = true;
      for (int ix = 1; ix < 53; ++ix) {
	for (int iy = 1; iy < 81; ++iy) {
	  if (h2->GetBinContent(ix, iy) < 1) {
	    cout << "dead pixel in ROC " << i << ": " << ix << "/" << iy << endl;
	    ++ndead;
	    hall2->Fill(ix-1, iy-1);
	  }
	}
      }
    }
  }
  hall2->SetTitle(Form("Dead pixels (complete module: %d)", ndead));
  c0.cd(5);
  if (OK) hall2->Draw("colz");


  // -- add timestamp of test
  c0.cd(4);
  TH1D *ha = (TH1D*)f->Get("HA");
  string ts = ha->GetTitle();
  ts.replace(ts.find("analog current measurements, start: "), 36, "");
  ts.replace(ts.find(" / sec:"), ts.length() - ts.find(" / sec:"), "");
  ts.replace(ts.rfind("."), ts.length() - ts.rfind("."), "");
  tl->SetTextSize(0.1);
  tl->DrawLatexNDC(0.1, 0.32, "Test start:");
  tl->DrawLatexNDC(0.1, 0.20, ts.c_str());
  ts = filename;
  ts.replace(0, ts.find("/")+1, "");
  ts.replace(ts.find(".root"), 5, "");
  tl->DrawLatexNDC(0.1, 0.80, ts.c_str());


  string pdfname = filename;
  pdfname.replace(pdfname.find(".root"), 5, Form("%s.pdf", mod.c_str()));
  pdfname.replace(pdfname.find("/"), 1, "-");
  cout << "pdfname = " << pdfname << endl;
  c0.SaveAs(pdfname.c_str());
  system(Form("/bin/ls  `pwd`/%s", pdfname.c_str()));
}
