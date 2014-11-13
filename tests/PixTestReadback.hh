#ifndef PIXTESTREADBACK_H
#define PIXTESTREADBACK_H

#include "PixTest.hh"
#include "PHCalibration.hh"

#include <TH1.h>
#include <TH2.h>
#include <TProfile2D.h>

#include <TTree.h>

class DLLEXPORT PixTestReadback: public PixTest {
public:
  PixTestReadback(PixSetup *, std::string);
  PixTestReadback();
  virtual ~PixTestReadback();
  void init(); 
  void setToolTips();
  void bookHist(std::string);  
  void runCommand(std::string command);
  virtual bool setParameter(std::string parName, std::string sval);
  bool setTrgFrequency(uint8_t TrgTkDel);
  void pgToDefault();
  void setHistos();
  void ProcessData(uint16_t numevents = 1000);
  void FinalCleaning();
  uint8_t daqReadback(std::string dac, uint8_t vana, int8_t parReadback);
  uint8_t daqReadback(std::string dac, double vana, int8_t parReadback);
  uint8_t daqReadbackIa();
  void CalibrateIa();
  void CalibrateVana();
  void CalibrateVd();
  void CalibrateVa();
  double getCalibratedVbg();
  void readbackVbg();
  void doTest();
  double getCalibratedIa();
  void setVana();
  void doDAQ();

private:

  void stop();


  uint8_t  fParReadback;
  uint16_t fParPeriod;
  uint16_t fParStretch; 
  bool     fParFillTree;
  uint16_t fParTriggerFrequency;
  bool	   fParResetROC;
  
  bool     fPhCalOK;
  PHCalibration fPhCal;
  bool	   fParOutOfRange;
  bool     fDaq_loop;

  double fPar0VdCal;
  double fPar1VdCal;
  double fPar0VaCal;
  double fPar1VaCal;
  double fPar0RbIaCal;
  double fPar1RbIaCal;
  double fPar0TbIaCal;
  double fPar1TbIaCal;
  double fPar2TbIaCal;

  bool fCalwVd;
  bool fCalwVa;

  double fRbVbg;

  

  std::vector<std::pair<std::string, uint8_t> > fPg_setup;

  std::vector<TH2D*> fHits;
  std::vector<TProfile2D*> fPhmap;
  std::vector<TH1D*> fPh;
  std::vector<TH1D*> fQ;
  std::vector<TProfile2D*> fQmap;

  ClassDef(PixTestReadback, 1)

};
#endif
