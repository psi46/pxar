#include "anaFullTest.hh"

#include <fstream>
#include <sstream>
#include <cstdlib>
#include <time.h>

#include <TROOT.h>
#include <TSystem.h>
#include <TStyle.h>
#include <TFile.h>
#include <TPad.h>
#include <TLegend.h>
#if defined(WIN32)
#include <Winsock2.h>
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
  fhVcalthr     = new TH1D("hVcalthr", "", 200, 0., 200.);       setHist(fhVcalthr, "VCAL thr [DAC]", "pixels", kBlack, 0.5, YMAX);

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

  // -- central histograms for all pixels on all ROCs
  fvTrimVcalThr.clear();
  fvTrimBits.clear();
  vector<TH1D*> v0, v1;
  for (int iroc = 0; iroc < fNrocs; ++iroc) {
    v0.clear();
    v1.clear();
    for (int j = 0; j < 4160; ++j) {
      v0.push_back(new TH1D(Form("roc%d_pix%d_thr", iroc, j), Form("roc%d_pix%d_thr", iroc, j), 500, 0., 100.));
      v1.push_back(new TH1D(Form("roc%d_pix%d_trim", iroc, j), Form("roc%d_pix%d_trim", iroc, j), 16, 0., 16.));
    }
    fvTrimVcalThr.push_back(v0); 
    fvTrimBits.push_back(v1); 
  }

  fvNtrig.push_back(5);  fvColor.push_back(kRed+1); 
  fvNtrig.push_back(10); fvColor.push_back(kYellow+2); 
  fvNtrig.push_back(15); fvColor.push_back(kGreen+1); 
  fvNtrig.push_back(20); fvColor.push_back(kCyan+2); 
  fvNtrig.push_back(40); fvColor.push_back(kBlue+1); 
  fvNtrig.push_back(50); fvColor.push_back(kMagenta+3); 


  fTS  = new timingSummary; 
  fTS->tPretest = new TH1D("pretest", "", 150, 0, 300);        fTS->tPretest->GetXaxis()->SetTitle("[sec]");
  fTS->tAlive   = new TH1D("alive", "", 100, 0, 50);           fTS->tAlive->GetXaxis()->SetTitle("[sec]");
  fTS->tBB      = new TH1D("bb", "", 100, 0, 200);             fTS->tBB->GetXaxis()->SetTitle("[sec]");
  fTS->tScurve  = new TH1D("scurve", "", 120, 0, 1200);        fTS->tScurve->GetXaxis()->SetTitle("[sec]");
  fTS->tTrim    = new TH1D("trim", "", 100, 0, 3000);          fTS->tTrim->GetXaxis()->SetTitle("[sec]");
  fTS->tTrimBit = new TH1D("trimbit", "", 100, 0, 3000);       fTS->tTrimBit->GetXaxis()->SetTitle("[sec]"); fTS->tTrimBit->SetLineColor(kRed);
  fTS->tPhOpt   = new TH1D("phopt", "", 200, 0, 1000);         fTS->tPhOpt->GetXaxis()->SetTitle("[sec]");
  fTS->tGain    = new TH1D("gain", "", 150, 0, 300);           fTS->tGain->GetXaxis()->SetTitle("[sec]");
  fTS->tReadback= new TH1D("readback", "", 150, 0, 300);       fTS->tReadback->GetXaxis()->SetTitle("[sec]");
  fTS->tFullTest= new TH1D("fulltest", "", 100, 0, 10000);     fTS->tFullTest->GetXaxis()->SetTitle("[sec]");


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

  if (0 == first) delete fSMS->BB; 
  fSMS->BB = new TH1D("BB", "", 16, 0., 16.);                       setHist(fSMS->BB, "ROC", "dead bumps", kBlue-4, 0., 40.);

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

  if (0 == first) delete fSMS->noiseLevel; 
  fSMS->noiseLevel = new TH1D("noiseLevel", "", 16, 0., 16.);       setHist(fSMS->noiseLevel, "ROC", "Noise level", kBlack, 0., 256.);

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

  if (0 == first) delete fSMS->distVcalThr;
  fSMS->distVcalThr = new TH1D("distVcalThr", "", 200, 0., 200.);         setHist(fSMS->distVcalThr, "VCAL THR [VCAL]", "entries/bin", kBlack, 0.5, -1.);



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

  cout << "criticals: " << criticals << endl;
  
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

  vector<double> vd; 
  
  // -- log file parsing
  readLogFile(dirname, "vcal mean:", vd);
  dumpVector(vd, fSMS->vcalTrimThr, "0"); 

  readLogFile(dirname, "vcal RMS:", vd);
  dumpVector(vd, fSMS->vcalTrimThrW, "0"); 

  readLogFile(dirname, "non-linearity mean:", vd);
  dumpVector(vd, fSMS->nonl, "0"); 

  readLogFile(dirname, "non-linearity RMS:", vd);
  dumpVector(vd, fSMS->nonlW, "0"); 

  // -- note: ideally this should be filled in fillRocDefects?
  readLogFile(dirname, "number of dead bumps (per ROC):", vd);
  dumpVector(vd, fSMS->BB, "0"); 
  summarizeVector(vd, fhBb); 


  // ROOT file 
  anaRocMap(dirname, "PixelAlive/PixelAlive", fSMS->dead, 0);
  anaRocMap(dirname, "PixelAlive/MaskTest", fSMS->mask, 1);
  anaRocMap(dirname, "PixelAlive/AddressDecodingTest", fSMS->addr, 1);

  fillRocHist(dirname, "Scurves/dist_thr_scurveVcal_Vcal", fSMS->vcalThr, 0);
  fillRocHist(dirname, "Scurves/dist_thr_scurveVcal_Vcal", fSMS->vcalThrW, 1);
  fillRocHist(dirname, "Scurves/dist_sig_scurveVcal_Vcal", fSMS->noise, 0);

  anaRocMap(dirname, "Scurves/sig_scurveVcal_Vcal", fSMS->distNoise, 2);
  anaRocMap(dirname, "Trim/thr_TrimThrFinal_vcal", fSMS->distVcalTrimThr, 2);
  anaRocMap(dirname, "Scurves/thr_scurveVcal_Vcal", fSMS->distVcalThr, 2);

  //  findNoiseLevel(dirname, "Pretest/pretestVthrCompCalDel", fSMS->noiseLevel);
  
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
  plotVsRoc(fSMS->vcalTrimThrW, xpos, 0.13, "same"); 
  plotVsRoc(fSMS->noise, xpos, 0.40, "same"); 

  TVirtualPad *c1 = c0->cd(6); 
  c1->Divide(1,2);
  c1->cd(1);
  fSMS->distNoise->Draw("hist");
  tl->SetTextSize(0.08); 
  tl->DrawLatex(0.20, 0.92, Form("<0.5: %d", static_cast<int>(fSMS->distNoise->Integral(0, fSMS->distNoise->FindBin(0.5))))); 
  tl->DrawLatex(0.55, 0.80, Form("mean: %7.3f", fSMS->distNoise->GetMean()));
  tl->DrawLatex(0.55, 0.70, Form("RMS:  %7.3f", fSMS->distNoise->GetRMS()));
  gPad->SetLogy(1); 

  c1->cd(2);
  fSMS->distVcalThr->Draw("hist");
  tl->SetTextColor(kBlack);
  tl->SetTextSize(0.08); 
  tl->DrawLatex(0.18, 0.80, Form("mean: %7.3f", fSMS->distVcalThr->GetMean()));
  tl->DrawLatex(0.18, 0.70, Form("RMS:  %7.3f", fSMS->distVcalThr->GetRMS()));
  gPad->SetLogy(1); 

  c0->cd(7); 
  tl->SetTextSize(0.05); 
  plotVsRoc(fSMS->dead, xpos, 0.80, "", 1); 
  plotVsRoc(fSMS->BB,   xpos, 0.74, "same", 1); 
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
  tl->DrawLatex(0.18, 0.92, "all ROC defects"); 


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
  showOverFlow(fhDuration);
  fhDuration->Draw();
  tl->SetTextColor(fhDuration->GetLineColor()); 
  tl->DrawLatex(0.18, 0.92, Form("Mean/RMS: %4.1f/%4.1f", fhDuration->GetMean(), fhDuration->GetRMS())); 

  c0->cd(5);
  showOverFlow(fhCritical);
  fhCritical->Draw();
  tl->SetTextColor(fhCritical->GetLineColor()); 
  tl->DrawLatex(0.18, 0.92, Form("Mean/RMS: %4.3f/%4.3f", fhCritical->GetMean(), fhCritical->GetRMS())); 

  c0->cd(6);
  gPad->SetLogy(1);
  showOverFlow(fhDead);
  fhDead->Draw();
  tl->SetTextColor(fhDead->GetLineColor()); 
  tl->DrawLatex(0.18, 0.92, "Dead"); 
  tl->DrawLatex(0.38, 0.75, Form("%4.3f/%4.3f", fhDead->GetMean(), fhDead->GetRMS())); 
  showOverFlow(fhBb);
  fhBb->Draw("same");
  tl->SetTextColor(fhBb->GetLineColor()); 
  tl->DrawLatex(0.58, 0.92, "BB"); 
  tl->DrawLatex(0.38, 0.65, Form("%4.3f/%4.3f", fhBb->GetMean(), fhBb->GetRMS())); 

  c1 = c0->cd(7);
  c1->Divide(1,2);
  c1->cd(1);
  gPad->SetLogy(1);
  showOverFlow(fhNoise);
  fhNoise->Draw();
  tl->SetTextColor(fhNoise->GetLineColor()); 
  tl->DrawLatex(0.18, 0.92, Form("noise: %4.3f/%4.3f", fhNoise->GetMean(), fhNoise->GetRMS())); 

  c1->cd(2);
  gPad->SetLogy(1);
  showOverFlow(fhVcalthr);
  fhVcalthr->Draw();
  tl->SetTextColor(fhVcalthr->GetLineColor()); 
  tl->DrawLatex(0.18, 0.92, Form("vcal thr: %4.3f/%4.3f", fhVcalthr->GetMean(), fhVcalthr->GetRMS())); 

  c0->cd(8);
  gPad->SetLogy(1);
  showOverFlow(fhVcaltrimthr);
  fhVcaltrimthr->Draw();
  tl->SetTextColor(fhVcaltrimthr->GetLineColor()); 
  tl->DrawLatex(0.18, 0.92, Form("trim thr: %4.3f/%4.3f", fhVcaltrimthr->GetMean(), fhVcaltrimthr->GetRMS())); 

  c0->cd(9);

  tl->SetTextSize(0.07); 
  tl->DrawLatex(0.0, 0.75, "# modules:");   tl->DrawLatex(0.5, 0.75, Form("%d", static_cast<int>(fhCritical->GetSumOfWeights()))); 
  tl->DrawLatex(0.0, 0.65, "# pixels:");    tl->DrawLatex(0.5, 0.65, Form("%d", static_cast<int>(fhNoise->GetSumOfWeights()))); 
  tl->DrawLatex(0.0, 0.55, "# low-thr (<25):");   
  tl->DrawLatex(0.5, 0.55, Form("%d", static_cast<int>(fhVcaltrimthr->Integral(0, fhVcaltrimthr->FindBin(25.))))); 
  tl->DrawLatex(0.5, 0.45, Form("(%3.2e)", fhVcaltrimthr->Integral(0, fhVcaltrimthr->FindBin(25.))/fhVcaltrimthr->Integral()));
  c0->SaveAs("summary.pdf"); 


}


// ----------------------------------------------------------------------
void anaFullTest::validateTrimTests() {

  addTrimTests("/scratch/ursl/pxar/150828-repro", 0); 

  c0->Clear();
  fvTrimVcalThr[0][1234]->Draw();
  c0->SaveAs("trimvcalthr-roc0-1234-ntrig5.pdf");

  fvTrimVcalThr[1][2345]->Draw();
  c0->SaveAs("trimvcalthr-roc1-2345-ntrig5.pdf");

  fvTrimBits[0][1234]->Draw();
  c0->SaveAs("trimbits-roc0-1234-ntrig5.pdf");

  fvTrimBits[1][2345]->Draw();
  c0->SaveAs("trimbits-roc1-2345-ntrig5.pdf");

  fTrimSummaries[0]->vana[0]->Draw();
  c0->SaveAs("vana-roc0-ntrig5.pdf");

  fTrimSummaries[0]->vthrcomp[0]->Draw();
  c0->SaveAs("vthrcomp-roc0-ntrig5.pdf");

  addTrimTests("/scratch/ursl/pxar/150828-repro", 1); 
  addTrimTests("/scratch/ursl/pxar/150828-repro", 2); 
  addTrimTests("/scratch/ursl/pxar/150828-repro", 3); 
  addTrimTests("/scratch/ursl/pxar/150828-repro", 4); 

  fTrimSummaries[4]->vthrcomp[0]->Draw();
  c0->SaveAs("vthrcomp-roc0-ntrig40.pdf");

  addTrimTests("/scratch/ursl/pxar/150828-repro", 5); 

  c0->Clear();

  gStyle->SetOptStat(0); 

  c0->Clear();
  fTrimSummaries[0]->trimVcalThr->Draw();
  fTrimSummaries[1]->trimVcalThr->Draw("same");
  fTrimSummaries[2]->trimVcalThr->Draw("same");
  fTrimSummaries[3]->trimVcalThr->Draw("same");
  fTrimSummaries[4]->trimVcalThr->Draw("same");
  fTrimSummaries[5]->trimVcalThr->Draw("same");

  TLegend *legg = new TLegend(0.6, 0.4, 0.85, 0.85, "Ntrig ");
  legg->SetFillStyle(0); 
  legg->SetBorderSize(0); 
  legg->SetTextSize(0.04);  
  legg->SetFillColor(0); 
  legg->SetTextFont(42); 

  legg->AddEntry(fTrimSummaries[0]->trimVcalThr, Form("5 (%d)", fTrimSummaries[0]->stat),  "l");
  legg->AddEntry(fTrimSummaries[1]->trimVcalThr, Form("10 (%d)", fTrimSummaries[1]->stat), "l");
  legg->AddEntry(fTrimSummaries[2]->trimVcalThr, Form("15 (%d)", fTrimSummaries[2]->stat), "l");
  legg->AddEntry(fTrimSummaries[3]->trimVcalThr, Form("20 (%d)", fTrimSummaries[3]->stat), "l");
  legg->AddEntry(fTrimSummaries[4]->trimVcalThr, Form("40 (%d)", fTrimSummaries[4]->stat), "l");
  legg->AddEntry(fTrimSummaries[5]->trimVcalThr, Form("50 (%d)", fTrimSummaries[5]->stat), "l");
  legg->Draw();
  c0->SaveAs("trimvcalthr.pdf");

  c0->Clear();
  fTrimSummaries[0]->trimBits->Draw();
  fTrimSummaries[1]->trimBits->Draw("same");
  fTrimSummaries[2]->trimBits->Draw("same");
  fTrimSummaries[3]->trimBits->Draw("same");
  fTrimSummaries[4]->trimBits->Draw("same");
  fTrimSummaries[5]->trimBits->Draw("same");
  c0->SaveAs("trimbits.pdf");

  // -- vs ntrig
  TH1D *h1 = new TH1D("h1", "", 6, 0., 6.); h1->Sumw2();
  setHist(h1, "ntrig", "mean threshold RMS", kBlack, 0.01, -1.); 
  TH1D *h2 = new TH1D("h2", "", 6, 0., 6.); h2->Sumw2();
  setHist(h2, "ntrig", "mean trimbit RMS", kBlack, 0.01, -1.); 
  TH1D *h3 = new TH1D("h3", "", 6, 0., 6.); h3->Sumw2();
  setHist(h3, "ntrig", "mean VthrComp RMS", kBlack, 0.01, -1.); 
  TH1D *h4 = new TH1D("h4", "", 6, 0., 6.); h4->Sumw2();
  setHist(h4, "ntrig", "mean Vtrim RMS", kBlack, 0.01, -1.); 

  // -- simply a histogram of the means:
  TH1D *h10 = new TH1D("h10", "", 50, 0., 5.); h10->Sumw2();
  setHist(h10, "VANA RMS", "ROCs", kBlack, 0.01, -1.); 
  TH1D *h11 = new TH1D("h11", "", 50, 0., 5.); h11->Sumw2();
  setHist(h11, "CALDEL RMS", "ROCs", kBlack, 0.01, -1.); 

  for (unsigned int i = 0; i < fvNtrig.size(); ++i) {
    h1->GetXaxis()->SetBinLabel(i+1, Form("%d", fvNtrig[i]));
    h2->GetXaxis()->SetBinLabel(i+1, Form("%d", fvNtrig[i]));
    h3->GetXaxis()->SetBinLabel(i+1, Form("%d", fvNtrig[i]));
    h4->GetXaxis()->SetBinLabel(i+1, Form("%d", fvNtrig[i]));

    h1->SetBinContent(i+1, fTrimSummaries[i]->trimVcalThr->GetMean());
    h1->SetBinError(i+1, fTrimSummaries[i]->trimVcalThr->GetMeanError());

    h2->SetBinContent(i+1, fTrimSummaries[i]->trimBits->GetMean());
    h2->SetBinError(i+1, fTrimSummaries[i]->trimBits->GetMeanError());

    h3->SetBinContent(i+1, fTrimSummaries[i]->rvthrcomp->GetMean());
    h3->SetBinError(i+1, fTrimSummaries[i]->rvthrcomp->GetMeanError());

    h4->SetBinContent(i+1, fTrimSummaries[i]->rvtrim->GetMean());
    h4->SetBinError(i+1, fTrimSummaries[i]->rvtrim->GetMeanError());

    for (int iroc = 0; iroc < fNrocs; ++iroc) {
      h10->Fill(fTrimSummaries[i]->vana[iroc]->GetRMS());
      h11->Fill(fTrimSummaries[i]->caldel[iroc]->GetRMS());
    }
  }


  c0->Clear();
  for (unsigned int i = 0; i < fvNtrig.size(); ++i) {
    fTrimSummaries[i]->trimVcalThr->Draw();
    c0->SaveAs(Form("trimvcalthr-ntrig%d.pdf", fvNtrig[i])); 
  }
  

  c0->Clear();
  h1->Draw("histl");
  c0->SaveAs("trimvcalthr-ntrig.pdf");

  c0->Clear();
  h2->Draw("histl");
  c0->SaveAs("trimbits-ntrig.pdf");

  gStyle->SetOptStat(1); 

  c0->Clear();
  h10->Draw();
  c0->SaveAs("vana.pdf");

  c0->Clear();
  h11->Draw();
  c0->SaveAs("caldel.pdf");


  gStyle->SetOptStat(0); 

  c0->Clear();
  h3->Draw();
  c0->SaveAs("vtrim-ntrig.pdf");

  c0->Clear();
  h4->Draw();
  c0->SaveAs("vthrcomp-ntrig.pdf");

  gStyle->SetOptStat(1); 

  c0->Clear();
  fTrimSummaries[0]->vana[0]->Draw();
  c0->SaveAs("vana-ntrig5.pdf");

 

}



// ----------------------------------------------------------------------
void anaFullTest::addTrimTests(std::string dir, int ioffset) {
  
  int ntrig = fvNtrig[ioffset];

  vector<string> dirs = glob(Form("%s/ntrig%d", dir.c_str(), ntrig), "d2097"); 
  cout << dirs.size() << endl;
  for (unsigned int idirs = 0; idirs < dirs.size(); ++idirs) {
    cout << dirs[idirs] << endl;
  }

  bookTrimSummary(ioffset); 

  // -- reset histograms for trim info
  for (int iroc = 0; iroc < fNrocs; ++iroc) {
    for (int i = 0; i < 4160; ++i) {
      fvTrimVcalThr[iroc][i]->Reset();
      fvTrimBits[iroc][i]->Reset();
    }
  }


  fTrimSummaries[ioffset]->stat = dirs.size(); 

  for (unsigned int idirs = 0; idirs < dirs.size(); ++idirs) {
    if (1) {
      readDacFile(Form("%s/ntrig%d/%s", dir.c_str(), ntrig, dirs[idirs].c_str()), "vana", fTrimSummaries[ioffset]->vana);
      readDacFile(Form("%s/ntrig%d/%s", dir.c_str(), ntrig, dirs[idirs].c_str()), "caldel", fTrimSummaries[ioffset]->caldel);
      readDacFile(Form("%s/ntrig%d/%s", dir.c_str(), ntrig, dirs[idirs].c_str()), "vthrcomp", fTrimSummaries[ioffset]->vthrcomp);
      readDacFile(Form("%s/ntrig%d/%s", dir.c_str(), ntrig, dirs[idirs].c_str()), "vtrim", fTrimSummaries[ioffset]->vtrim);
    }

    fillTrimInfo(Form("%s/ntrig%d/%s", dir.c_str(), ntrig, dirs[idirs].c_str()));
  }

  

  for (int iroc = 0; iroc < fNrocs; ++iroc) {
    fTrimSummaries[ioffset]->rvana->Fill(fTrimSummaries[ioffset]->vana[iroc]->GetRMS());
    fTrimSummaries[ioffset]->rcaldel->Fill(fTrimSummaries[ioffset]->caldel[iroc]->GetRMS());
    fTrimSummaries[ioffset]->rvthrcomp->Fill(fTrimSummaries[ioffset]->vthrcomp[iroc]->GetRMS());
    fTrimSummaries[ioffset]->rvtrim->Fill(fTrimSummaries[ioffset]->vtrim[iroc]->GetRMS());
    

    for (int i = 0; i < 4160; ++i) {
      fTrimSummaries[ioffset]->trimVcalThr->Fill(fvTrimVcalThr[iroc][i]->GetRMS());
      fTrimSummaries[ioffset]->trimBits->Fill(fvTrimBits[iroc][i]->GetRMS());
    }
  }

  
}
  

// ----------------------------------------------------------------------
void anaFullTest::fillTrimInfo(std::string dir) {

  cout << Form("open %s/pxar.root", dir.c_str()) << endl;
  TFile *f = TFile::Open(Form("%s/pxar.root", dir.c_str())); 
  TH2D *h1(0), *h2(0); 
  h1 = (TH2D*)f->Get("Trim/thr_TrimThrFinal_vcal_C0_V0");
  if (!h1) {
    f->Close();
    return;
  }

  for (int iroc = 0; iroc < fNrocs; ++iroc) {
    h1 = (TH2D*)f->Get(Form("Trim/thr_TrimThrFinal_vcal_C%d_V0", iroc));
    h2 = (TH2D*)f->Get(Form("Trim/TrimMap_C%d_V0", iroc));
    if (!h1) {
      cout << "histogram " << Form("Trim/thr_TrimThrFinal_vcal_C%d_V0", iroc) << " not found" << endl;
      continue;
    }

    if (!h2) {
      cout << "histogram " << Form("Trim/TrimMap_C%d_V0", iroc) << " not found" << endl;
      continue;
    }

    //     vector<TH1D*> vh1 =	fvTrimVcalThr[iroc];
    //     vector<TH1D*> vh2 =	fvTrimBits[iroc];
    for (int ix = 0; ix < 52; ++ix) {
      for (int iy = 0; iy < 80; ++iy) {
	fvTrimVcalThr[iroc][ix*80+iy]->Fill(h1->GetBinContent(ix+1, iy+1)); 
	fvTrimBits[iroc][ix*80+iy]->Fill(h2->GetBinContent(ix+1, iy+1)); 
	//	if (fvTrimVcalThr[iroc][ix*80+iy]->GetRMS() > 4) cout << "ROC " << iroc << " dir: " << dir << endl;
      }
    }
  }


  f->Close();
}


// ----------------------------------------------------------------------
void anaFullTest::bookTrimSummary(int ioffset) {

  int ntrig = fvNtrig[ioffset]; 

  vector<TH1D*> v; 
  trimSummary *rs = new trimSummary(); 
  rs->modName = Form("ntrig = %d", ntrig); 
  for (int iroc = 0; iroc <= 16; ++iroc) {
    rs->vana.push_back(new TH1D(Form("ntrig%d_roc%d_vana", ntrig, iroc), Form("ntrig%d_roc%d_vana", ntrig, iroc), 256, 0., 256.)); 
    rs->caldel.push_back(new TH1D(Form("ntrig%d_roc%d_caldel", ntrig, iroc), Form("ntrig%d_roc%d_caldel", ntrig, iroc), 256, 0., 256.)); 
    rs->vthrcomp.push_back(new TH1D(Form("ntrig%d_roc%d_vtrim", ntrig, iroc), Form("ntrig%d_roc%d_vtrim", ntrig, iroc), 256, 0., 256.)); 
    rs->vtrim.push_back(new TH1D(Form("ntrig%d_roc%d_vthrcomp", ntrig, iroc), Form("ntrig%d_roc%d_vthrcomp", ntrig, iroc), 256, 0., 256.));     
  }

  rs->trimBits    = new TH1D(Form("ntrig%d_trimbits", ntrig), "", 8, 0., 1.); 
  setHist(rs->trimBits, "RMS of trim bit reproducibility", "", fvColor[ioffset], 0.01, 40000.); 
  rs->trimVcalThr = new TH1D(Form("ntrig%d_trimvcalthr", ntrig), "", 100, 0., 2.); 
  setHist(rs->trimVcalThr, "RMS of threshold reproducibility", "", fvColor[ioffset], 0.01, 4000.); 

  rs->rvana = new TH1D(Form("ntrig%d_vana", ntrig), "", 100, 0., 5.); 
  setHist(rs->rvana, "RMS of VANA reproducibility", "", fvColor[ioffset], 0.01, 400.); 

  rs->rcaldel = new TH1D(Form("ntrig%d_caldel", ntrig), "", 100, 0., 5.); 
  setHist(rs->rcaldel, "RMS of CALDEL reproducibility", "", fvColor[ioffset], 0.01, 400.); 

  rs->rvthrcomp = new TH1D(Form("ntrig%d_vthrcomp", ntrig), "", 100, 0., 5.); 
  setHist(rs->rvthrcomp, "RMS of VTHRCOMP reproducibility", "", fvColor[ioffset], 0.01, 400.); 

  rs->rvtrim = new TH1D(Form("ntrig%d_vtrim", ntrig), "", 100, 0., 5.); 
  setHist(rs->rvtrim, "RMS of VTRIM reproducibility", "", fvColor[ioffset], 0.01, 400.); 

  fTrimSummaries.push_back(rs); 
 
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

  ms->nlpos = new TH1D(Form("%s_nlpos", modulename.c_str()), 
		    Form("%s_nlpos", modulename.c_str()), 
		    50, 0.5, 1.0); 

  ms->nlrms = new TH1D(Form("%s_nlrms", modulename.c_str()), 
		    Form("%s_nlrms", modulename.c_str()), 
		    50, 0., 0.1); 
  
  fModSummaries.insert(make_pair(modulename, ms)); 

}


// ----------------------------------------------------------------------
void anaFullTest::validateFullTests(string dir, string mname, string mpattern) {
  addFullTests(dir, mname, mpattern);

  TH1D *hVana = new TH1D("hVana", Form("Vana %s", fDiffMetric == 0?"difference":"RMS"), 5, 0., 5.);
  TH1D *hCaldel = new TH1D("hCaldel", Form("CalDel %s", fDiffMetric == 0?"difference":"RMS"), 5, 0., 5.);
  TH1D *hVthrcomp = new TH1D("hVthrcomp", Form("VthrComp %s", fDiffMetric == 0?"difference":"RMS"), 5, 0., 5.);
  TH1D *hVtrim = new TH1D("hVtrim", Form("Vtrim %s", fDiffMetric == 0?"difference":"RMS"), 25, 0., 25.);
  TH1D *hPhs = new TH1D("hPhs", Form("phscale %s", fDiffMetric == 0?"difference":"RMS"), 5, 0., 5.);
  TH1D *hPho = new TH1D("hPho", Form("phoffset %s", fDiffMetric == 0?"difference":"RMS"), 5, 0., 5.);

  map<string, moduleSummary*>::iterator ib = fModSummaries.begin();

  TH1D *hTrimThrPos = (TH1D*)ib->second->trimthrpos->Clone("hTrimThrPos");
  hTrimThrPos->SetTitle("Trim Threshold");
  hTrimThrPos->Reset();

  TH1D *hTrimThrRms = (TH1D*)ib->second->trimthrrms->Clone("hTrimThrRms");
  hTrimThrRms->SetTitle("Trim Threshold RMS");
  hTrimThrRms->Reset();

  TH1D *hnlpos = (TH1D*)ib->second->nlpos->Clone("hnlpos");
  hnlpos->SetTitle("nl");
  hnlpos->Reset();

  TH1D *hnlrms = (TH1D*)ib->second->nlrms->Clone("hnlrms");
  hnlrms->SetTitle("nl RMS");
  hnlrms->Reset();

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
      // -- the following results in the "correct" Entries and Integral
      if (it->second->trimthrpos->GetBinContent(ibin) > 0)
	for (int i = 0; i < it->second->trimthrpos->GetBinContent(ibin); ++i) 
	  hTrimThrPos->Fill(it->second->trimthrpos->GetBinCenter(ibin)); 
    }

    for (int ibin = 1; ibin <= it->second->trimthrrms->GetNbinsX(); ++ibin) {
      if (it->second->trimthrrms->GetBinContent(ibin) > 0)
	for (int i = 0; i < it->second->trimthrrms->GetBinContent(ibin); ++i) 
	  hTrimThrRms->Fill(it->second->trimthrrms->GetBinCenter(ibin)); 
    }

    for (int ibin = 1; ibin <= it->second->nlpos->GetNbinsX(); ++ibin) {
      if (it->second->nlpos->GetBinContent(ibin) > 0)
	for (int i = 0; i < it->second->nlpos->GetBinContent(ibin); ++i) 
	  hnlpos->Fill(it->second->nlpos->GetBinCenter(ibin)); 
    }

    for (int ibin = 1; ibin <= it->second->nlrms->GetNbinsX(); ++ibin) {
      if (it->second->nlrms->GetBinContent(ibin) > 0)
	for (int i = 0; i < it->second->nlrms->GetBinContent(ibin); ++i) 
	  hnlrms->Fill(it->second->nlrms->GetBinCenter(ibin)); 
    }
    
  }
			  
			  
  c0->Clear();
  c0->Divide(3,3);

  c0->cd(1);
  hVana->Draw();

  c0->cd(2);
  hCaldel->Draw();

  c0->cd(3);
  hVthrcomp->Draw();

  c0->cd(4);
  hVtrim->Draw();

  c0->cd(5);
  hPhs->Draw();

  c0->cd(6);
  hPho->Draw();

  c0->cd(7);
  hTrimThrPos->Draw();

  c0->cd(8);
  hTrimThrRms->Draw();

  c0->cd(9);
  hnlrms->Draw();

  // hnlpos->Draw();

  c0->SaveAs(Form("ftval-%s.pdf", mname.c_str())); 


}


// ----------------------------------------------------------------------
void anaFullTest::addFullTests(string dir, string mname, string mpattern) {
  vector<string> dirs = glob(dir, mname+mpattern); 
  cout << "# directories found: " << dirs.size() << endl;
  for (unsigned int idirs = 0; idirs < dirs.size(); ++idirs) {
    cout << "  " << dirs[idirs] << endl;
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
    readLogFile(dirs[idirs], "non-linearity mean:", fModSummaries[mname]->nlpos);
    readLogFile(dirs[idirs], "non-linearity RMS:", fModSummaries[mname]->nlrms);
      
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
  fModSummaries[mname]->nlpos->Draw();
  
  c0->SaveAs(Form("%s.pdf", mname.c_str())); 

}


// ----------------------------------------------------------------------
void anaFullTest::readLogFile(std::string dir, std::string tag, std::vector<double> &v) {
  ifstream INS; 
  
  string sline; 
  string::size_type s1;
  vector<double> x;
  INS.open(Form("%s/pxar.log", dir.c_str())); 
  while (getline(INS, sline)) {
    s1 = sline.find(tag.c_str()); 
    if (string::npos == s1) continue;
    sline = sline.substr(s1+tag.length()+1);
    break;
  }
  
  x = splitIntoRocs(sline);
  v.clear();
  copy(x.begin(), x.end(), back_inserter(v));
  INS.close(); 
}


// ----------------------------------------------------------------------
void anaFullTest::readLogFile(std::string dir, std::string tag, std::vector<TH1D*> hists) {

  ifstream INS; 

  string sline; 
  string::size_type s1;
  vector<double> x;
  INS.open(Form("%s/pxar.log", dir.c_str())); 
  while (getline(INS, sline)) {
    s1 = sline.find(tag.c_str()); 
    if (string::npos == s1) continue;
    sline = sline.substr(s1+tag.length()+1);
    break;
  }

  x = splitIntoRocs(sline);
  for (unsigned int i = 0; i < x.size(); ++i) {
    //    cout << "Filling into " << hists[i]->GetName() << " x = " << x[i] << endl;
    hists[i]->Fill(x[i]); 
  }

  INS.close(); 
}

// ----------------------------------------------------------------------
void anaFullTest::readLogFile(std::string dir, std::string tag, TH1D* hist) {

  ifstream INS; 

  string sline; 
  string::size_type s1;
  vector<double> x;
  INS.open(Form("%s/pxar.log", dir.c_str())); 
  while (getline(INS, sline)) {
    s1 = sline.find(tag.c_str()); 
    if (string::npos == s1) continue;
    sline = sline.substr(s1+tag.length()+1);
    break;
  }
  cout << sline << endl;
  x = splitIntoRocs(sline);
  for (unsigned int i = 0; i < x.size(); ++i) {
    //     cout << "Filling into " << hist->GetName() << " x = " << x[i] << endl;
    hist->Fill(x[i]); 
  }

  INS.close(); 
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
  } else if (string(rochist->GetName()) == string("distVcalThr")) {
    hs = fhVcalthr;
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
void anaFullTest::findNoiseLevel(std::string dirname, std::string /*hbasename*/, TH1D* /*rochist*/) {
  TFile *f = TFile::Open((dirname+"/pxar.root").c_str()); 
  
  TH2D *h(0); 
  int cnt(0); 
  
  vector<pair<int, int> > pixelList; 
  pixelList.push_back(make_pair(12,22)); 
  pixelList.push_back(make_pair(5,5)); 
  pixelList.push_back(make_pair(15,26)); 
  pixelList.push_back(make_pair(20,32)); 
  pixelList.push_back(make_pair(25,36)); 
  pixelList.push_back(make_pair(30,42)); 
  pixelList.push_back(make_pair(35,50)); 
  pixelList.push_back(make_pair(40,60)); 
  pixelList.push_back(make_pair(45,70)); 
  pixelList.push_back(make_pair(50,75)); 

  //try  ;
  int ipixel(-1); 
  for (unsigned int i = 0; i < pixelList.size(); ++i) {
    h = (TH2D*)f->Get(Form("pretestVthrCompCalDel_c%d_r%d_C0", pixelList[i].first, pixelList[i].second)); 
    if (h) {
      ipixel = i; 
      cout << "found " << h->GetName() << endl;
      break;
    }
  }  

  if (ipixel < 0) {
    cout << "did not find histograms for working pixel" << endl;
    return;
  }
  
  for (int i = 0; i < fNrocs; ++i) {
    h = (TH2D*)f->Get(Form("pretestVthrCompCalDel_c%d_r%d_C%d", pixelList[ipixel].first, pixelList[ipixel].second, i)); 
    cnt = 0; 
    if (h) {
      int x = static_cast<int>(h->GetMean()); 
      TH1D *hy = h->ProjectionY("_py", x, x); 
      cout << "last bin above 50%: " << hy->FindLastBinAbove(0.5*h->GetMaximum()) << endl;
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
  ifstream INS; 

  string sline; 
  string::size_type s1;
  int val(0); 
  cout << Form("%s/dacParameters%d_CX.dat, X = ", dir.c_str(), fTrimVcal);
  for (int i = 0; i < fNrocs; ++i) {
    INS.open(Form("%s/dacParameters%d_C%d.dat", dir.c_str(), fTrimVcal, i)); 
    cout << i << " "; 
    while (getline(INS, sline)) {
      if (sline[0] == '#') {continue;}
      if (sline[0] == '/') {continue;}
      if (sline[0] == '\n') {continue;}
      s1 = sline.find(dac.c_str()); 
      //      cout << sline << endl;
      if (string::npos != s1) {
	sline = sline.substr(s1+dac.length()+1); 
	val = atoi(sline.c_str()); 
	vals[i]->Fill(val); 
	vals[fNrocs]->Fill(val); 
      }
    }
    INS.close(); 
  }
  cout << endl;
}



// ----------------------------------------------------------------------
vector<string> anaFullTest::glob(string basedir, string basename) {
  cout << "Looking in " << basedir << " for " << basename << endl;
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
      lof.push_back(string(basedir+"/"+fname));
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
      //      cout << h->GetName() << " bin " << i+1 << " what? " << vh[i]->GetMean() << endl;
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
void anaFullTest::dumpVector(vector<double> vh, TH1D *h, string opt) {
  for (unsigned int i = 0; i < vh.size(); ++i) {
    if (opt == "0") {
      //      cout << h->GetName() << " bin " << i+1 << " what? " << vh[i] << endl;
      h->SetBinContent(i+1, vh[i]); 
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
void anaFullTest::summarizeVector(vector<double> vh, TH1D *h) {
  for (unsigned int i = 0; i < vh.size(); ++i) {
    h->Fill(vh[i]); 
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
  ifstream INS; 

  string sline; 
  string::size_type s1;
  INS.open(Form("%s/pxar.log", dir.c_str())); 
  while (getline(INS, sline)) {
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
  INS.close();
  return sline; 

}


// ----------------------------------------------------------------------
int anaFullTest::countWord(string dir, string pattern) {
  ifstream INS; 

  string sline; 
  string::size_type s1;
  INS.open(Form("%s/pxar.log", dir.c_str())); 
  int cnt(0); 
  while (getline(INS, sline)) {
    s1 = sline.find(pattern.c_str()); 
    if (string::npos == s1) continue;
    ++cnt;
  }
  INS.close();
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
  
  h0 = atoi(parse.substr(1, st0-1).c_str());
  m0 = atoi(parse.substr(st0+1, st1-st0-1).c_str());
  s0 = atoi(parse.substr(st1+1).c_str());

  parse = ss1; 
  st0 = parse.find(":");
  st1 = parse.find(":", st0+1);

  h1 = atoi(parse.substr(1, st0-1).c_str());
  m1 = atoi(parse.substr(st0+1, st1-st0-1).c_str());
  s1 = atoi(parse.substr(st1+1).c_str());
  
  struct timeval tv0 = {h0*60*60 + m0*60 + s0, 0};
  struct timeval tv1 = {h1*60*60 + m1*60 + s1, 0};
  
  return tv1.tv_sec - tv0.tv_sec;
}



// ----------------------------------------------------------------------
void anaFullTest::showOverFlow(TH1D* h) {
  h->SetBinContent(h->GetNbinsX(), h->GetBinContent(h->GetNbinsX()) + h->GetBinContent(h->GetNbinsX()+1));

}


// ----------------------------------------------------------------------
void anaFullTest::trimTestRanges(string dir, string pattern) {

  TH1D *h1 = new TH1D("h1", "thr_TrimThr0_vthrcomp", 256, 0., 256.); 
  TH1D *h2 = new TH1D("h2", "thr_TrimThr1_vcal", 256, 0., 256.); 
  TH1D *h3 = new TH1D("h3", "thr_TrimThr2_vcal", 256, 0., 256.); 

  TH2D* h(0); 
  vector<string> dirs = glob(dir, pattern); 
  for (unsigned int idirs = 0; idirs < dirs.size(); ++idirs) {
    cout << dirs[idirs] << endl;
    TFile *f = TFile::Open(Form("%s/%s/pxar.root", dir.c_str(), dirs[idirs].c_str())); 
    for (int i = 0; i < 16; ++i) {
      h = (TH2D*)f->Get(Form("Trim/thr_TrimThr0_vthrcomp_C%d_V0", i)); 
      dump2dTo1d(h, h1); 
      h = (TH2D*)f->Get(Form("Trim/thr_TrimThr1_vcal_C%d_V0", i)); 
      dump2dTo1d(h, h2); 
      h = (TH2D*)f->Get(Form("Trim/thr_TrimThr2_vcal_C%d_V0", i)); 
      dump2dTo1d(h, h3); 
    }
  }

  c0->Clear();
  c0->Divide(2,2);

  c0->cd(1);
  gPad->SetLogy(1);
  h1->Draw();

  c0->cd(2);
  gPad->SetLogy(1);
  h2->Draw();

  c0->cd(3);
  gPad->SetLogy(1);
  h3->Draw();
}


// ----------------------------------------------------------------------
void anaFullTest::dump2dTo1d(TH2D *h2, TH1D *h1) {
  for (int ix = 0; ix < h2->GetNbinsX(); ++ix) {
    for (int iy = 0; iy < h2->GetNbinsY(); ++iy) {
      h1->Fill(h2->GetBinContent(ix+1, iy+1)); 
    }
  }
}


// ----------------------------------------------------------------------
void anaFullTest::showAllTimings(string dir, string pattern, bool reset) {

  if (reset) {
    fTS->tPretest->Reset(); 
    fTS->tAlive->Reset(); 
    fTS->tBB->Reset(); 
    fTS->tScurve->Reset(); 
    fTS->tTrim->Reset(); 
    fTS->tTrimBit->Reset(); 
    fTS->tPhOpt->Reset(); 
    fTS->tGain->Reset(); 
    fTS->tReadback->Reset(); 
    fTS->tFullTest->Reset(); 
  }    
  
  vector<string> dirs = glob(dir, pattern); 
  for (unsigned int idirs = 0; idirs < dirs.size(); ++idirs) {
    cout << dirs[idirs] << endl;
    fullTestTiming(dirs[idirs], dir); 
  }

  gStyle->SetOptStat(0); 
  c0->Clear();
  c0->Divide(3,3);
  c0->cd(1); 
  fTS->tPretest->Draw();
  tl->DrawLatex(0.2, 0.92, Form("pretest: %2.1f min", fTS->tPretest->GetMean()/60.));
  tl->DrawLatex(0.7, 0.82, Form("(%d)", static_cast<int>(fTS->tPretest->GetEntries())));

  c0->cd(2); 
  fTS->tAlive->Draw();
  tl->DrawLatex(0.2, 0.92, Form("alive: %2.1f min", fTS->tAlive->GetMean()/60.));
  tl->DrawLatex(0.7, 0.82, Form("(%d)", static_cast<int>(fTS->tAlive->GetEntries())));

  c0->cd(3); 
  fTS->tBB->Draw();
  tl->DrawLatex(0.2, 0.92, Form("BB: %2.1f min", fTS->tBB->GetMean()/60.));
  tl->DrawLatex(0.7, 0.82, Form("(%d)", static_cast<int>(fTS->tBB->GetEntries())));

  c0->cd(4); 
  fTS->tScurve->Draw();
  tl->DrawLatex(0.2, 0.92, Form("scurve: %2.1f min", fTS->tScurve->GetMean()/60.));
  tl->DrawLatex(0.7, 0.82, Form("(%d)", static_cast<int>(fTS->tScurve->GetEntries())));

  c0->cd(5); 
  fTS->tTrim->Draw();
  fTS->tTrimBit->Draw("same");
  tl->DrawLatex(0.52, 0.92, Form("trim: %2.1f min", fTS->tTrim->GetMean()/60.));
  tl->DrawLatex(0.7, 0.82, Form("(%d)", static_cast<int>(fTS->tTrim->GetEntries())));
  tl->SetTextColor(kRed);
  tl->DrawLatex(0.05, 0.92, Form("trimbits: %2.1f min", fTS->tTrimBit->GetMean()/60.));
  tl->DrawLatex(0.7, 0.76, Form("(%d)", static_cast<int>(fTS->tTrimBit->GetEntries())));
  tl->SetTextColor(kBlack);

  c0->cd(6); 
  fTS->tPhOpt->Draw();
  tl->DrawLatex(0.2, 0.92, Form("phoptimization: %2.1f min", fTS->tPhOpt->GetMean()/60.));
  tl->DrawLatex(0.7, 0.82, Form("(%d)", static_cast<int>(fTS->tPhOpt->GetEntries())));

  c0->cd(7); 
  fTS->tGain->Draw();
  tl->DrawLatex(0.2, 0.92, Form("gain: %2.1f min", fTS->tGain->GetMean()/60.));
  tl->DrawLatex(0.7, 0.82, Form("(%d)", static_cast<int>(fTS->tGain->GetEntries())));

  c0->cd(8); 
  fTS->tReadback->Draw();
  tl->DrawLatex(0.2, 0.92, Form("readback: %2.1f min", fTS->tReadback->GetMean()/60.));
  tl->DrawLatex(0.7, 0.82, Form("(%d)", static_cast<int>(fTS->tReadback->GetEntries())));

  c0->cd(9); 
  fTS->tFullTest->Draw();
  tl->DrawLatex(0.2, 0.92, Form("fulltest: %2.1f min", fTS->tFullTest->GetMean()/60.));
  tl->DrawLatex(0.7, 0.82, Form("(%d)", static_cast<int>(fTS->tFullTest->GetEntries())));

  cout << "ShowAllTiming Summary" << endl;
  cout << "Pretest:    " << fTS->tPretest->GetMean() << endl;
  cout << "Alive:      " << fTS->tAlive->GetMean() << endl;
  cout << "BB:         " << fTS->tBB->GetMean() << endl;
  cout << "Scurve:     " << fTS->tScurve->GetMean() << endl;
  cout << "Trim:       " << fTS->tTrim->GetMean() << endl;
  cout << "TrimBit:    " << fTS->tTrimBit->GetMean() << endl;
  cout << "PhOpt:      " << fTS->tPhOpt->GetMean() << endl;
  cout << "Gain:       " << fTS->tGain->GetMean() << endl;
  cout << "Readback:   " << fTS->tReadback->GetMean() << endl;
  cout << "FullTest:   " << fTS->tFullTest->GetMean() << endl;

  
  c0->SaveAs("fullTestTiming.pdf"); 


}

// ----------------------------------------------------------------------
void anaFullTest::fullTestTiming(string dir, string basedir) {

  ifstream INS; 

  string sline; 
  string::size_type s1;

  vector<string> patterns; 
  patterns.push_back("INFO:   running: pretest");
  patterns.push_back("INFO:    PixTestAlive::aliveTest");
  patterns.push_back("INFO: PixTestBBMap::doTest() Ntrig");
  patterns.push_back("INFO: PixTestScurves::fullTest() ntrig");
  patterns.push_back("INFO:    PixTestTrim::trimTest() ntrig");
  patterns.push_back("INFO:    PixTestTrim::trimBitTest() ntrig");
  patterns.push_back("INFO: PixTestPhOptimization::doTest() Ntrig");
  patterns.push_back("INFO: PixTestGainPedestal::fullTest() ntrig");
  patterns.push_back("INFO: readReadbackCal:");
  patterns.push_back("INFO: pXar: this is the end, my friend");

  vector<string> startPoints; 


  INS.open(Form("%s/%s/pxar.log", basedir.c_str(), dir.c_str())); 
  while (getline(INS, sline)) {
    for (unsigned int i = 0; i < patterns.size(); ++i) {
      s1 = sline.find(patterns[i].c_str()); 
      if (string::npos != s1) {
	startPoints.push_back(sline); 
	break;
      }
    }
  }

  INS.close(); 

  int testD(0); 
  for (unsigned int i = 0; i < startPoints.size(); ++i) {
    if (i < startPoints.size() - 1) {
      testD = testDuration(startPoints[i], startPoints[i+1]);
      if (0 == i) fTS->tPretest->Fill(testD); 
      if (1 == i) fTS->tAlive->Fill(testD); 
      if (2 == i) fTS->tBB->Fill(testD); 
      if (3 == i) fTS->tScurve->Fill(testD); 
      if (4 == i) fTS->tTrim->Fill(testD); 
      if (5 == i) fTS->tTrimBit->Fill(testD); 
      if (6 == i) fTS->tPhOpt->Fill(testD); 
      if (7 == i) fTS->tGain->Fill(testD); 
      if (8 == i) fTS->tReadback->Fill(testD); 
      cout << Form("%4d from %s", testD, startPoints[i].c_str()) << endl;
    }
  }

  testD = testDuration(startPoints[0], startPoints[startPoints.size()-1]);
  fTS->tFullTest->Fill(testD); 
}
