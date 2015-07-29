#ifndef ANAFULLTEST_H
#define ANAFULLTEST_H

#include <iostream>
#include <map>

#include "TString.h"
#include "TObject.h"

#include "TH1.h"
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

};


// ----------------------------------------------------------------------
class DLLEXPORT anaFullTest {
  
 public: 
  anaFullTest(); 
  virtual ~anaFullTest(); 

  void showAllFullTests(std::string basename = "/scratch/ursl/pxar/modules");
  void showFullTest(std::string modname = "m2057", std::string basename = "/scratch/ursl/pxar/modules");

  void addFullTests(std::string mname = "D14-0006", std::string mpattern = "-000");
  void validateFullTests();
  void readDacFile(std::string dir, std::string dac, std::vector<TH1D*> hists);
  void readLogFile(std::string dir, std::string tag, std::vector<TH1D*> hists);
  void readLogFile(std::string dir, std::string tag, TH1D* hist);

  void fillRocHist(std::string dirname, std::string hbasename, TH1D* rochist, int mode);
  void anaRocMap(std::string dirname, std::string hbasename, TH1D* rochist, int mode);

  void bookModuleSummary(std::string modulename); 
  void bookSingleModuleSummary(std::string modulename); 

  std::vector<double> splitIntoRocs(std::string line); 
  std::vector<std::string> glob(std::string basedir, std::string basename);
  double diff(TH1D*); 

  void clearHistVector(std::vector<TH1D*>); 
  void dumpVector(std::vector<TH1D*>, TH1D *, std::string); 

  void projectRocHist(TH1D *h, double &mean, double &rms, int &total); 
  void plotVsRoc(TH1D *h, double tx = 0.20, double ty = 0.85, std::string s = "", int mode = 0); 

  std::string readLine(std::string dir, std::string pattern, int mode); 
  int countWord(std::string dir, std::string pattern); 

  void setHist(TH1D *h, std::string xtitle = "", std::string ytitle = "", int color = kBlack, double miny = 0., double maxy = 256.); 

private: 
  TLatex *tl; 
  TCanvas *c0;
  int fNrocs, fTrimVcal; 
  int fDiffMetric; // 0: maxbin - minbin (~ difference), 1: RMS, 2: ??

  std::vector<std::string>    fDacs; 
  std::map<std::string, moduleSummary*> fModSummaries;
  singleModuleSummary*        fSMS;

  ClassDef(anaFullTest, 1); // testing anaFullTest

};

#endif
