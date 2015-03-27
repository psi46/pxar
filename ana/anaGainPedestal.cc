#include "anaGainPedestal.hh"

#include <fstream>
#include <sstream>
#include <cstdlib>

#include <TROOT.h>
#include <TFile.h>
#include <TKey.h>
#include <TSystem.h>
#include <TStyle.h>
#if defined(WIN32)
#else
#include <TUnixSystem.h>
#endif

#include "PixUtil.hh"
#include "PixInitFunc.hh"

using namespace std;

// ----------------------------------------------------------------------
anaGainPedestal::anaGainPedestal(string dir, int nrocs): fDirectory(dir), fNrocs(nrocs) {
  cout << "anaGainPedestal ctor, nrocs = " << fNrocs << " directory = " << fDirectory << endl;
  c0 = (TCanvas*)gROOT->FindObject("c0"); 
  if (!c0) c0 = new TCanvas("c0","--c0--",0,0,656,700);

  fHists.clear();

}

// ----------------------------------------------------------------------
anaGainPedestal::~anaGainPedestal() {
  cout << "anaGainPedestal dtor" << endl;
  fHists.clear();
}


// ----------------------------------------------------------------------
void anaGainPedestal::makeAll(string directory, int mode) {
  readAsciiFiles(directory); 
  if (0 == mode) {
    fitErr();
  } else {
    fitTanH();
  }
}


// ----------------------------------------------------------------------
void anaGainPedestal::fitTanH(int roc, int col, int row, bool draw) {
  int lroc, lcol, lrow; 

  PixInitFunc pif; 
  TF1 *f(0); 
  TH1D *h(0); 

  map<string, TH1D*> hsummary;
  for (int i = 0; i < fNrocs; ++i) {
    h = new TH1D(Form("cd_C%d", i), Form("chi2/dof for ROC %d, TanH ", i), 100, 0., 5.);
    hsummary.insert(make_pair(Form("cd_C%d", i), h)); 
  }

  map<string, TH1D*>::iterator end = fHists.end(); 
  for (map<string, TH1D*>::iterator il = fHists.begin(); il != end; ++il) {
    PixUtil::str2rcr(il->first, lroc, lcol, lrow); 
    if (roc > -1 && lroc != roc) continue;
    if (col > -1 && lcol != col) continue;
    if (row > -1 && lrow != row) continue;

    h = il->second;
    f = pif.gpTanH(h);
    h->Fit(f);

    hsummary[Form("cd_C%d", lroc)]->Fill(f->GetChisquare()/f->GetNDF()); 
    if (draw) {
      c0->Modified();
      c0->Update();
    }
  }

  // -- plot summary
  end = hsummary.end(); 
  for (map<string, TH1D*>::iterator il = hsummary.begin(); il != end; ++il) {
    if (il->second->GetEntries() < 1) continue;
    il->second->Draw();
    c0->Modified();
    c0->Update();
  }
}



// ----------------------------------------------------------------------
void anaGainPedestal::fitErr(int roc, int col, int row, bool draw) {
  int lroc, lcol, lrow; 

  PixInitFunc pif; 
  TF1 *f(0); 
  TH1D *h(0); 

  map<string, TH1D*> hsummary;
  for (int i = 0; i < fNrocs; ++i) {
    h = new TH1D(Form("cd_C%d", i), Form("chi2/dof for ROC %d, errf ", i), 100, 0., 5.);
    hsummary.insert(make_pair(Form("cd_C%d", i), h)); 

    h = new TH1D(Form("p0_C%d", i), Form("p0 ROC %d, errf ", i), 100, 0., 1000.);
    hsummary.insert(make_pair(Form("p0_C%d", i), h)); 

    h = new TH1D(Form("p1_C%d", i), Form("p1 ROC %d, errf ", i), 100, 0., 1000.);
    hsummary.insert(make_pair(Form("p1_C%d", i), h)); 

    h = new TH1D(Form("p2_C%d", i), Form("p2 ROC %d, errf ", i), 100, 0., 10.);
    hsummary.insert(make_pair(Form("p2_C%d", i), h)); 

    h = new TH1D(Form("p3_C%d", i), Form("p3 ROC %d, errf ", i), 100, 0., 200.);
    hsummary.insert(make_pair(Form("p3_C%d", i), h)); 
  }

  vector<TH1D*> hproblems; 
  map<string, TH1D*>::iterator end = fHists.end(); 
  for (map<string, TH1D*>::iterator il = fHists.begin(); il != end; ++il) {
    PixUtil::str2rcr(il->first, lroc, lcol, lrow); 
    if (roc > -1 && lroc != roc) continue;
    if (col > -1 && lcol != col) continue;
    if (row > -1 && lrow != row) continue;

    h = il->second;
    f = pif.gpErr(h);
    if (draw) cout << "fitting " <<  h->GetName() << endl;
    h->Fit(f, (draw?"":"q"));
    hsummary[Form("cd_C%d", lroc)]->Fill(f->GetChisquare()/f->GetNDF()); 
    hsummary[Form("p0_C%d", lroc)]->Fill(f->GetParameter(0)); 
    hsummary[Form("p1_C%d", lroc)]->Fill(f->GetParameter(1)); 
    hsummary[Form("p2_C%d", lroc)]->Fill(f->GetParameter(2)); 
    hsummary[Form("p3_C%d", lroc)]->Fill(f->GetParameter(3)); 
    if (draw) {
      c0->Modified();
      c0->Update();
    }
    if (f->GetChisquare()/f->GetNDF() > 3) {
      cout << "problem: " << h->GetName() << endl;
      hproblems.push_back(h); 
    }
  }

  int ipad(1); 
  c0->Clear();

  // -- plot summary
  end = hsummary.end(); 
  for (map<string, TH1D*>::iterator il = hsummary.begin(); il != end; ++il) {
    if (il->second->GetEntries() < 1) continue;
    il->second->Draw();
    c0->SaveAs(Form("%s/gpErr-%s.pdf", fDirectory.c_str(), il->second->GetName()));
  }

  gStyle->SetOptFit(1);
  for (unsigned int i = 0; i < hproblems.size(); ++i) {
    hproblems[i]->Draw();
    c0->SaveAs(Form("%s/gpErr-problem-%s.pdf", fDirectory.c_str(), hproblems[i]->GetName())); 
  }
}

// ----------------------------------------------------------------------
void anaGainPedestal::readAsciiFiles(string directory) {
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
    x.clear();
    s0 = files[ifile].rfind("_C");
    s1 = files[ifile].rfind(".dat");
    sval = files[ifile].substr(s0+2, s1-s0-2); 
    iroc = atoi(sval.c_str()); 
    cout << files[ifile] << ": " << sval << " -> " << iroc << endl;
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
      hname = Form("gp_c%d_r%d_C%d", icol, irow, iroc);
      h1 = new TH1D(hname.c_str(), hname.c_str(), 2000, 0., 2000.); 
      h1->Sumw2(); 
      
      int i(0); 
      for (unsigned int i = 0; i < x.size(); ++i) {
	jstring >> sval;
	h1->SetBinContent(x[i]+1, atoi(sval.c_str())); 
	h1->SetBinError(x[i]+1, 0.02*atoi(sval.c_str())); 
      }
      fHists.insert(make_pair(hname, h1)); 
    }
    IN.close(); 
  }
 
}


// ----------------------------------------------------------------------
void anaGainPedestal::readRootFile(string filename) {
  TFile *f = TFile::Open(filename.c_str()); 
  f->cd("GainPedestal");

  TH1D* h(0); 
  string hname, sname; 
  int cnt(0), roc(0), row(0), col(0); 
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
vector<string> anaGainPedestal::glob(string directory, string basename) {
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


