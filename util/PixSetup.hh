#ifndef PIXSETUP_H
#define PIXSETUP_H

#include <string>
#include <map>

#include "api.h"

#include "PixTestParameters.hh"
#include "ConfigParameters.hh"
#include "PixMonitor.hh"

class DLLEXPORT PixSetup {
public:
  PixSetup(pxar::pxarCore *, PixTestParameters *, ConfigParameters *);
  PixSetup(std::string verbosity, PixTestParameters *, ConfigParameters *);
  PixSetup();
  ~PixSetup();
  void init(); 

  PixTestParameters* getPixTestParameters() {return fPixTestParameters;}
  ConfigParameters * getConfigParameters()  {return fConfigParameters;}
  pxar::pxarCore*    getApi() {return fApi;}
  PixMonitor*        getPixMonitor() {return fPixMonitor;}
  bool               doAnalysisOnly() {return fDoAnalysisOnly;}
  void               setDoAnalysisOnly(bool x) {fDoAnalysisOnly = x;}
  bool               useRootLogon() {return fUseRootLogon;} 
  void               setUseRootLogon(bool x) {fUseRootLogon = x;} 
  void               killApi(); 
  void               setRootFileUpdate(bool x) {fDoUpdateRootFile = x;}
  bool               doRootFileUpdate() {return fDoUpdateRootFile;}

  void               writeDacParameterFiles();
  void               writeTrimFiles();
  void               writeTbmParameterFiles();
  

  void              *fPxarMemory, *fPxarMemHi;
private: 
  bool              fDoUpdateRootFile; 
  bool              fDoAnalysisOnly; 
  bool              fUseRootLogon;

  pxar::pxarCore    *fApi; 
  PixTestParameters *fPixTestParameters; 
  ConfigParameters  *fConfigParameters;   
  PixMonitor        *fPixMonitor; 

};

#endif

