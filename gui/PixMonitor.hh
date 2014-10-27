#ifndef PIXMONITOR_H
#define PIXMONITOR_H

#include "pxardllexport.h"

#include <iostream>
#include <sstream>

#include <time.h>
#include <stdio.h>
#include <TGFrame.h>
#include <TGTextView.h>
#include <TGButton.h>
#include <TGLabel.h>
#include <TGNumberEntry.h>
#include <TSystem.h>

class PixGui; 

class DLLEXPORT PixMonitor: public TQObject {
public:
  PixMonitor(TGGroupFrame *f, PixGui *p);
  virtual ~PixMonitor();
  virtual void handleButtons(Int_t id = -1);
  virtual void Update();
  virtual std::string stringify(int x);

private:
  PixGui                 *fGui; 
  TGLabel                *fAna;
  TGLabel                *fDigi;
  TGVerticalFrame        *fMonitorFrame;
  TGTextEntry            *fNmrAna;
  TGTextEntry            *fNmrDigi;
  TGTextBuffer           *fAnaFileBuffer;
  TGTextBuffer           *fDigiFileBuffer;
  TGTextButton           *fAnaButton;
  TGTextButton           *fDigiButton;
  TGHorizontalFrame        *fHFrame1;
  TGHorizontalFrame        *fHFrame2;

  TGLabel                *fTemperatureDegree ;
  TGHorizontalFrame      *fHFrame_TDegree ; 
  TGTextEntry            *fNmrTDegree ; 
  TGTextBuffer           *fTDegreeFileBuffer ;



  time_t                fActTime;
  struct tm                *fTimeinfo;

  static const int TESTNUMBER = 100;
  enum CommandIdentifiers {
    B_DRAWANA = TESTNUMBER + 21,
    B_DRAWDIGI
  };

  ClassDef(PixMonitor, 1)

};
#endif
