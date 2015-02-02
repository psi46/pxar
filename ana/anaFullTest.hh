#ifndef ANAFULLTEST_H
#define ANAFULLTEST_H

#include <iostream>
#include <map>

#include "TString.h"
#include "TObject.h"

#include "TH1.h"
#include "TF1.h"
#include "TCanvas.h"

#include "pxardllexport.h"


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



class DLLEXPORT anaFullTest {
  
 public: 
  anaFullTest(); 
  ~anaFullTest(); 

  void addFullTests(std::string mname = "D14-0006", std::string mpattern = "-000");
  void validateFullTests();
  void readDacFile(std::string dir, std::string dac, std::vector<TH1D*> hists);
  void readLogFile(std::string dir, std::string tag, std::vector<TH1D*> hists);
  void readLogFile(std::string dir, std::string tag, TH1D* hist);

  void bookModuleSummary(std::string modulename); 

  std::vector<double> splitIntoRocs(std::string line); 
  std::vector<std::string> glob(std::string basename);
  double diff(TH1D*); 


private: 
  TCanvas *c0;
  int fNrocs, fTrimVcal; 
  int fDiffMetric; // 0: maxbin - minbin (~ difference), 1: RMS, 2: ??

  std::vector<std::string>    fDacs; 
  std::map<std::string, moduleSummary*> fModSummaries;

  ClassDef(anaFullTest, 1); // testing anaFullTest

};

#endif
