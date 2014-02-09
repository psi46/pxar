#ifndef PIXTESTMAPTHR_H
#define PIXTESTMAPTHR_H

#include "api.h"
#include "constants.h" // FLAG_USE_CALS
#include "PixTest.hh"

class PixTestMapThr: public PixTest {
public:
  PixTestMapThr(PixSetup *, std::string);
  PixTestMapThr();
  virtual ~PixTestMapThr();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void bookHist(std::string); 

  void doTest(); 

private:

  int     fParNtrig; 

  ClassDef(PixTestMapThr, 1);

};
#endif
