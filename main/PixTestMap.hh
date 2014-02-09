#ifndef PIXTESTMAP_H
#define PIXTESTMAP_H

#include "api.h"
#include "PixTest.hh"

class PixTestMap: public PixTest {
public:
  PixTestMap(PixSetup *, std::string);
  PixTestMap();
  virtual ~PixTestMap();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void doTest(); 

private:

  int     fParNtrig; 
  int     fParVcal; 

  ClassDef(PixTestMap, 1);

};
#endif
