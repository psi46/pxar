// -- author: Thomas Weiler
// -- based on BBTest from Rudolf Schimassek
// -- based on MapPh from Daniel Pitzl
#ifndef PIXTESTBB4MAP_H
#define PIXTESTBB4MAP_H

#include "PixTest.hh"
#include <vector>

class DLLEXPORT PixTestBB4Map: public PixTest {
public:
  PixTestBB4Map(PixSetup *, std::string);
  PixTestBB4Map();
  virtual ~PixTestBB4Map();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 
  void clear(std::vector<TH2D*>* maps);
  void mergevectors(std::vector<TH2D*>* one, std::vector<TH2D*>* two);

  void doTest(); 

private:

  //get a pulse height map
  void mapeff(std::vector<TH2D*>* maps);
  //recursive function for threshold scan (despite DataMissingEvents exceptions)
  //void VthrTest(int startidx, int endidx, int vthrcomp, std::vector<TH2D*>* maps, int* threshold);

  int     fParSaveCalDelMaps;
  int     fParNtrig; 
  int     fParCals;
  int     fParCalDelLo;
  int     fParCalDelHi;
  int     fParCalDelStep;
  int     fParVthrCompLo;
  int     fParVthrCompHi;
  int     fParVthrCompStep;
  int     fParVcal;
  int     fParNoisyPixels;
  float   fParCut;
  ClassDef(PixTestBB4Map, 1)

};
#endif
