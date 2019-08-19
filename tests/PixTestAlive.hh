#ifndef PIXTESTALIVE_H
#define PIXTESTALIVE_H

#include "PixTest.hh"

class DLLEXPORT PixTestAlive: public PixTest {
public:
  PixTestAlive(PixSetup *, std::string);
  PixTestAlive();
  virtual ~PixTestAlive();
  virtual bool setParameter(std::string parName, std::string sval);
  void init();
  void setToolTips();
  std::string toolTip(std::string what);
  void bookHist(std::string);

  void runCommand(std::string);
  void aliveTest();
  void maskTest();
  void addressDecodingTest();

  void doTest();

private:

  uint16_t    fParNtrig;
  int         fParVcal;
  bool        fParMaskDeadPixels, fParSaveMaskedPixels;
  std::string fParMaskFileName;

  ClassDef(PixTestAlive, 1)

};
#endif
