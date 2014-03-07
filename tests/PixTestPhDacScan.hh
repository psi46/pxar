#ifndef PIXTESTPHDACSCAN_H
#define PIXTESTPHDACSCAN_H

#include "api.h"
#include "PixTest.hh"

class DLLEXPORT PixTestPhDacScan: public PixTest {
public:
  PixTestPhDacScan(PixSetup *, std::string);
  PixTestPhDacScan();
  virtual ~PixTestPhDacScan();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void bookHist(std::string); 
  
  void doTest(); 

private:

  int     fParNtrig; 
  std::string fParDAC; 
  int     fParLoDAC, fParHiDAC;
  std::vector<std::pair<int, int> > fPIX; 

  ClassDef(PixTestPhDacScan, 1);

};
#endif
