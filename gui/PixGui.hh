#ifndef PIXGUI_H
#define PIXGUI_H

#ifdef __CINT__
#undef __GNUC__
typedef char __signed;
typedef char int8_t;
#endif

#include <vector>
#include <map>
#include <iostream>
#include <string>

#include <TGComboBox.h>
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
#include <TApplication.h>
#include <TCanvas.h>
#include <TSystem.h>
#include <TRandom.h>
#include <TH1.h>
#include <TRootEmbeddedCanvas.h>
#include <TVirtualX.h>
#include <TVirtualStreamerInfo.h>  // w/o this dict compilation problems

#include "api.h"

class ConfigParameters;
class PixParTab;
class PixTab;
class PixMonitorFrame;
class PixSetup;
class PixTest;
class PixTestParameters;

class DLLEXPORT PixGui: public TGMainFrame {
public:
  PixGui(const TGWindow *p, UInt_t w, UInt_t h, PixSetup *setup);
  ~PixGui();

  void Cleanup();
  void CloseWindow();

  void handleButtons(Int_t id = -1);
  void createTab(const char*);
  //  void createParTab();
  void selectedTab(int);
  void selectProbes(int);
  void changeRootFile();
  bool isHvOn() {return fHV;}
  void hvOn();
  void hvOff();
  void powerOn();
  void powerOff();
  bool isPowerOff() {return !fPower;}

  PixTest* createTest(std::string);

  TGCompositeFrame	*fhFrame;
  TGTab               	*getTabs() {return fTabs;}
  PixTab                *getPixTab() {return fPixTab;}

  int getWidth() {return fWidth;}
  int getHeight() {return fHeight;}
  pxar::pxarCore* getApi() {return fApi;}
  PixSetup* getPixSetup() {return fPixSetup;}

  void updateSelectedRocs(std::map<int, int> a);

  ULong_t   fRed, fGreen, fYellow, fWhite, fDarkSeaGreen, fDarkOrange, fLavender, fDarkGray, fDarkSalmon;

  std::string getHdiType();

private:

  static const int TESTNUMBER = 300;
  enum CommandIdentifiers {
    B_FILENAME = TESTNUMBER + 21,
    B_DIRECTORY,
    B_EXIT,
    B_WRITEALLFILES,
    B_PINGUI,
    B_POWER,
    B_HV
  };

  TTimer        	*fTimer;
  TGTab               	*fTabs;
  //  TGCompositeFrame     	*fParTab;
  TGTextBuffer          *fRootFileNameBuffer, *fDirNameBuffer, *fReadbackBuffer;
  TGTextButton		*fbtnPower, *fbtnHV;
  TGSlider		*fpowerSlider;
  TGSlider		*fhvSlider;
  TGLabel		*flblPower;
  TGLabel		*flblHV;
  TGHorizontalFrame 	*fH1;
  TGHorizontalFrame	*fH2;

  TGComboBox            *fSignalBoxA[2];
  TGComboBox            *fSignalBoxD[2];
  TGComboBox            *fD1TBM, *fD2TBM;

  std::vector<PixTest *> fTestList;
  bool			 fDebug;
  bool			 fPower, fHV;

  PixSetup               *fPixSetup;
  pxar::pxarCore         *fApi;
  ConfigParameters       *fConfigParameters;
  PixTestParameters      *fTestParameters;
  PixMonitorFrame        *fMonitor;
  PixParTab              *fParTab;

  std::vector<PixTab*>   fPixTabList;
  PixTab                 *fPixTab;

  int                    fWidth, fHeight;
  std::string            fOldDirectory;

  int                    fBorderN, fBorderT, fBorderL;  // normal, tiny, large

  ClassDef(PixGui, 1); //

};

#endif
