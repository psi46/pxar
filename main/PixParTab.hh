#ifndef PIXPARTAB_H
#define PIXPARTAB_H

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
#include <TGButton.h>

#include "PixGui.hh"
#include "ConfigParameters.hh"

class PixParTab: public TQObject {
public:
  PixParTab(PixGui *p, ConfigParameters *c, std::string tabname);
  PixParTab();
  void init(PixGui *p, ConfigParameters *c, std::string tabname); 
  virtual ~PixParTab();



  virtual void handleButtons(Int_t id = -1); 
  virtual void setParameter(); 

  std::string getName() {return fTabName;}
  TGCompositeFrame* getCompositeFrame() {return fTabFrame;}
  TGCompositeFrame* getHorizontalFrame() {return fhFrame;}

protected: 
  
  // -- frames and widgets
  TGCompositeFrame     	*fTabFrame;
  TGHorizontalFrame     *fhFrame;
  TGVerticalFrame 	*fV1;
  TGVerticalFrame 	*fV2;

  std::string            fTabName; 
  
  std::map<std::string, void*>  fTbTextEntries;
  std::vector<std::string>      fTbParIds;

  std::vector<std::map<std::string, void*> > fTbmTextEntries;
  std::vector<std::vector<std::string> >     fTbmParIds;
  std::vector<TGCheckButton*>                fSelectTbm;

  std::vector<std::map<std::string, void*> > fRocTextEntries;
  std::vector<std::vector<std::string> >     fRocParIds;
  std::vector<TGCheckButton*>                fSelectRoc;


  PixGui                *fGui; 
  ConfigParameters      *fConfigParameters; 

  static const int TABNUMBER = 0;
  enum CommandIdentifiers {
    B_SELECTALL = 120,  B_DESELECTALL = 121
  };


  ClassDef(PixParTab, 1)

};

#endif
