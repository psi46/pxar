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
  void setToolTips();
  void bookHist(std::string); 
  void dummyAnalysis(); 
  void output4moreweb();

  void doTest(); 

private:

  uint16_t fParNtrig; 
  int      fParVcal; 

  ClassDef(PixTestAlive, 1);

};
#endif
