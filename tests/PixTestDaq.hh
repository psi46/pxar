#ifndef PIXTESTDAQ_H
#define PIXTESTDAQ_H

#include "PixTest.hh"
#include "PHCalibration.hh"

#include <TH1.h>
#include <TH2.h>
#include <TProfile2D.h>

#include <TTree.h>

class DLLEXPORT PixTestDaq: public PixTest {
public:
  PixTestDaq(PixSetup *, std::string);
  PixTestDaq();
  virtual ~PixTestDaq();
  void init(); 
  void setToolTips();
  void bookHist(std::string);
  void doRunMaskHotPixels();
  void doStop();
  void runCommand(std::string command);
  virtual bool setParameter(std::string parName, std::string sval);
  void ProcessData(uint16_t numevents = 1000);
  void doDaqRun();

  void doTest();

private:

  bool     fParDelayTBM;
  bool     fParFillTree;
  uint16_t fParStretch;
  uint16_t fParTriggerFrequency;
  uint32_t fParNtrig; 
  uint16_t fParIter;
  bool     fRunDaqTrigger;
  uint16_t fParSeconds;
  
  bool     fPhCalOK;
  PHCalibration fPhCal;
  bool	   fParOutOfRange;
  bool     fDaq_loop;

  
  std::vector<std::pair<std::string, uint8_t> > fPg_setup;

  std::vector<TH2D*> fHitMap;
  std::vector<TProfile2D*> fPhmap;
  std::vector<TH1D*> fPh;
  std::vector<TH1D*> fQ;
  std::vector<TProfile2D*> fQmap;

  std::vector<std::vector<std::pair<int, int> > > fHotPixels;

  ClassDef(PixTestDaq, 1)

};
#endif
