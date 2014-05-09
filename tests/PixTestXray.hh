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

private:

  int     fParNtrig, fParIter; 
  int     fParStretch; 
  int     fParVthrCompMin, fParVthrCompMax; 
  bool    fParFillTree;

  ClassDef(PixTestXray, 1)

};
#endif
