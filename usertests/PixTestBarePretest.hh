#ifndef PIXTESTBAREPRETEST_H
#define PIXTESTBAREPRETEST_H

#include "PixTest.hh"

class DLLEXPORT PixTestBarePretest: public PixTest {
public:
  PixTestBarePretest(PixSetup *, std::string);
  PixTestBarePretest();
  virtual ~PixTestBarePretest();
  virtual bool setParameter(std::string parName, std::string sval);
  void init();
  void setToolTips();
  void runCommand(std::string);
  void bookHist(std::string);

  void doTest();
  void setVana();
  void programROC();
  void setVthrCompCalDel();
  void setVthrCompId();
  void setCalDel();
  

private:

  int     fTargetIa;
  int     fNoiseWidth;
  int     fNoiseMargin;
  int     fParNtrig;
  int     fParVcal, fParDeltaVthrComp;
  bool    fProblem;

  ClassDef(PixTestBarePretest, 1)

};
#endif
