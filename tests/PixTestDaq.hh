#ifndef PIXTESTDAQ_H
#define PIXTESTDAQ_H

#include "PixTest.hh"
#include <TH1.h>
#include <TH2.h>

#include <TTree.h>

class DLLEXPORT PixTestDaq: public PixTest {
public:
  PixTestDaq(PixSetup *, std::string);
  PixTestDaq();
  virtual ~PixTestDaq();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void doTest(); 
  void runCommand(std::string command);

  bool setTrgFrequency(uint8_t TrgTkDel);
  void pgToDefault(std::vector<std::pair<std::string, uint8_t> > pg_setup);
  void FinalCleaning();
  void ProcessData(uint16_t numevents = 1000);

private:
  void stop();

  uint32_t fParNtrig; 
  uint16_t fParStretch; 
  bool     fParFillTree;
  uint16_t fParSeconds;
  uint16_t  fParTriggerFrequency;
  uint16_t fParIter;
  bool	   fParDelayTBM;

  std::vector<std::pair<std::string, uint8_t> > fPg_setup;

  std::vector<TH2D*> fHits;
  std::vector<TH2D*> fPhmap;
  std::vector<TH1D*> fPh;
  bool fDaq_loop;

  ClassDef(PixTestDaq, 1)

};
#endif
