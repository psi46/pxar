#ifndef PIXSETUP_H
#define PIXSETUP_H

#include <string>
#include <map>

#include "api.h"

#include "PixTestParameters.hh"
#include "ConfigParameters.hh"

class DLLEXPORT PixSetup {
public:
  PixSetup(pxar::api *, PixTestParameters *, ConfigParameters *);
  PixSetup(std::string verbosity, PixTestParameters *, ConfigParameters *);
  PixSetup();
  ~PixSetup();
  void init(); 

  PixTestParameters* getPixTestParameters() {return fPixTestParameters;}
  ConfigParameters * getConfigParameters()  {return fConfigParameters;}
  pxar::api*         getApi() {return fApi;}
  bool               doAnalysisOnly() {return fDoAnalysisOnly;}
  void               setDoAnalysisOnly(bool x) {fDoAnalysisOnly = x;}
  bool               useRootLogon() {return fUseRootLogon;} 
  void               setUseRootLogon(bool x) {fUseRootLogon = x;} 
  void               killApi(); 
  void               setMoreWebCloning(bool x) {fMoreWebCloning = x;}
  bool               doMoreWebCloning() {return fMoreWebCloning;}
private: 
  bool              fMoreWebCloning;
  bool              fDoAnalysisOnly; 
  bool              fUseRootLogon;

  pxar::api         *fApi; 
  PixTestParameters *fPixTestParameters; 
  ConfigParameters  *fConfigParameters;   

};

#endif

