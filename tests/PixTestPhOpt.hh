#ifndef PixTestPhOpt_H
#define PixTestPhOpt_H

#include "PixTest.hh"

class TH2D;

class DLLEXPORT PixTestPhOpt: public PixTest {
public:
  PixTestPhOpt(PixSetup *, std::string);
  PixTestPhOpt();
  virtual ~PixTestPhOpt();

  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void bookHist(std::string); 

  void doTest(); 
  void scan(std::string name);

private:

  std::map<std::string, TH2D*> fMaps;

  int     fParNtrig, fVcalLow, fVcalHigh, fPhMin, fPhMax;
  bool    fFlagSinglePix;

  ClassDef(PixTestPhOpt, 1)

};
#endif
