#ifndef PIXSETUP_H
#define PIXSETUP_H

#include <string>
#include <map>

#include <TObject.h> 

#include "api.h"

#include "PixTestParameters.hh"
#include "ConfigParameters.hh"
#include "PixModule.hh"
#include "SysCommand.hh"

class PixSetup: public TObject {
public:
  PixSetup(pxar::api *, PixTestParameters *, ConfigParameters *, SysCommand *);
  PixSetup();
  ~PixSetup();
  void init(); 

  PixTestParameters* getPixTestParameters() {return fPixTestParameters;}
  ConfigParameters * getConfigParameters()  {return fConfigParameters;}
  PixModule*         getModule()  {return fModule;}
  SysCommand*        getSysCommand()  {return fSysCommand;}
  pxar::api*         getApi() {return fApi;}
private: 
  bool              fDebug; 
  SysCommand        *fSysCommand;
  pxar::api         *fApi; 
  PixTestParameters *fPixTestParameters; 
  ConfigParameters  *fConfigParameters;   
  PixModule         *fModule; 

  ClassDef(PixSetup, 1); 
};

#endif

