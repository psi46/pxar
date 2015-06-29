#ifndef PIXTESTBB2MAP_H
#define PIXTESTBB2MAP_H

#include "PixTest.hh"

class DLLEXPORT PixTestBB2Map: public PixTest {
public:
  PixTestBB2Map(PixSetup *, std::string);
  PixTestBB2Map();
  virtual ~PixTestBB2Map();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();

  void doTest(); 
  double getBB2MapCut(TH1D *hPlSizePerRoc);
  void setVana();
  void setVthrCompCalDelForCals();

private:

  int          fTargetIa;
  int          fParNtrig; 
  int          fParVcalS; 
  int          fParPlWidth; 
  
  ClassDef(PixTestBB2Map, 1); 

};
#endif
