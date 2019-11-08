#ifndef PixTestPhOptimizationOld_H
#define PixTestPhOptimizationOld_H

#include "PixTest.hh"


class DLLEXPORT PixTestPhOptimizationOld: public PixTest {
public:
  PixTestPhOptimizationOld(PixSetup *, std::string);
  PixTestPhOptimizationOld();
  virtual ~PixTestPhOptimizationOld();
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
  void DrawPhMaps(std::map<int, int> &minVcal, std::vector<std::pair<uint8_t, std::pair<int, int> > > &badPixels);
  void DrawPhCurves(std::map<int, pxar::pixel > &maxpixels, std::map<int, pxar::pixel > &minpixels, std::map<uint8_t, int> &po_opt, std::map<uint8_t, int> &ps_opt);

  void optimiseOnMapsNew(std::map<uint8_t, int> &po_opt, std::map<uint8_t, int> &ps_opt,  std::vector<TH2D* > &th2_max, std::vector<TH2D* > &th2_min, std::vector<TH2D* > &th2_sol);

  void getTH2fromMaps(std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_max,   std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_min, std::vector<TH2D* > &th2_max, std::vector<TH2D* > &th2_min);


  void doTest();

private:

  int         fParNtrig;
  int         fSafetyMarginLow;
  int         fMinThr;
  double      fQuantMax;
  int         fVcalMax;

  ClassDef(PixTestPhOptimizationOld, 1)

};
#endif
