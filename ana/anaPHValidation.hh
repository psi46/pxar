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


// ----------------------------------------------------------------------
//
// anaPHValidation
// ---------------
//
// (1) validate the PH optimization procedure by analyzing the
//     gain-pedestal calibration results in phCalibration_*.dat files
//
// (2) compare the phscale and phoffset DACs between T = -20C and T = +10C
//     for the same module
//
// History: 2020/03/10 First shot
// ----------------------------------------------------------------------

class anaPHValidation {

 public:
  anaPHValidation(std::string pdfdir, int nrocs = 16);
  virtual ~anaPHValidation();

  // -- mode: 0 (error function), 1 (pol1)
  void makeAll(std::string directory = "/Users/ursl/pxar/pxar/data/phvalidation/T+10/", std::string basename = "M", int mode = 0);
  void makeOneModule(std::string directory, int mode = 0 );

  // -- compare DACs between two different settings
  void compareAllDACs(std::string basename = "M", std::string dacbase = "dacParameters50",
		  std::string dir1 = "../data/phvalidation/T+10/", std::string dir2 = "../data/phvalidation/T-20/");

  void compareDAC(std::string dac = "phscale", double xmin = 0., double xmax = 256.,
		  std::string basename = "M", std::string dacbase = "dacParameters50",
		  std::string dir1 = "../data/phvalidation/T+10/", std::string dir2 = "../data/phvalidation/T-20/");

  // -- test methods
  void fitPixel(std::string directory, int iroc, int icol, int irow);
  void test(double y0 = 42., double y1 = 50.);

  // -- main method for studying the PH optimization
  void fitErr(int roc = -1, int col = -1, int row = -1, bool draw = false);

  // -- main method for studying the gain/pedestal calibration
  void fitPol1(int roc = -1, int col = -1, int row = -1, bool draw = false);

  // -- left-over. Resuscitate for (non-)linearity studies
  void fitTanH(int roc = -1, int col = -1, int row = -1, bool draw = false);


  // -- utilities
  void readAsciiFiles(std::string directory, bool createHists);
  void readRootFile(std::string filename);
  void cleanup();
  int  readDacFromFile(std::string dac, std::string dacfile);
  std::vector<std::string> glob(std::string directory, std::string basename = "phCalibration_");


  void shrinkPad(double b = 0.1, double l = 0.1, double r = 0.1, double t = 0.1);
  void setTitles(TH1 *h, const char *sx, const char *sy,
		 float size = 0.05, float xoff = 1.1, float yoff = 1.1, float lsize = 0.05, int font = 42);
  void setHist(TH1 *h, int color = kBlack, int symbol = 20, double size = 1., double width = 2.);
  void setFilledHist(TH1 *h, int lcol = kBlack, int fcol = kYellow, int fstyle = 1000, int width = 1);


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
