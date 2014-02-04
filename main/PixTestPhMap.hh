#ifndef PIXTESTPHMAP_H
#define PIXTESTPHMAP_H

#include "api.h"
#include "PixTest.hh"

class PixTestPhMap: public PixTest {
public:
  PixTestPhMap(PixSetup *, std::string);
  PixTestPhMap();
  virtual ~PixTestPhMap();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void bookHist(std::string); 

  void doTest(); 

private:

  int     fParNtrig; 

  ClassDef(PixTestPhMap, 1);

};
#endif
