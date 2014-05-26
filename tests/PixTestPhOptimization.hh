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
  void GetMaxPhPixel(pxar::pixel &maxpixel, std::vector<std::pair<int, int> > &badPixels);
  void GetMinPixel(pxar::pixel &minpixel, std::vector<pxar::pixel> &thrmap, std::vector<std::pair<int, int> > &badPixels);
  int InsideRangePH(int po_opt,  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_max,   std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_min);
  void doTest(); 

private:

  int     fParNtrig; 
  std::string fParDAC; 
  int     fParDacVal;
  bool fFlagSinglePix;

  ClassDef(PixTestPhOptimization, 1)

};
#endif
