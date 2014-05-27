#ifndef PIXTESTIV_H
#define PIXTESTIV_H

#include "PixTest.hh"

class DLLEXPORT PixTestIV: public PixTest {
public:
  PixTestIV(PixSetup *, std::string);
  PixTestIV();
  virtual ~PixTestIV();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void bookHist(std::string); 
  void doTest();

private:

  uint32_t fParVoltageMax;
  uint32_t fParVoltageStep;

  ClassDef(PixTestIV, 1)

};
#endif
