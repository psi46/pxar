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
  void bookHist(std::string);

  void runCommand(std::string);
  void doTest();
  void phMap();

private:

  int     fParNtrig;
  std::string fParDAC;
  int     fParDacVal;

  ClassDef(PixTestPh, 1)

};
#endif
