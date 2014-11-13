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
  void BlacklistPixels(std::vector<std::pair<uint8_t, std::pair<int, int> > > &badPixels, int aliveTrig);  
  void SetMinThr();
  pxar::pixel* RandomPixel(std::vector<std::pair<uint8_t, std::pair<int, int> > > &badPixels, uint8_t iroc);
  void GetMaxPhPixel(std::map<int, pxar::pixel> &maxpixel, std::vector<std::pair<uint8_t, std::pair<int, int> > > &badPixels);
  void GetMinPhPixel(std::map<int, pxar::pixel> &minpixel, std::map<int, int> &minVcal, std::vector<std::pair<uint8_t, std::pair<int, int> > > &badPixels);
  void MaxPhVsDacDac(std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_max, std::map<int, pxar::pixel> maxpixels);
  void MinPhVsDacDac(std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_min, std::map<int, pxar::pixel> minpixels, std::map<int, int> &minVcal);
  std::map<uint8_t, int> InsideRangePH(std::map<uint8_t,int> &po_opt,  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_max,   std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_min);
  std::map<uint8_t, int> CentrePhRange(std::map<uint8_t, int> &po_opt, std::map<uint8_t, int> &ps_opt,  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_max,   std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_min);
  std::map<uint8_t, int> StretchPH(std::map<uint8_t, int> &po_opt, std::map<uint8_t, int> &ps_opt,  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_max,   std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_min);
  void DrawPhMaps(std::map<int, int> &minVcal, std::vector<std::pair<uint8_t, std::pair<int, int> > > &badPixels);
  void DrawPhCurves(std::map<int, pxar::pixel > &maxpixels, std::map<int, pxar::pixel > &minpixels, std::map<uint8_t, int> &po_opt, std::map<uint8_t, int> &ps_opt);
  void doTest(); 

private:

  int         fParNtrig; 
  std::string fParDAC; 
  int         fParDacVal;
  bool        fFlagSinglePix;
  int         fSafetyMarginUp;
  int         fSafetyMarginLow;
  int         fMinThr;

  ClassDef(PixTestPhOptimization, 1)

};
#endif
