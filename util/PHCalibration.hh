#ifndef PHCALIBRATION_H
#define PHCALIBRATION_H

#include "pxardllexport.h"

#include <string>

#include "ConfigParameters.hh"

class DLLEXPORT PHCalibration  {

public: 
  /// 0 = error function
  /// 1 = tanH
  PHCalibration(int mode = 0); 
  void setMode(int mode = 0) {fMode = mode;}
  int getMode() {return fMode; }

  ~PHCalibration(); 

  double vcal(int iroc, int icol, int irow, double ph);
  double ph(int iroc, int icol, int irow, double vcal);

  double vcalErr(int iroc, int icol, int irow, double ph);
  double phErr(int iroc, int icol, int irow, double vcal);

  double vcalTanH(int iroc, int icol, int irow, double ph);
  double phTanH(int iroc, int icol, int irow, double vcal);

  void setPHParameters(std::vector<std::vector<gainPedestalParameters> > ); 
  bool initialized() {return (fParameters.size() > 0);}
  std::string getParameters(int iroc, int icol, int irow); 

 private: 
  int fMode; 
  std::vector<std::vector<gainPedestalParameters> > fParameters;
  
};

#endif
