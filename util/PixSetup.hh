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
  void               setDummy(bool x) {fIsDummy = x;}
  bool               isDummy() {return fIsDummy;}
private: 
  bool              fDebug; 
  bool              fIsDummy; 
  bool              fDoAnalysisOnly; 
  bool              fUseRootLogon;

  pxar::api         *fApi; 
  PixTestParameters *fPixTestParameters; 
  ConfigParameters  *fConfigParameters;   

  ClassDef(PixSetup, 1); 
};

#endif

