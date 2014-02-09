#ifndef PIXTESTDACSCANTHR_H
#define PIXTESTDACSCANTHR_H

#include "api.h"
#include "PixTest.hh"

class PixTestDacScanThr: public PixTest {
public:
  PixTestDacScanThr(PixSetup *, std::string);
  PixTestDacScanThr();
  virtual ~PixTestDacScanThr();
  virtual bool setParameter(std::string parName, std::string sval);
  void init();
  void setToolTips();
  void bookHist(std::string);

  void doTest();

private:

  int     fParNtrig;
  std::string fParDAC;
  int     fParLoDAC, fParHiDAC;
  int     fParCals;
  std::vector<std::pair<int, int> > fPIX;

  ClassDef(PixTestDacScanThr, 1);

};
#endif
