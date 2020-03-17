#include "anaPHValidation.hh"

#include <algorithm>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdlib>

#include <TROOT.h>
#include <TFile.h>
#include <TKey.h>
#include <TSystem.h>
#include <TStyle.h>
#include <TPaveStats.h>
#include <TFitResult.h>
#if defined(WIN32)
#else
#include <TUnixSystem.h>
#endif

#include "PixUtil.hh"
#include "PixInitFunc.hh"

using namespace std;

// ----------------------------------------------------------------------
anaPHValidation::anaPHValidation(string dir, int nrocs): fNrocs(16), fDirectory(dir), fModule("nada") {
  cout << "anaPHValidation ctor, nrocs = " << fNrocs << " directory = " << fDirectory << endl;
  c0 = (TCanvas*)gROOT->FindObject("c0");
  if (!c0) c0 = new TCanvas("c0","--c0--",0,0,656,700);

  fHists.clear();

}

// ----------------------------------------------------------------------
anaPHValidation::~anaPHValidation() {
  cout << "anaPHValidation dtor" << endl;
  fHists.clear();
}

// ----------------------------------------------------------------------
void anaPHValidation::cleanup() {
  map<string, TH1D*>::iterator end = fhsummary.end();
  for (map<string, TH1D*>::iterator il = fhsummary.begin(); il != end; ++il) {
    delete il->second;
  }
  fhsummary.clear();

  map<string, TH2D*>::iterator end2 = fh2summary.end();
  for (map<string, TH2D*>::iterator il = fh2summary.begin(); il != end2; ++il) {
    delete il->second;
  }
  fh2summary.clear();

  fhproblems.clear();

  map<string, TH1D*>::iterator end3 = fHists.end();
  int cnt(0);
  for (map<string, TH1D*>::iterator il = fHists.begin(); il != end3; ++il) {
    il->second->Reset();
    ++cnt;
  }
  cout << " reset " << cnt << " histograms" << endl;

}


// ----------------------------------------------------------------------
void anaPHValidation::makeAll(string basedir, string basename, int mode) {

  vector<string> modules = glob(basedir, "M");

  for (unsigned int im = 0; im < modules.size(); ++im) {
    cout << modules[im] << endl;
    if (im > 0) cleanup();
    string directory = basedir + modules[im];
    readAsciiFiles(directory, (im==0)?true:false);
    if (0 == mode) {
      fitErr();
    } else if (1 == mode) {
      fitPol1();
    }
  }
}

// ----------------------------------------------------------------------
void anaPHValidation::makeOneModule(string directory, int mode) {
  readAsciiFiles(directory, true);
  if (0 == mode) {
    fitErr();
  } else if (1 == mode) {
    fitPol1();
  }
}


// ----------------------------------------------------------------------
void anaPHValidation::compareAllDACs(string basename, string dacbase, string dir1, string dir2) {
  compareDAC("caldel", 0, 250, basename, dacbase, dir1, dir2);
  compareDAC("vana", 50, 150, basename, dacbase, dir1, dir2);

  compareDAC("phscale", 50, 150, basename, dacbase, dir1, dir2);
  compareDAC("phoffset", 100, 200, basename, dacbase, dir1, dir2);

  compareDAC("vtrim", 0, 250, basename, dacbase, dir1, dir2);
  compareDAC("vthrcomp", 0, 250, basename, dacbase, dir1, dir2);

}

// ----------------------------------------------------------------------
void anaPHValidation::compareDAC(string dac, double xmin, double xmax, string modbase, string dacbase, string dir1, string dir2) {

  vector<string> mdir1 = glob(dir1, modbase);
  vector<string> mdir2 = glob(dir2, modbase);

  string ldir1 = dir1;
  if (string("/") == ldir1[ldir1.size()-1]) ldir1.erase(ldir1.size()-1);
  ldir1 = ldir1.substr(ldir1.rfind("/")+1);
  string ldir2 = dir2;
  if (string("/") == ldir2[ldir2.size()-1]) ldir2.erase(ldir2.size()-1);
  ldir2 = ldir2.substr(ldir2.rfind("/")+1);

  vector<string>
    *mdir = &mdir2,
    *ndir = &mdir1;

  // -- find common modules in the two directories
  if (mdir1.size() < mdir2.size()) {
    mdir = &mdir1;
    ndir = &mdir2;
  }

  vector<string> modules;
  for (unsigned int i = 0; i < mdir->size(); ++i) {
    for (unsigned int j = 0; j < ndir->size(); ++j) {
      if (mdir->at(i) == ndir->at(j)) {
	modules.push_back(mdir->at(i));
      }
    }
  }

  TH1D *h1 = new TH1D("hdac1", Form("%s: (%s)", dac.c_str(), ldir1.c_str()), static_cast<int>(xmax-xmin), xmin, xmax);
  setTitles(h1, Form("%s", dac.c_str()), "Entries / Bin", 0.05, 1.0);
  setFilledHist(h1);
  h1->SetMinimum(1.e-3);
  TH1D *h2 = new TH1D("hdac2", Form("%s: (%s)", dac.c_str(), ldir2.c_str()), static_cast<int>(xmax-xmin), xmin, xmax);
  setTitles(h2, Form("%s", dac.c_str()), "Entries / Bin", 0.05, 1.0);
  setFilledHist(h2);
  h2->SetMinimum(1.e-3);

  TH1D *hdiff = new TH1D("hdiff", Form("%s: (%s) - (%s)", dac.c_str(), ldir2.c_str(), ldir1.c_str()), 100, -50., 50);
  setTitles(hdiff, Form("#Delta %s: (%s) - (%s)", dac.c_str(), ldir2.c_str(), ldir1.c_str()), "Entries / Bin", 0.03, 1.4, 1.4);
  setFilledHist(hdiff); hdiff->SetTitleSize(0.05, "y");
  hdiff->SetMinimum(1.e-3);
  TH2D *hcorr = new TH2D("hcorr", Form("%s: (%s) vs (%s)", dac.c_str(), ldir2.c_str(), ldir1.c_str()), 40, xmin, xmax, 40, xmin, xmax);
  setTitles(hcorr, Form("%s (%s)", dac.c_str(), ldir1.c_str()), Form("%s (%s)", dac.c_str(), ldir2.c_str()), 0.03, 1.4, 2.0);
  setFilledHist(hcorr, kBlack, kBlue);

  for (unsigned int imod = 0; imod < modules.size(); ++imod) {
    for (unsigned int iroc = 0; iroc < 16; ++iroc) {
      double val1(-1.), val2(-1.);
      string dacfile1 = Form("%s/%s/%s_C%d.dat", dir1.c_str(), modules[imod].c_str(), dacbase.c_str(), iroc);
      string dacfile2 = Form("%s/%s/%s_C%d.dat", dir2.c_str(), modules[imod].c_str(), dacbase.c_str(), iroc);
      val1 = static_cast<double>(readDacFromFile(dac, dacfile1));
      val2 = static_cast<double>(readDacFromFile(dac, dacfile2));
      h1->Fill(val1);
      h2->Fill(val2);
      hdiff->Fill(val2 - val1);
      hcorr->Fill(val1, val2);
      cout << dac << ": " << val1 << "/" << val2 << " from " << dacfile1 << " and " << dacfile2 << endl;
    }
  }

  c0->Clear();
  c0->Divide(2,2);
  c0->cd(1);
  shrinkPad(0.1, 0.15);
  h1->Draw();
  c0->cd(2);
  shrinkPad(0.1, 0.15);
  h2->Draw();
  c0->cd(3);
  shrinkPad(0.1, 0.15);
  hdiff->Draw();
  c0->cd(4);
  shrinkPad(0.1, 0.15);
  hcorr->Draw("box");

  TPaveStats *ps = (TPaveStats*)hcorr->GetListOfFunctions()->FindObject("stats");
  if (ps) {
    ps->SetX1NDC(0.5); ps->SetX2NDC(0.90);
    ps->SetY1NDC(0.15); ps->SetY2NDC(0.45);
  } else {
    cout << "did not find TPaveStats!" << endl;
  }

  TLine *tl = new TLine();
  tl->DrawLine(xmin, xmin, xmax, xmax);

  c0->Modified();
  c0->Update();

  c0->SaveAs(Form("%s/phval-compareDAC-%s.pdf", fDirectory.c_str(), dac.c_str()));


}


// ----------------------------------------------------------------------
void anaPHValidation::fitPixel(std::string directory, int roc, int col, int row) {

  static bool readAscii(false);
  if (!readAscii) {
    readAscii = true;
    readAsciiFiles(directory, true);
  }

  PixInitFunc pif;
  TF1 *f(0);
  TH1D *h(0);

  map<string, TH1D*>::iterator end3 = fHists.end();
  int lroc, lcol, lrow;
  for (map<string, TH1D*>::iterator il = fHists.begin(); il != end3; ++il) {
    PixUtil::str2rcr(il->first, lroc, lcol, lrow);
    if (roc > -1 && lroc != roc) continue;
    if (col > -1 && lcol != col) continue;
    if (row > -1 && lrow != row) continue;
    h = il->second;
    if (0 == h) break;
    if (h->GetMaximum() < 1) {
      cout << "skipping " << h->GetName()  << " with no entries" << endl;
      continue;
    }

    f = pif.gpErr(h);
    cout << "fitting " <<  h->GetName() << endl;
    gStyle->SetOptFit();
    h->SetTitle(Form("%s (%s)", h->GetTitle(), fModule.c_str()));
    h->Fit(f, "");

    c0->Modified();
    c0->Update();
    TPaveStats *ps = (TPaveStats*)c0->GetPrimitive("stats");
    if (ps) {
      ps->SetX1NDC(0.5); ps->SetX2NDC(0.90);
      ps->SetY1NDC(0.15); ps->SetY2NDC(0.45);
    } else {
      cout << "did not find TPaveStats!" << endl;
    }
    c0->Modified();
    c0->Update();
    break;
  }

  c0->SaveAs(Form("%s/phval-fitpixel-%s-c%d_r%d_C%d.pdf", fDirectory.c_str(), fModule.c_str(), col, row, roc));

}

// ----------------------------------------------------------------------
void anaPHValidation::test(double y0, double y1) {
}



// ----------------------------------------------------------------------
void anaPHValidation::fitTanH(int roc, int col, int row, bool draw) {
}



// ----------------------------------------------------------------------
void anaPHValidation::fitPol1(int roc, int col, int row, bool draw) {
  int lroc, lcol, lrow;

  PixInitFunc pif;
  double xmin(80.), xmax(631.);
  pif.fLo = xmin;
  pif.fHi = xmax;
  TF1 *f(0);
  TH1D *h(0);
  TH2D *h2(0);

  // -- define histograms per ROC and below per module. NO histograms are defined for EVERYTHING.
  for (int i = 0; i < fNrocs; ++i) {
    h = new TH1D(Form("cd_%s_C%d", fModule.c_str(), i), Form("chi2/dof for MOD %s ROC%d, pol1 ", fModule.c_str(), i), 100, 0., 5.);
    fhsummary.insert(make_pair(Form("cd_%s_C%d", fModule.c_str(), i), h));

    h = new TH1D(Form("p0_%s_C%d", fModule.c_str(), i), Form("p0 (pedestal) MOD %s ROC %d, pol1 ", fModule.c_str(), i), 100, 0., 100.);
    fhsummary.insert(make_pair(Form("p0_%s_C%d", fModule.c_str(), i), h));

    h = new TH1D(Form("p1_%s_C%d", fModule.c_str(), i), Form("p1 (slope) MOD %s ROC %d, pol1 ", fModule.c_str(), i), 200, 0., 1.);
    fhsummary.insert(make_pair(Form("p1_%s_C%d", fModule.c_str(), i), h));

    h2 = new TH2D(Form("map_p0_%s_C%d", fModule.c_str(), i), Form("p0 (pedestal) MOD %s ROC %d, pol1 ", fModule.c_str(), i), 52, 0., 52., 80, 0., 80.);
    h2->SetMinimum(0.);    h2->SetMaximum(100.);
    fh2summary.insert(make_pair(Form("map_p0_%s_C%d", fModule.c_str(), i), h2));

    h2 = new TH2D(Form("map_p1_%s_C%d", fModule.c_str(), i), Form("p1 (slope) MOD %s ROC %d, pol1 ", fModule.c_str(), i), 52, 0., 52., 80, 0., 80.);
    h2->SetMinimum(0.);    h2->SetMaximum(0.3);
    fh2summary.insert(make_pair(Form("map_p1_%s_C%d", fModule.c_str(), i), h2));

  }

  h = new TH1D(Form("cd_%s", fModule.c_str()), Form("chi2/dof module %s, pol1 ", fModule.c_str()), 100, 0., 5.);
  fhsummary.insert(make_pair(Form("cd_%s", fModule.c_str()), h));
  TH1D *h1modcd = h;

  h = new TH1D(Form("p0_%s", fModule.c_str()), Form("p0 (pedestal) module %s, pol1 ", fModule.c_str()), 100, 0., 100.);
  fhsummary.insert(make_pair(Form("p0_%s", fModule.c_str()), h));
  TH1D *h1modp0 = h;

  h = new TH1D(Form("p1_%s", fModule.c_str()), Form("p1 (slope) module %s, pol1 ", fModule.c_str()), 200, 0., 1.);
  fhsummary.insert(make_pair(Form("p1_%s", fModule.c_str()), h));
  TH1D *h1modp1 = h;

  h2 = new TH2D(Form("map_p0_%s", fModule.c_str()), Form("p0 (pedestal) module %s, pol1 ", fModule.c_str()), 52, 0., 52., 80, 0., 80.);
  h2->SetMinimum(0.);    h2->SetMaximum(100.);
  fh2summary.insert(make_pair(Form("map_p0_%s", fModule.c_str()), h2));
  TH2D *h2modsump0 = h2;

  h2 = new TH2D(Form("map_p1_%s", fModule.c_str()), Form("p1 (slope) module %s, pol1 ", fModule.c_str()), 52, 0., 52., 80, 0., 80.);
  h2->SetMinimum(0.);
  fh2summary.insert(make_pair(Form("map_p1_%s", fModule.c_str()), h2));
  TH2D *h2modsump1 = h2;


  TH1D *hcd, *hp0, *hp1;
  TH2D *h2p0, *h2p1;
  int cnt(0);
  map<string, TH1D*>::iterator end3 = fHists.end();
  for (map<string, TH1D*>::iterator il = fHists.begin(); il != end3; ++il) {
    ++cnt;
    if (0 == cnt%10000) cout << "Fit #" << cnt << endl;
    PixUtil::str2rcr(il->first, lroc, lcol, lrow);
    if (roc > -1 && lroc != roc) continue;
    if (col > -1 && lcol != col) continue;
    if (row > -1 && lrow != row) continue;

    hcd = fhsummary[Form("cd_%s_C%d", fModule.c_str(), lroc)];
    hp0 = fhsummary[Form("p0_%s_C%d", fModule.c_str(), lroc)];
    hp1 = fhsummary[Form("p1_%s_C%d", fModule.c_str(), lroc)];
    h2p0 = fh2summary[Form("map_p0_%s_C%d", fModule.c_str(), lroc)];
    h2p1 = fh2summary[Form("map_p1_%s_C%d", fModule.c_str(), lroc)];

    h = il->second;
    f = pif.gpPol1(h);
    if (0 == h) break;
    if (h->GetMaximum() < 1) {
      cout << "skipping " << h->GetName()  << " with no entries" << endl;
      continue;
    }

    if (draw) {
      cout << "fitting " <<  h->GetName() << endl;
      gStyle->SetOptFit();
    }
    h->Fit(f, (draw?"r":"rq"), "", xmin, xmax);
    double cd = f->GetChisquare()/f->GetNDF();
    double p0 = f->GetParameter(0);
    double p1 = f->GetParameter(1);
    hcd->Fill(cd);
    h1modcd->Fill(cd);
    hp0->Fill(p0);
    h1modp0->Fill(p0);
    hp1->Fill(p1);
    h1modp1->Fill(p1);
    // -- maps
    h2p0->Fill(lcol, lrow, p0);
    h2p1->Fill(lcol, lrow, p1);
    h2modsump0->Fill(lcol, lrow, p0);
    h2modsump1->Fill(lcol, lrow, p1);

    if (draw) {
      c0->Modified();
      c0->Update();
      TPaveStats *ps = (TPaveStats*)c0->GetPrimitive("stats");
      if (ps) {
	ps->SetX1NDC(0.5); ps->SetX2NDC(0.90);
	ps->SetY1NDC(0.15); ps->SetY2NDC(0.45);
      } else {
	cout << "did not find TPaveStats!" << endl;
      }
      c0->Modified();
      c0->Update();
    }
    if ((f->GetNDF() > 0) && f->GetChisquare()/f->GetNDF() > 20) {
      cout << "problem: " << h->GetName() << endl;
      fhproblems.push_back(h);
    }
  }

  h2modsump0->Scale(1./16.);
  h2modsump1->Scale(1./16.);

  if ((roc > -1) && (col > -1) && (row > -1)) return;

  c0->Clear();

  // -- plot summaries
  map<string, TH1D*>::iterator end = fhsummary.end();
  for (map<string, TH1D*>::iterator il = fhsummary.begin(); il != end; ++il) {
    if (il->second->GetEntries() < 1) continue;
    il->second->Draw();
    c0->SaveAs(Form("%s/gp-%s.pdf", fDirectory.c_str(), il->second->GetName()));
  }

  map<string, TH2D*>::iterator end2 = fh2summary.end();
  gStyle->SetOptStat(0);
  for (map<string, TH2D*>::iterator il = fh2summary.begin(); il != end2; ++il) {
    if (il->second->GetEntries() < 1) continue;
    il->second->Draw("colz");
    c0->SaveAs(Form("%s/gp-%s.pdf", fDirectory.c_str(), il->second->GetName()));
  }



  gStyle->SetOptFit(1);
  for (unsigned int i = 0; i < fhproblems.size(); ++i) {
    fhproblems[i]->Draw();
    c0->Modified();
    c0->Update();
    TPaveStats *ps = (TPaveStats*)c0->GetPrimitive("stats");
    if (ps) {
      ps->SetX1NDC(0.5); ps->SetX2NDC(0.90);
      ps->SetY1NDC(0.15); ps->SetY2NDC(0.45);
    } else {
      cout << "did not find TPaveStats!" << endl;
    }
    c0->Modified();
    c0->Update();
    c0->SaveAs(Form("%s/gp-problem-%s-%s.pdf", fDirectory.c_str(), fModule.c_str(), fhproblems[i]->GetName()));
  }



}



// ----------------------------------------------------------------------
void anaPHValidation::fitErr(int roc, int col, int row, bool draw) {
  int lroc, lcol, lrow;

  PixInitFunc pif;
  TF1 *f(0);
  TH1D *h(0);
  TH2D *h2(0);

  // -- define histograms per ROC and below per module. NO histograms are defined for EVERYTHING.
  for (int i = 0; i < fNrocs; ++i) {
    h = new TH1D(Form("cd_%s_C%d", fModule.c_str(), i), Form("chi2/dof for MOD %s ROC%d, errf ", fModule.c_str(), i), 100, 0., 20.);
    fhsummary.insert(make_pair(Form("cd_%s_C%d", fModule.c_str(), i), h));

    h = new TH1D(Form("p0_%s_C%d", fModule.c_str(), i), Form("p0 (step) MOD %s ROC %d, errf ", fModule.c_str(), i), 100, 0., 1000.);
    fhsummary.insert(make_pair(Form("p0_%s_C%d", fModule.c_str(), i), h));

    h = new TH1D(Form("p1_%s_C%d", fModule.c_str(), i), Form("p1 (slope) MOD %s ROC %d, errf ", fModule.c_str(), i), 120, 0., 1200.);
    fhsummary.insert(make_pair(Form("p1_%s_C%d", fModule.c_str(), i), h));

    h = new TH1D(Form("p2_%s_C%d", fModule.c_str(), i), Form("p2 (floor) MOD %s ROC %d, errf ", fModule.c_str(), i), 100, 0., 10.);
    fhsummary.insert(make_pair(Form("p2_%s_C%d", fModule.c_str(), i), h));

    h = new TH1D(Form("p3_%s_C%d", fModule.c_str(), i), Form("p3 (half plateau) MOD %s ROC %d, errf ", fModule.c_str(), i), 100, 0., 200.);
    fhsummary.insert(make_pair(Form("p3_%s_C%d", fModule.c_str(), i), h));

    h2 = new TH2D(Form("curve_%s_C%d", fModule.c_str(), i), Form("curve MOD %s ROC %d, errf ", fModule.c_str(), i), 200, 0., 2000., 256, 0., 256.);
    h2->SetMaximum(500);
    fh2summary.insert(make_pair(Form("curve_%s_C%d", fModule.c_str(), i), h2));
  }

  h = new TH1D(Form("cd_%s", fModule.c_str()), Form("chi2/dof module %s, errf ", fModule.c_str()), 100, 0., 20.);
  fhsummary.insert(make_pair(Form("cd_%s", fModule.c_str()), h));
  TH1D *h1modcd = h;

  h = new TH1D(Form("p0_%s", fModule.c_str()), Form("p0 (step) module %s, errf ", fModule.c_str()), 100, 0., 1000.);
  fhsummary.insert(make_pair(Form("p0_%s", fModule.c_str()), h));
  TH1D *h1modp0 = h;

  h = new TH1D(Form("p1_%s", fModule.c_str()), Form("p1 (slope) module %s, errf ", fModule.c_str()), 120, 0., 1200.);
  fhsummary.insert(make_pair(Form("p1_%s", fModule.c_str()), h));
  TH1D *h1modp1 = h;

  h = new TH1D(Form("p2_%s", fModule.c_str()), Form("p2 (floor) module %s, errf ", fModule.c_str()), 100, 0., 10.);
  fhsummary.insert(make_pair(Form("p2_%s", fModule.c_str()), h));
  TH1D *h1modp2 = h;

  h = new TH1D(Form("p3_%s", fModule.c_str()), Form("p3 (half plateau) module %s, errf ", fModule.c_str()), 100, 0., 200.);
  fhsummary.insert(make_pair(Form("p3_%s", fModule.c_str()), h));
  TH1D *h1modp3 = h;

  h2 = new TH2D(Form("curve_%s", fModule.c_str()), Form("curve module %s, errf ", fModule.c_str()), 200, 0., 2000., 256, 0., 256.);
  fh2summary.insert(make_pair(Form("curve_%s", fModule.c_str()), h2));
  TH2D *h2modsum = h2;

  TH1D *hcd, *hp0, *hp1, *hp2, *hp3;
  TH2D *h2cu;
  int cnt(0);
  map<string, TH1D*>::iterator end3 = fHists.end();
  for (map<string, TH1D*>::iterator il = fHists.begin(); il != end3; ++il) {
    ++cnt;
    if (0 == cnt%10000) cout << "Fit #" << cnt << endl;
    PixUtil::str2rcr(il->first, lroc, lcol, lrow);
    if (roc > -1 && lroc != roc) continue;
    if (col > -1 && lcol != col) continue;
    if (row > -1 && lrow != row) continue;

    hcd = fhsummary[Form("cd_%s_C%d", fModule.c_str(), lroc)];
    hp0 = fhsummary[Form("p0_%s_C%d", fModule.c_str(), lroc)];
    hp1 = fhsummary[Form("p1_%s_C%d", fModule.c_str(), lroc)];
    hp2 = fhsummary[Form("p2_%s_C%d", fModule.c_str(), lroc)];
    hp3 = fhsummary[Form("p3_%s_C%d", fModule.c_str(), lroc)];
    h2cu= fh2summary[Form("curve_%s_C%d", fModule.c_str(), lroc)];

    h = il->second;
    if (0 == h) break;
    if (h->GetMaximum() < 1) {
      cout << "skipping " << h->GetName()  << " with no entries" << endl;
      continue;
    }

    f = pif.gpErr(h);
    if (draw) {
      cout << "fitting " <<  h->GetName() << endl;
      gStyle->SetOptFit();
    }
    h->Fit(f, (draw?"":"q"));
    hcd->Fill(f->GetChisquare()/f->GetNDF());
    h1modcd->Fill(f->GetChisquare()/f->GetNDF());
    hp0->Fill(f->GetParameter(0));
    h1modp0->Fill(f->GetParameter(0));
    hp1->Fill(f->GetParameter(1));
    h1modp1->Fill(f->GetParameter(1));
    hp2->Fill(f->GetParameter(2));
    h1modp2->Fill(f->GetParameter(2));
    hp3->Fill(f->GetParameter(3));
    h1modp3->Fill(f->GetParameter(3));
    for (int i = 0; i < h2modsum->GetNbinsX(); ++i) {
      double vcal = h2modsum->GetXaxis()->GetBinCenter(i + 1);
      double ph = f->Eval(vcal);
      h2cu->Fill(vcal, ph);
      h2modsum->Fill(vcal, ph);
    }

    if (draw) {
      c0->Modified();
      c0->Update();
      TPaveStats *ps = (TPaveStats*)c0->GetPrimitive("stats");
      if (ps) {
	ps->SetX1NDC(0.5); ps->SetX2NDC(0.90);
	ps->SetY1NDC(0.15); ps->SetY2NDC(0.45);
      } else {
	cout << "did not find TPaveStats!" << endl;
      }
      c0->Modified();
      c0->Update();
    }
    if ((f->GetNDF() > 0) && f->GetChisquare()/f->GetNDF() > 20) {
      cout << "problem: " << h->GetName() << endl;
      fhproblems.push_back(h);
    }
  }


  if ((roc > -1) && (col > -1) && (row > -1)) return;

  c0->Clear();

  // -- plot summaries
  map<string, TH1D*>::iterator end = fhsummary.end();
  for (map<string, TH1D*>::iterator il = fhsummary.begin(); il != end; ++il) {
    if (il->second->GetEntries() < 1) continue;
    il->second->Draw();
    c0->SaveAs(Form("%s/phval-%s.pdf", fDirectory.c_str(), il->second->GetName()));
  }

  map<string, TH2D*>::iterator end2 = fh2summary.end();
  gStyle->SetOptStat(0);
  for (map<string, TH2D*>::iterator il = fh2summary.begin(); il != end2; ++il) {
    if (il->second->GetEntries() < 1) continue;
    double entries = il->second->ProjectionY("blepy", 40, 40)->Integral();
    il->second->SetTitle(Form("%s (N = %5.0f)", il->second->GetTitle(), entries));
    il->second->Draw("colz");
    c0->SaveAs(Form("%s/phval-%s.pdf", fDirectory.c_str(), il->second->GetName()));
  }

  gStyle->SetOptFit(1);
  for (unsigned int i = 0; i < fhproblems.size(); ++i) {
    fhproblems[i]->Draw();
    c0->Modified();
    c0->Update();
    TPaveStats *ps = (TPaveStats*)c0->GetPrimitive("stats");
    if (ps) {
      ps->SetX1NDC(0.5); ps->SetX2NDC(0.90);
      ps->SetY1NDC(0.15); ps->SetY2NDC(0.45);
    } else {
      cout << "did not find TPaveStats!" << endl;
    }
    c0->Modified();
    c0->Update();
    c0->SaveAs(Form("%s/phval-problem-%s-%s.pdf", fDirectory.c_str(), fModule.c_str(), fhproblems[i]->GetName()));
  }
}

// ----------------------------------------------------------------------
void anaPHValidation::readAsciiFiles(string directory, bool createHists) {
  // -- set module name
  fModule = directory.substr(directory.find_last_of("/", directory.size()-2)+1);
  PixUtil::replaceAll(fModule, "/", "");
  cout << "fModule ->" << fModule << "<-" << endl;

  vector<string> files = glob(directory);

  TH1D *h1(0);

  ifstream IN;
  char buffer[1000];
  string sline, sval;
  int val(0);
  string::size_type s0, s1;
  vector<int> x;
  string srow, scol, hname;
  int iroc(-1), irow(-1), icol(-1);
  for (unsigned int ifile = 0; ifile < files.size(); ++ifile) {
    cout << "Reading " << files[ifile] ;
    x.clear();
    s0 = files[ifile].rfind("_C");
    s1 = files[ifile].rfind(".dat");
    sval = files[ifile].substr(s0+2, s1-s0-2);
    iroc = atoi(sval.c_str());
    IN.open(Form("%s/%s", directory.c_str(), files[ifile].c_str()));
    while (IN.getline(buffer, 1000, '\n')) {
      sline = buffer;
      if (string::npos != sline.find("Low range:")) {
	sline = sline.substr(string("Low range: ").length());
	istringstream istring(sline);
	while (istring >> sval) {
	  val = atoi(sval.c_str());
	  x.push_back(val);
	}
      }
      if (string::npos != sline.find("High range:")) {
	sline = sline.substr(string("High range: ").length());
	istringstream istring(sline);
	while (istring >> sval) {
	  val = 7*atoi(sval.c_str());
	  x.push_back(val);
	}
	break;
      }
    }

    cout << " ... filling histograms " << endl;
    while (IN.getline(buffer, 1000, '\n')) {
      if (buffer[0] == '#') {continue;}
      if (buffer[0] == '/') {continue;}
      if (buffer[0] == '\n') {continue;}
      sline = buffer;
      if (string::npos == sline.find("Pix")) continue;
      s0 = sline.find("Pix");
      sval = sline.substr(s0+3);
      istringstream istring(sval);
      istring >> scol >> srow;
      icol = atoi(scol.c_str());
      irow = atoi(srow.c_str());

      sval = sline.substr(0, s0);
      istringstream jstring(sline);
      hname = Form("phval_c%d_r%d_C%d", icol, irow, iroc);
      if (createHists) {
	h1 = new TH1D(hname.c_str(), hname.c_str(), 2000, 0., 2000.);
	h1->Sumw2();
	h1->SetMinimum(0.);
	h1->SetMaximum(256.);
	fHists.insert(make_pair(hname, h1));
      } else {
	h1 = fHists[hname];
      }

      // -- fill it:
      for (unsigned int i = 0; i < x.size(); ++i) {
	jstring >> sval;
	int ival = atoi(sval.c_str());
	if (ival > 0) {
	  h1->SetBinContent(x[i]+1, ival);
	  h1->SetBinError(x[i]+1, 0.03*ival);
	}
      }
    }
    IN.close();
  }

}


// ----------------------------------------------------------------------
void anaPHValidation::readRootFile(string filename) {
  TFile *f = TFile::Open(filename.c_str());
  f->cd("GainPedestal");

  //  fHists.reserve(fNrocs*4160);

  TH1D* h(0);
  string hname, sname;
  for (int roc = 0; roc < 16; ++roc) {
    for (int col = 0; col < 52; ++col) {
      for (int row = 0; row < 80; ++row) {
	hname = Form("gainPedestal_c%d_r%d_C%d_V0", col, row, roc);
	h = 0;
	h = (TH1D*)gDirectory->Get(hname.c_str());
	if (h) fHists.insert(make_pair(hname, h));
      }
    }
  }
}



// ----------------------------------------------------------------------
vector<string> anaPHValidation::glob(string directory, string basename) {
  vector<string> lof;
#if defined(WIN32)
#else
  TString fname;
  const char *file;
  TSystem *lunix = gSystem;
  void *pDir = lunix->OpenDirectory(directory.c_str());
  while ((file = lunix->GetDirEntry(pDir))) {
    fname = file;
    if (fname.Contains(basename.c_str())) {
      lof.push_back(string(fname));
    }
  }
#endif
  std::sort(lof.begin(), lof.end());
  return lof;
}


// ----------------------------------------------------------------------
int anaPHValidation::readDacFromFile(string dac, string dacfile) {
  ifstream INS;

  string sline;
  string::size_type s1;
  int val(-1);
  for (int i = 0; i < fNrocs; ++i) {
    INS.open(Form("%s", dacfile.c_str()));
    while (getline(INS, sline)) {
      if (sline[0] == '#') continue;
      if (sline[0] == '/') continue;
      if (sline[0] == '\n') continue;
      if (string::npos == sline.find(dac)) continue;
      s1 = sline.rfind(dac);
      sline = sline.substr(s1+dac.length()+1);
      val = atoi(sline.c_str());
      break;
    }
    INS.close();
  }
  return val;
}


// ----------------------------------------------------------------------
void anaPHValidation::setTitles(TH1 *h, const char *sx, const char *sy, float size,
				float xoff, float yoff, float lsize, int font) {
  if (h == 0) {
    cout << " Histogram not defined" << endl;
  } else {
    h->SetXTitle(sx);                  h->SetYTitle(sy);
    h->SetTitleOffset(xoff, "x");      h->SetTitleOffset(yoff, "y");
    h->SetTitleSize(size, "x");        h->SetTitleSize(size, "y");
    h->SetLabelSize(lsize, "x");       h->SetLabelSize(lsize, "y");
    h->SetLabelFont(font, "x");        h->SetLabelFont(font, "y");
    h->GetXaxis()->SetTitleFont(font); h->GetYaxis()->SetTitleFont(font);
    h->SetNdivisions(508, "X");
  }
}


// ----------------------------------------------------------------------
void anaPHValidation::setHist(TH1 *h, Int_t color, Int_t symbol, Double_t size, Double_t width) {
  h->SetLineColor(color);   h->SetLineWidth(width);
  h->SetMarkerColor(color); h->SetMarkerStyle(symbol);  h->SetMarkerSize(size);
  h->SetStats(kFALSE);
  h->SetFillStyle(0); h->SetFillColor(color);
}

// ----------------------------------------------------------------------
void anaPHValidation::setFilledHist(TH1 *h, Int_t color, Int_t fillcolor, Int_t fillstyle, Int_t width) {
  // Note: 3004, 3005 are crosshatches
  // ----- 1000       is solid
  //       kYellow    comes out gray on bw printers
  h->SetLineColor(color);     h->SetLineWidth(width);
  h->SetFillStyle(fillstyle); h->SetFillColor(fillcolor);
}


// ----------------------------------------------------------------------
void anaPHValidation::shrinkPad(double b, double l, double r, double t) {
  gPad->SetBottomMargin(b);
  gPad->SetLeftMargin(l);
  gPad->SetRightMargin(r);
  gPad->SetTopMargin(t);
}
