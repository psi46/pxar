#ifndef PIXSETUP_H
#define PIXSETUP_H

#include <string>
#include <map>

#include <TObject.h> 

#include "api.h"

#include "PixTestParameters.hh"
#include "ConfigParameters.hh"

class PixSetup: public TObject {
public:
  PixSetup(pxar::pxarCore *, PixTestParameters *, ConfigParameters *);
  PixSetup(std::string verbosity, PixTestParameters *, ConfigParameters *);
  PixSetup();
  ~PixSetup();
  void init(); 

  PixTestParameters* getPixTestParameters() {return fPixTestParameters;}
  ConfigParameters * getConfigParameters()  {return fConfigParameters;}
  pxar::pxarCore*    getApi() {return fApi;}
  bool               doAnalysisOnly() {return fDoAnalysisOnly;}
  void               setDoAnalysisOnly(bool x) {fDoAnalysisOnly = x;}
  bool               useRootLogon() {return fUseRootLogon;} 
  void               setUseRootLogon(bool x) {fUseRootLogon = x;} 
  void               killApi(); 

private: 
  bool              fDebug; 
  bool              fDoAnalysisOnly; 
  bool              fUseRootLogon;

  pxar::pxarCore    *fApi; 
  PixTestParameters *fPixTestParameters; 
  ConfigParameters  *fConfigParameters;   

  ClassDef(PixSetup, 1); 
};

#endif

