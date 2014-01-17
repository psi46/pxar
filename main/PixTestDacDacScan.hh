#ifndef PIXTESTDACDACSCAN_H
#define PIXTESTDACDACSCAN_H

#include "api.h"
#include "PixTest.hh"

class PixTestDacDacScan: public PixTest {
public:
  PixTestDacDacScan(PixSetup *, std::string);
  PixTestDacDacScan();
  virtual ~PixTestDacDacScan();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void bookHist(std::string); 

  void doTest(); 

private:

  int     fParNtrig; 
  std::string fParDAC1, fParDAC2; 
  int     fParLoDAC1, fParHiDAC1;
  int     fParLoDAC2, fParHiDAC2;
  std::vector<std::pair<int, int> > fPIX; 

  ClassDef(PixTestDacDacScan, 1);

};
#endif
