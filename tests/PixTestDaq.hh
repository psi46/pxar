#ifndef PIXTESTDAQ_H
#define PIXTESTDAQ_H

#include "api.h"
#include "PixTest.hh"

using namespace std;
using namespace pxar;

class PixTestDaq: public PixTest {
public:
  PixTestDaq(PixSetup *, std::string);
  PixTestDaq();
  virtual ~PixTestDaq();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void doTest(); 

private:

  int     fParNtrig; 

  map<int, int> id2idx; // map the ROC ID onto the index of the ROC

  ClassDef(PixTestDaq, 1);

};
#endif
