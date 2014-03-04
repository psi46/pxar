#ifndef PIXTEST_H
#define PIXTEST_H

#ifdef __CINT__
#undef __GNUC__
typedef char __signed; 
typedef char int8_t; 
#endif

#include <string>
#include <map>
#include <list>

#include <TQObject.h> 
#include <TH1.h> 
#include <TH2.h> 
#include <TDirectory.h> 
#include <TFile.h>
#include <TSystem.h>

#include "api.h"

#include "PixInitFunc.hh"
#include "PixSetup.hh"
#include "PixTestParameters.hh"

///
/// PixTest
/// =======
/// 
/// Base class for all tests. 
/// 
/// 
/// Provides common utilities 
/// - mapping between roc index and roc ID
/// - keeps a list of the histograms and another list with their display options (fHistList, fHistOptions)
/// - booking of histgrams that are versioned between different test invocations (e.g. bookTH2D) 
/// - retrieve histograms instead readout decoding (e.g. efficiencyMap)
/// - list of selected pixels (fPIX)
///
class PixTest: public TQObject {
public:
  /// constructor requires PixSet to get test parameters and config parameters
  PixTest(PixSetup *a, std::string name);
  PixTest();
  virtual ~PixTest();
  /// sets all test parameters
  void init();
  /// use if you want, or define the histograms in the specific member functions
  void bookHist(std::string name);
  /// to create the histograms even without working pxar/core
  virtual void dummyAnalysis(); 
  /// to be filled per test
  virtual void doAnalysis();
  /// function connected to "DoTest" button of PixTab
  virtual void doTest(); 
  /// allow execution of any button in the test 
  virtual void runCommand(std::string command); 
  /// create output suitable for moreweb
  virtual void output4moreweb();

  /// implement this to provide updated tool tips if the user changes test parameters
  virtual void setToolTips();
  /// hint to what happens if the user hits the "summary" button
  virtual std::string getSummaryTip() {return fSummaryTip;}
  /// get the string describing the test (called from PixTab::updateToolTips)
  virtual std::string getTestTip() {return fTestTip;}
  /// get the hist display options (if stored in fHistOptions) 
  virtual std::string getHistOption(TH1*);

  /// work-around to cope with suboptimal pxar/core
  int pixelThreshold(std::string dac, int ntrig, int dacmin, int dacmax);
  /// returns TH2D's with hit maps
  std::vector<TH2D*> efficiencyMaps(std::string name, uint16_t ntrig = 10); 
  /// returns (mostly) TH2D's with maps of thresholds (plus additional histograms if "result" is set so)
  std::vector<TH1*> scurveMaps(std::string dac, std::string name, int ntrig = 10, int daclo = 0, int dachi = 255, int result = 3); 
  /// returns TH2D's for the threshold
  std::vector<TH1*> thrMaps(std::string dac, std::string name, int ntrig = 10);

  /// book a TH1D, adding version information to the name and title 
  TH1D* bookTH1D(std::string sname, std::string title, int nbins, double xmin, double xmax); 
  /// book a TH2D, adding version information to the name and title 
  TH2D* bookTH2D(std::string sname, std::string title, int nbinsx, double xmin, double xmax, int nbinsy, double ymin, double max); 

  /// select some pattern of pixels if not enabling the complete ROC. Enables the complete ROC if npix > 999
  virtual void sparseRoc(int npix = 8);

  /// creates a 1D distribution of a map
  TH1D* distribution(TH2D *, int nbins, double xmin, double xmax, bool zeroSuppressed = false); 
  /// fit an s-curve to a distribution. Fills fThreshold, fThresholdE, fSigma, fSigmaE
  bool threshold(TH1 *); 
  /// find first bin above 50% level. Fills fThreshold, fThresholdE, fSigma, fSigmaE
  int simpleThreshold(TH1 *); 
  /// maximum allowable VthrComp: "reserve" DAC units below the last 4 consecutive DAC values above "frac"
  std::vector<int> getMaximumVthrComp(int ntrig, double frac = 0.8, int reserve = 10);
  
  /// cache a DAC value
  void cache(std::string dacname); 
  /// restore a DAC value after caching it
  void restore(std::string dacname); 

  /// combine all available ROC maps into a module map
  TH2D* moduleMap(std::string histname); 

  /// delete histogams from HistList
  void clearHistList(); 

  /// returns the test name
  std::string getName() {return fName; }
  /// ???
  void resetDirectory();
  /// return fDirectory
  TDirectory* getDirectory() {return fDirectory;}

  /// returns a vector of test parameter names and string values
  std::vector<std::pair<std::string, std::string> > getParameters() {return fParameters;} 
  /// return by reference the INT value of a parameter
  bool getParameter(std::string parName, int &); 
  /// return by reference the FLOAT value of a parameter
  bool getParameter(std::string parName, float &); 
  /// set the string value of a parameter
  virtual bool setParameter(std::string parName, std::string sval); 
  /// print all parameters and values
  void dumpParameters(); 
  /// utility to set histogram titles
  void setTitles(TH1 *h, const char *sx, const char *sy, 
		 float size = 0.05, float xoff = 1.1, float yoff = 1.1, float lsize = 0.05, int font = 42);

  /// set the mapping between ROC ID and index
  void setId2Idx(std::map<int, int> a);
  /// provide the mapping between ROC ID and index
  int getIdFromIdx(int idx); 
  /// is ROC ID selected?
  bool selectedRoc(int id);

  /// decrepit, do not use
  static std::string stripPos(std::string); 

  /// signal to PixTab that the test is done (and to update the canvas)
  void testDone(); // *SIGNAL*
  /// signal to PixTab to update the canvas
  void update();  // *SIGNAL*
  /// allow forward iteration through list of histograms
  TH1* nextHist(); 
  /// allow backward iteration through list of histograms
  TH1* previousHist(); 
  

protected: 

  int histCycle(std::string hname);
  void fillMap(TH2D *hmod, TH2D *hroc, int iroc); 

  pxar::api            *fApi;
  PixSetup             *fPixSetup; 
  PixTestParameters    *fTestParameters; 
  PixInitFunc          *fPIF;   

  double               fThreshold, fThresholdE, fSigma, fSigmaE; 

  std::string           fName, fTestTip, fSummaryTip; 

  std::vector<std::pair<std::string, std::string> > fParameters;

  std::vector<uint8_t>  fCacheVal; 
  std::string           fCacheDac;

  TDirectory            *fDirectory; 
  std::list<TH1*>       fHistList;
  std::map<TH1*, std::string> fHistOptions;
  std::list<TH1*>::iterator fDisplayedHist;  

  std::vector<std::pair<int, int> > fPIX; // range of enabled pixels for time-consuming tests
  std::map<int, int>    fId2Idx; // map the ROC ID onto the (results vector) index of the ROC

  ClassDef(PixTest, 1); // testing PixTest

};

#endif
