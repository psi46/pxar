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
  void getVthrCompThr();
  

private:

  int     fTargetIa;
  int     fNoiseWidth;
  int     fNoiseMargin;
  int     fParNtrig;
  int     fParVcal, fParDeltaVthrComp;
  bool    fProblem; 
  bool    fFinal;
  
  //-- VthrComp for X-rays calibration
  std::pair<std::vector<TH2D*>,std::vector<TH2D*> > xEfficiencyMaps(std::string name, uint16_t ntrig, uint16_t FLAGS);
  std::string getVthrCompString(std::vector<uint8_t>rocIds,std::vector<int> VthrComp);
  std::vector<int> xPixelAliveSingleSweep();

  ClassDef(PixTestPretest, 1)

};
#endif
