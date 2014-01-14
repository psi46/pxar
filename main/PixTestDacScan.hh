#ifndef PIXTESTDACSCAN_H
#define PIXTESTDACSCAN_H

#include "api.h"
#include "PixTest.hh"

class PixTestDacScan: public PixTest {
public:
  PixTestDacScan(PixSetup *, std::string);
  PixTestDacScan();
  virtual ~PixTestDacScan();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  
  void doTest(); 

private:

  int     fParNtrig; 
  std::string fParDAC; 

  ClassDef(PixTestDacScan, 1);

};
#endif
