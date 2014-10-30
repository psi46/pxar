#ifndef PIXTEST_H
#define PIXTEST_H
 
/** Declare all classes that need to be included in shared libraries on Windows
 *  as class DLLEXPORT className
 */
#include "pxardllexport.h"

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
#include <TProfile2D.h> 
#include <TTree.h> 
#include <TDirectory.h> 
#include <TFile.h>
#include <TSystem.h>
#include <TTimeStamp.h>

#include "api.h"
#include "log.h"

#include "PixInitFunc.hh"
#include "PixSetup.hh"
#include "PixTestParameters.hh"
#include "shist256.hh"

typedef struct { 
  uint16_t dac;
  uint16_t header; 
  uint16_t trailer; 
  uint16_t numDecoderErrors;
  uint8_t  npix;
  uint8_t  proc[2000];
  uint8_t  pcol[2000];
  uint8_t  prow[2000];
  double   pval[2000];
  double   pq[2000];
} TreeEvent;


///
/// PixTest
/// =======
/// 
/// Base class for all tests. If you write a test for pxar, it should inherit from this class.
/// 
/// 
/// Provides common utilities 
/// - mapping between roc index and roc ID
/// - keeps a list of the histograms and another list with their display options (fHistList, fHistOptions)
/// - booking of histgrams that are versioned between different test invocations (e.g. bookTH2D) 
/// - retrieve histograms instead readout decoding (e.g. efficiencyMap)
/// - list of selected pixels (fPIX)
///
class DLLEXPORT PixTest: public TQObject {
public:
  /// constructor requires PixSet to get test parameters and config parameters
  PixTest(PixSetup *a, std::string name);
  PixTest();
  virtual ~PixTest();
  /// sets all test parameters
  void init();
  /// use if you want, or define the histograms in the specific member functions
  void bookHist(std::string name);
  /// book a minimal tree with pixel events
  void bookTree();
  /// to be filled per test
  virtual void doAnalysis();
  /// function connected to "DoTest" button of PixTab
  virtual void doTest(); 
  /// allow execution of any button in the test 
  virtual void runCommand(std::string command); 
  /// create output suitable for moreweb
  virtual void output4moreweb();
  /// save DACs to file
  void saveDacs(); 
  /// save trim bits to file
  void saveTrimBits(); 
  /// save TB parameters to file
  void saveTbParameters(); 
  /// create vector (per ROC) of vector of dead pixels
  std::vector<std::vector<std::pair<int, int> > > deadPixels(int ntrig);
  /// mask all pixels mentioned in the mask file
  void maskPixels();     
  /// query whether test 'failed'
  bool testProblem() {return fProblem;}

  /// implement this to provide updated tool tips if the user changes test parameters
  virtual void setToolTips();
  /// hint to what happens if the user hits the "stop" button
  virtual std::string getStopTip() {return fStopTip;}
  /// hint to what happens if the user hits the "summary" button
  virtual std::string getSummaryTip() {return fSummaryTip;}
  /// get the string describing the test (called from PixTab::updateToolTips)
  virtual std::string getTestTip() {return fTestTip;}
  /// get the hist display options (if stored in fHistOptions) 
  virtual std::string getHistOption(TH1*);

  /// work-around to cope with suboptimal pxar/core
  int pixelThreshold(std::string dac, int ntrig, int dacmin, int dacmax);
  /// kind of another work-around (splitting the range, adjusting ntrig, etc)
  void dacScan(std::string dac, int ntrig, int dacmin, int dacmax, std::vector<shist256*> maps, int ihit, int flag = 0);
  /// do the scurve analysis
  void scurveAna(std::string dac, std::string name, std::vector<shist256*> maps, std::vector<TH1*> &resultMaps, int result);
  /// determine PH error interpolation
  void getPhError(std::string dac, int dacmin, int dacmax, int FLAGS, int ntrig);
  /// returns TH2D's with pulseheight maps
  std::vector<TH2D*> phMaps(std::string name, uint16_t ntrig = 10, uint16_t FLAGS = FLAG_FORCE_MASKED); 
  /// returns TH2D's with hit maps
  std::vector<TH2D*> efficiencyMaps(std::string name, uint16_t ntrig = 10, uint16_t FLAGS = FLAG_FORCE_MASKED); 
  /// returns (mostly) TH2D's with maps of thresholds (plus additional histograms if "result" is set so)
  /// ihit controls whether a hitmap (ihit == 1) or PH map (ihit == 2) is returned
  /// flag allows to pass in other flags
  /// result controls the amount of information (histograms) returned:
  /// result & 0x1: thr        maps
  /// result & 0x2: sig        maps
  /// result & 0x4: noise edge maps
  /// result & 0x8: also dump distributions for those maps enabled with 1,2, or 3
  /// result &0x10: dump 'problematic' threshold histogram fits
  std::vector<TH1*> scurveMaps(std::string dac, std::string name, int ntrig = 10, int daclo = 0, int dachi = 255, 
			       int result = 15, int ihit = 1, int flag = FLAG_FORCE_MASKED); 
  /// returns TH2D's for the threshold, the user flag argument is intended for selecting calS and will be OR'ed with other flags
  std::vector<TH1*> thrMaps(std::string dac, std::string name, uint8_t dacmin, uint8_t dachi, int ntrig, uint16_t flag = 0);
  std::vector<TH1*> thrMaps(std::string dac, std::string name, int ntrig, uint16_t flag = 0);


  /// Calculate average number of hits per pixel, and if this average is above
  /// a certain threshold, then look if there are any pixels which are a margin of
  /// safety above the average, and then mask these pixels and add them to the mask
  /// list.
  std::vector<std::pair<int,int> > checkHotPixels(TH2D* h);

  /// Return pixelAlive map and additional hit map when running with external source
  std::pair<std::vector<TH2D*>,std::vector<TH2D*> > xEfficiencyMaps(std::string name, uint16_t ntrig, 
								    uint16_t FLAGS = FLAG_CHECK_ORDER | FLAG_FORCE_UNMASKED);

  /// determine hot pixels with high occupancy
  void maskHotPixels(std::vector<TH2D*>); 
  /// send reset to ROC(s)
  void resetROC();
  /// send reset to TBM(s)
  void resetTBM();
  /// set up DAQ (including call to setTriggerFrequency)
  uint16_t prepareDaq(int triggerFreq, uint8_t trgTkDel);
  /// set trigger frequence [kHz] and trigger token delay
  uint16_t setTriggerFrequency(int triggerFreq, uint8_t TrgTkDel);
  /// functions for DAQ
  void finalCleanup();
  void pgToDefault();

  /// book a TH1D, adding version information to the name and title 
  TH1D* bookTH1D(std::string sname, std::string title, int nbins, double xmin, double xmax); 
  /// book a TH2D, adding version information to the name and title 
  TH2D* bookTH2D(std::string sname, std::string title, int nbinsx, double xmin, double xmax, int nbinsy, double ymin, double max); 
  /// book a TProfile2D, adding version information to the name and title 
  TProfile2D* bookTProfile2D(std::string sname, std::string title, 
			    int nbinsx, double xmin, double xmax, int nbinsy, double ymin, double max,
			    std::string option = ""); 
  /// fill the results of a api::getEfficiencyVsDAC into a TH1D; if icol/irow/iroc are > -1, then fill only 'correct' pixels
  void fillDacHist(std::vector<std::pair<uint8_t, std::vector<pxar::pixel> > > &results, TH1D *h, 
		   int icol = -1, int irow = -1, int iroc = -1); 

  /// select some pattern of pixels if not enabling the complete ROC. Enables the complete ROC if npix > 999
  virtual void sparseRoc(int npix = 8);

  /// creates a 1D distribution of a map
  TH1D* distribution(TH2D *, int nbins, double xmin, double xmax); 
  /// fit an s-curve to a distribution. Fills fThreshold, fThresholdE, fSigma, fSigmaE
  bool threshold(TH1 *); 
  /// find first bin above 50% level. Fills fThreshold, fThresholdE, fSigma, fSigmaE
  int simpleThreshold(TH1 *); 
  /// maximum allowable VthrComp
  std::vector<int> getMaximumVthrComp(int ntrig, double frac = 0.8, int reserve = 10);
  /// minimum allowable VthrComp; reserve indicate the separation from the minimum VthrComp where noise sets in
  std::vector<int> getMinimumVthrComp(std::vector<TH1*>, int reserve = 10, double nsigma = 3.);
  /// return minimum threshold in a set of maps
  double getMinimumThreshold(std::vector<TH1*>);
  /// return maximum threshold in a set of maps
  double getMaximumThreshold(std::vector<TH1*>);
  /// return a list of TH* that have 'name' as part to their histogram name
  std::vector<TH1*> mapsWithString(std::vector<TH1*>, std::string name);
  std::vector<TH2D*> mapsWithString(std::vector<TH2D*>, std::string name);

  /// produce eye-catching printouts
  void print(std::string, pxar::TLogLevel log = pxar::logINFO); 
  void banner(std::string, pxar::TLogLevel log = pxar::logINFO); 
  void bigBanner(std::string, pxar::TLogLevel log = pxar::logINFO); 
  
  /// cache all DACs 
  void cacheDacs(bool verbose = false); 
  /// restore all DACs
  void restoreDacs(bool verbose = false); 

  /// return from all ROCs the DAC dacName
  std::vector<uint8_t> getDacs(std::string dacName); 
  /// set on all ROCs the DAC dacName
  void setDacs(std::string dacName, std::vector<uint8_t> dacVector); 
  /// return from all ROCs the DAC dacName as a string
  std::string getDacsString(std::string dacName); 

  /// combine all available ROC maps into a module map
  virtual TH1* moduleMap(std::string histname); 

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
  /// return the string value of a parameter
  std::string getParameter(std::string parName);
  /// set the string value of a parameter
  virtual bool setParameter(std::string parName, std::string sval); 
  /// allow setting DACs in scripts for entire DUT
  virtual void setDAC(std::string parName, uint8_t val) {fApi->setDAC(parName, val);}
  /// allow setting DACs in scripts for spcific ROCs
  virtual void setDAC(std::string parName, uint8_t val, uint8_t rocid) {fApi->setDAC(parName, val, rocid);}
  /// print all parameters and values
  void dumpParameters(); 
  /// utility to set histogram titles
  void setTitles(TH1 *h, const char *sx, const char *sy, 
		 float size = 0.05, float xoff = 1.1, float yoff = 1.1, float lsize = 0.05, int font = 42);

  /// set the mapping between ROC ID and index
  void setId2Idx(std::map<int, int> a);
  /// provide the mapping between ROC ID and index
  int getIdFromIdx(int idx); 
  /// provide the mapping between ROC index and ID
  int getIdxFromId(int id); 
  /// is ROC ID selected?
  bool selectedRoc(int id);
  /// clear selected pixel list
  void clearSelectedPixels();
  /// add a selected pixel to the internal parameter list
  void addSelectedPixels(std::string sval);
  /// change the local parameter
  bool setTestParameter(std::string parname, std::string value);

  /// decrepit, do not use
  static std::string stripPos(std::string); 

  /// signal to PixTab that the test is done (and to update the canvas)
  void testDone(); // *SIGNAL*
  /// signal to PixTab to update the canvas
  void update();  // *SIGNAL*
  /// turn HV off
  void hvOff();  // *SIGNAL*
  /// turn HV on
  void hvOn();  // *SIGNAL*
  /// turn DTB power off
  void powerOff();  // *SIGNAL*
  /// turn DTB power on
  void powerOn();  // *SIGNAL*
  /// allow forward iteration through list of histograms
  TH1* nextHist(); 
  /// allow backward iteration through list of histograms
  TH1* previousHist(); 
  

protected: 

  int histCycle(std::string hname);   ///< determine histogram cycle
  void fillMap(TH2D *hmod, TH2D *hroc, int iroc);  ///< provides the coordinate transformation to module map

  pxar::pxarCore            *fApi;  ///< pointer to the API
  PixSetup             *fPixSetup;  ///< all necessary stuff in one place
  PixTestParameters    *fTestParameters;  ///< the repository of all test parameters
  PixInitFunc          *fPIF;    ///< function instantiation and automatic initialization

  double               fThreshold, fThresholdE, fSigma, fSigmaE;  ///< variables for passing back s-curve results
  double               fThresholdN; ///< variable for passing back the threshold where noise leads to loss of efficiency
  int                  fNtrig; 
  std::vector<double>  fPhErrP0, fPhErrP1; 
  uint32_t             fNDaqErrors; 

  std::string           fName, fTestTip, fSummaryTip, fStopTip; ///< information for this test

  std::vector<std::pair<std::string, std::string> > fParameters; ///< the parameters of this test

  std::vector<std::vector<std::pair<std::string,uint8_t> > >  fDacCache; ///< vector for all ROCs 

  TDirectory            *fDirectory; ///< where the root histograms will end up
  std::list<TH1*>       fHistList; ///< list of histograms available in PixTab::next and PixTab::previous
  std::map<TH1*, std::string> fHistOptions; ///< options can be stored with each histogram
  std::list<TH1*>::iterator fDisplayedHist;  ///< pointer to the histogram currently displayed

  std::vector<std::pair<int, int> > fPIX; ///< range of enabled pixels for time-consuming tests
  std::map<int, int>    fId2Idx; ///< map the ROC ID onto the (results vector) index of the ROC
  TTree                *fTree; 
  TreeEvent             fTreeEvent;
  TTimeStamp           *fTimeStamp; 

  bool                  fProblem;
  

  // -- data members for DAQ purposes
  std::vector<std::pair<std::string, uint8_t> > fPg_setup;
  std::vector<std::vector<std::pair<int, int> > > fHotPixels;

  ClassDef(PixTest, 1); // testing PixTest

};

#endif
