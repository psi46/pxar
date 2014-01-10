#include <iostream>

#include <TApplication.h>
#include <TGButton.h>
#include <TRandom.h>
#include <TSystem.h>
#include <TCanvas.h>
#include <TGTab.h>
#include <TGLabel.h>
#include <TGButtonGroup.h>

#include "PixParTab.hh"
#include "log.h"


using namespace std;
using namespace pxar;

ClassImp(PixParTab)

// ----------------------------------------------------------------------
PixParTab::PixParTab(PixGui *p, ConfigParameters *cfg, string tabname) {
  init(p, cfg, tabname); 

  fTabFrame = fGui->getTabs()->AddTab(fTabName.c_str());
  fTabFrame->SetLayoutManager(new TGVerticalLayout(fTabFrame));

  fhFrame = new TGHorizontalFrame(fTabFrame);

  fTabFrame->AddFrame(fhFrame, new TGLayoutHints(kLHintsRight | kLHintsExpandX | kLHintsExpandY));

  TGTextEntry *te(0); 
  TGLabel *tl(0); 
  TGTextBuffer *tb(0); 
  TGTextButton *tset(0); 

  TGCheckButton *tcb(0); 
  TGHorizontalFrame *hFrame(0); 
  TGVerticalFrame *vFrame(0); 
  TGGroupFrame *g1Frame(0), *g2Frame(0); 

  // -- TB Parameters
  vFrame = new TGVerticalFrame(fhFrame);
  fhFrame->AddFrame(vFrame, new TGLayoutHints(kLHintsLeft, 15, 15, 15, 15));
  g1Frame = new TGGroupFrame(vFrame, "Testboard");
  int cnt(0); 
  vector<pair<string, uint8_t> > amap = fConfigParameters->getTbParameters();
  for (unsigned int i = 0; i < amap.size(); ++i) {
    hFrame = new TGHorizontalFrame(g1Frame, 300, 30, kLHintsExpandX); 
    g1Frame->AddFrame(hFrame, new TGLayoutHints(kLHintsRight | kLHintsTop));
    LOG(logINFO) << "Creating TGTextEntry for " << amap[i].first; 
    tb = new TGTextBuffer(5); 
    tl = new TGLabel(hFrame, amap[i].first.c_str());
    tl->SetWidth(100);
    hFrame->AddFrame(tl, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 

    te  = new TGTextEntry(hFrame, tb, cnt); te->SetWidth(100); 
    hFrame->AddFrame(te, new TGLayoutHints(kLHintsCenterY | kLHintsCenterX, 2, 2, 2, 2)); 
    fTbParIds.push_back(amap[i].first); 
    fTbTextEntries.insert(make_pair(amap[i].first, te)); 

    te->SetText(Form("%d", int(amap[i].second)));
    te->Connect("ReturnPressed()", "PixTab", this, "setParameter()");

    tset = new TGTextButton(hFrame, "Set", cnt);
    tset->Connect("Clicked()", "PixTab", this, "setParameter()");
    hFrame->AddFrame(tset, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 

    ++cnt;
  }
  vFrame->AddFrame(g1Frame);

  // -- TBM Parameters
  g2Frame = new TGGroupFrame(vFrame, "TBM DAC");
  vector<vector<pair<string, uint8_t> > >   cmap = fConfigParameters->getTbmDacs();
  if (cmap.size() > 0) {
    amap = cmap[0];  // FIXME!!!
    std::vector<std::string>     parids;
    std::map<std::string, void*> textentries; 
    for (unsigned int i = 0; i < amap.size(); ++i) {
      hFrame = new TGHorizontalFrame(g2Frame, 300, 30, kLHintsExpandX); 
      g2Frame->AddFrame(hFrame, new TGLayoutHints(kLHintsRight | kLHintsTop));
      LOG(logINFO) << "Creating TGTextEntry for " << amap[i].first; 
      tb = new TGTextBuffer(5); 
      tl = new TGLabel(hFrame, amap[i].first.c_str());
      tl->SetWidth(100);
      hFrame->AddFrame(tl, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 
      
      te  = new TGTextEntry(hFrame, tb, cnt); te->SetWidth(100); 
      hFrame->AddFrame(te, new TGLayoutHints(kLHintsCenterY | kLHintsCenterX, 2, 2, 2, 2)); 
      parids.push_back(amap[i].first); 
      textentries.insert(make_pair(amap[i].first, te)); 
      
      te->SetText(Form("%d", int(amap[i].second)));
      te->Connect("ReturnPressed()", "PixTab", this, "setParameter()");
      
      tset = new TGTextButton(hFrame, "Set", cnt);
      tset->Connect("Clicked()", "PixTab", this, "setParameter()");
      hFrame->AddFrame(tset, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 
      
      ++cnt;
    }
    fTbmTextEntries.push_back(textentries);
    fTbmParIds.push_back(parids);
    vFrame->AddFrame(g2Frame);
    g1Frame->SetWidth(g2Frame->GetDefaultWidth());
  }
  

  // -- DAC Parameters
  vFrame = new TGVerticalFrame(fhFrame);
  fhFrame->AddFrame(vFrame, new TGLayoutHints(kLHintsLeft, 15, 15, 15, 15));
  //  gFrame = new TGGroupFrame(vFrame, "ROC DAC");
  //   TGGroupFrame *grFrame = new TGGroupFrame(vFrame, "ROC selection");
  //   vFrame->AddFrame(grFrame, new TGLayoutHints(kLHintsTop, 2, 2, 2, 2));
  //  hFrame = new TGHorizontalFrame(vFrame);
  //  vFrame->AddFrame(hFrame, new TGLayoutHints(kLHintsTop, 2, 2, 2, 2));

  hFrame = new TGHorizontalFrame(vFrame, 300, 30, kLHintsExpandX); 
  vFrame->AddFrame(hFrame); 
  hFrame->AddFrame(tset = new TGTextButton(hFrame, "Select all", B_SELECTALL));
  tset->Connect("Clicked()", "PixParTab", this, "handleButtons()");
  hFrame->AddFrame(tset = new TGTextButton(hFrame, "Deselect all", B_DESELECTALL));
  tset->Connect("Clicked()", "PixParTab", this, "handleButtons()");
  
  TGCompositeFrame *bGroup = new TGCompositeFrame(vFrame, 60, 20, kHorizontalFrame |kSunkenFrame);
  for (int i = 0; i < fConfigParameters->getNrocs(); ++i) {
    tcb = new TGCheckButton(bGroup, Form("%d", i), i); 
    bGroup->AddFrame(tcb, new TGLayoutHints(kLHintsLeft, 2, 2, 2, 2)); 
    fSelectRoc.push_back(tcb); 
  }
  
  vFrame->AddFrame(bGroup, new TGLayoutHints(kLHintsCenterX|kLHintsCenterY, 1, 1, 1, 1));  

  hFrame = new TGHorizontalFrame(vFrame);
  vFrame->AddFrame(hFrame, new TGLayoutHints(kLHintsBottom, 2, 2, 2, 2));
  
  g1Frame = new TGGroupFrame(hFrame, "DACs1");
  hFrame->AddFrame(g1Frame, new TGLayoutHints(kLHintsLeft, 2, 2, 2, 2));
  g2Frame = new TGGroupFrame(hFrame, "DACs2");
  hFrame->AddFrame(g2Frame, new TGLayoutHints(kLHintsRight, 2, 2, 2, 2));
  
  cmap = fConfigParameters->getRocDacs();
  if (cmap.size() > 0) {
    std::vector<std::string>     parids;
    std::map<std::string, void*> textentries; 
    amap = cmap[0];   // FIXME!!! // FIXME!!! // FIXME!!! // FIXME!!! // FIXME!!! // FIXME!!!
    unsigned int idac(0); 
    for (idac = 0; idac < 0.5*amap.size(); ++idac) {
      hFrame = new TGHorizontalFrame(g1Frame, 300, 30, kLHintsExpandX); 
      g1Frame->AddFrame(hFrame, new TGLayoutHints(kLHintsRight | kLHintsTop));
      LOG(logINFO) << "Creating TGTextEntry for " << amap[idac].first; 
      tb = new TGTextBuffer(5); 
      tl = new TGLabel(hFrame, amap[idac].first.c_str());
      tl->SetWidth(100);
      hFrame->AddFrame(tl, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 
      
      te  = new TGTextEntry(hFrame, tb, cnt); te->SetWidth(100); 
      hFrame->AddFrame(te, new TGLayoutHints(kLHintsCenterY | kLHintsCenterX, 2, 2, 2, 2)); 
      parids.push_back(amap[idac].first); 
      textentries.insert(make_pair(amap[idac].first, te)); 
      
      te->SetText(Form("%d", int(amap[idac].second)));
      te->Connect("ReturnPressed()", "PixTab", this, "setParameter()");
      
      tset = new TGTextButton(hFrame, "Set", cnt);
      tset->Connect("Clicked()", "PixTab", this, "setParameter()");
      hFrame->AddFrame(tset, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 
      
      ++cnt;
    }
    
    for (idac = 0.5*amap.size(); idac < amap.size(); ++idac) {
      hFrame = new TGHorizontalFrame(g2Frame, 300, 30, kLHintsExpandX); 
      g2Frame->AddFrame(hFrame, new TGLayoutHints(kLHintsRight | kLHintsTop));
      LOG(logINFO) << "Creating TGTextEntry for " << amap[idac].first; 
      tb = new TGTextBuffer(5); 
      tl = new TGLabel(hFrame, amap[idac].first.c_str());
      tl->SetWidth(100);
      hFrame->AddFrame(tl, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 
      
      te  = new TGTextEntry(hFrame, tb, cnt); te->SetWidth(100); 
      hFrame->AddFrame(te, new TGLayoutHints(kLHintsCenterY | kLHintsCenterX, 2, 2, 2, 2)); 
      parids.push_back(amap[idac].first); 
      textentries.insert(make_pair(amap[idac].first, te)); 
      
      te->SetText(Form("%d", int(amap[idac].second)));
      te->Connect("ReturnPressed()", "PixTab", this, "setParameter()");
      
      tset = new TGTextButton(hFrame, "Set", cnt);
      tset->Connect("Clicked()", "PixTab", this, "setParameter()");
      hFrame->AddFrame(tset, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 
      
      ++cnt;
    }

    fTbmTextEntries.push_back(textentries);
    fTbmParIds.push_back(parids);
  }


  fTabFrame->Layout();
  fTabFrame->MapSubwindows();
  fTabFrame->MapWindow();

}


// ----------------------------------------------------------------------
PixParTab::PixParTab() {
  init(0, 0, "nada");
}

// ----------------------------------------------------------------------
void PixParTab::init(PixGui *p, ConfigParameters *cfg, std::string tabname) {
  LOG(logINFO) << "PixParTab::init()";
  fGui = p;
  fConfigParameters = cfg; 
  fTabName = tabname; 
}

// ----------------------------------------------------------------------
// PixParTab destructor
PixParTab::~PixParTab() {
  LOG(logINFO) << "PixParTab destructor";
}


// ----------------------------------------------------------------------
void PixParTab::handleButtons(Int_t id) {
  
  if (!fGui->getTabs()) return;
  
  if (id == -1) {
    TGButton *btn = (TGButton*)gTQSender;
    id = btn->WidgetId();
  }
  
  switch (id) {
  case B_SELECTALL: {
    LOG(logINFO) << "SELECT ALL";
    for (unsigned int i = 0; i < fSelectRoc.size(); ++i) {
      fSelectRoc[i]->SetState(kButtonDown); 
    }
    break;
  }

  case B_DESELECTALL: {
    LOG(logINFO) << "DESELECT ALL";
    for (unsigned int i = 0; i < fSelectRoc.size(); ++i) {
      fSelectRoc[i]->SetState(kButtonUp); 
    }
    break;
  }
    
  }
}


// ----------------------------------------------------------------------
void PixParTab::setParameter() {
  if (!fGui->getTabs()) return;
  LOG(logINFO)  << "PixParTab::setParameter: ";

  TGButton *btn = (TGButton *) gTQSender;
  int id(-1); 
  id = btn->WidgetId();
  if (-1 == id) {
    LOG(logINFO) << "ASLFDKHAPIUDF ";
    return; 
  }


  cout << "FIXME FIXME " << endl;

//   string svalue = ((TGTextEntry*)(fParTextEntries[fParIds[id]]))->GetText(); 
  
//   LOG(logINFO) << "xxxPressed():  ID = " << id 
// 	       << " -> " << fParIds[id] 
// 	       << " to value " << svalue;

  //FIXME fTest->setParameter(fParIds[id], svalue); 

  //FIXME if (1) fTest->dumpParameters();
  
} 
