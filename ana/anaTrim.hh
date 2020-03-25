#ifndef ANATRIM_H
#define ANATRIM_H

#include <iostream>
#include <map>

#include "TString.h"
#include "TObject.h"

#include "TH1.h"
#include "TF1.h"
#include "TCanvas.h"

class  anaTrim {

 public:
  anaTrim(std::string pdfdir = ".");
  virtual ~anaTrim();

  void makeAll(std::string directory, int mode = 0 );
  void showTrimBits(std::string rootfile);

private:
  TCanvas                     *c0;
  std::string                  fDirectory;

  ClassDef(anaTrim, 1); // testing anaTrim

};

#endif
