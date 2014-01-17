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
  std::pair<int, int> fPIX1, fPIX2, fPIX3, fPIX4; 

  ClassDef(PixTestDacScan, 1);

};
#endif
