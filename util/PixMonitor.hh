#ifndef PIXMONITOR_H
#define PIXMONITOR_H

#include "pxardllexport.h"

#ifdef __CINT__
#undef __GNUC__
typedef char __signed;
typedef char int8_t; 
#endif

#include <string>
#include <map>

#include <TH1.h>
#include <TQObject.h> 

#include "api.h"

class DLLEXPORT PixMonitor: public TQObject {
public:
  PixMonitor(pxar::pxarCore *, std::string); 
  ~PixMonitor();
  void init(); 

  void update(); 
  double getIana() {return fIana;}
  double getIdig() {return fIdig;}
  double getTemp() {return fTemp;}

  void dumpSummaries();
  void drawHist(std::string hname); 

private: 
  TH1D* extendHist(TH1D *h, int nbins);
  UInt_t getHistMinSec(TH1D *h);

  pxar::pxarCore  *fApi;
  std::string      fHdiType;
  double           fIana, fIdig, fTemp;

  std::vector<std::pair<UInt_t, std::pair<double, double> > > fMeasurements;
  std::vector<std::pair<UInt_t, double> > fRtdMeasurements;

  ClassDef(PixMonitor, 1); // testing PixMonitor

};

#endif

