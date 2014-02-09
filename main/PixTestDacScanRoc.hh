#ifndef PIXTESTDACSCANROC_H
#define PIXTESTDACSCANROC_H

#include "api.h"
#include "PixTest.hh"

class PixTestDacScanRoc: public PixTest {
public:
  PixTestDacScanRoc(PixSetup *, std::string);
  PixTestDacScanRoc();
  virtual ~PixTestDacScanRoc();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 
  
  void doTest(); 

private:

  int     fParNtrig; 
  std::string fParDAC; 
  int     fParLoDAC, fParHiDAC;

  ClassDef(PixTestDacScanRoc, 1);

};
#endif
