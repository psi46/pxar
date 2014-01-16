#include <iostream>

#include <TApplication.h>
#include <TGButton.h>
#include <TRandom.h>
#include <TSystem.h>
#include <TCanvas.h>
#include <TGTab.h>
#include <TGLabel.h>

#include "PixTab.hh"
#include "log.h"

using namespace std;
using namespace pxar; 

ClassImp(PixTab)

// ----------------------------------------------------------------------
PixTab::PixTab(PixGui *p, PixTest *test, string tabname) {
  init(p, test, tabname); 
  
  UInt_t w = fGui->getTabs()->GetWidth(); 
  UInt_t h = fGui->getTabs()->GetHeight(); 

  fTabFrame = fGui->getTabs()->AddTab(fTabName.c_str());
 
  //  fhFrame = new TGHorizontalFrame(fTabFrame, w, h);
  fhFrame = new TGCompositeFrame(fTabFrame, w, h, kHorizontalFrame);
  
  // -- 2 vertical frames
  fV1 = new TGVerticalFrame(fhFrame);
  fhFrame->AddFrame(fV1, new TGLayoutHints(kLHintsLeft | kLHintsExpandX | kLHintsExpandY, 15, 15, 15, 15));
  fV2 = new TGVerticalFrame(fhFrame);
  fhFrame->AddFrame(fV2, new TGLayoutHints(kLHintsRight | kLHintsExpandX | kLHintsExpandY, 15, 15, 15, 15));

  // -- fV1: create and add Embedded Canvas
  fEc1 = new TRootEmbeddedCanvas(Form("%s", tabname.c_str()), fV1, 500, 500);
  fV1->AddFrame(fEc1, new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX | kLHintsExpandY, 15, 15, 15, 15));

  // -- fV2: create parameter TGText boxes for test
  map<string, string> amap = fTest->getParameters();
  TGTextEntry *te(0); 
  TGLabel *tl(0); 
  TGTextBuffer *tb(0); 
  TGTextButton *tset(0); 

  TGHorizontalFrame *hFrame(0); 

  int cnt(0); 
  for (map<string, string>::iterator imap = amap.begin(); imap != amap.end(); ++imap) {  
    hFrame = new TGHorizontalFrame(fV2, 300, 30, kLHintsExpandX); 
    fV2->AddFrame(hFrame, new TGLayoutHints(kLHintsRight | kLHintsTop));
    //    LOG(logINFO) << "Creating TGTextEntry for " << imap->first;
    tb = new TGTextBuffer(5); 
    tl = new TGLabel(hFrame, imap->first.c_str());
    tl->SetWidth(100);
    hFrame->AddFrame(tl, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 

    te  = new TGTextEntry(hFrame, tb, cnt); te->SetWidth(100); 
    hFrame->AddFrame(te, new TGLayoutHints(kLHintsCenterY | kLHintsCenterX, 2, 2, 2, 2)); 
    fParIds.push_back(imap->first); 
    fParTextEntries.insert(make_pair(imap->first, te)); 

    te->SetText(imap->second.c_str());
    te->Connect("ReturnPressed()", "PixTab", this, "setParameter()");

    tset = new TGTextButton(hFrame, "Set", cnt);
    tset->Connect("Clicked()", "PixTab", this, "setParameter()");
    hFrame->AddFrame(tset, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 

    ++cnt;
  }


  hFrame = new TGHorizontalFrame(fV2); 
  TGTextButton * previous = new TGTextButton(hFrame, "Previous");
  previous->Connect("Clicked()", "PixTab", this, "previousHistogram()");
  hFrame->AddFrame(previous, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 5, 5, 3, 4));

  TGTextButton * next = new TGTextButton(hFrame, "Next");
  next->Connect("Clicked()", "PixTab", this, "nextHistogram()");
  hFrame->AddFrame(next, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 5, 5, 3, 4));

  TGTextButton * update = new TGTextButton(hFrame, "Update");
  update->Connect("Clicked()", "PixTab", this, "update()");
  hFrame->AddFrame(update, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 5, 5, 3, 4));

  TGTextButton * clear = new TGTextButton(hFrame, "Clear");
  clear->Connect("Clicked()", "PixTab", this, "clearCanvas()");
  hFrame->AddFrame(clear, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 5, 5, 3, 4));

  fV2->AddFrame(hFrame, new TGLayoutHints(kLHintsLeft | kLHintsBottom, 5, 5, 3, 4));


  hFrame = new TGHorizontalFrame(fV2); 
  // -- create doTest Button
  TGTextButton *bDoTest = new TGTextButton(hFrame, " doTest ", B_DOTEST);
  bDoTest->ChangeOptions(bDoTest->GetOptions() | kFixedWidth);
  hFrame->AddFrame(bDoTest, new TGLayoutHints(kLHintsLeft | kLHintsTop, 2, 20, 2, 2));
  bDoTest->Connect("Clicked()", "PixTest", test, "doTest()");
  
  // -- create print Button
  TGTextButton *bPrint = new TGTextButton(hFrame, " print ", B_PRINT);
  bPrint->ChangeOptions(bPrint->GetOptions() | kFixedWidth);
  hFrame->AddFrame(bPrint, new TGLayoutHints(kLHintsLeft | kLHintsTop, 2, 20, 2, 2));
  bPrint->Connect("Clicked()", "PixTab", this, "handleButtons(Int_t)");
  
  // -- create close Button
  TGTextButton *bClose = new TGTextButton(hFrame, " close ", B_CLOSETAB);
  bClose->ChangeOptions(bClose->GetOptions() | kFixedWidth);
  hFrame->AddFrame(bClose, new TGLayoutHints(kLHintsRight | kLHintsTop, 2, 20, 2, 2));
  bClose->Connect("Clicked()", "PixTab", this, "handleButtons(Int_t)");
  
  fV2->AddFrame(hFrame, new TGLayoutHints(kLHintsLeft | kLHintsBottom, 5, 5, 3, 4));

//   fhFrame->MapSubwindows();
//   fhFrame->Resize(fhFrame->GetDefaultSize());
//   fhFrame->MapWindow();

  fTabFrame->AddFrame(fhFrame, new TGLayoutHints(kLHintsRight | kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 5));
  fTabFrame->MapSubwindows();
  fTabFrame->Resize(fTabFrame->GetDefaultSize());
  fTabFrame->MapWindow();

}


// ----------------------------------------------------------------------
PixTab::PixTab() {
  init(0, 0, "nada");
}

// ----------------------------------------------------------------------
void PixTab::init(PixGui *p, PixTest *test, string tabname) {
  //  LOG(logINFO) << "PixTab::init()";
  fGui = p;
  fTest = test; 
  fTabName = tabname; 
  fCount = 0; 
}

// ----------------------------------------------------------------------
// PixTab destructor
PixTab::~PixTab() {
  //  LOG(logINFO) << "PixTab destructor";
}


// ----------------------------------------------------------------------
void PixTab::handleButtons(Int_t id) {

  if (!fGui->getTabs()) return;

  if (id == -1) {
    TGButton *btn = (TGButton*)gTQSender;
    id = btn->WidgetId();
  }
  
  switch (id) {
    case B_DOTEST: {
      LOG(logINFO) << "and now what???";
    }

    case B_PRINT: {
      fCount++;
      LOG(logINFO) << Form("Canvas printed to %s-%d.pdf", fTabName.c_str(), fCount);
      fEc1->GetCanvas()->SaveAs(Form("%s-%d.pdf", fTabName.c_str(), fCount));
      break;
    }

    case B_CLOSETAB: {
      delete fEc1; fEc1 = 0;
      LOG(logINFO) << Form("Tab %s closed", fTabName.c_str());
      fGui->getTabs()->RemoveTab(fGui->getTabs()->GetCurrent());
      fGui->getTabs()->Layout();
      delete fTabFrame; 
      delete fhFrame; 
      delete fV1; 
      delete fV2; 
      delete this; 
      break;
    }
  }
}


// ----------------------------------------------------------------------
void PixTab::setParameter() {
  if (!fGui->getTabs()) return;
  //  LOG(logINFO)  << "PixTab::setParameter: ";

  TGButton *btn = (TGButton *) gTQSender;
  int id(-1); 
  id = btn->WidgetId();
  if (-1 == id) {
    LOG(logINFO) << "ASLFDKHAPIUDF ";
    return; 
  }

  string svalue = ((TGTextEntry*)(fParTextEntries[fParIds[id]]))->GetText(); 
  
  LOG(logINFO) << "xxxPressed():  ID = " << id 
	       << " -> " << fParIds[id] 
	       << " to value " << svalue;

  fTest->setParameter(fParIds[id], svalue); 

  if (1) fTest->dumpParameters();
  
} 


// ----------------------------------------------------------------------
void PixTab::clearCanvas() {
  TCanvas *c = fEc1->GetCanvas();
  c->Clear(); 
  update();
}

// ----------------------------------------------------------------------
void PixTab::nextHistogram() {
  TH1 *h = fTest->nextHist(); 
  if (h) {
    h->Draw("colz");
    update(); 
  } else {
    LOG(logINFO) << "no previous histogram found ";
  }

}


// ----------------------------------------------------------------------
void PixTab::previousHistogram() {
  TH1 *h = fTest->previousHist(); 
  if (h) {
    h->Draw("colz");
    update(); 
  } else {
    LOG(logINFO)  << "no previous histogram found ";
  }
}


// ----------------------------------------------------------------------
void PixTab::update() {
  TCanvas *c = fEc1->GetCanvas();
  c->cd();
  c->Modified(); 
  c->Update(); 
}

