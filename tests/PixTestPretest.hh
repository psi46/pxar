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
  std::string toolTip(std::string what);
  void runCommand(std::string);
  void bookHist(std::string);

  void doTest();
  void setVana();
  void programROC();
  /// Doug's timing setting
  void setTimings();
  /// Wolfram's timing setting (optimized)
  void findTiming();
  void findWorkingPixel();
  void setVthrCompCalDel();
  void setVthrCompId();
  void setCalDel();

private:

  int     fTargetIa;
  int     fNoiseWidth;
  int     fNoiseMargin;
  int     fIterations;
  int     fParNtrig;
  int     fParVcal, fParDeltaVthrComp;
  double  fParFracCalDel;
  int     fIgnoreProblems;

  ClassDef(PixTestPretest, 1)

};
#endif
