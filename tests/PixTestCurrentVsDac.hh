#ifndef PIXTESTCURRENTVSDAC_H
#define PIXTESTCURRENTVSDAC_H

#include "PixTest.hh"

class DLLEXPORT PixTestCurrentVsDac: public PixTest {
public:
  PixTestCurrentVsDac(PixSetup *, std::string);
  PixTestCurrentVsDac();
  virtual ~PixTestCurrentVsDac();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void bookHist(std::string); 
  
  void doTest(); 

private:

  std::string fParDAC; 

  ClassDef(PixTestCurrentVsDac, 1);

};
#endif
