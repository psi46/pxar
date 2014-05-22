#ifndef PIXTESTTRIM_H
#define PIXTESTTRIM_H

#include "PixTest.hh"

class DLLEXPORT PixTestTrim: public PixTest {
public:
  PixTestTrim(PixSetup *, std::string);
  PixTestTrim();
  virtual ~PixTestTrim();
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
  void dummyAnalysis(); 
  void output4moreweb();

private:

  int     fParVcal, fParNtrig, fParVthrCompLo, fParVthrCompHi, fParVcalLo, fParVcalHi; 
  std::vector<std::pair<int, int> > fPIX; 
  int fTrimBits[16][52][80]; 
  
  ClassDef(PixTestTrim, 1)

};
#endif
