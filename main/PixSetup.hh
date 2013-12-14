#ifndef PIXSETUP_H
#define PIXSETUP_H

#include <string>
#include <map>

#include <TQObject.h> 
#include <TDirectory.h> 

#include "TBInterface.hh"
#include "PixTestParameters.hh"
#include "PixSetup.hh"
#include "PixModule.hh"

class PixSetup: public TObject {
public:
  PixSetup(TBInterface *, PixTestParameters *, PixModule *);
  PixSetup();
  void init(); 

private: 
  TBInterface       *fTB; 
  PixTestParameters *fTP; 
  PixModule         *fModule; 
};


#endif

