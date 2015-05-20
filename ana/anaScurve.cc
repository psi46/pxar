#include "anaScurve.hh"

#include <fstream>
#include <sstream>
#include <cstdlib>

#include <TROOT.h>
#include <TFile.h>
#include <TKey.h>
#include <TSystem.h>
#include <TStyle.h>
#include <TFitResult.h>
#if defined(WIN32)
#else
#include <TUnixSystem.h>
#endif

#include "PixUtil.hh"
#include "PixInitFunc.hh"

using namespace std;

// ----------------------------------------------------------------------
anaScurve::anaScurve(string dir, int nrocs): fNrocs(nrocs), fDirectory(dir) {
  cout << "anaScurve ctor, nrocs = " << fNrocs << " directory = " << fDirectory << endl;
  c0 = (TCanvas*)gROOT->FindObject("c0"); 
  if (!c0) c0 = new TCanvas("c0","--c0--",0,0,656,700);

  fHists.clear();

}

// ----------------------------------------------------------------------
anaScurve::~anaScurve() {
  cout << "anaScurve dtor" << endl;
  fHists.clear();
}


// ----------------------------------------------------------------------
void anaScurve::makeAll(string directory, int /*mode*/) {
  readAsciiFiles(directory); 
  fitErr();
}


// ----------------------------------------------------------------------
void anaScurve::test(double /*y0*/, double /*y1*/) {
  TH1D *h = new TH1D("h", "h", 50, 20., 70.); 
  h->SetMarkerStyle(20); 

  if (1) {

    h->SetBinContent(21, 1.);
    h->SetBinContent(22, 2.);
    h->SetBinContent(23, 15.);
    h->SetBinContent(24, 25.);
    h->SetBinContent(25, 35.);
    h->SetBinContent(26, 50.);
    h->SetBinContent(27, 49.);
    h->SetBinContent(28, 50.);
    h->SetBinContent(29, 48.);
  }


  if (0) {

    h->SetBinContent(1, 2.);
    h->SetBinContent(2, 1.);
    h->SetBinContent(3, 0.);
    h->SetBinContent(4, 0.);
    h->SetBinContent(5, 0.);
    h->SetBinContent(6, 2.);
    h->SetBinContent(7, 1.);
    h->SetBinContent(8, 2.);
    h->SetBinContent(9, 0.);

    h->SetBinContent(10, 6.);
    h->SetBinContent(11, 5.);
    h->SetBinContent(12, 4.);
    h->SetBinContent(13, 5.);
    h->SetBinContent(14, 4.);
    h->SetBinContent(15, 10.);
    h->SetBinContent(16, 8.);
    h->SetBinContent(17, 12.);
    h->SetBinContent(18, 9.);
    h->SetBinContent(19, 14.);

    h->SetBinContent(20, 20.);
    h->SetBinContent(21, 13.);
    h->SetBinContent(22, 14.);
    h->SetBinContent(23, 15.);
    h->SetBinContent(24, 20.);
    h->SetBinContent(25, 24.);
    h->SetBinContent(26, 26.);
    h->SetBinContent(27, 27.);
    h->SetBinContent(28, 30.);
    h->SetBinContent(29, 31.);

    h->SetBinContent(30, 32.);
    h->SetBinContent(31, 32.);
    h->SetBinContent(32, 34.);
    h->SetBinContent(33, 36.);
    h->SetBinContent(34, 35.);
    h->SetBinContent(35, 36.);
    h->SetBinContent(36, 37.);
    h->SetBinContent(37, 38.);
    h->SetBinContent(38, 42.);
    h->SetBinContent(39, 45.);

    h->SetBinContent(40, 46.);
    h->SetBinContent(41, 45.);
    h->SetBinContent(42, 47.);
    h->SetBinContent(43, 44.);
    h->SetBinContent(44, 47.);
    h->SetBinContent(45, 48.);
    h->SetBinContent(46, 49.);
    h->SetBinContent(47, 50.);
    h->SetBinContent(48, 47.);
    h->SetBinContent(49, 50.);
    h->SetBinContent(50, 49.);
  }
  
  PixInitFunc pif; 
  if (1) {
    TF1 *f = pif.errScurve(h);
    cout << "fitting " <<  h->GetName() << " with error function" << endl;
    TFitResultPtr fr = h->Fit(f, "s");
    fr->PrintCovMatrix(cout);
    c0->SaveAs("errf.pdf");
  }


}


// ----------------------------------------------------------------------
void anaScurve::fitErr(int roc, int col, int row, bool draw) {
  int lroc, lcol, lrow; 

  PixInitFunc pif; 
  TF1 *f(0); 
  TH1D *h(0); 

  map<string, TH1D*> hsummary;
  if (col < 0 && row < 0 && roc < 0) {
    for (int i = 0; i < fNrocs; ++i) {
      h = new TH1D(Form("cd_C%d", i), Form("chi2/dof for ROC %d, errf ", i), 100, 0., 5.);
      hsummary.insert(make_pair(Form("cd_C%d", i), h)); 
      
      h = new TH1D(Form("p0_C%d", i), Form("p0 ROC %d, errf ", i), 100, 0., 200.);
      hsummary.insert(make_pair(Form("p0_C%d", i), h)); 
      
      h = new TH1D(Form("p1_C%d", i), Form("p1 ROC %d, errf ", i), 100, 0., 10.);
      hsummary.insert(make_pair(Form("p1_C%d", i), h)); 
    }
  }

  vector<TH1D*> hproblems; 
  for (unsigned int ihist = 0; ihist < fHists.size(); ++ihist) {
    PixUtil::str2rcr(fHists[ihist].first, lroc, lcol, lrow); 
    if (roc > -1 && lroc != roc) continue;
    if (col > -1 && lcol != col) continue;
    if (row > -1 && lrow != row) continue;

    h = fHists[ihist].second;
    if (0 == h) break;
    f = pif.errScurve(h);
    if (draw) cout << "fitting " <<  h->GetName() << endl;
    h->Fit(f, (draw?"r":"rq"));

    //     f = pif.errScurve(h);
    //     if (draw) cout << "fitting " <<  h->GetName() << endl;
    //     f->SetParameter(1, 4.*f->GetParameter(1)); 
    //     h->Fit(f, (draw?"r":"rq"));

    if (col < 0 && row < 0 && roc < 0) {
      hsummary[Form("cd_C%d", lroc)]->Fill(f->GetChisquare()/f->GetNDF()); 
      hsummary[Form("p0_C%d", lroc)]->Fill(f->GetParameter(0)); 
      hsummary[Form("p1_C%d", lroc)]->Fill(f->GetParameter(1)); 
    }
    if (draw) {
      c0->Modified();
      c0->Update();
    }
    if ((f->GetNDF() > 0) && f->GetChisquare()/f->GetNDF() > 20) {
      cout << "problem: " << h->GetName() << endl;
      hproblems.push_back(h); 
    }
  }


  // -- plot summary
  if (col < 0 && row < 0 && roc < 0) {
    c0->Clear();
    map<string, TH1D*>::iterator end = hsummary.end(); 
    for (map<string, TH1D*>::iterator il = hsummary.begin(); il != end; ++il) {
      if (il->second->GetEntries() < 1) continue;
      il->second->Draw();
      c0->SaveAs(Form("%s/scErr-%s.pdf", fDirectory.c_str(), il->second->GetName()));
    }
    
    gStyle->SetOptFit(1);
    for (unsigned int i = 0; i < hproblems.size(); ++i) {
      hproblems[i]->Draw();
      c0->SaveAs(Form("%s/scErr-problem-%s.pdf", fDirectory.c_str(), hproblems[i]->GetName())); 
    }
  }
}


// ----------------------------------------------------------------------
void anaScurve::readAsciiFiles(string directory) {
  vector<string> files = glob(directory); 

  fHists.reserve(fNrocs*4160); 

  TH1D *h1(0); 
  
  ifstream IN; 
  char buffer[1000];
  string sline, sval; 
  int startbin(0), nbins(0); 
  string::size_type s0, s1;
  string srow, scol, hname; 
  int idx(0), iroc(-1), irow(0), icol(0); 
  for (unsigned int ifile = 0; ifile < files.size(); ++ifile) {
    s0 = files[ifile].rfind("_C");
    s1 = files[ifile].rfind(".dat");
    sval = files[ifile].substr(s0+2, s1-s0-2); 
    iroc = atoi(sval.c_str()); 
    IN.open(Form("%s/%s", directory.c_str(), files[ifile].c_str())); 
    IN.getline(buffer, 1000, '\n');

    while (IN.getline(buffer, 1000, '\n')) {
      if (buffer[0] == '#') {continue;}
      if (buffer[0] == '/') {continue;}
      if (buffer[0] == '\n') {continue;}
      sline = buffer; 
      
      PixUtil::idx2rcr(idx, iroc, icol, irow);
      
      istringstream jstring(sline);
      hname = Form("sc_c%d_r%d_C%d", icol, irow, iroc);
      h1 = new TH1D(hname.c_str(), hname.c_str(), 256, 0., 256.); 
      h1->Sumw2(); 
      
      jstring >> sval;
      nbins = atoi(sval.c_str()); 
      jstring >> sval; 
      startbin = atoi(sval.c_str()) + 1; 
      for (int i = 0; i < nbins; ++i) {
	jstring >> sval;
	h1->SetBinContent(startbin+i, atoi(sval.c_str())); 
	h1->SetBinError(startbin+i, 1.);
      }
      fHists.push_back(make_pair(hname, h1)); 
      ++idx;
      if (0) {
	h1->Draw();
	c0->Modified();
	c0->Update();
      }
    }
    IN.close(); 
  }
 
}


// ----------------------------------------------------------------------
void anaScurve::readRootFile(string filename) {
  TFile *f = TFile::Open(filename.c_str()); 
  f->cd("GainPedestal");

  fHists.reserve(fNrocs*4160); 

  TH1D* h(0); 
  string hname, sname; 
  for (int roc = 0; roc < 16; ++roc) {
    for (int col = 0; col < 52; ++col) {
      for (int row = 0; row < 80; ++row) {
	hname = Form("gainPedestal_c%d_r%d_C%d_V0", col, row, roc); 
	h = 0; 
	h = (TH1D*)gDirectory->Get(hname.c_str()); 
	if (h) fHists.push_back(make_pair(hname, h)); 
      }
    }
  }
}



// ----------------------------------------------------------------------
vector<string> anaScurve::glob(string directory, string basename) {
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
  return lof; 
}


