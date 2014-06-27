#ifndef PIXTESTHIGHRATE_H
#define PIXTESTHIGHRATE_H

#include "PixTest.hh"
#include "PHCalibration.hh"

#include <TProfile2D.h>


class DLLEXPORT PixTestHighRate: public PixTest {
public:
  PixTestHighRate(PixSetup *, std::string);
  PixTestHighRate();
  virtual ~PixTestHighRate();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void runCommand(std::string command); 
  void doTest();
  void doXPixelAlive();
  void doRunDaq(); 
  void maskHotPixels(); 
 
  bool setTrgFrequency(uint8_t TrgTkDel);
  void finalCleanup();
  void pgToDefault(std::vector<std::pair<std::string, uint8_t> > pg_setup);

  void readData();
  void doHitMap(int nseconds = 1);

  double meanHit(TH2D*); 
  double noiseLevel(TH2D*); 
  int    countHitsAndMaskPixels(TH2D*, double noiseLevel, int iroc); 

private:

  int      fParTriggerFrequency;
  int      fParRunSeconds; 
  bool     fParFillTree;
  bool	   fParDelayTBM;
  uint16_t fParNtrig; 
  int      fParVcal; 

  bool          fPhCalOK;
  PHCalibration fPhCal;

  bool    fDaq_loop;
  
  std::vector<std::pair<std::string, uint8_t> > fPg_setup;

  std::vector<TH2D*> fHitMap;

  ClassDef(PixTestHighRate, 1)

};
#endif
