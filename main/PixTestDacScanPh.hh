#ifndef PIXTESTDACSCANPH_H
#define PIXTESTDACSCANPH_H

#include "api.h"
#include "constants.h" // FLAG_USE_CALS
#include "PixTest.hh"

class PixTestDacScanPh: public PixTest {
public:
  PixTestDacScanPh(PixSetup *, std::string);
  PixTestDacScanPh();
  virtual ~PixTestDacScanPh();
  virtual bool setParameter(std::string parName, std::string sval);
  void init();
  void bookHist(std::string);

  void doTest();

private:

  int     fParNtrig;
  std::string fParDAC;
  int     fParLoDAC, fParHiDAC;
  int     fParCals;
  std::vector<std::pair<int, int> > fPIX;

  ClassDef(PixTestDacScanPh, 1);

};
#endif
