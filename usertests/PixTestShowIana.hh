// -- author: Wolfram Erdmann
#ifndef PIXTESTSHOWIANA_H
#define PIXTESTSHOWIANA_H

#include "constants.h" // FLAG_USE_CALS
#include "PixTest.hh"

class DLLEXPORT PixTestShowIana: public PixTest {
public:
  PixTestShowIana(PixSetup *, std::string);
  PixTestShowIana();
  virtual ~PixTestShowIana();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 
  uint8_t readRocADC(uint8_t);

  void doTest(); 

private:


  ClassDef(PixTestShowIana, 1)

};
#endif
