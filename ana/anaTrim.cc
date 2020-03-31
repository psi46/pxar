#include "anaTrim.hh"

#include <fstream>
#include <sstream>
#include <cstdlib>

#include <TROOT.h>
#include <TH2.h>
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
  PixUtil::replaceAll(pdfname, "../", "");
  PixUtil::replaceAll(pdfname, "/", "__");
  // string::size_type m1 = pdfname.rfind("/");
  // pdfname = pdfname.substr(m1+1);

  pdfname = fDirectory + "/anaTrim-thrDiff-" + pdfname;

  TFile *f = TFile::Open(filename.c_str());
  bool ok = f->cd("Trim");

  if (!ok) {
    cout << "no Trim directory found" << endl;
    return;
  }

  vector<TH1D *> htb14, htb13, htb11, htb7, htb0;
  vector<int> vtb, vdead;
  vtb.push_back(14);  vtb.push_back(13);  vtb.push_back(11);  vtb.push_back(7);
  for (int iroc = 0; iroc < 16; ++iroc) {
    for (int itb = 0; itb < 4; ++itb) {
      TH1D *h(0);
      h = (TH1D*)gDirectory->Get(Form("TrimBit%d_C%d_V0", vtb[itb], iroc));
      if (h) {
	if (0 == itb) {
	  h->SetLineColor(kRed);
	  htb14.push_back(h);
	  TH2D *h2 = (TH2D*)gDirectory->Get(Form("thr_TrimBitsThr0_Vcal_C%d_V0", iroc));
	  int ndead(0);
	  for (int ix = 0; ix < h2->GetNbinsX(); ++ix) {
	    for (int iy = 0; iy < h2->GetNbinsY(); ++iy) {
	      if (h2->GetBinContent(ix+1, iy+1) < 1) {
		++ndead;
	      }
	    }
	  }
	  vdead.push_back(ndead);
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
    tl->SetTextColor(kBlack);   tl->DrawLatexNDC(0.65, 0.85, Form("dead: %d", vdead[iroc]));
    tl->SetTextColor(kBlack);   tl->DrawLatexNDC(0.65, 0.80, "Entries:");
    tl->SetTextColor(kRed);     tl->DrawLatexNDC(0.65, 0.75, Form("TB0: %4.0f", htb14[iroc]->Integral(1, htb14[iroc]->GetNbinsX()+1)));
    tl->SetTextColor(kMagenta); tl->DrawLatexNDC(0.65, 0.70, Form("TB1: %4.0f", htb13[iroc]->Integral(1, htb13[iroc]->GetNbinsX()+1)));
    tl->SetTextColor(kBlue);    tl->DrawLatexNDC(0.65, 0.65, Form("TB2: %4.0f", htb11[iroc]->Integral(1, htb11[iroc]->GetNbinsX()+1)));
    tl->SetTextColor(kBlack);   tl->DrawLatexNDC(0.65, 0.60, Form("TB3: %4.0f", htb7[iroc]->Integral(1, htb7[iroc]->GetNbinsX()+1)));

    tl->SetTextSize(0.05);
    tl->SetTextColor(kBlack);
    tl->DrawLatexNDC(0.50, 0.91, Form("!LSB: %3.0f", ntb7+ntb11+ntb13));
    tl->SetTextColor(kRed);
    tl->DrawLatexNDC(0.17, 0.91, Form("<3 LSB: %3.0f", ntb14));


    tl->SetTextSize(0.08);
    tl->SetTextColor(kBlack);
    tl->DrawLatexNDC(0.78, 0.91, Form("C%d", iroc));

  }


  c0->cd();
  tl->SetTextSize(0.015);
  tl->SetTextAngle(90.);
  tl->DrawLatexNDC(0.99, 0.05, rootfile.c_str());


  cout << "Save to " << pdfname << endl;
  c0->SaveAs(pdfname.c_str());
}
