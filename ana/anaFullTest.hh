#ifndef ANAFULLTEST_H
#define ANAFULLTEST_H

#include <iostream>
#include <map>

#include "TString.h"
#include "TObject.h"

#include "TH1.h"
#include "TH2.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TLatex.h"

#include "pxardllexport.h"

// ----------------------------------------------------------------------
struct moduleSummary {
  std::string moduleName; 
  // one hist per ROC, filled n iterations times
  std::vector<TH1D*> vana, caldel, vthrcomp, vtrim, phscale, phoffset;
  std::vector<TH1D*> ndeadpixels, ndeadbumps, deadsep; 

  // summary of module
  TH1D *rmsVana, *rmsCaldel, *rmsVthrcomp, *rmsVtrim, *rmsPhscale, *rmsPhoffset; 

  // absolute values that should be the same everywhere
  TH1D *trimthrpos, *trimthrrms, *p1pos, *p1rms; 
};


// ----------------------------------------------------------------------
struct singleModuleSummary {
  std::string moduleName; 
  // histograms showing DACs vs iroc
  TH1D *vana, *caldel, *vthrcomp, *vtrim, *phscale, *phoffset; 
  // histograms showing something vs iroc
  TH1D *noise, *vcalThr, *vcalThrW, *vcalTrimThr, *vcalTrimThrW, *relGainW, *pedestalW, *nonl, *nonlW; 
  TH1D *dead, *bb, *mask, *addr; 

  TH2D *defectMap;
  TH1D *distNoise, *distVcalTrimThr;

};


// ----------------------------------------------------------------------
class DLLEXPORT anaFullTest {
  
 public: 
  anaFullTest(); 
  virtual ~anaFullTest(); 

  void showAllFullTests(std::string basename = "/scratch/ursl/pxar/modules", std::string pattern = "d");
  void showFullTest(std::string modname = "m2057", std::string basename = "/scratch/ursl/pxar/modules");

  void addFullTests(std::string mname = "D14-0006", std::string mpattern = "-000");
  void validateFullTests();
  void readDacFile(std::string dir, std::string dac, std::vector<TH1D*> hists);
  void readLogFile(std::string dir, std::string tag, std::vector<TH1D*> hists);
  void readLogFile(std::string dir, std::string tag, std::vector<double> &v);
  void readLogFile(std::string dir, std::string tag, TH1D* hist);

  void fillRocHist(std::string dirname, std::string hbasename, TH1D* rochist, int mode);
  void anaRocMap(std::string dirname, std::string hbasename, TH1D* rochist, int mode);
  void fillRocDefects(std::string dirname, TH2D* defectMap);

  void bookModuleSummary(std::string modulename); 
  void bookSingleModuleSummary(std::string modulename, int first); 

  std::vector<double> splitIntoRocs(std::string line); 
  std::vector<std::string> glob(std::string basedir, std::string basename);
  double diff(TH1D*); 

  void clearHistVector(std::vector<TH1D*>); 

  void dumpVector(std::vector<TH1D*>, TH1D *, std::string); 
  void dumpVector(std::vector<double>, TH1D *, std::string); 
  void summarizeVector(std::vector<TH1D*>, TH1D *); 
  void summarizeVector(std::vector<double>, TH1D *); 

  void projectRocHist(TH1D *h, double &mean, double &rms, int &total); 
  void plotVsRoc(TH1D *h, double tx = 0.20, double ty = 0.85, std::string s = "", int mode = 0); 

  std::string readLine(std::string dir, std::string pattern, int mode); 
  int countWord(std::string dir, std::string pattern); 
  int testDuration(std::string startTest, std::string endTest);

  void setHist(TH1D *h, std::string xtitle = "", std::string ytitle = "", int color = kBlack, double miny = 0., double maxy = 256.); 

private: 
  TLatex *tl; 
  TCanvas *c0;
  int fNrocs, fTrimVcal; 
  int fDiffMetric; // 0: maxbin - minbin (~ difference), 1: RMS, 2: ??

  std::vector<std::string>    fDacs; 
  std::map<std::string, moduleSummary*> fModSummaries;
  singleModuleSummary*        fSMS;

  TH1D *fhDuration, *fhCritical, *fhDead, *fhBb, *fhMask, *fhAddr;
  TH1D *fhNoise, *fhVcaltrimthr;
  TH1D *fhVana, *fhCaldel, *fhPhoffset, *fhPhscale, *fhVtrim, *fhVthrcomp; 


  ClassDef(anaFullTest, 1); // testing anaFullTest

};

#endif
