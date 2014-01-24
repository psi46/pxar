#ifndef PIXTESTTRIM_H
#define PIXTESTTRIM_H

#include "api.h"
#include "PixTest.hh"

class PixTestTrim: public PixTest {
public:
  PixTestTrim(PixSetup *, std::string);
  PixTestTrim();
  virtual ~PixTestTrim();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void bookHist(std::string); 
  
  void doTest(); 

private:

  int     fParVcal, fParNtrig, fParVcthrCompLo, fParVcthrCompHi, fParVcalLo, fParVcalHi; 
  std::vector<std::pair<int, int> > fPIX; 

  ClassDef(PixTestTrim, 1);

};
#endif
