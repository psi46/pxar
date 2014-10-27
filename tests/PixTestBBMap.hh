#ifndef PIXTESTBBMAP_H
#define PIXTESTBBMAP_H

#include "PixTest.hh"

#include <TH1.h>
#include <TSpectrum.h>

class DLLEXPORT PixTestBBMap: public PixTest {
public:
  PixTestBBMap(PixSetup *, std::string);
  PixTestBBMap();
  virtual ~PixTestBBMap();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();

  void doTest(); 
  void output4moreweb();
  int  fitPeaks(TH1D *h, TSpectrum &s, int npeaks);

private:
  int          fParNtrig; 
  int          fParVcalS; 
  
  ClassDef(PixTestBBMap, 1); 

};
#endif
