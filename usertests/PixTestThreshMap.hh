#ifndef PIXTESTTHRESH_H
#define PIXTESTTHRESH_H

#include "PixTest.hh"

#define ROCNUMROWS 80
#define ROCNUMCOLS 52

class DLLEXPORT PixTestThreshMap: public PixTest {
public:
  PixTestThreshMap(PixSetup *, std::string);
  PixTestThreshMap();
  virtual ~PixTestThreshMap();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void doTest(); 

private:

  std::string fParDac;
  uint8_t fParThresholdLevel;
  uint16_t fParNtrig;
  int 	   fParLoDAC; 
  int 	   fParHiDAC;
  int      fParStepSize;
  bool     fParRisingEdge;
  bool     fParCalS;

  std::map<int, int> id2idx; // map the ROC ID onto the index of the ROC

  ClassDef(PixTestThreshMap, 1)

};
#endif
