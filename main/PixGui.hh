#ifndef PIXGUI_H
#define PIXGUI_H

#ifdef __CINT__ 
#undef __GNUC__ 
typedef char __signed; 
typedef char int8_t; 
#endif 

#include <vector>
#include <iostream>
#include <string>

#include <TGFrame.h>
#include <TGTab.h>
#include <TGButton.h>
#include <TGLabel.h>
#include <TGSlider.h>
#include <TGSplitter.h>
#include <TGTextView.h>
#include <TGTextEntry.h>
#include <TGTextEdit.h>
#include <TGTextBuffer.h>
#include <TGComboBox.h>
#include <TApplication.h>
#include <TCanvas.h>
#include <TSystem.h>
#include <TRandom.h>
#include <TH1.h>
#include <TRootEmbeddedCanvas.h>
#include <TVirtualStreamerInfo.h>  // w/o this dict compilation problems

#include "api.h"

#include "SysCommand.hh"
#include "ConfigParameters.hh"
#include "PixTest.hh"
#include "PixTestParameters.hh"
#include "PixSetup.hh"

//fixme #include "monitorSource.hh"

class PixGui: public TGMainFrame {
public:
  PixGui(const TGWindow *p, UInt_t w, UInt_t h, PixSetup *setup);
  ~PixGui();

  void handleButtons(Int_t id = -1);
  void createTab(char*);
  void createParTab();

  void doSetfConsole();
  void closeWindow();
  PixTest* createTest(std::string); 

  TGCompositeFrame	*fhFrame;
  TGTab               	*getTabs() {return fTabs;}

  int getWidth() {return fWidth;}
  int getHeight() {return fHeight;}
  
private: 
  TTimer	*fTimer;
  TGComboBox 	*fcmbTests;

  static const int TESTNUMBER = 300;
  enum CommandIdentifiers {
    B_FILENAME = TESTNUMBER + 21,
    B_EXIT,
    B_POWER,
    B_HV
  };

  TGTab               	*fTabs;
  TGCompositeFrame     	*fParTab;
  //fixme  monitorSource		*fMonitoring;
  TGTextView		*fLogger;
  TGTextEntry		*fConsole;
  TGTextBuffer		*fConsoleBuffer;
  TGTextBuffer          *fRootFileNameBuffer;
  TGTextButton		*fbtnPower;
  TGTextButton		*fbtnHV;
  TGSlider		*fpowerSlider;
  TGSlider		*fhvSlider;
  TGLabel		*flblPower;
  TGLabel		*flblHV;
  TGHorizontalFrame 	*fH1;
  TGHorizontalFrame	*fH2;
  std::vector<TH1*>      fHistList; 
  std::vector<PixTest *> fTestList; 
  bool			 fDebug;
  bool			 fPower, fHV;


  SysCommand             *fSysCommand;
  
  PixSetup               *fPixSetup; 
  pxar::api              *fApi;
  ConfigParameters       *fConfigParameters;  
  PixTestParameters      *fTestParameters;

  int                    fWidth, fHeight; 

  ClassDef(PixGui, 1); //

};

#endif
