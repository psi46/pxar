#ifndef PixTestPh_H
#define PixTestPh_H

#include "PixTest.hh"


class DLLEXPORT PixTestPh: public PixTest {
public:
  PixTestPh(PixSetup *, std::string);
  PixTestPh();
  virtual ~PixTestPh();
  virtual bool setParameter(std::string parName, std::string sval);
  void init();
  void setToolTips();
  std::string toolTip(std::string what);
  void bookHist(std::string);

  void runCommand(std::string);
  void doTest();
  void phMap();
  void optimize();
  void scan(std::string name);
  void adjustVthrComp();

 private:

  int     fParNtrig;
  std::string fParDAC;
  int     fParDacVal;
  // -- PH optimziation
  std::map<std::string, TH2D*> fMaps;
  int    fAdjustVthrComp, fVcalLow, fVcalHigh, fPhScaleMin, fPhOffsetMin, fPhMin, fPhMax;

  ClassDef(PixTestPh, 1)

};
#endif
