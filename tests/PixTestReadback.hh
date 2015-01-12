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
  std::vector<uint8_t> daqReadback(std::string dac, uint8_t vana, int8_t parReadback);
  std::vector<uint8_t> daqReadback(std::string dac, uint8_t vana, unsigned int roc, int8_t parReadback);
  std::vector<uint8_t> daqReadback(std::string dac, double vana, int8_t parReadback);
  std::vector<uint8_t> daqReadbackIa();
  void CalibrateIa();
  void CalibrateVana();
  void CalibrateVd();
  void CalibrateVa();
  std::vector<double> getCalibratedVbg();
  void readbackVbg();
  void doTest();
  std::vector<double> getCalibratedIa();
  double getCalibratedIa(unsigned int roc);
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

  std::vector<double> fPar0VdCal;  
  std::vector<double> fPar1VdCal;  
  std::vector<double> fPar0VaCal;  
  std::vector<double> fPar1VaCal;  
  std::vector<double> fPar0RbIaCal;
  std::vector<double> fPar1RbIaCal;
  std::vector<double> fPar0TbIaCal;
  std::vector<double> fPar1TbIaCal;
  std::vector<double> fPar2TbIaCal;

  std::vector<std::vector<std::pair<std::string, double> > > fRbCal;

  bool fCalwVd;
  bool fCalwVa;

  std::vector<double> fRbVbg;

  

  std::vector<std::pair<std::string, uint8_t> > fPg_setup;

  std::vector<TH2D*> fHits;
  std::vector<TProfile2D*> fPhmap;
  std::vector<TH1D*> fPh;
  std::vector<TH1D*> fQ;
  std::vector<TProfile2D*> fQmap;

  ClassDef(PixTestReadback, 1)

};
#endif
