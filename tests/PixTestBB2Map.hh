#ifndef PIXTESTBB2MAP_H
#define PIXTESTBB2MAP_H

#include "PixTest.hh"

class DLLEXPORT PixTestBB2Map: public PixTest {
public:
  PixTestBB2Map(PixSetup *, std::string);
  PixTestBB2Map();
  virtual ~PixTestBB2Map();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();

  void doTest(); 

private:
  int          fParNtrig; 
  int          fParVcalS; 
  int          fParPlWidth; 
  
  ClassDef(PixTestBB2Map, 1); 

};
#endif
