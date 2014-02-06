#ifndef PIXTESTPRETEST_H
#define PIXTESTPRETEST_H

#include "api.h"
#include "PixTest.hh"

class PixTestPretest: public PixTest {
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
  void setVthrCompId();
  void setCalDel();
  void saveDacs();

private:

  int     fTargetIa;
  int     fNoiseWidth;
  int     fNoiseMargin;
  int     fParNtrig;

  ClassDef(PixTestPretest, 1);

};
#endif
