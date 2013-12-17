#ifndef PIXPARTAB_H
#define PIXPARTAB_H

#include <string>
#include <list>

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

class PixParTab: public TQObject {
public:
  PixParTab(PixGui *p, std::string tabname);
  PixParTab();
  void init(PixGui *p, std::string tabname); 
  virtual ~PixParTab();


  virtual void handleButtons(Int_t id = -1); 
  std::string getName() {return fTabName;}

protected: 
  // -- frames and widgets
  TGCompositeFrame     	*fTabFrame;
  TGHorizontalFrame     *fhFrame;
  TGVerticalFrame 	*fV1;
  TGVerticalFrame 	*fV2;

  std::string            fTabName; 

  PixGui                *fGui; 


  static const int TABNUMBER = 0;
  enum CommandIdentifiers {
    B_DOSET = TABNUMBER + 21,
    B_CLOSETAB,
    B_PRINT,
  };


  ClassDef(PixParTab, 1)

};

#endif
