// -- author: Daniel Pitzl
#ifndef PIXTESTMAPTHR_H
#define PIXTESTMAPTHR_H

#include "api.h"
#include "PixTest.hh"

class PixTestMapThr: public PixTest {
public:
  PixTestMapThr(PixSetup *, std::string);
  PixTestMapThr();
  virtual ~PixTestMapThr();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void doTest(); 

private:

  int     fParNtrig; 

  ClassDef(PixTestMapThr, 1);

};
#endif
