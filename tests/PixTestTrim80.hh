#ifndef PIXTESTTRIM80_H
#define PIXTESTTRIM80_H

#include "PixTest.hh"

class DLLEXPORT PixTestTrim80: public PixTest {
public:
  PixTestTrim80(PixSetup *, std::string);
  PixTestTrim80();
  virtual ~PixTestTrim80();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void runCommand(std::string); 
  void trimBitTest();
  void trimTest();

  int adjustVtrim(); 
  std::vector<TH1*> trimStep(std::string name, int corrections, std::vector<TH1*> calMapOld, int vcalMin, int vcalMax); 
  void setTrimBits(int itrim = -1); 

  void doTest(); 

private:

  int     fParVcal, fParNtrig; 
  std::vector<std::pair<int, int> > fPIX; 
  int fTrimBits[16][52][80]; 
  
  ClassDef(PixTestTrim80, 1)

};
#endif
