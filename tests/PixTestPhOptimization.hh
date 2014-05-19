#ifndef PixTestPhOptimization_H
#define PixTestPhOptimization_H

#include "PixTest.hh"


class DLLEXPORT PixTestPhOptimization: public PixTest {
public:
  PixTestPhOptimization(PixSetup *, std::string);
  PixTestPhOptimization();
  virtual ~PixTestPhOptimization();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void bookHist(std::string); 
  
  void doTest(); 

private:

  int     fParNtrig; 
  std::string fParDAC; 
  int     fParDacVal;

  ClassDef(PixTestPhOptimization, 1)

};
#endif
