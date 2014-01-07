#ifndef PIXTESTGAINCALIBRATION_H
#define PIXTESTGAINCALIBRATION_H

#include "api.h"
#include "PixTest.hh"

class PixTestGainCalibration: public PixTest {
public:
  PixTestGainCalibration(PixSetup *a, std::string name);
  PixTestGainCalibration();
  virtual ~PixTestGainCalibration();
  virtual bool setParameter(std::string parName, std::string sval); 
  
  void doTest(); 
  
  
private:

  ClassDef(PixTestGainCalibration, 1)

};
#endif
