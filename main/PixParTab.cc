#include <iostream>
#include "PixParTab.hh"

#include <TApplication.h>
#include <TGButton.h>
#include <TRandom.h>
#include <TSystem.h>
#include <TCanvas.h>
#include <TGTab.h>
#include <TGLabel.h>

using namespace std;

ClassImp(PixParTab)

// ----------------------------------------------------------------------
PixParTab::PixParTab(PixGui *p, string tabname) {
  init(p, tabname); 
  
  UInt_t w = 800; 
  UInt_t h = 800; 

  fTabFrame = fGui->getTabs()->AddTab(fTabName.c_str());
  fTabFrame->SetLayoutManager(new TGVerticalLayout(fTabFrame));

  fhFrame = new TGHorizontalFrame(fTabFrame, w, h);

  fTabFrame->AddFrame(fhFrame, new TGLayoutHints(kLHintsRight | kLHintsExpandX | kLHintsExpandY));

//   fhFrame->Resize(fTabFrame->GetDefaultSize());
//   fTabFrame->Resize(fTabFrame->GetDefaultSize());
  fTabFrame->Layout();
  fTabFrame->MapSubwindows();
  fTabFrame->MapWindow();
//   fhFrame->Resize(fTabFrame->GetDefaultSize());
//   fTabFrame->Resize(fTabFrame->GetDefaultSize());

  fTabFrame->AddFrame(fhFrame, new TGLayoutHints(kLHintsRight | kLHintsExpandX | kLHintsExpandY));

  fGui->getTabs()->SetTab(tabname.c_str()); 

}


// ----------------------------------------------------------------------
PixParTab::PixParTab() {
  init(0, "nada");
}

// ----------------------------------------------------------------------
void PixParTab::init(PixGui *p, std::string tabname) {
  cout << "PixParTab::init()" << endl;
  fGui = p;
  fTabName = tabname; 
}

// ----------------------------------------------------------------------
// PixParTab destructor
PixParTab::~PixParTab() {
  cout << "PixParTab destructor" << endl;
}


// ----------------------------------------------------------------------
void PixParTab::handleButtons(Int_t id) {
  
  if (!fGui->getTabs()) return;
  
  if (id == -1) {
    TGButton *btn = (TGButton*)gTQSender;
    id = btn->WidgetId();
  }
  
  switch (id) {
  case B_DOSET: {
    cout << "and now what???" << endl;
  }
    
  case B_PRINT: {
    cout << "dump parameters into file" << endl;
    break;
  }
    
  case B_CLOSETAB: {
    fGui->getTabs()->RemoveTab(fGui->getTabs()->GetCurrent());
    fGui->getTabs()->Layout();
    delete this; 
    break;
  }
  }
}
