#ifndef ANASCURVE_H
#define ANASCURVE_H

#include <iostream>
#include <map>

#include "TString.h"
#include "TObject.h"

#include "TH1.h"
#include "TF1.h"
#include "TCanvas.h"

class  anaScurve {

 public:
  anaScurve(std::string dir, int nrocs = 16);
  virtual ~anaScurve();

  void makeAll(std::string directory, int mode = 0 );
  void readAsciiFiles(std::string directory);
  void readRootFile(std::string filename);
  void fitErr(int roc = -1, int col = -1, int row = -1, bool draw = false);
  void test(double y0 = 42., double y1 = 50.);

  // -- utilities
  std::vector<std::string> glob(std::string directory, std::string basename = "SCurveData_C");

private:
  TCanvas                     *c0;
  int                          fNrocs;
  std::string                  fDirectory;
  std::vector<std::pair<std::string, TH1D*> > fHists;

  ClassDef(anaScurve, 1); // testing anaScurve

};

#endif
