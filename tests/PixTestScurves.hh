#ifndef PIXTESTSCURVES_H
#define PIXTESTSCURVES_H

#include "PixTest.hh"

class DLLEXPORT PixTestScurves: public PixTest {
public:
  PixTestScurves(PixSetup *, std::string);
  PixTestScurves();
  virtual ~PixTestScurves();
  virtual bool setParameter(std::string parName, std::string sval);
  void init();
  void setToolTips();
  std::string toolTip(std::string what);
  void bookHist(std::string);

  void runCommand(std::string command);
  void thrMap();
  void fitS();
  void scurves();
  void adjustVcal();

  void doTest();
  void fullTest();

private:

  std::string fParDac;
  int         fParNtrig, fParDacLo, fParDacHi, fParDacsPerStep, fParNtrigPerStep, fAdjustVcal, fDumpAll, fDumpProblematic, fDumpOutputFile;

  ClassDef(PixTestScurves, 1)

};
#endif
