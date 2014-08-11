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

  /**
   * Call sweep to aquire IV curve.
   * sweep_auto is preferred if HV supply can do a sweep internally
   * otherwise, sweep_manual is called
   */
  void sweepAuto(std::vector<double>        &voltageMeasurements,
                 std::vector<double>        &currentMeasurements,
                 std::vector<unsigned long> &timeStamps, 
                 TH1D                  *h1);
  void sweepManual(std::vector<double>        &voltageMeasurements,
                   std::vector<double>        &currentMeasurements,
                   std::vector<unsigned long> &timeStamps, 
                   TH1D                  *h1);
  void writeOutput(std::vector<double>        &voltageMeasurements,
                   std::vector<double>        &currentMeasurements,
                   std::vector<TTimeStamp>    &timeStamps);
  
  double fParVoltageMin;
  double fParVoltageMax;
  double fParVoltageStep;
  double fParDelay;
  bool fStop;
  std::string fParPort; 

  ClassDef(PixTestIV, 1)

};
#endif
