#ifndef PIXTESTTHRESH_H
#define PIXTESTTHRESH_H

#include "api.h"
#include "PixTest.hh"

#define ROCNUMROWS 80
#define ROCNUMCOLS 52

using namespace std;
using namespace pxar;

class PixTestThreshMap: public PixTest {
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

  uint16_t fParNtrig;
  int 	   fParLoDAC; 
  int 	   fParHiDAC; 
  bool 	   bBumpBond;

  map<int, int> id2idx; // map the ROC ID onto the index of the ROC

  ClassDef(PixTestThreshMap, 1);

};
#endif
