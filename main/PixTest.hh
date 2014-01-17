#ifndef PIXTEST_H
#define PIXTEST_H

#include <string>
#include <map>
#include <list>

#include <TQObject.h> 
#include <TH1.h> 
#include <TDirectory.h> 
#include <TFile.h>
#include <TSystem.h>

#include "api.h"

#include "PixSetup.hh"
#include "PixTestParameters.hh"

class PixTest: public TQObject {
public:
  PixTest(PixSetup *a, std::string name);
  PixTest();
  virtual ~PixTest();
  void init(PixSetup *, std::string name);
  void bookHist(std::string name);

  void clearHist(); 
  virtual void doTest(); 
  virtual void doAnalysis();
  
  std::string getName() {return fName; }
  std::map<std::string, std::string> getParameters() {return fParameters;} 
  bool getParameter(std::string parName, int &); 
  bool getParameter(std::string parName, float &); 
  void dumpParameters(); 
  void setTitles(TH1 *h, const char *sx, const char *sy, 
		 float size = 0.05, float xoff = 1.1, float yoff = 1.1, float lsize = 0.05, int font = 42);

  virtual bool setParameter(std::string parName, std::string sval); 


  void testDone(); // *SIGNAL*
  void update();  // *SIGNAL*
  TH1* nextHist(); 
  TH1* previousHist(); 
  

protected: 
  pxar::api            *fApi;
  PixSetup             *fPixSetup; 
  PixTestParameters    *fTestParameters; 
  
  std::string           fName; 

  std::map<std::string, std::string> fParameters;

  TDirectory            *fDirectory; 
  std::list<TH1*>       fHistList;
  std::list<TH1*>::iterator fDisplayedHist;  

  ClassDef(PixTest, 1); // testing PixTest

};

#endif
