#ifndef PIXTESTGAINCALIBRATION_H
#define PIXTESTGAINCALIBRATION_H

#include "PixTest.hh"

class PixTestGainCalibration: public PixTest {
public:
  PixTestGainCalibration(PixSetup &a, std::string name);
  PixTestGainCalibration(TBInterface *, std::string name, PixTestParameters *);
  PixTestGainCalibration();
  virtual ~PixTestGainCalibration();
  virtual bool setParameter(std::string parName, std::string sval); 
  
  void doTest(); 
  
  
private:

  ClassDef(PixTestGainCalibration, 1)

};
#endif
