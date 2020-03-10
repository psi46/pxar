#ifndef ANAPHVALIDATION_H
#define ANAPHVALIDATION_H

#include <iostream>
#include <map>

#include "TString.h"
#include "TObject.h"

#include "TH1.h"
#include "TH2.h"
#include "TF1.h"
#include "TCanvas.h"


class anaPHValidation {

 public:
  anaPHValidation(std::string dir, int nrocs = 16);
  virtual ~anaPHValidation();

  void makeAll();
  void cleanup();
  void makeOneModule(std::string directory, int mode = 0 );
  void readAsciiFiles(std::string directory, bool createHists);
  void fitPixel(std::string directory, int iroc, int icol, int irow);
  void readRootFile(std::string filename);
  void test(double y0 = 42., double y1 = 50.);
  void fitTanH(int roc = -1, int col = -1, int row = -1, bool draw = false);
  void fitErr(int roc = -1, int col = -1, int row = -1, bool draw = false);

  // -- utilities
  std::vector<std::string> glob(std::string directory, std::string basename = "phCalibration_");

private:
  TCanvas                     *c0;
  int                          fNrocs;
  std::string                  fDirectory, fModule;
  std::map<std::string, TH1D*> fHists;

  std::map<std::string, TH1D*> fhsummary;
  std::map<std::string, TH2D*> fh2summary;
  std::vector<TH1D*>           fhproblems;


  ClassDef(anaPHValidation, 1); // testing anaGainPedestal

};

#endif
