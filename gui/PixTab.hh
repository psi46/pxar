#ifndef PIXTAB_H
#define PIXTAB_H

#include <string>

#include <TQObject.h> 
#include <TGFrame.h>
#include <TGTab.h>
#include <TRootEmbeddedCanvas.h>
#include <TGSlider.h>
#include <TSlider.h>
#include <TGNumberEntry.h>
#include <TGSplitter.h>
#include <TGTextBuffer.h>
#include <TGTextEntry.h>
#include <TGTextView.h>
#include <TGStatusBar.h>

#include "PixGui.hh"
#include "PixTest.hh"

class PixTab: public TQObject {
public:
  PixTab(PixGui *p, PixTest *test, std::string tabname);
  PixTab();
  void init(PixGui *p, PixTest *test, std::string tabname); 
  virtual ~PixTab();
  std::string getName() {return fTabName;}
  void statusBarUpdate(Int_t event, Int_t px, Int_t py, TObject *selected);

  virtual void handleButtons(Int_t id = -1); 
  virtual void buttonClicked(); 
  virtual void boxChecked(); 
  virtual void setParameter(); 

  void update();
  void updateToolTips();
  void clearCanvas();
  void nextHistogram();
  void previousHistogram();
  void clearHistList();
  
protected: 

  // -- frames and widgets
  TGCompositeFrame     	*fTabFrame;
  TGCompositeFrame     	*fhFrame;
  TGVerticalFrame 	*fV1;
  TGVerticalFrame 	*fV2;

  std::map<std::string, void*>  fParTextEntries;
  std::vector<std::string>      fParIds;

  TRootEmbeddedCanvas  	*fEc1;
  TGStatusBar           *fStatusBar;

  TGTextButton          *fbDoTest, *fbModMap, *fbBrowser; 
  
  static const int TESTNUMBER = 0;
  enum CommandIdentifiers {
    B_DOTEST = TESTNUMBER + 21,
    B_DOSTOP,
    B_CLOSETAB,
    B_MODMAP,
    B_BROWSER,
    B_PRINT
  };

  std::string           fTabName; 
  int			fCount;

  PixGui               *fGui; 
  PixTest              *fTest; 

  int                   fBorderN, fBorderT, fBorderL;  // normal, tiny, large
  
  ClassDef(PixTab, 1)

};

#endif
