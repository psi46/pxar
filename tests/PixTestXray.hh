#ifndef PIXTESTXRAY_H
#define PIXTESTXRAY_H

#include "PixTest.hh"


class DLLEXPORT PixTestXray: public PixTest {
public:
  PixTestXray(PixSetup *, std::string);
  PixTestXray();
  virtual ~PixTestXray();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void doTest();

  bool setTrgFrequency(uint8_t TrgTkDel);
  void finalCleanup();
  void pgToDefault(std::vector<std::pair<std::string, uint8_t> > pg_setup);

  void readData();
  void analyzeData();
  double meanHit(TH2D*); 
  double noiseLevel(TH2D*); 
  int   countHitsAndMaskPixels(TH2D*, double noiseLevel, int iroc); 

private:

  int     fParNtrig; 
  int     fParTriggerFrequency;
  int     fParSeconds; 
  int     fParVthrCompMin, fParVthrCompMax; 
  bool    fParFillTree;
  int     fVthrComp;
  std::vector<std::pair<std::string, uint8_t> > fPg_setup;
  bool    fDaq_loop;

  std::vector<TH1D*> fHits, fMpix;
  std::vector<TH2D*> fHitMap;

  ClassDef(PixTestXray, 1)

};
#endif
