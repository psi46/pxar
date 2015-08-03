#include "anaFullTest.hh"

#include <fstream>
#include <sstream>
#include <cstdlib>
#include <time.h>

#include <TROOT.h>
#include <TSystem.h>
#include <TStyle.h>
#include <TFile.h>
#if defined(WIN32)
#else
#include <TUnixSystem.h>
#endif

#include "PixUtil.hh"


using namespace std;

// ----------------------------------------------------------------------
anaFullTest::anaFullTest(): fNrocs(16), fTrimVcal(35) {
  cout << "anaFullTest ctor" << endl;
  c0 = (TCanvas*)gROOT->FindObject("c0"); 
  if (!c0) c0 = new TCanvas("c0","--c0--",0,0,656,700);

  tl = new TLatex(); 
  tl->SetNDC(kTRUE); 
  tl->SetTextSize(0.06); 

  fSMS = new singleModuleSummary; 

  double YMAX(-1.);
  fhDuration    = new TH1D("hDuration", "", 50, 0., 10000.);    setHist(fhDuration, "test duration [seconds]", "tests", kBlack, 0., YMAX);
  fhCritical    = new TH1D("hCritical", "", 20, 0., 20.);        setHist(fhCritical, "#criticals seen", "tests", kBlack, 0., YMAX);
  fhDead        = new TH1D("hDead", "", 40, 0., 40.);            setHist(fhDead, "pixels", "", kBlack, 0.5, YMAX);
  fhMask        = new TH1D("hMask", "", 40, 0., 40.);            setHist(fhMask, "pixels", "", kRed, 0.5, YMAX);
  fhAddr        = new TH1D("hAddr", "", 40, 0., 40.);            setHist(fhAddr, "pixels", "", kGreen+2, 0., YMAX);
  fhBb          = new TH1D("hBb", "", 40, 0., 40.);              setHist(fhBb, "pixels", "", kBlue-4, 0.5, YMAX);

  fhNoise       = new TH1D("hNoise", "", 100, 0., 10.);          setHist(fhNoise, "noise [VCAL]", "pixels", kGreen+2, 0.5, YMAX);
  fhVcaltrimthr = new TH1D("hVcaltrimthr", "", 100, 0., 50.);    setHist(fhVcaltrimthr, "VCAL trim thr", "pixels", kBlack, 0.5, YMAX);

  fhVana        = new TH1D("hVana", "", 256, 0., 256);           setHist(fhVana, "DAC", "ROCs", kBlack, 0., YMAX);
  fhCaldel      = new TH1D("hCaldel", "", 256, 0., 256);         setHist(fhCaldel, "DAC", "ROCs", kRed, 0., YMAX);
  fhPhoffset    = new TH1D("hPhoffset", "", 256, 0., 256);       setHist(fhPhoffset, "DAC", "ROCs", kRed+2, 0., YMAX);
  fhPhscale     = new TH1D("hPfscale", "", 256, 0., 256);        setHist(fhPhscale, "DAC", "ROCs", kYellow+2, 0., YMAX);
  fhVtrim       = new TH1D("hVtrim", "", 256, 0., 256);          setHist(fhVtrim, "DAC", "ROCs", kRed, 0., YMAX);
  fhVthrcomp    = new TH1D("hVthrcomp", "", 256, 0., 256);       setHist(fhVthrcomp, "DAC", "ROCs", kBlack, 0., YMAX);


  fDiffMetric = 0;

  fDacs.clear(); 
  fDacs.push_back("vana"); 
  fDacs.push_back("caldel"); 
  fDacs.push_back("vthrcomp"); 
  fDacs.push_back("vtrim"); 
  fDacs.push_back("phscale"); 
  fDacs.push_back("phoffset"); 
}

// ----------------------------------------------------------------------
anaFullTest::~anaFullTest() {
  cout << "anaFullTest dtor" << endl;

}


// ----------------------------------------------------------------------
void anaFullTest::bookSingleModuleSummary(string modulename, int first) {
  
  fSMS->moduleName = modulename; 

  TH1::SetDefaultSumw2(kTRUE);

  if (0 == first) delete fSMS->noise; 
  fSMS->noise = new TH1D("noise", "", 16, 0., 16.);                 setHist(fSMS->noise, "ROC", "noise", kGreen+2, 0., 4.);

  if (0 == first) delete fSMS->vcalThr; 
  fSMS->vcalThr = new TH1D("vcalThr", "", 16, 0., 16.);             setHist(fSMS->vcalThr, "ROC", "VCAL THR", kBlack, 0., 150.);

  if (0 == first) delete fSMS->vcalThrW; 
  fSMS->vcalThrW = new TH1D("vcalThrW", "", 16, 0., 16.);           setHist(fSMS->vcalThrW, "ROC", "VCAL THR width", kBlack, 0., 10.);

  if (0 == first) delete fSMS->vcalTrimThr; 
  fSMS->vcalTrimThr = new TH1D("vcalTrimThr", "", 16, 0., 16.);     setHist(fSMS->vcalTrimThr, "ROC", "VCAL THR (trimmed)", kBlue, 0., -1.);

  if (0 == first) delete fSMS->vcalTrimThrW; 
  fSMS->vcalTrimThrW = new TH1D("vcalTrimThrW", "", 16, 0., 16.);   setHist(fSMS->vcalTrimThrW, "ROC", "VCAL THR width (trimmed)", kBlue, 0., 4.);

  if (0 == first) delete fSMS->relGainW; 
  fSMS->relGainW = new TH1D("relGainW", "", 16, 0., 16.);           setHist(fSMS->relGainW, "ROC", "rel gain width", kBlack, 0., 40.);

  if (0 == first) delete fSMS->pedestalW; 
  fSMS->pedestalW = new TH1D("pedestalW", "", 16, 0., 16.);         setHist(fSMS->pedestalW, "ROC", "pedestal width", kBlack, 0., 40.);

  if (0 == first) delete fSMS->nonl; 
  fSMS->nonl = new TH1D("nonl", "", 16, 0., 16.);                   setHist(fSMS->nonl, "ROC", "non-linearity", kBlue, 0., 1.1);

  if (0 == first) delete fSMS->nonlW; 
  fSMS->nonlW = new TH1D("nonlW", "", 16, 0., 16.);                 setHist(fSMS->nonlW, "ROC", "non-linearity width", kBlue, 0., 1.1);

  if (0 == first) delete fSMS->dead; 
  fSMS->dead = new TH1D("dead", "", 16, 0., 16.);                   setHist(fSMS->dead, "ROC", "dead pixels", kBlack, 0., 40.);

  if (0 == first) delete fSMS->bb; 
  fSMS->bb = new TH1D("bb", "", 16, 0., 16.);                       setHist(fSMS->bb, "ROC", "dead bumps", kBlue-4, 0., 40.);

  if (0 == first) delete fSMS->mask; 
  fSMS->mask = new TH1D("mask", "", 16, 0., 16.);                   setHist(fSMS->mask, "ROC", "mask defects", kRed, 0., 40.);

  if (0 == first) delete fSMS->addr; 
  fSMS->addr = new TH1D("addr", "", 16, 0., 16.);                   setHist(fSMS->addr, "ROC", "address defects", kGreen+2, 0., 40.);

  if (0 == first) delete fSMS->vana; 
  fSMS->vana = new TH1D("vana", "", 16, 0., 16.);                   setHist(fSMS->vana, "ROC", "VANA", kBlack, 0., 256.);

  if (0 == first) delete fSMS->caldel; 
  fSMS->caldel = new TH1D("caldel", "", 16, 0., 16.);               setHist(fSMS->caldel, "ROC", "CALDEL", kRed, 0., 256.);

  if (0 == first) delete fSMS->vthrcomp; 
  fSMS->vthrcomp = new TH1D("vthrcomp", "", 16, 0., 16.);           setHist(fSMS->vthrcomp, "ROC", "VTHRCOMP", kBlack, 0., 256.);

  if (0 == first) delete fSMS->vtrim; 
  fSMS->vtrim = new TH1D("vtrim", "", 16, 0., 16.);                 setHist(fSMS->vtrim, "ROC", "VTRIM", kRed, 0., 256.);

  if (0 == first) delete fSMS->phscale; 
  fSMS->phscale = new TH1D("phscale", "", 16, 0., 16.);             setHist(fSMS->phscale, "ROC", "PHSCALE", kYellow+2, 0., 256.);

  if (0 == first) delete fSMS->phoffset; 
  fSMS->phoffset = new TH1D("phoffset", "", 16, 0., 16.);           setHist(fSMS->phoffset, "ROC", "PHOFFSET", kRed+2, 0., 256.);

  if (0 == first) delete fSMS->defectMap; 
  fSMS->defectMap = new TH2D("defectMap", "", 52, 0., 52., 80, 0., 80.); 
  fSMS->defectMap->SetXTitle("column");          
  fSMS->defectMap->SetYTitle("row"); 
  fSMS->defectMap->SetMinimum(0.); 
  fSMS->defectMap->SetMaximum(4.01); 
  
  if (0 == first) delete fSMS->distNoise; 
  fSMS->distNoise = new TH1D("distNoise", "", 100, 0., 10.);           setHist(fSMS->distNoise, "noise [VCAL]", "entries/bin", kGreen+2, 0.5, -1.);

  if (0 == first) delete fSMS->distVcalTrimThr;
  fSMS->distVcalTrimThr = new TH1D("distVcalTrimThr", "", 90, 0., 45.); setHist(fSMS->distVcalTrimThr, "VCAL trim THR [VCAL]", "entries/bin", kBlack, 0.5, -1.);

}




// ----------------------------------------------------------------------
void anaFullTest::showAllFullTests(string dir, string pattern) {
  vector<string> dirs = glob(dir, pattern); 
  for (unsigned int idirs = 0; idirs < dirs.size(); ++idirs) {
    cout << dirs[idirs] << endl;
    showFullTest(dirs[idirs], dir); 
  }
  
}


// ----------------------------------------------------------------------
void anaFullTest::showFullTest(string modname, string basename) {

  string dirname = basename + string("/") + modname; 

  tl->SetTextSize(0.05); 
  static int first(1); 
  bookSingleModuleSummary(modname, first); 
  if (1 == first) {
    first = 0; 
  }

  string date = Form("Date:        %s", readLine(dirname, "Today:", 1).c_str()); 
  PixUtil::replaceAll(date, "Date:", ""); 
  PixUtil::cleanupString(date); 

  int ncritical = countWord(dirname, "CRITICAL:");
  fhCritical->Fill(ncritical); 
  string criticals = Form("%d", ncritical); 

  string startTest = Form("Start:       %s", readLine(dirname, "INFO: *** Welcome to pxar ***", 2).c_str()); 
  string endTest   = Form("End:   %s", readLine(dirname, "INFO: pXar: this is the end, my friend", 2).c_str()); 

  PixUtil::replaceAll(startTest, "Start:", ""); 
  PixUtil::replaceAll(startTest, "[", ""); 
  PixUtil::replaceAll(startTest, "]", ""); 

  PixUtil::replaceAll(endTest, "End:", ""); 
  PixUtil::replaceAll(endTest, "[", ""); 
  PixUtil::replaceAll(endTest, "]", ""); 

  int seconds = testDuration(startTest, endTest); 
  fhDuration->Fill(seconds); 
  string duration = Form("%d:%d:%d", seconds/3600, (seconds-seconds/3600*3600)/60, seconds%60);

  // -- remove the sub-second digits
  startTest = startTest.substr(0, startTest.rfind(".")); 
  PixUtil::cleanupString(startTest);
 
  vector<TH1D*> vh; 
  TH1D *h(0); 
  for (int iroc = 0; iroc < fNrocs+1; ++iroc) {
    h = (TH1D*)gROOT->Get(Form("dac_C%d", iroc)); 
    if (!h) h = new TH1D(Form("dac_C%d", iroc), Form("dac_C%d", iroc), 256, 0., 256.); 
    vh.push_back(h); 
  }

  // -- DAC file parsing
  clearHistVector(vh); 
  readDacFile(dirname, "vana", vh);
  dumpVector(vh, fSMS->vana, "0"); 
  summarizeVector(vh, fhVana); 
  
  clearHistVector(vh); 
  readDacFile(dirname, "caldel", vh);
  dumpVector(vh, fSMS->caldel, "0"); 
  summarizeVector(vh, fhCaldel); 

  clearHistVector(vh); 
  readDacFile(dirname, "vthrcomp", vh);
  dumpVector(vh, fSMS->vthrcomp, "0"); 
  summarizeVector(vh, fhVthrcomp); 

  clearHistVector(vh); 
  readDacFile(dirname, "vtrim", vh);
  dumpVector(vh, fSMS->vtrim, "0"); 
  summarizeVector(vh, fhVtrim); 

  clearHistVector(vh); 
  readDacFile(dirname, "phscale", vh);
  dumpVector(vh, fSMS->phscale, "0"); 
  summarizeVector(vh, fhPhscale); 

  clearHistVector(vh); 
  readDacFile(dirname, "phoffset", vh);
  dumpVector(vh, fSMS->phoffset, "0"); 
  summarizeVector(vh, fhPhoffset); 
  
  // -- log file parsing
  clearHistVector(vh); 
  readLogFile(dirname, "vcal mean:", vh);
  dumpVector(vh, fSMS->vcalTrimThr, "0"); 

  clearHistVector(vh); 
  readLogFile(dirname, "vcal RMS:", vh);
  dumpVector(vh, fSMS->vcalTrimThrW, "0"); 

  clearHistVector(vh); 
  readLogFile(dirname, "non-linearity mean:", vh);
  dumpVector(vh, fSMS->nonl, "0"); 

  clearHistVector(vh); 
  readLogFile(dirname, "non-linearity RMS:", vh);
  dumpVector(vh, fSMS->nonlW, "0"); 

  // -- note: ideally this should be filled in fillRocDefects?
  clearHistVector(vh); 
  readLogFile(dirname, "number of dead bumps (per ROC):", vh);
  dumpVector(vh, fSMS->bb, "0"); 
  summarizeVector(vh, fhBb); 


  // ROOT file 
  anaRocMap(dirname, "PixelAlive/PixelAlive", fSMS->dead, 0);
  anaRocMap(dirname, "PixelAlive/MaskTest", fSMS->mask, 1);
  anaRocMap(dirname, "PixelAlive/AddressDecodingTest", fSMS->addr, 1);

  fillRocHist(dirname, "Scurves/dist_thr_scurveVcal_Vcal", fSMS->vcalThr, 0);
  fillRocHist(dirname, "Scurves/dist_thr_scurveVcal_Vcal", fSMS->vcalThrW, 1);
  fillRocHist(dirname, "Scurves/dist_sig_scurveVcal_Vcal", fSMS->noise, 0);

  anaRocMap(dirname, "Scurves/sig_scurveVcal_Vcal", fSMS->distNoise, 2);
  anaRocMap(dirname, "Trim/thr_TrimThrFinal_vcal", fSMS->distVcalTrimThr, 2);
  
  fillRocDefects(dirname, fSMS->defectMap);


  gStyle->SetOptStat(0); 
  c0->Clear(); 
  c0->Divide(3, 3);

  c0->cd(1); 
  double xpos(0.20);

  plotVsRoc(fSMS->vana, xpos, 0.2); 
  plotVsRoc(fSMS->caldel, xpos, 0.8, "same"); 

  c0->cd(2); 
  plotVsRoc(fSMS->phscale, xpos, 0.2); 
  plotVsRoc(fSMS->phoffset, xpos, 0.8, "same"); 

  plotVsRoc(fSMS->vtrim, xpos, 0.55, "same"); 
  plotVsRoc(fSMS->vthrcomp, xpos, 0.35, "same"); 

  c0->cd(3); 
  fSMS->distVcalTrimThr->Draw("hist");
  tl->SetTextSize(0.05); 
  tl->DrawLatex(0.20, 0.92, Form("<25: %d", static_cast<int>(fSMS->distVcalTrimThr->Integral(0, fSMS->distVcalTrimThr->FindBin(25.))))); 
  tl->DrawLatex(0.25, 0.80, Form("mean: %7.3f", fSMS->distVcalTrimThr->GetMean()));
  tl->DrawLatex(0.25, 0.75, Form("RMS:  %7.3f", fSMS->distVcalTrimThr->GetRMS()));
  gPad->SetLogy(1); 
  
  c0->cd(4); 
  plotVsRoc(fSMS->vcalThr, xpos, 0.8); 
  plotVsRoc(fSMS->vcalTrimThr, xpos, 0.2, "same"); 

//   c0->cd(5); 
//   plotVsRoc(fSMS->nonl, xpos, 0.85); 
//   plotVsRoc(fSMS->nonlW, xpos, 0.2, "same"); 

  c0->cd(5); 
  plotVsRoc(fSMS->vcalThrW, xpos, 0.80, ""); 
  plotVsRoc(fSMS->vcalTrimThrW, xpos, 0.15, "same"); 
  plotVsRoc(fSMS->noise, xpos, 0.40, "same"); 

  c0->cd(6); 
  fSMS->distNoise->Draw("hist");
  tl->SetTextSize(0.05); 
  tl->DrawLatex(0.20, 0.92, Form("<0.5: %d", static_cast<int>(fSMS->distNoise->Integral(0, fSMS->distNoise->FindBin(0.5))))); 
  tl->DrawLatex(0.55, 0.80, Form("mean: %7.3f", fSMS->distNoise->GetMean()));
  tl->DrawLatex(0.55, 0.75, Form("RMS:  %7.3f", fSMS->distNoise->GetRMS()));
  gPad->SetLogy(1); 

  c0->cd(7); 
  plotVsRoc(fSMS->dead, xpos, 0.80, "", 1); 
  plotVsRoc(fSMS->bb,   xpos, 0.74, "same", 1); 
  plotVsRoc(fSMS->mask, xpos, 0.68, "same", 1); 
  plotVsRoc(fSMS->addr, xpos, 0.62, "same", 1); 

  c0->cd(8); 
  int customPalette[4];
  customPalette[0] = kBlue-4;
  customPalette[1] = kBlack;
  customPalette[2] = kRed;
  customPalette[3] = kGreen+2;

  gStyle->SetPalette(4, customPalette); 
  fSMS->defectMap->Draw("col");

  tl->SetTextSize(0.07); 
  tl->SetTextColor(kBlack); 
  tl->DrawLatex(0.18, 0.92, "all modules defects"); 


  c0->cd(9); 
  tl->SetTextSize(0.13); 
  tl->SetTextColor(kBlack); 
  tl->DrawLatex(0.0, 0.90, modname.c_str()); 

  tl->SetTextSize(0.08); 
  tl->DrawLatex(0.0, 0.75, "Date:");           tl->DrawLatex(0.5, 0.75, date.c_str()); 
  tl->DrawLatex(0.0, 0.65, "Start:");          tl->DrawLatex(0.5, 0.65, startTest.c_str()); 
  tl->DrawLatex(0.0, 0.55, "Duration:");       tl->DrawLatex(0.5, 0.55, duration.c_str()); 
  tl->DrawLatex(0.0, 0.35, "# crit. errors:"); tl->DrawLatex(0.5, 0.35, criticals.c_str()); 

  
  c0->SaveAs(Form("%s.pdf", modname.c_str())); 



  
  gStyle->SetOptStat(0); 
  c0->Clear(); 
  c0->Divide(3, 3);
  

  tl->SetTextSize(0.07); 

  c0->cd(1);
  fhVana->Draw();
  tl->SetTextColor(fhVana->GetLineColor()); 
  tl->DrawLatex(0.18, 0.92, "VANA"); 
  tl->DrawLatex(0.55, 0.75, Form("%3.1f/%3.1f", fhVana->GetMean(), fhVana->GetRMS())); 
  fhCaldel->Draw("same");
  tl->SetTextColor(fhCaldel->GetLineColor()); 
  tl->DrawLatex(0.58, 0.92, "CALDEL"); 
  tl->DrawLatex(0.55, 0.65, Form("%3.1f/%3.1f", fhCaldel->GetMean(), fhCaldel->GetRMS())); 

  c0->cd(2);
  fhPhscale->Draw();
  tl->SetTextColor(fhPhscale->GetLineColor()); 
  tl->DrawLatex(0.18, 0.92, "PHSCALE"); 
  tl->DrawLatex(0.55, 0.75, Form("%3.1f/%3.1f", fhPhscale->GetMean(), fhPhscale->GetRMS())); 
  fhPhoffset->Draw("same");
  tl->SetTextColor(fhPhoffset->GetLineColor()); 
  tl->DrawLatex(0.58, 0.92, "PHOFFSET"); 
  tl->DrawLatex(0.55, 0.65, Form("%3.1f/%3.1f", fhPhoffset->GetMean(), fhPhoffset->GetRMS())); 

  c0->cd(3);
  fhVthrcomp->Draw();
  tl->SetTextColor(fhVthrcomp->GetLineColor()); 
  tl->DrawLatex(0.18, 0.92, "VTHRCOMP"); 
  tl->DrawLatex(0.55, 0.75, Form("%3.1f/%3.1f", fhVthrcomp->GetMean(), fhVthrcomp->GetRMS())); 
  fhVtrim->Draw("same");
  tl->SetTextColor(fhVtrim->GetLineColor()); 
  tl->DrawLatex(0.58, 0.92, "VTRIM"); 
  tl->DrawLatex(0.55, 0.65, Form("%3.1f/%3.1f", fhVtrim->GetMean(), fhVtrim->GetRMS())); 

  c0->cd(4);
  fhDuration->Draw();
  tl->SetTextColor(fhDuration->GetLineColor()); 
  tl->DrawLatex(0.18, 0.92, Form("Mean/RMS: %4.1f/%4.1f", fhDuration->GetMean(), fhDuration->GetRMS())); 

  c0->cd(5);
  fhCritical->Draw();
  tl->SetTextColor(fhCritical->GetLineColor()); 
  tl->DrawLatex(0.18, 0.92, Form("Mean/RMS: %4.3f/%4.3f", fhCritical->GetMean(), fhCritical->GetRMS())); 

  c0->cd(6);
  gPad->SetLogy(1);
  fhDead->Draw();
  tl->SetTextColor(fhDead->GetLineColor()); 
  tl->DrawLatex(0.18, 0.92, "Dead"); 
  tl->DrawLatex(0.38, 0.75, Form("%4.3f/%4.3f", fhDead->GetMean(), fhDead->GetRMS())); 
  fhBb->Draw("same");
  tl->SetTextColor(fhBb->GetLineColor()); 
  tl->DrawLatex(0.58, 0.92, "BB"); 
  tl->DrawLatex(0.38, 0.65, Form("%4.3f/%4.3f", fhBb->GetMean(), fhBb->GetRMS())); 

  c0->cd(7);
  gPad->SetLogy(1);
  fhNoise->Draw();
  tl->SetTextColor(fhNoise->GetLineColor()); 
  tl->DrawLatex(0.18, 0.92, Form("noise: %4.3f/%4.3f", fhNoise->GetMean(), fhNoise->GetRMS())); 

  c0->cd(8);
  gPad->SetLogy(1);
  fhVcaltrimthr->Draw();
  tl->SetTextColor(fhVcaltrimthr->GetLineColor()); 
  tl->DrawLatex(0.18, 0.92, Form("trim thr: %4.3f/%4.3f", fhVcaltrimthr->GetMean(), fhVcaltrimthr->GetRMS())); 

  c0->cd(9);

  tl->SetTextSize(0.07); 
  tl->DrawLatex(0.0, 0.75, "# modules:");   tl->DrawLatex(0.5, 0.75, Form("%d", static_cast<int>(fhCritical->GetEntries()))); 
  tl->DrawLatex(0.0, 0.65, "# pixels:");    tl->DrawLatex(0.5, 0.65, Form("%d", static_cast<int>(fhNoise->GetEntries()))); 
  tl->DrawLatex(0.0, 0.55, "# low-thr (<25):");   
  tl->DrawLatex(0.5, 0.55, Form("%d", static_cast<int>(fhVcaltrimthr->Integral(0, fhVcaltrimthr->FindBin(25.))))); 
  tl->DrawLatex(0.5, 0.45, Form("(%3.2e)", fhVcaltrimthr->Integral(0, fhVcaltrimthr->FindBin(25.))/fhVcaltrimthr->Integral()));
  c0->SaveAs("anaFullTest-summary.pdf"); 


}



// ----------------------------------------------------------------------
void anaFullTest::bookModuleSummary(string modulename) {

  moduleSummary *ms = new moduleSummary; 
  ms->moduleName = modulename; 

  for (unsigned int idac = 0; idac < fDacs.size(); ++idac) {
    for (int iroc = 0; iroc < fNrocs+1; ++iroc) {
      TH1D *h = new TH1D(Form("%s_%s_C%d", modulename.c_str(), fDacs[idac].c_str(), iroc), 
			 Form("%s_%s_C%d", modulename.c_str(), fDacs[idac].c_str(), iroc), 
			 256, 0., 256.); 
      if (fDacs[idac] == "vana") ms->vana.push_back(h); 
      if (fDacs[idac] == "caldel") ms->caldel.push_back(h); 
      if (fDacs[idac] == "vthrcomp") ms->vthrcomp.push_back(h); 
      if (fDacs[idac] == "vtrim") ms->vtrim.push_back(h); 
      if (fDacs[idac] == "phscale") ms->phscale.push_back(h); 
      if (fDacs[idac] == "phoffset") ms->phoffset.push_back(h); 
    }
    TH1D *h = new TH1D(Form("%s_%s_rms", modulename.c_str(), fDacs[idac].c_str()), 
		       Form("%s_%s_rms", modulename.c_str(), fDacs[idac].c_str()), 
		       50, 0., 5.); 

    if (fDacs[idac] == "vana") ms->rmsVana = h; 
    if (fDacs[idac] == "caldel") ms->rmsCaldel = h; 
    if (fDacs[idac] == "vthrcomp") ms->rmsVthrcomp = h; 
    if (fDacs[idac] == "vtrim") ms->rmsVtrim = h; 
    if (fDacs[idac] == "phscale") ms->rmsPhscale = h; 
    if (fDacs[idac] == "phoffset") ms->rmsPhoffset = h; 
  }

  string what(""); 
  for (int iroc = 0; iroc < fNrocs+1; ++iroc) {
    what = "ndeadpixels"; 
    TH1D *h = new TH1D(Form("%s_%s_C%d", modulename.c_str(), what.c_str(), iroc), 
		       Form("%s_%s_C%d", modulename.c_str(), what.c_str(), iroc), 
		       100, 0., 100.);
    ms->ndeadpixels.push_back(h); 

    what = "ndeadbumps"; 
    h = new TH1D(Form("%s_%s_C%d", modulename.c_str(), what.c_str(), iroc), 
		 Form("%s_%s_C%d", modulename.c_str(), what.c_str(), iroc), 
		 100, 0., 100.);
    ms->ndeadbumps.push_back(h); 

    what = "deadsep"; 
    h = new TH1D(Form("%s_%s_C%d", modulename.c_str(), what.c_str(), iroc), 
		 Form("%s_%s_C%d", modulename.c_str(), what.c_str(), iroc), 
		 150, 0., 150.);
    ms->deadsep.push_back(h); 
    
  }  

  ms->trimthrpos = new TH1D(Form("%s_trimthrpos", modulename.c_str()), 
			    Form("%s_trimthrpos", modulename.c_str()), 
			    80, 34., 36.); 

  ms->trimthrpos->SetXTitle("VCAL [DAC]");

  ms->trimthrrms = new TH1D(Form("%s_trimthrrms", modulename.c_str()), 
			    Form("%s_trimthrrms", modulename.c_str()), 
			    50, 0., 2.); 
  ms->trimthrrms->SetXTitle("VCAL [DAC]");

  ms->p1pos = new TH1D(Form("%s_p1pos", modulename.c_str()), 
		    Form("%s_p1pos", modulename.c_str()), 
		    50, 0.5, 1.0); 

  ms->p1rms = new TH1D(Form("%s_p1rms", modulename.c_str()), 
		    Form("%s_p1rms", modulename.c_str()), 
		    50, 0., 0.2); 
  
  fModSummaries.insert(make_pair(modulename, ms)); 

}


// ----------------------------------------------------------------------
void anaFullTest::validateFullTests() {
  // -- PSI module
  addFullTests("D14-0001", "-003");
  // -- ETH modules 
  addFullTests("D14-0006", "-000");
  addFullTests("D14-0008", "-000");
  addFullTests("D14-0009", "-000");

  TH1D *hVana = new TH1D("hVana", Form("Vana %s", fDiffMetric == 0?"difference":"RMS"), 50, 0., 5.);
  TH1D *hCaldel = new TH1D("hCaldel", Form("CalDel %s", fDiffMetric == 0?"difference":"RMS"), 50, 0., 5.);
  TH1D *hVthrcomp = new TH1D("hVthrcomp", Form("VthrComp %s", fDiffMetric == 0?"difference":"RMS"), 50, 0., 5.);
  TH1D *hVtrim = new TH1D("hVtrim", Form("Vtrim %s", fDiffMetric == 0?"difference":"RMS"), 50, 0., 10.);
  TH1D *hPhs = new TH1D("hPhs", Form("phscale %s", fDiffMetric == 0?"difference":"RMS"), 50, 0., 5.);
  TH1D *hPho = new TH1D("hPho", Form("phoffset %s", fDiffMetric == 0?"difference":"RMS"), 50, 0., 5.);

  map<string, moduleSummary*>::iterator ib = fModSummaries.begin();

  TH1D *hTrimThrPos = (TH1D*)ib->second->trimthrpos->Clone("hTrimThrPos");
  hTrimThrPos->SetTitle("Trim Threshold");
  hTrimThrPos->Reset();

  TH1D *hTrimThrRms = (TH1D*)ib->second->trimthrrms->Clone("hTrimThrRms");
  hTrimThrRms->SetTitle("Trim Threshold RMS");
  hTrimThrRms->Reset();

  TH1D *hp1Pos = (TH1D*)ib->second->p1pos->Clone("hp1Pos");
  hp1Pos->SetTitle("p1");
  hp1Pos->Reset();

  TH1D *hp1Rms = (TH1D*)ib->second->p1rms->Clone("hp1Rms");
  hp1Rms->SetTitle("p1 RMS");
  hp1Rms->Reset();

  for (map<string, moduleSummary*>::iterator it = fModSummaries.begin(); it != fModSummaries.end(); ++it) {

    cout << "summarizing " << it->second->moduleName << endl;
    for (int iroc = 0; iroc < fNrocs; ++iroc) {
      hVana->Fill(diff(it->second->vana[iroc])); 
      hCaldel->Fill(diff(it->second->caldel[iroc])); 
      hVthrcomp->Fill(diff(it->second->vthrcomp[iroc])); 
      hVtrim->Fill(diff(it->second->vtrim[iroc])); 
      hPhs->Fill(diff(it->second->phscale[iroc])); 
      hPho->Fill(diff(it->second->phoffset[iroc])); 
    }
    
    for (int ibin = 1; ibin <= it->second->trimthrpos->GetNbinsX(); ++ibin) {
      hTrimThrPos->SetBinContent(ibin, hTrimThrPos->GetBinContent(ibin)+it->second->trimthrpos->GetBinContent(ibin)); 
    }

    for (int ibin = 1; ibin <= it->second->trimthrrms->GetNbinsX(); ++ibin) {
      hTrimThrRms->SetBinContent(ibin, hTrimThrRms->GetBinContent(ibin) + it->second->trimthrrms->GetBinContent(ibin)); 
    }

    for (int ibin = 1; ibin <= it->second->p1pos->GetNbinsX(); ++ibin) {
      hp1Pos->SetBinContent(ibin, hp1Pos->GetBinContent(ibin)+it->second->p1pos->GetBinContent(ibin)); 
    }

    for (int ibin = 1; ibin <= it->second->p1rms->GetNbinsX(); ++ibin) {
      hp1Rms->SetBinContent(ibin, hp1Rms->GetBinContent(ibin) + it->second->p1rms->GetBinContent(ibin)); 
    }
    
  }
			  
			  
  c0->Clear();
  c0->cd(1);

  hVana->Draw();
  c0->SaveAs("ftval-vana.pdf"); 
  
  hCaldel->Draw();
  c0->SaveAs("ftval-caldel.pdf"); 

  hVthrcomp->Draw();
  c0->SaveAs("ftval-vthrcomp.pdf"); 

  hVtrim->Draw();
  c0->SaveAs("ftval-vtrim.pdf"); 

  hPhs->Draw();
  c0->SaveAs("ftval-phscale.pdf"); 

  hPho->Draw();
  c0->SaveAs("ftval-phoffset.pdf"); 

  hTrimThrPos->Draw();
  c0->SaveAs("ftval-trimthrpos.pdf"); 

  hTrimThrRms->Draw();
  c0->SaveAs("ftval-trimthrrms.pdf"); 

  hp1Pos->Draw();
  c0->SaveAs("ftval-p1pos.pdf"); 

  hp1Rms->Draw();
  c0->SaveAs("ftval-p1rms.pdf"); 


}


// ----------------------------------------------------------------------
void anaFullTest::addFullTests(string mname, string mpattern) {
  vector<string> dirs = glob(".", mname+mpattern); 
  cout << dirs.size() << endl;
  for (unsigned int idirs = 0; idirs < dirs.size(); ++idirs) {
    cout << dirs[idirs] << endl;
  }

  bookModuleSummary(mname); 

  for (unsigned int idirs = 0; idirs < dirs.size(); ++idirs) {
    readDacFile(dirs[idirs], "vana", fModSummaries[mname]->vana);
    readDacFile(dirs[idirs], "caldel", fModSummaries[mname]->caldel);
    readDacFile(dirs[idirs], "vthrcomp", fModSummaries[mname]->vthrcomp);
    readDacFile(dirs[idirs], "vtrim", fModSummaries[mname]->vtrim);
    readDacFile(dirs[idirs], "phscale", fModSummaries[mname]->phscale);
    readDacFile(dirs[idirs], "phoffset", fModSummaries[mname]->phoffset);

   
//     readLogFile(dirs[idirs], "number of dead pixels (per ROC):", fModSummaries[mname]->ndeadpixels);
//     readLogFile(dirs[idirs], "number of dead bumps (per ROC):", fModSummaries[mname]->ndeadbumps);
//     readLogFile(dirs[idirs], "separation cut       (per ROC):", fModSummaries[mname]->deadsep);    
    readLogFile(dirs[idirs], "vcal mean:", fModSummaries[mname]->trimthrpos);
    readLogFile(dirs[idirs], "vcal RMS:", fModSummaries[mname]->trimthrrms);
    readLogFile(dirs[idirs], "p1 mean:", fModSummaries[mname]->p1pos);
    readLogFile(dirs[idirs], "p1 RMS:", fModSummaries[mname]->p1rms);
      
    /*
      vcal mean:
      vcal RMS: 
      bits mean:
      bits RMS: 

      VthrComp mean:
      VthrComp RMS:
      Vcal mean:
      Vcal RMS:
     */
  }

  for (int iroc = 0; iroc < fNrocs; ++iroc) {
    fModSummaries[mname]->rmsVana->Fill(diff(fModSummaries[mname]->vana[iroc])); 
    fModSummaries[mname]->rmsCaldel->Fill(diff(fModSummaries[mname]->caldel[iroc])); 
    fModSummaries[mname]->rmsVthrcomp->Fill(diff(fModSummaries[mname]->vthrcomp[iroc])); 
    fModSummaries[mname]->rmsVtrim->Fill(diff(fModSummaries[mname]->vtrim[iroc])); 
    fModSummaries[mname]->rmsPhscale->Fill(diff(fModSummaries[mname]->phscale[iroc])); 
    fModSummaries[mname]->rmsPhoffset->Fill(diff(fModSummaries[mname]->phoffset[iroc])); 
  }

  c0->Clear(); 
  c0->Divide(3,3);

  c0->cd(1); 
  fModSummaries[mname]->rmsVana->Draw();
  c0->cd(2); 
  fModSummaries[mname]->rmsCaldel->Draw();
  
  c0->cd(3); 
  fModSummaries[mname]->rmsVthrcomp->Draw();
  
  c0->cd(4); 
  fModSummaries[mname]->rmsVtrim->Draw();

  c0->cd(5); 
  fModSummaries[mname]->rmsPhscale->Draw();

  c0->cd(6); 
  fModSummaries[mname]->rmsPhoffset->Draw();

  c0->cd(7); 
  fModSummaries[mname]->trimthrpos->Draw();

  c0->cd(8); 
  fModSummaries[mname]->trimthrrms->Draw();

  c0->cd(9); 
  fModSummaries[mname]->p1pos->Draw();
  
  c0->SaveAs(Form("%s.pdf", mname.c_str())); 

}

// ----------------------------------------------------------------------
void anaFullTest::readLogFile(std::string dir, std::string tag, std::vector<TH1D*> hists) {

  ifstream IN; 

  char buffer[1000];
  string sline; 
  string::size_type s1;
  vector<double> x;
  IN.open(Form("%s/pxar.log", dir.c_str())); 
  while (IN.getline(buffer, 1000, '\n')) {
    sline = buffer; 
    s1 = sline.find(tag.c_str()); 
    if (string::npos == s1) continue;
    sline = sline.substr(s1+tag.length()+1);
    break;
  }

  x = splitIntoRocs(sline);
  for (unsigned int i = 0; i < x.size(); ++i) {
    //     cout << "Filling into " << hist->GetName() << " x = " << x[i] << endl;
    hists[i]->Fill(x[i]); 
  }

  IN.close(); 
}

// ----------------------------------------------------------------------
void anaFullTest::readLogFile(std::string dir, std::string tag, TH1D* hist) {

  ifstream IN; 

  char buffer[1000];
  string sline; 
  string::size_type s1;
  vector<double> x;
  IN.open(Form("%s/pxar.log", dir.c_str())); 
  while (IN.getline(buffer, 1000, '\n')) {
    sline = buffer; 
    s1 = sline.find(tag.c_str()); 
    if (string::npos == s1) continue;
    sline = sline.substr(s1+tag.length()+1);
    break;
  }
  x = splitIntoRocs(sline);
  for (unsigned int i = 0; i < x.size(); ++i) {
    //     cout << "Filling into " << hist->GetName() << " x = " << x[i] << endl;
    hist->Fill(x[i]); 
  }

  IN.close(); 
}


// ----------------------------------------------------------------------
void anaFullTest::fillRocHist(string dirname, string hbasename, TH1D* rochist, int mode) {
  TFile *f = TFile::Open((dirname+"/pxar.root").c_str()); 
  TH1D *h(0); 
  for (int i = 0; i < fNrocs; ++i) {
    h = (TH1D*)f->Get(Form("%s_C%d_V0", hbasename.c_str(), i)); 
    if (h) {
      if (0 == mode) {
	rochist->SetBinContent(i+1, h->GetMean()); 
      } else if (1 == mode) {
	rochist->SetBinContent(i+1, h->GetRMS()); 
      }
    }
  }
  f->Close();
}


// ----------------------------------------------------------------------
void anaFullTest::anaRocMap(std::string dirname, std::string hbasename, TH1D* rochist, int mode) {
  TFile *f = TFile::Open((dirname+"/pxar.root").c_str()); 

  TH1D *hs(0); 
  if (string(rochist->GetName()) == string("dead")) {
    hs = fhDead;
  } else if (string(rochist->GetName()) == string("mask")) {
    hs = fhMask;
  } else if (string(rochist->GetName()) == string("addr")) {
    hs = fhAddr;
  } else if (string(rochist->GetName()) == string("distNoise")) {
    hs = fhNoise;
  } else if (string(rochist->GetName()) == string("distVcalTrimThr")) {
    hs = fhVcaltrimthr;
  }
  
  TH2D *h(0); 
  int cnt(0); 
  for (int i = 0; i < fNrocs; ++i) {
    h = (TH2D*)f->Get(Form("%s_C%d_V0", hbasename.c_str(), i)); 
    cnt = 0; 
    if (h) {
      if (0 == mode) {
	// -- zero entries are missing
	for (int ix = 0; ix < h->GetNbinsX(); ++ix) {
	  for (int iy = 0; iy < h->GetNbinsY(); ++iy) {
	    if (h->GetBinContent(ix+1, iy+1) < 0.1) ++cnt;
	  }
	} 
	rochist->SetBinContent(i+1, cnt); 
	hs->Fill(cnt);
      } else if (1 == mode) {
	// -- negative entries are missing (zero are dead pixels)
	for (int ix = 0; ix < h->GetNbinsX(); ++ix) {
	  for (int iy = 0; iy < h->GetNbinsY(); ++iy) {
	    if (h->GetBinContent(ix+1, iy+1) < -0.1) ++cnt;
	  }
	} 
	rochist->SetBinContent(i+1, cnt); 
	hs->Fill(cnt);
      } else if (2 == mode) {
	// -- create histogram (projection)
	double val(-1.); 
	for (int ix = 0; ix < h->GetNbinsX(); ++ix) {
	  for (int iy = 0; iy < h->GetNbinsY(); ++iy) {
	    val = h->GetBinContent(ix+1, iy+1);
	    rochist->Fill(val); 
	    hs->Fill(val);
	  }
	} 
      } 
    }
  }
  f->Close();
}


// ----------------------------------------------------------------------
void anaFullTest::fillRocDefects(string dirname, TH2D *hmap) {

  TFile *f = TFile::Open((dirname+"/pxar.root").c_str()); 

  vector<TH1D*> vh; 
  TH1D *h(0); 
  for (int iroc = 0; iroc < fNrocs+1; ++iroc) {
    h = (TH1D*)gROOT->Get(Form("h_C%d", iroc)); 
    if (!h) h = new TH1D(Form("h_C%d", iroc), Form("h_C%d", iroc), 256, 0., 256.); 
    vh.push_back(h); 
  }

  // -- dead bumps
  readLogFile(dirname, "separation cut       (per ROC):", vh);
  TH2D *h2(0); 
  for (int i = 0; i < fNrocs; ++i) {
    h2 = (TH2D*)f->Get(Form("BumpBonding/thr_calSMap_VthrComp_C%d_V0", i)); 
    if (h2) {
      for (int ix = 0; ix < h2->GetNbinsX(); ++ix) {
	for (int iy = 0; iy < h2->GetNbinsY(); ++iy) {
	  if (h2->GetBinContent(ix+1, iy+1) > vh[i]->GetMean()) {
	    hmap->SetBinContent(ix+1, iy+1, 1); 
	  }
	}
      } 
      
    }
  }

  // -- dead pixels
  for (int i = 0; i < fNrocs; ++i) {
    h2 = (TH2D*)f->Get(Form("PixelAlive/PixelAlive_C%d_V0", i)); 
    if (h2) {
      for (int ix = 0; ix < h2->GetNbinsX(); ++ix) {
	for (int iy = 0; iy < h2->GetNbinsY(); ++iy) {
	  if (h2->GetBinContent(ix+1, iy+1) < 1) {
	    hmap->SetBinContent(ix+1, iy+1, 2); 
	  }
	}
      } 
      
    }
  }

  // -- mask problems
  for (int i = 0; i < fNrocs; ++i) {
    h2 = (TH2D*)f->Get(Form("PixelAlive/MaskTest_C%d_V0", i)); 
    if (h2) {
      for (int ix = 0; ix < h2->GetNbinsX(); ++ix) {
	for (int iy = 0; iy < h2->GetNbinsY(); ++iy) {
	  if (h2->GetBinContent(ix+1, iy+1) < -0.1) {
	    hmap->SetBinContent(ix+1, iy+1, 3); 
	  }
	}
      } 
    }
  }

  // -- address problems
  for (int i = 0; i < fNrocs; ++i) {
    h2 = (TH2D*)f->Get(Form("PixelAlive/AddressDecodingTest_C%d_V0", i)); 
    if (h2) {
      for (int ix = 0; ix < h2->GetNbinsX(); ++ix) {
	for (int iy = 0; iy < h2->GetNbinsY(); ++iy) {
	  if (h2->GetBinContent(ix+1, iy+1) < -0.1) {
	    hmap->SetBinContent(ix+1, iy+1, 4); 
	  }
	}
      } 
    }
  }

  

}
 



// ----------------------------------------------------------------------
void anaFullTest::readDacFile(string dir, string dac, vector<TH1D*> vals) {
  ifstream IN; 

  char buffer[1000];
  string sline; 
  string::size_type s1;
  int val(0); 
  for (int i = 0; i < fNrocs; ++i) {
    IN.open(Form("%s/dacParameters%d_C%d.dat", dir.c_str(), fTrimVcal, i)); 
    while (IN.getline(buffer, 1000, '\n')) {
      if (buffer[0] == '#') {continue;}
      if (buffer[0] == '/') {continue;}
      if (buffer[0] == '\n') {continue;}
      sline = buffer; 
      s1 = sline.find(dac.c_str()); 
      if (string::npos != s1) {
	sline = sline.substr(s1+dac.length()+1); 
	val = atoi(sline.c_str()); 
	vals[i]->Fill(val); 
	vals[fNrocs]->Fill(val); 
      }
    }
    IN.close(); 
  }

}



// ----------------------------------------------------------------------
vector<string> anaFullTest::glob(string basedir, string basename) {
  vector<string> lof; 
#if defined(WIN32)
#else
  TString fname;
  const char *file;
  TSystem *lunix = gSystem; //new TUnixSystem();
  void *pDir = lunix->OpenDirectory(basedir.c_str());
  while ((file = lunix->GetDirEntry(pDir))) {
    fname = file;
    if (fname.Contains(basename.c_str())) {
      lof.push_back(string(fname));
    }
  }  
#endif
  return lof; 
}


// ----------------------------------------------------------------------
double anaFullTest::diff(TH1D *h) {

  if (0 == fDiffMetric) {
    int minbin = h->FindFirstBinAbove(0.1); 
    int maxbin = h->FindLastBinAbove(0.1); 
    return (maxbin-minbin);
  } else if (1 == fDiffMetric) {
    return h->GetRMS(); 
  }

  return -1.; 
}


// ----------------------------------------------------------------------
std::vector<double> anaFullTest::splitIntoRocs(std::string line) {
  istringstream istring(line);
  vector<double> result; 
  double x(0.); 
  //  cout << "splitting: " << line << endl;
  for (int iroc = 0; iroc < fNrocs; ++iroc) {
    istring >> x;
    result.push_back(x); 
  }

  for (unsigned int i = 0; i < result.size(); ++i) {
    //    cout << result[i] << " "; 
  }
  //  cout << endl;

  return result;
}


// ----------------------------------------------------------------------
void anaFullTest::clearHistVector(vector<TH1D*> vh) {
  for (unsigned int i = 0; i < vh.size(); ++i) {
    vh[i]->Reset();
  }
}


// ----------------------------------------------------------------------
void anaFullTest::dumpVector(vector<TH1D*> vh, TH1D *h, string opt) {
  for (unsigned int i = 0; i < vh.size(); ++i) {
    if (opt == "0") {
      h->SetBinContent(i+1, vh[i]->GetMean()); 
    } else if (opt == "1") {
      h->SetBinContent(i+1, vh[i]->GetRMS()); 
    } else if (opt == "2") {
      h->SetBinContent(i+1, vh[i]->GetMean()); 
      h->SetBinError(i+1, vh[i]->GetMeanError()); 
    }
  }

}

// ----------------------------------------------------------------------
void anaFullTest::summarizeVector(vector<TH1D*> vh, TH1D *h) {
  for (unsigned int i = 0; i < vh.size(); ++i) {
    h->Fill(vh[i]->GetMean()); 
  }

}


// ----------------------------------------------------------------------
void anaFullTest::setHist(TH1D *h, std::string sx, std::string sy, int color, double miny, double maxy) {

  h->SetLineWidth(2); 
  h->SetMarkerSize(1.3); 

  double xoff(1.2), yoff(1.2), size(0.05), lsize(0.05); 

  h->SetXTitle(sx.c_str());          h->SetYTitle(sy.c_str()); 
  h->SetTitleOffset(xoff, "x");      h->SetTitleOffset(yoff, "y");
  h->SetTitleSize(size, "x");        h->SetTitleSize(size, "y");
  h->SetLabelSize(lsize, "x");       h->SetLabelSize(lsize, "y");

  h->SetMinimum(miny); 
  if (maxy > 0.) h->SetMaximum(maxy); 
  
  h->SetMarkerColor(color); 
  h->SetLineColor(color); 
  

}


// ----------------------------------------------------------------------
void anaFullTest::plotVsRoc(TH1D *h, double tx, double ty, std::string s, int mode) {
  h->Draw(s.c_str()); 
  tl->SetTextColor(h->GetLineColor());
  int total; 
  double mean, rms;
  projectRocHist(h, mean, rms, total); 
  if (0 == mode) {
    tl->DrawLatex(tx, ty, Form("%s (%4.2f, %4.2f)", h->GetName(), mean, rms)); 
  } else if (1 == mode) {
    tl->DrawLatex(tx, ty, Form("%s (%d)", h->GetName(), total)); 
  }
}


// ----------------------------------------------------------------------
void anaFullTest::projectRocHist(TH1D *h, double &mean, double &rms, int &total) {

  TH1D *h1 = new TH1D("h1", "", 1000, h->GetMinimum(), h->GetMaximum()); 
  total = 0; 
  for (int i = 1; i <= h->GetNbinsX(); ++i) {
    h1->Fill(h->GetBinContent(i)); 
    total += h->GetBinContent(i);
  }
  mean = h1->GetMean(); 
  rms  = h1->GetRMS(); 
  delete h1; 
}


// ----------------------------------------------------------------------
string anaFullTest::readLine(string dir, string pattern, int mode) {

  //  cout << "readLine: " << Form("%s/pxar.log", dir.c_str()) << endl;
  ifstream IN; 

  char buffer[1000];
  string sline; 
  string::size_type s1;
  IN.open(Form("%s/pxar.log", dir.c_str())); 
  while (IN.getline(buffer, 1000, '\n')) {
    sline = buffer; 
    s1 = sline.find(pattern.c_str()); 
    if (string::npos == s1) continue;
    if (0 == mode) {
      // return entire line
      break;
    } else if (1 == mode) {
      // return string after pattern
      sline = sline.substr(s1+pattern.length()+1);
    } else if (2 == mode) {
      // return string beforepattern
      sline = sline.substr(0, s1);
    }
    break;
  }
  IN.close();
  return sline; 

}


// ----------------------------------------------------------------------
int anaFullTest::countWord(string dir, string pattern) {
  ifstream IN; 

  char buffer[1000];
  string sline; 
  string::size_type s1;
  IN.open(Form("%s/pxar.log", dir.c_str())); 
  int cnt(0); 
  while (IN.getline(buffer, 1000, '\n')) {
    sline = buffer; 
    s1 = sline.find(pattern.c_str()); 
    if (string::npos == s1) continue;
    ++cnt;
  }
  IN.close();
  return cnt; 
}


// ----------------------------------------------------------------------
int anaFullTest::testDuration(string startTest, string endTest) {
  int h0, h1, m0, m1, s0, s1; 
  string ss0 = startTest.substr(0, startTest.rfind(".")); 
  string ss1 = endTest.substr(0, endTest.rfind(".")); 

  string::size_type st0, st1;

  string parse = ss0; 
  st0 = parse.find(":");
  st1 = parse.find(":", st0+1);
  
  h0 = atoi(parse.substr(0, st0).c_str());
  m0 = atoi(parse.substr(st0+1, st1-st0-1).c_str());
  s0 = atoi(parse.substr(st1+1).c_str());

  parse = ss1; 
  st0 = parse.find(":");
  st1 = parse.find(":", st0+1);

  h1 = atoi(parse.substr(0, st0).c_str());
  m1 = atoi(parse.substr(st0+1, st1-st0-1).c_str());
  s1 = atoi(parse.substr(st1+1).c_str());
  
  struct timeval tv0 = {h0*60*60 + m0*60 + s0, 0};
  struct timeval tv1 = {h1*60*60 + m1*60 + s1, 0};
  
  return tv1.tv_sec - tv0.tv_sec;
}
