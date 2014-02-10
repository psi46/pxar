#ifndef PIXTESTDACSCANPIX_H
#define PIXTESTDACSCANPIX_H

#include "api.h"
#include "PixTest.hh"

class PixTestDacScanPix: public PixTest {
public:
  PixTestDacScanPix(PixSetup *, std::string);
  PixTestDacScanPix();
  virtual ~PixTestDacScanPix();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 
  
  void doTest(); 

private:

  int     fParNtrig; 
  std::string fParDAC; 
  int     fParLoDAC, fParHiDAC;

  ClassDef(PixTestDacScanPix, 1);

};
#endif
