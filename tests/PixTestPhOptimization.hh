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
  void BlacklistPixels(std::vector<std::pair<int, int> > &badPixels, int aliveTrig);  
  pxar::pixel* RandomPixel(std::vector<std::pair<int, int> > &badPixels);
  void doTest(); 

private:

  int     fParNtrig; 
  std::string fParDAC; 
  int     fParDacVal;
  bool fFlagSinglePix;

  ClassDef(PixTestPhOptimization, 1)

};
#endif
