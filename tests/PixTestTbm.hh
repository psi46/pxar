#ifndef PIXTESTTBM_H
#define PIXTESTTBM_H

#include "api.h"
#include "PixTest.hh"

class PixTestTbm: public PixTest {
public:
  PixTestTbm(PixSetup *, std::string);
  PixTestTbm();
  virtual ~PixTestTbm();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  
  void bookHist(std::string); 

  void doTest(); 

private:

  int     fParNtrig; 
  int     fParVcal; 

  ClassDef(PixTestTbm, 1);

};
#endif
