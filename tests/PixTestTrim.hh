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
  void getVthrCompThr();
  std::vector<std::pair<int,int> > checkHotPixels(TH2D* h);

  int adjustVtrim(); 
  std::vector<TH1*> trimStep(std::string name, int corrections, std::vector<TH1*> calMapOld, int vcalMin, int vcalMax); 
  void setTrimBits(int itrim = -1); 

  void doTest(); 
  void output4moreweb();

private:

  int     fParVcal, fParNtrig; 
  std::vector<std::pair<int, int> > fPIX; 
  int fTrimBits[16][52][80]; 
  
  //-- VthrComp for X-rays calibration
  bool    fFinal;
  std::pair<std::vector<TH2D*>,std::vector<TH2D*> > xEfficiencyMaps(std::string name, uint16_t ntrig, uint16_t FLAGS);
  std::string getVthrCompString(std::vector<uint8_t>rocIds,std::vector<int> VthrComp);
  std::vector<int> xPixelAliveSingleSweep();
  
  ClassDef(PixTestTrim, 1)

};
#endif
