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
  void bookHist(std::string); 

  void runCommand(std::string command); 
  void thrMap(); 
  void fitS(); 
  void scurves(); 
  void adjustVcal();

  void doTest(); 
  void output4moreweb();

private:

  std::string fParDac;
  int         fParNtrig, fParNpix, fParDacLo, fParDacHi, fAdjustVcal;

  ClassDef(PixTestScurves, 1)

};
#endif
