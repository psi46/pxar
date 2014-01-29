#ifndef PIXTESTSCURVES_H
#define PIXTESTSCURVES_H

#include "api.h"
#include "PixTest.hh"

class PixTestScurves: public PixTest {
public:
  PixTestScurves(PixSetup *, std::string);
  PixTestScurves();
  virtual ~PixTestScurves();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void bookHist(std::string); 

  void doTest(); 

private:

  std::string fParDac;
  int         fParNtrig, fParDacLo, fParDacHi;

  ClassDef(PixTestScurves, 1);

};
#endif
