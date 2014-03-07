// -- author: Wolfram Erdmann
#ifndef PIXTESTBBMAP_H
#define PIXTESTBBMAP_H

#include "api.h"
#include "PixTest.hh"

class DLLEXPORT PixTestBBMap: public PixTest {
public:
  PixTestBBMap(PixSetup *, std::string);
  PixTestBBMap();
  virtual ~PixTestBBMap();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void doTest(); 

private:

  int     fParNtrig; 

  ClassDef(PixTestBBMap, 1);

};
#endif
