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
#include "PixMonitor.hh"

//fixme #include "monitorSource.hh"

class PixGui: public TGMainFrame {
public:
  PixGui(const TGWindow *p, UInt_t w, UInt_t h, PixSetup *setup);
  ~PixGui();

  void Cleanup(); 
  void CloseWindow();

  void handleButtons(Int_t id = -1);
  void createTab(const char*);
  void createParTab();
  void selectedTab(int); 

  PixTest* createTest(std::string); 

  TGCompositeFrame	*fhFrame;
  TGTab               	*getTabs() {return fTabs;}

  int getWidth() {return fWidth;}
  int getHeight() {return fHeight;}
  pxar::api* getApi() {return fApi;}
  
private: 

  static const int TESTNUMBER = 300;
  enum CommandIdentifiers {
    B_FILENAME = TESTNUMBER + 21,
    B_EXIT,
    B_POWER,
    B_HV
  };

  TTimer        	*fTimer;
  TGComboBox 	        *fcmbTests;
  TGTab               	*fTabs;
  TGCompositeFrame     	*fParTab;
  TGTextBuffer          *fRootFileNameBuffer;
  TGTextButton		*fbtnPower;
  TGTextButton		*fbtnHV;
  TGSlider		*fpowerSlider;
  TGSlider		*fhvSlider;
  TGLabel		*flblPower;
  TGLabel		*flblHV;
  TGHorizontalFrame 	*fH1;
  TGHorizontalFrame	*fH2;
  std::vector<PixTest *> fTestList; 
  bool			 fDebug;
  bool			 fPower, fHV;

  PixSetup               *fPixSetup; 
  pxar::api              *fApi;
  ConfigParameters       *fConfigParameters;  
  PixTestParameters      *fTestParameters;
  PixMonitor             *fMonitor; 

  int                    fWidth, fHeight; 

  ClassDef(PixGui, 1); //

};

#endif
