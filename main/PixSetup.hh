#ifndef PIXSETUP_H
#define PIXSETUP_H

#include <string>
#include <map>

#include <TObject.h> 

#include "api.h"

#include "PixTestParameters.hh"
#include "ConfigParameters.hh"
#include "SysCommand.hh"

class PixSetup: public TObject {
public:
  PixSetup(pxar::api *, PixTestParameters *, ConfigParameters *, SysCommand *);
  PixSetup();
  ~PixSetup();
  void init(); 

  PixTestParameters* getPixTestParameters() {return fPixTestParameters;}
  ConfigParameters * getConfigParameters()  {return fConfigParameters;}
  SysCommand*        getSysCommand()  {return fSysCommand;}
  pxar::api*         getApi() {return fApi;}
  bool               doAnalysisOnly() {return fDoAnalysisOnly;}
  void               setDoAnalysisOnly(bool x) {fDoAnalysisOnly = x;}
private: 
  bool              fDebug; 
  bool              fDoAnalysisOnly; 
  SysCommand        *fSysCommand;
  pxar::api         *fApi; 
  PixTestParameters *fPixTestParameters; 
  ConfigParameters  *fConfigParameters;   

  ClassDef(PixSetup, 1); 
};

#endif

