#ifndef PIXTESTALIVE_H
#define PIXTESTALIVE_H

#include "api.h"
#include "PixTest.hh"

class PixTestAlive: public PixTest {
public:
  PixTestAlive(PixSetup *, std::string);
  PixTestAlive();
  virtual ~PixTestAlive();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void bookHist(std::string); 

  void doTest(); 

private:

  int     fParNtrig; 
  int     fParVcal; 

  std::vector<TH2D*> fMaps; 
  ClassDef(PixTestAlive, 1);

};
#endif
