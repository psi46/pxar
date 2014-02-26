// -- author: Daniel Pitzl
#ifndef PIXTESTDACSCANCURRENT_H
#define PIXTESTDACSCANCURRENT_H

#include "PixTest.hh"

class PixTestDacScanCurrent: public PixTest {
public:
  PixTestDacScanCurrent(PixSetup *, std::string);
  PixTestDacScanCurrent();
  virtual ~PixTestDacScanCurrent();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 
  
  void doTest(); 

private:

  std::string fParDAC; 

  ClassDef(PixTestDacScanCurrent, 1);

};
#endif
