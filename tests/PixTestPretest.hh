#ifndef PIXTESTPRETEST_H
#define PIXTESTPRETEST_H

#include "PixTest.hh"

class DLLEXPORT PixTestPretest: public PixTest {
public:
  PixTestPretest(PixSetup *, std::string);
  PixTestPretest();
  virtual ~PixTestPretest();
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
  void setPhRange();

private:

  int     fTargetIa;
  int     fNoiseWidth;
  int     fNoiseMargin;
  int     fParNtrig;
  int     fParVcal, fParDeltaVthrComp;

  ClassDef(PixTestPretest, 1)

};
#endif
