#ifndef PIXTESTMAPPH_H
#define PIXTESTMAPPH_H

#include "api.h"
#include "constants.h" // FLAG_USE_CALS
#include "PixTest.hh"

class PixTestMapPh: public PixTest {
public:
  PixTestMapPh(PixSetup *, std::string);
  PixTestMapPh();
  virtual ~PixTestMapPh();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void doTest(); 

private:

  int     fParNtrig; 
  int     fParCals;

  ClassDef(PixTestMapPh, 1);

};
#endif
