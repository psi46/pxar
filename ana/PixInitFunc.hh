#ifndef PIXINITFUNC_H
#define PIXINITFUNC_H

#include "pxardllexport.h"

#include "TString.h"
#include "TObject.h"
#include "TH1.h"
#include "TF1.h"

#include <iostream>


class DLLEXPORT PixInitFunc: public TObject {

public:

  PixInitFunc(); 
  ~PixInitFunc(); 

  void resetLimits(); 
  void limitPar(int ipar, double lo, double hi); 
  bool doNotFit() {return fDoNotFit;}

  TF1* errScurve(TH1 *h); 
  TF1* weibullCdf(TH1 *h); 
  TF1* gpTanPol(TH1 *h); 
  TF1* gpTanH(TH1 *h);
  
  void initPol1(double &p0, double &p1, TH1 *h);
  void initExpo(double &p0, double &p1, TH1 *h);

  bool fDoNotFit;
  double fLo, fHi;
  bool fLimit[20];
  double fLimitLo[20], fLimitHi[20]; 

  ClassDef(PixInitFunc, 1); // testing PixInitFunc

};

#endif
