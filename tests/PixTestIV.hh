#ifndef PIXTESTIV_H
#define PIXTESTIV_H

#include <vector>

#include "TTimeStamp.h"

#include "PixTest.hh"

class DLLEXPORT PixTestIV: public PixTest {
public:
  PixTestIV(PixSetup *, std::string);
  PixTestIV();
  virtual ~PixTestIV();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void bookHist(std::string); 
  void doTest();
  void stop(); 

private:
  void writeOutput(std::vector<double>        &voltageMeasurements,
                   std::vector<double>        &currentMeasurements,
                   std::vector<TTimeStamp>    &timeStamps);
  
  double fParVoltageStart;      //volts
  double fParVoltageStop;       //volts
  double fParVoltageStep;       //volts
  double fParDelay;             //seconds
  double fParCompliance;        //uA
  bool fStop;
  std::string fParPort; 

  ClassDef(PixTestIV, 1)

};
#endif
