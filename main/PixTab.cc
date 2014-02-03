#include <iostream>

#include <TApplication.h>
#include <TGButton.h>
#include <TGToolTip.h>
#include <TRandom.h>
#include <TSystem.h>
#include <TCanvas.h>
#include <TGTab.h>
#include <TGLabel.h>
#include <TH2.h>
#include <TStyle.h>

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
  fV1->AddFrame(fEc1, new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX | kLHintsExpandY, 0, 0, 10, 0));


  // -- status bar
  Int_t wid = fEc1->GetCanvasWindowId();  
  TCanvas *myc = new TCanvas(Form("%sCanvas", tabname.c_str()), 10, 10, wid);
  fEc1->AdoptCanvas(myc);
  myc->Connect("ProcessedEvent(Int_t,Int_t,Int_t,TObject*)","PixTab",this, "statusBarUpdate(Int_t,Int_t,Int_t,TObject*)");

  Int_t parts[] = {45, 55};
  fStatusBar = new TGStatusBar(fV1, 50, 10, kVerticalFrame);
  fStatusBar->SetParts(parts, 2);
  fStatusBar->Draw3DCorner(kFALSE);
  fV1->AddFrame(fStatusBar, new TGLayoutHints(kLHintsExpandX, 0, 0, 10, 0));

  // -- fV2: create parameter TGText boxes for test
  map<string, string> amap = fTest->getParameters();
  TGTextEntry *te(0); 
  TGLabel *tl(0); 
  TGTextBuffer *tb(0); 
  TGTextButton *tset(0); 

  TGHorizontalFrame *hFrame(0); 

  int cnt(0); 
  for (map<string, string>::iterator imap = amap.begin(); imap != amap.end(); ++imap) {  
    if (!imap->second.compare("button")) {
      continue;
    }

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
    tset->SetToolTipText("set the parameter\nor click *return* after changing the numerical value");
    tset->GetToolTip()->SetDelay(2000); // add a bit of delay to ease button hitting

    ++cnt;
  }

  // -- add buttons
  cnt = 1000; 
  for (map<string, string>::iterator imap = amap.begin(); imap != amap.end(); ++imap) {  
    if (!imap->second.compare("button")) {
      hFrame = new TGHorizontalFrame(fV2, 300, 30, kLHintsExpandX); 
      tset = new TGTextButton(hFrame, imap->first.c_str(), cnt);
      hFrame->AddFrame(tset, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 2, 2, 2)); 
      tset->SetToolTipText("run this test");
      tset->GetToolTip()->SetDelay(2000); // add a bit of delay to ease button hitting
      tset->Connect("Clicked()", "PixTab", this, "buttonClicked()");
      fV2->AddFrame(hFrame, new TGLayoutHints(kLHintsRight | kLHintsTop));
      ++cnt; 
    }
  }

  hFrame = new TGHorizontalFrame(fV2); 
  TGTextButton * previous = new TGTextButton(hFrame, "Previous");
  previous->SetToolTipText("display previous histogram in this test's list");
  previous->Connect("Clicked()", "PixTab", this, "previousHistogram()");
  hFrame->AddFrame(previous, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 5, 5, 3, 4));

  TGTextButton * next = new TGTextButton(hFrame, "Next");
  next->SetToolTipText("display next histogram in this test's list");
  next->Connect("Clicked()", "PixTab", this, "nextHistogram()");
  hFrame->AddFrame(next, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 5, 5, 3, 4));

  TGTextButton * update = new TGTextButton(hFrame, "Update");
  update->SetToolTipText("update canvas");
  update->Connect("Clicked()", "PixTab", this, "update()");
  hFrame->AddFrame(update, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 5, 5, 3, 4));

  TGTextButton * clear = new TGTextButton(hFrame, "Clear");
  clear->SetToolTipText("clear canvas");
  clear->Connect("Clicked()", "PixTab", this, "clearCanvas()");
  hFrame->AddFrame(clear, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, 5, 5, 3, 4));

  fV2->AddFrame(hFrame, new TGLayoutHints(kLHintsLeft | kLHintsBottom, 5, 5, 3, 4));


  hFrame = new TGHorizontalFrame(fV2); 
  // -- create doTest Button
  fbDoTest = new TGTextButton(hFrame, " doTest ", B_DOTEST);
  fbDoTest->ChangeOptions(fbDoTest->GetOptions() | kFixedWidth);
  hFrame->AddFrame(fbDoTest, new TGLayoutHints(kLHintsLeft | kLHintsTop, 2, 20, 2, 2));
  fbDoTest->Connect("Clicked()", "PixTest", test, "doTest()");
  
  // -- create stop Button
  TGTextButton *bStop = new TGTextButton(hFrame, " stop ", B_DOSTOP);
  bStop->SetToolTipText("not yet implemented (should interrupt the test at a convenient place)");
  bStop->ChangeOptions(bStop->GetOptions() | kFixedWidth);
  hFrame->AddFrame(bStop, new TGLayoutHints(kLHintsLeft | kLHintsTop, 2, 20, 2, 2));
  bStop->Connect("Clicked()", "PixTab", this, "handleButtons(Int_t)");

  // -- create print Button
  TGTextButton *bPrint = new TGTextButton(hFrame, " print ", B_PRINT);
  bPrint->SetToolTipText("create a pdf of the canvas");
  bPrint->ChangeOptions(bPrint->GetOptions() | kFixedWidth);
  hFrame->AddFrame(bPrint, new TGLayoutHints(kLHintsLeft | kLHintsTop, 2, 20, 2, 2));
  bPrint->Connect("Clicked()", "PixTab", this, "handleButtons(Int_t)");

  // -- create module map button
  fbModMap = new TGTextButton(hFrame, " summary ", B_MODMAP);
  fbModMap->ChangeOptions(fbModMap->GetOptions() | kFixedWidth);
  hFrame->AddFrame(fbModMap, new TGLayoutHints(kLHintsLeft | kLHintsTop, 2, 20, 2, 2));
  fbModMap->Connect("Clicked()", "PixTab", this, "handleButtons(Int_t)");
  
  // -- create close Button
  TGTextButton *bClose = new TGTextButton(hFrame, " close ", B_CLOSETAB);
  bClose->ChangeOptions(bClose->GetOptions() | kFixedWidth);
  bClose->SetToolTipText("close the test tab");
  hFrame->AddFrame(bClose, new TGLayoutHints(kLHintsRight | kLHintsTop, 2, 20, 2, 2));
  bClose->Connect("Clicked()", "PixTab", this, "handleButtons(Int_t)");
  
  fV2->AddFrame(hFrame, new TGLayoutHints(kLHintsLeft | kLHintsBottom, 5, 5, 3, 4));

  updateToolTips();

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
      LOG(logDEBUG) << "and now what???";
      break;
    }

    case B_DOSTOP: {
      LOG(logDEBUG) << "and now what???";
      break;
    }

    case B_MODMAP: {
      fTest->moduleMap("what");
      break;
    }

    case B_PRINT: {
      fCount++;
      LOG(logINFO) << Form("Canvas printed to %s-%d.pdf", fTabName.c_str(), fCount);
      fEc1->GetCanvas()->SaveAs(Form("%s-%d.pdf", fTabName.c_str(), fCount));
      break;
    }

    case B_CLOSETAB: {
      delete fEc1; fEc1 = 0;
      LOG(logDEBUG) << Form("Tab %s closed", fTabName.c_str());
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
void PixTab::buttonClicked() {

  if (!fGui->getTabs()) return;

  TGButton *btn = (TGButton*)gTQSender;
  fTest->runCommand(btn->GetTitle()); 

} 

// ----------------------------------------------------------------------
void PixTab::setParameter() {
  if (!fGui->getTabs()) return;
  //  LOG(logINFO)  << "PixTab::setParameter: ";

  TGButton *btn = (TGButton *) gTQSender;
  int id(-1); 
  id = btn->WidgetId();
  if (-1 == id) {
    LOG(logDEBUG) << "ASLFDKHAPIUDF ";
    return; 
  }

  string svalue = ((TGTextEntry*)(fParTextEntries[fParIds[id]]))->GetText(); 
  
  LOG(logDEBUG) << "xxxPressed():  ID = " << id 
	       << " -> " << fParIds[id] 
	       << " to value " << svalue;

  fTest->setParameter(fParIds[id], svalue); 

  if (1) fTest->dumpParameters();
  updateToolTips();
} 


// ----------------------------------------------------------------------
void PixTab::clearCanvas() {
  TCanvas *c = fEc1->GetCanvas();
  c->Clear(); 
  update();
}

// ----------------------------------------------------------------------
void PixTab::nextHistogram() {
  clearCanvas();

  TH1 *h = fTest->nextHist(); 
  if (h) {
    if (h->InheritsFrom(TH2::Class())) {
      h->Draw("colz");
    } else {
      h->Draw();
    }
    update(); 
  } else {
    LOG(logDEBUG) << "no previous histogram found ";
  }

}


// ----------------------------------------------------------------------
void PixTab::previousHistogram() {
  clearCanvas();
  TH1 *h = fTest->previousHist(); 
  if (h) {
    if (h->InheritsFrom(TH2::Class())) {
      h->Draw("colz");
    } else {
      h->Draw();
    }
    update(); 
  } else {
    LOG(logDEBUG)  << "no previous histogram found ";
  }
}


// ----------------------------------------------------------------------
void PixTab::update() {
  TCanvas *c = fEc1->GetCanvas();
  TList* l = c->GetListOfPrimitives();
  TIter next(l);
  while (TObject *h = next()) {
    if (h->InheritsFrom(TH1::Class())) {
      if (h->InheritsFrom(TH2::Class())) {
	gStyle->SetOptStat(0); 
	h->Draw("colz"); // this is maybe not required
	break;
      }	else {
	gStyle->SetOptStat(1111111); 
	h->Draw();
	break;
      }
    }
  }
  
  c->cd();
  c->Modified(); 
  c->Update(); 
}


// ----------------------------------------------------------------------
void PixTab::statusBarUpdate(Int_t event, Int_t px, Int_t py, TObject *selected) {
  const char *text0, *text2;
  text0 = selected->GetName();
  fStatusBar->SetText(text0, 0);
  if (event == kKeyPress) {
    LOG(logDEBUG) << "key pressed?"; 
  }
  //     sprintf(text1, "%d,%d", px, py);
  //   fStatusBar->SetText(text1, 1);
  if (selected->InheritsFrom(TH1::Class())) {
    string trafo = selected->GetObjectInfo(px,py);
    string::size_type s1 = trafo.find("binx"); 
    string trafo1 = trafo.substr(s1).c_str(); 
    float x, y, val; 
    if (selected->InheritsFrom(TH2::Class())) {
      sscanf(trafo1.c_str(), "binx=%f, biny=%f, binc=%f", &x, &y, &val);
      if (52 == ((TH2D*)selected)->GetNbinsX() && 80 == ((TH2D*)selected)->GetNbinsY()) {
	text2 = Form("c=%.0f, r=%.0f, value=%4.3f", x-1, y-1, val); 
      } else if (160 == ((TH2D*)selected)->GetNbinsX() && 416 == ((TH2D*)selected)->GetNbinsY()) {
	text2 = Form("x=%.0f, y=%.0f, value=%4.3f", x-1, y-1, val); 
      } else {
	text2 = trafo.c_str(); 
      }
    } else {
      sscanf(trafo1.c_str(), "binx=%f, binc=%f", &x, &val);
      text2 = Form("x=%.0f, value=%4.3f", x-1, val); 
    }
  } else {
    text2 = selected->GetObjectInfo(px,py);
  }
  fStatusBar->SetText(text2, 1);
}


// ----------------------------------------------------------------------
void PixTab::updateToolTips() {
  string tooltip = string(Form("%s test algorithm (patience may be required)", fTest->getName().c_str())) 
    + string("\n") + fTest->getTestTip()
    ;

  fbDoTest->SetToolTipText(tooltip.c_str()); 
  
  tooltip = string(Form("summary plot for %s", fTest->getName().c_str())) 
    + string("\n") + fTest->getSummaryTip()
    ;
  fbModMap->SetToolTipText(tooltip.c_str());
  
}
