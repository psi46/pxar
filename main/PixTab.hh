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

#include "PixGui.hh"
#include "PixTest.hh"

class PixTab: public TQObject {
public:
  PixTab(PixGui *p, PixTest *test, std::string tabname);
  PixTab();
  void init(PixGui *p, PixTest *test, std::string tabname); 
  virtual ~PixTab();
  std::string getName() {return fTabName;}

  virtual void handleButtons(Int_t id = -1); 
  virtual void setParameter(); 

  void update();
  void clearCanvas();
  void nextHistogram();
  void previousHistogram();
  
protected: 

  // -- frames and widgets
  TGCompositeFrame     	*fTabFrame;
  TGHorizontalFrame     *fhFrame;
  TGVerticalFrame 	*fV1;
  TGVerticalFrame 	*fV2;

  //  map<std::string, void*>  fParTextBuffers, fParTextEntries, fParLabels;
  map<std::string, void*>  fParTextEntries;
  vector<std::string>      fParIds;

  TRootEmbeddedCanvas  	*fEc1;
  
  static const int TESTNUMBER = 0;
  enum CommandIdentifiers {
    B_DOTEST = TESTNUMBER + 21,
    B_CLOSETAB,
    B_PRINT,
  };

  std::string           fTabName; 
  int			fCount;

  PixGui               *fGui; 
  PixTest              *fTest; 

  ClassDef(PixTab, 1)

};

#endif
