#ifndef PHCALIBRATION_H
#define PHCALIBRATION_H

#include "pxardllexport.h"

#include <string>

#include "ConfigParameters.hh"

class DLLEXPORT PHCalibration  {

public: 
  PHCalibration(); 
  ~PHCalibration(); 
  double vcal(int iroc, int icol, int irow, double ph);
  double ph(int iroc, int icol, int irow, double vcal);

  void setPHParameters(std::vector<std::vector<gainPedestalParameters> > ); 
  void setMode(std::string mode = "tanh") {fMode = mode;}
  std::string getMode() {return fMode; }

 private: 
  std::string fMode; 
  std::vector<std::vector<gainPedestalParameters> > fParameters;
  
};

#endif
