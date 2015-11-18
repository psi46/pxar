#ifndef PixTestDacDacScanMultipixMULTIPIX_H
#define PixTestDacDacScanMultipixMULTIPIX_H

#include "PixTest.hh"

class DLLEXPORT PixTestDacDacScanMultipix: public PixTest {
public:
  PixTestDacDacScanMultipix(PixSetup *, std::string);
  PixTestDacDacScanMultipix();
  virtual ~PixTestDacDacScanMultipix();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void doTest(); 
  TH1* moduleMap(std::string histname);

private:

  int     fParNtrig; 
  int     fParPHmap; 
  int     fParAllPixels; 
  std::string fParDAC1, fParDAC2; 
  int     fParLoDAC1, fParHiDAC1;
  int     fParLoDAC2, fParHiDAC2;



  ClassDef(PixTestDacDacScanMultipix, 1)

};
#endif
