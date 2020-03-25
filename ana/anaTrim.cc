#include "anaTrim.hh"

#include <fstream>
#include <sstream>
#include <cstdlib>

#include <TROOT.h>
#include <TFile.h>
#include <TKey.h>
#include <TSystem.h>
#include <TStyle.h>
#include <TLatex.h>
#include <TFitResult.h>
#if defined(WIN32)
#else
#include <TUnixSystem.h>
#endif

#include "PixUtil.hh"
#include "PixInitFunc.hh"

using namespace std;

// ----------------------------------------------------------------------
anaTrim::anaTrim(string dir): fDirectory(dir) {
  cout << "anaTrim ctor, directory = " << fDirectory << endl;
  c0 = (TCanvas*)gROOT->FindObject("c0");
  if (!c0) c0 = new TCanvas("c0","--c0--",0,0,656,700);

}

// ----------------------------------------------------------------------
anaTrim::~anaTrim() {
  cout << "anaTrim dtor" << endl;
}


// ----------------------------------------------------------------------
void anaTrim::makeAll(string directory, int /*mode*/) {
}


// ----------------------------------------------------------------------
void anaTrim::showTrimBits(std::string rootfile) {
  string filename(rootfile);
  string pdfname(rootfile);
  PixUtil::replaceAll(pdfname, ".root", ".pdf");
  string::size_type m1 = pdfname.rfind("/");
  pdfname = pdfname.substr(m1+1);

  pdfname = fDirectory + "/anaTrim-thrDiff-" + pdfname;

  TFile *f = TFile::Open(filename.c_str());
  bool ok = f->cd("Trim");

  if (!ok) {
    cout << "no Trim directory found" << endl;
    return;
  }

  vector<TH1D *> htb14, htb13, htb11, htb7;
  vector<int> vtb;
  vtb.push_back(14);  vtb.push_back(13);  vtb.push_back(11);  vtb.push_back(7);
  for (int iroc = 0; iroc < 16; ++iroc) {
    for (int itb = 0; itb < 4; ++itb) {
      TH1D *h(0);
      h = (TH1D*)gDirectory->Get(Form("TrimBit%d_C%d_V0", vtb[itb], iroc));
      if (h) {
	if (0 == itb) {
	  h->SetLineColor(kRed);
	  htb14.push_back(h);
	}
	if (1 == itb) {
	  h->SetLineColor(kMagenta);
	  htb13.push_back(h);
	}
	if (2 == itb) {
	  h->SetLineColor(kBlue);
	  htb11.push_back(h);
	}
	if (3 == itb) {
	  h->SetLineColor(kBlack);
	  htb7.push_back(h);
	}
      } else {
	cout << "did not find " << Form("TrimBit%d_C%d_V0", vtb[itb], iroc) << " returning ..." << endl;
	return;
      }

    }
  }


  c0->Clear();
  c0->Divide(4,4);

  gStyle->SetOptStat(0);
  TLatex *tl = new TLatex();
  for (int iroc = 0; iroc < 16; ++iroc) {

    float ntb14 = htb14[iroc]->Integral(0, htb14[iroc]->FindBin(2.2));
    float ntb13 = htb13[iroc]->Integral(0, htb13[iroc]->FindBin(2.2));
    float ntb11 = htb11[iroc]->Integral(0, htb11[iroc]->FindBin(2.2));
    float ntb7  = htb7[iroc]->Integral(0, htb7[iroc]->FindBin(2.2));

    c0->cd(iroc+1);
    gPad->SetLogy(1);
    htb14[iroc]->SetTitle("");
    htb14[iroc]->SetAxisRange(0., 40., "X");
    htb14[iroc]->SetMinimum(0.5);

    htb14[iroc]->Draw("hist");
    htb13[iroc]->Draw("samehist");
    htb11[iroc]->Draw("samehist");
    htb7[iroc]->Draw("samehist");


    tl->SetTextSize(0.05);
    tl->SetTextColor(kBlack);
    tl->DrawLatexNDC(0.50, 0.91, Form("!LSB: %3.0f", ntb7+ntb11+ntb13));
    tl->SetTextColor(kRed);
    tl->DrawLatexNDC(0.17, 0.91, Form("<3 LSB: %3.0f", ntb14));


    tl->SetTextSize(0.08);
    tl->SetTextColor(kBlack);
    tl->DrawLatexNDC(0.78, 0.91, Form("C%d", iroc));

  }

  tl->SetTextSize(0.05);
  tl->SetTextColor(kRed);     tl->DrawLatexNDC(0.78, 0.85, "TB 0");
  tl->SetTextColor(kMagenta); tl->DrawLatexNDC(0.78, 0.80, "TB 1");
  tl->SetTextColor(kBlue);    tl->DrawLatexNDC(0.78, 0.75, "TB 2");
  tl->SetTextColor(kBlack);   tl->DrawLatexNDC(0.78, 0.70, "TB 3");

  c0->cd();
  tl->SetTextSize(0.02);
  tl->SetTextAngle(90.);
  tl->DrawLatexNDC(0.99, 0.05, rootfile.c_str());


  cout << "Save to " << pdfname << endl;
  c0->SaveAs(pdfname.c_str());
}
