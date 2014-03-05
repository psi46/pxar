#ifndef PIXTESTTRIM_H
#define PIXTESTTRIM_H

#include "api.h"
#include "PixTest.hh"

class PixTestTrim: public PixTest {
public:
  PixTestTrim(PixSetup *, std::string);
  PixTestTrim();
  virtual ~PixTestTrim();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void runCommand(std::string); 
  void bookHist(std::string); 

  void trimBitTest();

  int adjustVtrim(); 
  std::vector<TH1*> trimStep(int corrections, std::vector<TH1*> calMapOld); 
  void setTrimBits(); 
  void doTest(); 
  void dummyAnalysis(); 
  void output4moreweb();

private:

  int     fParVcal, fParNtrig, fParVthrCompLo, fParVthrCompHi, fParVcalLo, fParVcalHi; 
  std::vector<std::pair<int, int> > fPIX; 
  int     fTrimBits[16][52][80]; 
  
  ClassDef(PixTestTrim, 1);

};
#endif
