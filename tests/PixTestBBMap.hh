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
  virtual void init(); 
  virtual void resetDirectory();
  // virtual void writeOutput();
  virtual void setToolTips();

  void doTest(); 
  int  fitPeaks(TH1D *h, TSpectrum &s, int npeaks);

private:
  int          fParNtrig; 
  int          fParVcalS; 
  int          fDumpAll, fDumpProblematic;
  ClassDef(PixTestBBMap, 1); 

};
#endif
