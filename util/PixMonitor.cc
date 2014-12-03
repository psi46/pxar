#include <iostream>
#include "PixMonitor.hh"
#include "log.h"
#include <cstdlib>

#include "rsstools.hh"
#include "shist256.hh"

#include <TString.h>
#include <TTimeStamp.h>
#include <TDirectory.h>
#include <TFile.h>

using namespace std;
using namespace pxar;

// ----------------------------------------------------------------------
PixMonitor::PixMonitor(pxarCore *a): fApi(a), fIana(0.), fIdig(0.) {
}

// ----------------------------------------------------------------------
PixMonitor::~PixMonitor() {
  LOG(logDEBUG) << "PixMonitor dtor"; 
}

// ----------------------------------------------------------------------
void PixMonitor::init() {
  LOG(logDEBUG) << "PixMonitor init"; 
}

// ----------------------------------------------------------------------
void PixMonitor::dumpSummaries() {
  gFile->cd();
  LOG(logDEBUG) << "PixMonitor::dumpSummaries"; 
  ULong_t begSec = fMeasurements[0].first;
  ULong_t endSec = fMeasurements[fMeasurements.size()-1].first+1;
  TTimeStamp ts(begSec); 
  //  cout << "begSec: " << begSec << " endSec: " << endSec << " endSec-begSec: " << endSec-begSec << " nbins: " << endSec-begSec << endl;
  TH1D *ha = new TH1D("HA", Form("analog current measurements, start: %s / sec:%ld", ts.AsString("lc"), begSec), endSec-begSec, 0., endSec-begSec);
  ha->SetXTitle(Form("seconds after %s", ts.AsString("lc"))); 
  ha->SetTitleSize(0.03, "X");
  ha->SetTitleOffset(1.5, "X");

  TH1D *hd = new TH1D("HD", Form("digital current measurements, start: %s / sec:%ld", ts.AsString("lc"), begSec), endSec-begSec, 0., endSec-begSec);
  hd->SetXTitle(Form("seconds after %s", ts.AsString("lc"))); 
  hd->SetTitleSize(0.03, "X");
  hd->SetTitleOffset(1.5, "X");

  for (unsigned int i = 0; i < fMeasurements.size(); ++i) {
    int ibin = fMeasurements[i].first - begSec;
    ha->SetBinContent(ibin+1, fMeasurements[i].second.first);
    hd->SetBinContent(ibin+1, fMeasurements[i].second.second);
  }
  
  ha->Draw();
  hd->Draw();

  ha->SetDirectory(gFile); 
  ha->Write();

  hd->SetDirectory(gFile); 
  hd->Write();
}

// ----------------------------------------------------------------------
void PixMonitor::update() {
  int NBINS(10); 
  fIana = fApi->getTBia();
  fIdig = fApi->getTBid();
  TTimeStamp ts; 
  ULong_t seconds  = ts.GetSec();
  
  TH1D *ha = (TH1D*)gDirectory->Get("ha"); 
  if (0 == ha) {
    ha = new TH1D("ha", Form("analog current, start: %s / sec:%ld", ts.AsString("lc"), seconds), NBINS, 0, NBINS); 
    ha->SetXTitle(Form("seconds after %s", ts.AsString("lc"))); 
    ha->SetTitleSize(0.03, "X");
    ha->SetTitleOffset(1.5, "X");
  }
  int histMinSec = getHistMinSec(ha); 
  int ibin = seconds - histMinSec; 
  //  if (0 == ibin) ibin = 1; 
  if (ibin > ha->GetNbinsX()) ha = extendHist(ha, ibin); 
  ha->SetBinContent(ibin+1, fIana); 

  TH1D *hd = (TH1D*)gDirectory->Get("hd"); 
  if (0 == hd) {
    hd = new TH1D("hd", Form("digital current, start: %s / sec:%ld", ts.AsString("lc"), seconds), NBINS, 0, NBINS); 
    hd->SetXTitle(Form("seconds after %s", ts.AsString("lc"))); 
    hd->SetTitleSize(0.03, "X");
    hd->SetTitleOffset(1.5, "X");
  }
  if (ibin > hd->GetNbinsX()) hd = extendHist(hd, ibin); 
  hd->SetBinContent(ibin+1, fIdig); 

  fMeasurements.push_back(make_pair(seconds, make_pair(fIana, fIdig))); 
  

}

// ----------------------------------------------------------------------
TH1D* PixMonitor::extendHist(TH1D *h, int ibin) {
  if (0 == h) return 0;
  int nbins = h->GetNbinsX(); 
  TH1D *hnew = new TH1D("nhew", h->GetTitle(), ibin + 10, 0, ibin + 10);
  for (int i = 1; i <= nbins; ++i) {
    hnew->SetBinContent(i, h->GetBinContent(i)); 
    hnew->SetXTitle(h->GetXaxis()->GetTitle()); 
    hnew->SetTitleSize(0.03, "X");
    hnew->SetTitleOffset(1.5, "X");
  }
  string hname = h->GetName(); 
  delete h; 
  hnew->SetName(hname.c_str()); 
  return hnew;
}


// ----------------------------------------------------------------------
UInt_t PixMonitor::getHistMinSec(TH1D *h) {
  string htitle = h->GetTitle();
  string::size_type s1 = htitle.rfind("sec:"); 
  string sseconds = htitle.substr(s1+4);
  UInt_t seconds = atoi(sseconds.c_str()); 
  return seconds;
}

