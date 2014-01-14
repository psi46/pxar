#ifndef PIXTESTSETUP_H
#define PIXTESTSETUP_H

#include "api.h"
#include "PixTest.hh"

class PixTestSetup: public PixTest {
public:
  PixTestSetup(PixSetup *, std::string);
  PixTestSetup();
  virtual ~PixTestSetup();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 

  void doTest(); 

private:

  int     fParNtrig; 
  int     fParVcal; 

  ClassDef(PixTestSetup, 1);

};
#endif
