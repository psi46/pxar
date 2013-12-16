#ifndef PIXGUI_H
#define PIXGUI_H

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

#include "SysCommand.hh"
#include "ConfigParameters.hh"
#include "TBInterface.hh"
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
  void doSetfConsole();
  void closeWindow();
  PixTest *createTest(TBInterface *, std::string); 

  TGCompositeFrame	*fhFrame;
  TGTab               	*getTabs() {return fTabs;}
  
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
  
  TBInterface            *fTb;
  ConfigParameters       *fConfigParameters;  
  PixTestParameters      *fTestParameters;

  ClassDef(PixGui, 1); //

};

#endif
