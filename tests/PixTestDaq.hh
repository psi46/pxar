#ifndef PIXTESTDAQ_H
#define PIXTESTDAQ_H

#include "api.h"
#include "PixTest.hh"

class PixTestDaq: public PixTest {
public:
  PixTestDaq(PixSetup *, std::string);
  PixTestDaq();
  virtual ~PixTestDaq();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void doTest(); 

private:

  int     fParNtrig; 

  ClassDef(PixTestDaq, 1);

};
#endif
