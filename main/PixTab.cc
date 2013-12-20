#include <iostream>
#include "PixTab.hh"

#include <TApplication.h>
#include <TGButton.h>
#include <TRandom.h>
#include <TSystem.h>
#include <TCanvas.h>
#include <TGTab.h>
#include <TGLabel.h>


using namespace std;

ClassImp(PixTab)

// ----------------------------------------------------------------------
PixTab::PixTab(PixGui *p, PixTest *test, string tabname) {
  init(p, test, tabname); 
  
  UInt_t w = p->getWidth(); 
  UInt_t h = p->getHeight(); 

  fTabFrame = fGui->getTabs()->AddTab(fTabName.c_str());
  fTabFrame->SetLayoutManager(new TGVerticalLayout(fTabFrame));
 
  fhFrame = new TGHorizontalFrame(fTabFrame, w, 0.75*h);
  
  // -- 2 vertical frames
  fV1 = new TGVerticalFrame(fhFrame, 0.6*w, 0.75*h, kFixedWidth);
  fhFrame->AddFrame(fV1, new TGLayoutHints(kLHintsLeft | kLHintsExpandY));
  fV2 = new TGVerticalFrame(fhFrame, 0.4*w, 0.75*h);
  fhFrame->AddFrame(fV2, new TGLayoutHints(kLHintsRight | kLHintsExpandX | kLHintsExpandY));

  // -- fV1: create and add Embedded Canvas
  TGLayoutHints *L4 = new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX | kLHintsExpandY, 15, 15, 15, 15);
  fEc1 = new TRootEmbeddedCanvas("ec1", fV1, 0.6*w, 0.75*h);
  fV1->AddFrame(fEc1, L4);


  // -- fV2: create parameter TGText boxes for test
  cout << "map tab individually for test: " << fTest->getName() << endl;
  map<string, string> amap = fTest->getParameters();
  TGTextEntry *te(0); 
  TGLabel *tl(0); 
  TGTextBuffer *tb(0); 
  TGTextButton *tset(0); 

  TGHorizontalFrame *hFrame(0); 

  int cnt(0); 
  for (map<string, string>::iterator imap = amap.begin(); imap != amap.end(); ++imap) {  
    hFrame = new TGHorizontalFrame(fV2, 300, 30, kFixedWidth); 
    fV2->AddFrame(hFrame, new TGLayoutHints(kLHintsLeft | kLHintsTop));
    cout << "Creating TGTextEntry for " << imap->first << endl;
    tb = new TGTextBuffer(20); 
    tl = new TGLabel(hFrame, imap->first.c_str());
    hFrame->AddFrame(tl, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 

    te  = new TGTextEntry(hFrame, tb, cnt); te->SetWidth(100); 
    hFrame->AddFrame(te, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 
    fParIds.push_back(imap->first); 
    fParTextEntries.insert(make_pair(imap->first, te)); 

    te->SetText(imap->second.c_str());
    te->Connect("ReturnPressed()", "PixTab", this, "setParameter()");

    tset = new TGTextButton(hFrame, "Set", cnt);
    tset->Connect("Clicked()", "PixTab", this, "setParameter()");
    hFrame->AddFrame(tset, new TGLayoutHints(kLHintsCenterY | kLHintsRight, 2, 2, 2, 2)); 

    ++cnt;
    //    break;
  }


  hFrame = new TGHorizontalFrame(fV2, 300, 50, kFixedWidth); 
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

  fV2->AddFrame(hFrame, new TGLayoutHints(kLHintsLeft | kLHintsBottom));


  hFrame = new TGHorizontalFrame(fV2, 300, 50, kFixedWidth); 
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
  
  fV2->AddFrame(hFrame, new TGLayoutHints(kLHintsLeft | kLHintsBottom));

  fTabFrame->Layout();
  fTabFrame->MapSubwindows();
  fTabFrame->MapWindow();

  fTabFrame->AddFrame(fhFrame, new TGLayoutHints(kLHintsRight | kLHintsExpandX | kLHintsExpandY));
  //  fGui->getTabs()->SetTab(tabname.c_str()); 

}


// ----------------------------------------------------------------------
PixTab::PixTab() {
  init(0, 0, "nada");
}

// ----------------------------------------------------------------------
void PixTab::init(PixGui *p, PixTest *test, std::string tabname) {
  cout << "PixTab::init()" << endl;
  fGui = p;
  fTest = test; 
  fTabName = tabname; 
  fCount = 0; 
}

// ----------------------------------------------------------------------
// PixTab destructor
PixTab::~PixTab() {
  cout << "PixTab destructor" << endl;
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
      cout << "and now what???" << endl;
    }

    case B_PRINT: {
      fCount++;
      cout << Form("Canvas printed to %s-%d.pdf", fTabName.c_str(), fCount) << endl;
      fEc1->GetCanvas()->SaveAs(Form("%s-%d.pdf", fTabName.c_str(), fCount));
      break;
    }

    case B_CLOSETAB: {
      delete fEc1; fEc1 = 0;
      cout << Form("Tab %s closed", fTabName.c_str()) << endl;
      fGui->getTabs()->RemoveTab(fGui->getTabs()->GetCurrent());
      fGui->getTabs()->Layout();
      delete this; 
      break;
    }
  }
}


// ----------------------------------------------------------------------
void PixTab::setParameter() {
  if (!fGui->getTabs()) return;
  cout << "PixTab::setParameter: " <<  endl;

  TGButton *btn = (TGButton *) gTQSender;
  int id(-1); 
  id = btn->WidgetId();
  if (-1 == id) {
    cout << "ASLFDKHAPIUDF " << endl;
    return; 
  }

  string svalue = ((TGTextEntry*)(fParTextEntries[fParIds[id]]))->GetText(); 
  
  cout << "xxxPressed():  ID = " << id 
       << " -> " << fParIds[id] 
       << " to value " << svalue
       << endl;


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
    cout << "no previous histogram found " << endl;
  }

}


// ----------------------------------------------------------------------
void PixTab::previousHistogram() {
  TH1 *h = fTest->previousHist(); 
  if (h) {
    h->Draw("colz");
    update(); 
  } else {
    cout << "no previous histogram found " << endl;
  }
}


// ----------------------------------------------------------------------
void PixTab::update() {
  TCanvas *c = fEc1->GetCanvas();
  c->Modified(); 
  c->Update(); 
}

