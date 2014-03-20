#ifndef PIXPARTAB_H
#define PIXPARTAB_H

#include <string>

#include <TQObject.h> 
#include <TGFrame.h>
#include <TGToolTip.h>
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
  virtual void selectRoc(Int_t id = -1); 
  virtual void selectTbm(Int_t id = -1); 
  virtual std::vector<int> getSelectedRocs();
  virtual std::vector<int> getSelectedTbms();

  virtual void setTbParameter(); 
  virtual void setPowerSettings(); 
  virtual void setPgSettings();
  virtual void setTbmParameter(); 
  virtual void setRocParameter(); 
  virtual void setLemo(); 
  virtual void initTestboard(); 

  virtual void saveTbParameters();
  virtual void saveTbmParameters();
  virtual void saveDacParameters();
  virtual void saveTrimParameters();

  virtual void updateSelection(); 
  virtual void updateParameters();

  std::string getName() {return fTabName;}
  TGCompositeFrame* getCompositeFrame() {return fTabFrame;}
  TGCompositeFrame* getHorizontalFrame() {return fhFrame;}

protected: 
  
  // -- frames and widgets
  TGCompositeFrame     	*fTabFrame;
  TGCompositeFrame     	*fhFrame;
  //  TGHorizontalFrame     *fhFrame;
  TGVerticalFrame 	*fV1;
  TGVerticalFrame 	*fV2;

  std::string            fTabName; 
  
  std::map<std::string, TGTextEntry*>  fTbTextEntries;
  std::vector<std::string>      fTbParIds;

  std::map<std::string, TGTextEntry*>  fPowerTextEntries;
  std::vector<std::string>      fPowerParIds;

  std::map<std::string, TGTextEntry*>  fPgTextEntries;
  std::vector<std::string>      fPgParIds;

  std::map<std::string, TGTextEntry*>          fTbmTextEntries;
  std::map<int, std::string>                   fTbmTextMap;
  std::vector<std::map<std::string, uint8_t> > fTbmParIds;
  std::vector<TGCheckButton*>                  fSelectTbm;
  int                                          fSelectedTbm;

  std::map<std::string, TGTextEntry*>          fRocTextEntries;
  std::map<int, std::string>                   fRocTextMap;
  std::vector<std::map<std::string, uint8_t> > fRocParIds;
  std::vector<TGCheckButton*>                  fSelectRoc;
  int                                          fSelectedRoc;

  PixGui                *fGui; 
  ConfigParameters      *fConfigParameters; 

  static const int TABNUMBER = 0;
  enum CommandIdentifiers {
    B_SELECTALL = 120,  B_DESELECTALL = 121,
    B_LEMOD1 = 1000
  };

  int                   fBorderR, fBorderL, fBorderT, fBorderB;


  ClassDef(PixParTab, 1)

};

#endif
