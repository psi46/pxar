#ifndef PIXTESTBB3MAP_H
#define PIXTESTBB3MAP_H

#include "PixTest.hh"

#include <TH1.h>
#include <TSpectrum.h>

class DLLEXPORT PixTestBB3Map: public PixTest {
public:
  PixTestBB3Map(PixSetup *, std::string);
  PixTestBB3Map();
  virtual ~PixTestBB3Map();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();

  void doTest(); 
  TF1* fitPeaks(TH1D *h, TSpectrum &s, int npeaks);

private:
  int          fParNtrig; 
  int          fParVcalS; 
  int          fDumpAll, fDumpProblematic;
  ClassDef(PixTestBB3Map, 1); 

};
#endif
