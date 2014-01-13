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

  Pixel_t colDarkSeaGreen;    
  gClient->GetColorByName("DarkSeaGreen", colDarkSeaGreen);
  TGTabElement *tabel = fGui->getTabs()->GetTabTab(fTabName.c_str());
  tabel->ChangeBackground(colDarkSeaGreen);


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
  vector<pair<string, uint8_t> > amap = fConfigParameters->getTbParameters();
  for (unsigned int i = 0; i < amap.size(); ++i) {
    hFrame = new TGHorizontalFrame(g1Frame, 300, 30, kLHintsExpandX); 
    g1Frame->AddFrame(hFrame, new TGLayoutHints(kLHintsRight | kLHintsTop));
    LOG(logINFO) << "Creating TGTextEntry for " << amap[i].first; 
    tb = new TGTextBuffer(5); 
    tl = new TGLabel(hFrame, amap[i].first.c_str());
    tl->SetWidth(100);
    hFrame->AddFrame(tl, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 

    te  = new TGTextEntry(hFrame, tb, i); te->SetWidth(100); 
    hFrame->AddFrame(te, new TGLayoutHints(kLHintsCenterY | kLHintsCenterX, 2, 2, 2, 2)); 
    fTbParIds.push_back(amap[i].first); 
    fTbTextEntries.insert(make_pair(amap[i].first, te)); 

    te->SetText(Form("%d", int(amap[i].second)));
    te->Connect("ReturnPressed()", "PixParTab", this, "setTbParameter()");

    tset = new TGTextButton(hFrame, "Set", i);
    tset->Connect("Clicked()", "PixParTab", this, "setTbParameter()");
    hFrame->AddFrame(tset, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 
  }

  vector<pair<string, double> > dmap = fConfigParameters->getTbPowerSettings();
  for (unsigned int i = 0; i < dmap.size(); ++i) {
    hFrame = new TGHorizontalFrame(g1Frame, 300, 30, kLHintsExpandX); 
    g1Frame->AddFrame(hFrame, new TGLayoutHints(kLHintsRight | kLHintsTop));
    LOG(logINFO) << "Creating TGTextEntry for " << dmap[i].first; 
    tb = new TGTextBuffer(5); 
    tl = new TGLabel(hFrame, dmap[i].first.c_str());
    tl->SetWidth(100);
    hFrame->AddFrame(tl, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 

    te  = new TGTextEntry(hFrame, tb, i); te->SetWidth(100); 
    hFrame->AddFrame(te, new TGLayoutHints(kLHintsCenterY | kLHintsCenterX, 2, 2, 2, 2)); 
    fPowerParIds.push_back(dmap[i].first); 
    fPowerTextEntries.insert(make_pair(dmap[i].first, te)); 

    te->SetText(Form("%5.3f", float(dmap[i].second)));
    te->Connect("ReturnPressed()", "PixParTab", this, "setPowerSettings()");

    tset = new TGTextButton(hFrame, "Set", i);
    tset->Connect("Clicked()", "PixParTab", this, "setPowerSettings()");
    hFrame->AddFrame(tset, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 
  }

  vFrame->AddFrame(g1Frame);

  // -- TBM Parameters
  TGCompositeFrame *bGroup = new TGCompositeFrame(vFrame, 60, 20, kHorizontalFrame |kSunkenFrame);
  for (int i = 0; i < fConfigParameters->getNtbms(); ++i) {
    tcb = new TGCheckButton(bGroup, Form("%d", i), i); 
    bGroup->AddFrame(tcb, new TGLayoutHints(kLHintsLeft, 2, 2, 2, 2)); 
    fSelectTbm.push_back(tcb); 
  }
  if (fSelectTbm.size() > 0) fSelectTbm[0]->SetState(kButtonDown);
  vFrame->AddFrame(bGroup, new TGLayoutHints(kLHintsCenterX|kLHintsCenterY, 1, 1, 1, 1));  

  g2Frame = new TGGroupFrame(vFrame, "DAC of first selected TBM");
  vector<vector<pair<string, uint8_t> > >   cmap = fConfigParameters->getTbmDacs();
  if (cmap.size() > 0) {
    int itbm(0); 
    for (unsigned int i = 0; i < fSelectTbm.size(); ++i) {
      if (kButtonDown == fSelectTbm[i]->GetState()) {
	itbm = i; 
	break;
      }
    }
    amap = cmap[itbm];  
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
      
      te  = new TGTextEntry(hFrame, tb, i); te->SetWidth(100); 
      hFrame->AddFrame(te, new TGLayoutHints(kLHintsCenterY | kLHintsCenterX, 2, 2, 2, 2)); 
      parids.push_back(amap[i].first); 
      textentries.insert(make_pair(amap[i].first, te)); 
      
      te->SetText(Form("%d", int(amap[i].second)));
      te->Connect("ReturnPressed()", "PixParTab", this, "setTbmParameter()");
      
      tset = new TGTextButton(hFrame, "Set", i);
      tset->Connect("Clicked()", "PixParTab", this, "setTbmParameter()");
      hFrame->AddFrame(tset, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 
      
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
  
  bGroup = new TGCompositeFrame(vFrame, 60, 20, kHorizontalFrame |kSunkenFrame);
  for (int i = 0; i < fConfigParameters->getNrocs(); ++i) {
    tcb = new TGCheckButton(bGroup, Form("%d", i), i); 
    bGroup->AddFrame(tcb, new TGLayoutHints(kLHintsLeft, 2, 2, 2, 2)); 
    fSelectRoc.push_back(tcb); 
  }
  if (fSelectRoc.size() > 0) fSelectRoc[0]->SetState(kButtonDown);
  
  vFrame->AddFrame(bGroup, new TGLayoutHints(kLHintsCenterX|kLHintsCenterY, 1, 1, 1, 1));  

  hFrame = new TGHorizontalFrame(vFrame);
  vFrame->AddFrame(hFrame, new TGLayoutHints(kLHintsBottom, 2, 2, 2, 2));
  
  g1Frame = new TGGroupFrame(hFrame, "DACs of first selected ROC");
  hFrame->AddFrame(g1Frame, new TGLayoutHints(kLHintsLeft, 2, 2, 2, 2));
  g2Frame = new TGGroupFrame(hFrame, "DACs of first selected ROC");
  hFrame->AddFrame(g2Frame, new TGLayoutHints(kLHintsRight, 2, 2, 2, 2));
  
  cmap = fConfigParameters->getRocDacs();
  if (cmap.size() > 0) {
    std::vector<std::string>     parids;
    std::map<std::string, void*> textentries; 
    int iroc(0); 
    for (unsigned int i = 0; i < fSelectRoc.size(); ++i) {
      if (kButtonDown == fSelectRoc[i]->GetState()) {
	iroc = i; 
	break;
      }
    }
    amap = cmap[iroc]; 
    unsigned int idac(0); 
    for (idac = 0; idac < 0.5*amap.size(); ++idac) {
      hFrame = new TGHorizontalFrame(g1Frame, 300, 30, kLHintsExpandX); 
      g1Frame->AddFrame(hFrame, new TGLayoutHints(kLHintsRight | kLHintsTop));
      LOG(logINFO) << "Creating TGTextEntry for " << amap[idac].first; 
      tb = new TGTextBuffer(5); 
      tl = new TGLabel(hFrame, amap[idac].first.c_str());
      tl->SetWidth(100);
      hFrame->AddFrame(tl, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 
      
      te  = new TGTextEntry(hFrame, tb, idac); te->SetWidth(100); 
      hFrame->AddFrame(te, new TGLayoutHints(kLHintsCenterY | kLHintsCenterX, 2, 2, 2, 2)); 
      parids.push_back(amap[idac].first); 
      textentries.insert(make_pair(amap[idac].first, te)); 
      
      te->SetText(Form("%d", int(amap[idac].second)));
      te->Connect("ReturnPressed()", "PixParTab", this, "setRocParameter()");
      
      tset = new TGTextButton(hFrame, "Set", idac);
      tset->Connect("Clicked()", "PixParTab", this, "setRocParameter()");
      hFrame->AddFrame(tset, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 
      
    }
    
    for (idac = 0.5*amap.size(); idac < amap.size(); ++idac) {
      hFrame = new TGHorizontalFrame(g2Frame, 300, 30, kLHintsExpandX); 
      g2Frame->AddFrame(hFrame, new TGLayoutHints(kLHintsRight | kLHintsTop));
      LOG(logINFO) << "Creating TGTextEntry for " << amap[idac].first; 
      tb = new TGTextBuffer(5); 
      tl = new TGLabel(hFrame, amap[idac].first.c_str());
      tl->SetWidth(100);
      hFrame->AddFrame(tl, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 
      
      te  = new TGTextEntry(hFrame, tb, idac); te->SetWidth(100); 
      hFrame->AddFrame(te, new TGLayoutHints(kLHintsCenterY | kLHintsCenterX, 2, 2, 2, 2)); 
      parids.push_back(amap[idac].first); 
      textentries.insert(make_pair(amap[idac].first, te)); 
      
      te->SetText(Form("%d", int(amap[idac].second)));
      te->Connect("ReturnPressed()", "PixParTab", this, "setRocParameter()");
      
      tset = new TGTextButton(hFrame, "Set", idac);
      tset->Connect("Clicked()", "PixParTab", this, "setRocParameter()");
      hFrame->AddFrame(tset, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 
      
    }

    fRocTextEntries.push_back(textentries);
    fRocParIds.push_back(parids);
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
void PixParTab::setTbParameter() {
  if (!fGui->getTabs()) return;
  LOG(logINFO)  << "PixParTab::setTbParameter: ";

  TGButton *btn = (TGButton *) gTQSender;
  int id(-1); 
  id = btn->WidgetId();
  if (-1 == id) {
    LOG(logINFO) << "ASLFDKHAPIUDF ";
    return; 
  }

  string svalue = ((TGTextEntry*)(fTbTextEntries[fTbParIds[id]]))->GetText(); 
  uint8_t udac = atoi(svalue.c_str()); 

  cout << "FIXME FIXME: ID = " << id << " -> " << fTbParIds[id] << " set to " << svalue << endl;
  fConfigParameters->setTbParameter(fTbParIds[id], udac); 

  initTestboard(); 

} 

// ----------------------------------------------------------------------
void PixParTab::setPgSettings() {
  if (!fGui->getTabs()) return;
  LOG(logINFO)  << "PixParTab::setPgSettings: ";

  TGButton *btn = (TGButton *) gTQSender;
  int id(-1); 
  id = btn->WidgetId();
  if (-1 == id) {
    LOG(logINFO) << "ASLFDKHAPIUDF ";
    return; 
  }

  string svalue = ((TGTextEntry*)(fPgTextEntries[fPgParIds[id]]))->GetText(); 
  uint8_t udac = atoi(svalue.c_str()); 

  cout << "FIXME FIXME: ID = " << id << " -> " << fPgParIds[id] << " set to " << svalue << endl;

  initTestboard(); 

} 


// ----------------------------------------------------------------------
void PixParTab::setPowerSettings() {
  if (!fGui->getTabs()) return;
  LOG(logINFO)  << "PixParTab::setPowerSettings: ";

  TGButton *btn = (TGButton *) gTQSender;
  int id(-1); 
  id = btn->WidgetId();
  if (-1 == id) {
    LOG(logINFO) << "ASLFDKHAPIUDF ";
    return; 
  }

  string svalue = ((TGTextEntry*)(fPowerTextEntries[fPowerParIds[id]]))->GetText(); 
  double udac = atof(svalue.c_str()); 
  
  cout << "FIXME FIXME: ID = " << id << " -> " << fPowerParIds[id] << " set to " << svalue << endl;
  fConfigParameters->setTbPowerSettings(fPowerParIds[id], udac); 
  
  initTestboard(); 

  // FIXME UPDATE CONFIGPARAMETERS!

} 


// ----------------------------------------------------------------------
void PixParTab::initTestboard() {

  vector<pair<string, uint8_t> > sig_delays = fConfigParameters->getTbSigDelays();
  vector<pair<string, double> > power_settings = fConfigParameters->getTbPowerSettings();
  vector<pair<uint16_t, uint8_t> > pg_setup = fConfigParameters->getTbPgSettings();;

  LOG(logINFO) << "Re-programming TB"; 

  fGui->getApi()->initTestboard(sig_delays, power_settings, pg_setup);

}


// ----------------------------------------------------------------------
void PixParTab::setTbmParameter() {
  if (!fGui->getTabs()) return;
  LOG(logINFO)  << "PixParTab::setTbmParameter: ";

  TGButton *btn = (TGButton *) gTQSender;
  int id(-1); 
  id = btn->WidgetId();
  if (-1 == id) {
    LOG(logINFO) << "ASLFDKHAPIUDF ";
    return; 
  }
  int itbm(-1); 
  for (unsigned int i = 0; i < fSelectTbm.size(); ++i) {
    if (kButtonDown == fSelectTbm[i]->GetState()) {
      itbm = i; 
      break;
    }
  }
  cout << "TBM " << itbm << " is selected" << endl;
  if (itbm > -1) {
    vector<string> v = fTbmParIds[itbm]; 
    map<string, void *> t = fTbmTextEntries[itbm];
    string svalue = ((TGTextEntry*)(t[v[id]]))->GetText(); 
    uint8_t udac = atoi(svalue.c_str()); 
    LOG(logINFO)<< "xxx: ID = " << id << " -> " << v[id] << " set to svalue = " << svalue << " int = " << int(udac);
    fGui->getApi()->setTbmReg(v[id], udac, itbm);
  }

  // FIXME UPDATE CONFIGPARAMETERS!
  
} 


// ----------------------------------------------------------------------
void PixParTab::setRocParameter() {
  if (!fGui->getTabs()) return;
  LOG(logINFO)  << "PixParTab::setRocParameter: ";

  TGButton *btn = (TGButton *) gTQSender;
  int id(-1); 
  id = btn->WidgetId();
  if (-1 == id) {
    LOG(logINFO) << "ASLFDKHAPIUDF ";
    return; 
  }

  int iroc(-1); 
  for (unsigned int i = 0; i < fSelectRoc.size(); ++i) {
    if (kButtonDown == fSelectRoc[i]->GetState()) {
      iroc = i; 
      break;
    }
  }
  LOG(logINFO) << "ROC " << iroc << " is selected. id = " << id;
  if (iroc > -1) {
    vector<string> v = fRocParIds[iroc]; 
    map<string, void *> t = fRocTextEntries[iroc];
    string svalue = ((TGTextEntry*)(t[v[id]]))->GetText(); 
    uint8_t udac = atoi(svalue.c_str()); 
    LOG(logINFO)<< "xxx: ID = " << id << " -> " << v[id] << " set to svalue = " << svalue << " int = " << int(udac);
    fGui->getApi()->setDAC(v[id], udac, iroc);
  }

  // FIXME UPDATE CONFIGPARAMETERS!
 
} 
