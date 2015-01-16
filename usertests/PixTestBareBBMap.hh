// -- author: Wolfram Erdmann
#ifndef PIXTESTBAREBBMAP_H
#define PIXTESTBAREBBMAP_H

#include "PixTest.hh"

class DLLEXPORT PixTestBareBBMap: public PixTest {
public:
  PixTestBareBBMap(PixSetup *, std::string);
  PixTestBareBBMap();
  virtual ~PixTestBareBBMap();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();

  void doTest(); 

private:
  int          fParNtrig; 
  int          fParVcalS; 
  int          fParPlWidth; 
  
  ClassDef(PixTestBareBBMap, 1); 

};
#endif
