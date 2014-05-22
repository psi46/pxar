// -- author: Daniel Pitzl
#ifndef PIXTESTSETPH_H
#define PIXTESTSETPH_H

#include "PixTest.hh"

class DLLEXPORT PixTestSetPh: public PixTest {
public:
  PixTestSetPh(PixSetup *, std::string);
  PixTestSetPh();
  virtual ~PixTestSetPh();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void doTest(); 

private:

  int     fParNtrig; 
  int     fParCals;

  ClassDef(PixTestSetPh, 1)

};
#endif
