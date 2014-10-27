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
#include <TBrowser.h>

#include "PixTab.hh"
#include "log.h"

using namespace std;
using namespace pxar; 

ClassImp(PixTab)

// ----------------------------------------------------------------------
PixTab::PixTab(PixGui *p, PixTest *test, string tabname) {
  init(p, test, tabname); 

  fBorderN = 2; 
  fBorderL = 10; 
  fBorderT = 1;

  UInt_t w = fGui->getTabs()->GetWidth(); 
  UInt_t h = fGui->getTabs()->GetHeight(); 

  fTabFrame = fGui->getTabs()->AddTab(fTabName.c_str());

  //  fhFrame = new TGHorizontalFrame(fTabFrame, w, h);
  fhFrame = new TGCompositeFrame(fTabFrame, w, h, kHorizontalFrame);
  
  // -- 2 vertical frames
  fV1 = new TGVerticalFrame(fhFrame);
  fhFrame->AddFrame(fV1, new TGLayoutHints(kLHintsLeft | kLHintsExpandX | kLHintsExpandY, fBorderN, fBorderN, fBorderN, fBorderN));
  fV2 = new TGVerticalFrame(fhFrame);
  fhFrame->AddFrame(fV2, new TGLayoutHints(kLHintsRight | kLHintsExpandX | kLHintsExpandY, fBorderN, fBorderN, fBorderN, fBorderN));

  // -- fV1: create and add Embedded Canvas
  fEc1 = new TRootEmbeddedCanvas(Form("%s", tabname.c_str()), fV1, 500, 500);
  fV1->AddFrame(fEc1, new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX | kLHintsExpandY, 1, 1, 1, 1));


  // -- status bar
  Int_t wid = fEc1->GetCanvasWindowId();  
  TCanvas *myc = new TCanvas(Form("%sCanvas", tabname.c_str()), 10, 10, wid);
  fEc1->AdoptCanvas(myc);
  myc->Connect("ProcessedEvent(Int_t,Int_t,Int_t,TObject*)","PixTab",this, "statusBarUpdate(Int_t,Int_t,Int_t,TObject*)");

  Int_t parts[] = {45, 55};
  fStatusBar = new TGStatusBar(fV1, 50, 10, kVerticalFrame);
  fStatusBar->SetParts(parts, 2);
  fStatusBar->Draw3DCorner(kFALSE);
  fV1->AddFrame(fStatusBar, new TGLayoutHints(kLHintsExpandX, 1, 1, 1, 1));

  // -- fV2: create parameter TGText boxes for test
  vector<pair<string, string> > amap = fTest->getParameters();
  TGTextEntry *te(0); 
  TGLabel *tl(0); 
  TGTextBuffer *tb(0); 
  TGTextButton *tset(0);
  TGCheckButton *tcheck(0); 

  TGHorizontalFrame *hFrame(0); 

  int cnt(0); 
  for (unsigned int i = 0; i < amap.size(); ++i) {
    if (amap[i].second == "button") {
      hFrame = new TGHorizontalFrame(fV2, 300, 30, kLHintsExpandX); 
      tset = new TGTextButton(hFrame, amap[i].first.c_str(), 1234);
      hFrame->AddFrame(tset, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN)); 
      tset->SetToolTipText("run this subtest");
      tset->GetToolTip()->SetDelay(2000); // add a bit of delay to ease button hitting
      tset->Connect("Clicked()", "PixTab", this, "buttonClicked()");
      tset->SetBackgroundColor(fGui->fDarkSalmon);
      fV2->AddFrame(hFrame, new TGLayoutHints(kLHintsRight | kLHintsTop));
      continue;
    }

    if (string::npos != amap[i].second.find("checkbox")) {
      hFrame = new TGHorizontalFrame(fV2, 300, 30, kLHintsExpandX);
      tcheck = new TGCheckButton(hFrame, amap[i].first.c_str(), 1234);
      hFrame->AddFrame(tcheck, new TGLayoutHints(kLHintsRight, fBorderN, fBorderN, fBorderN, fBorderN)); 
      fV2->AddFrame(hFrame, new TGLayoutHints(kLHintsRight | kLHintsTop));
      if (string::npos != amap[i].second.find("(1)")) {
	tcheck->SetState(kButtonDown);
      } else if (string::npos != amap[i].second.find("(0)")) {
	tcheck->SetState(kButtonUp);
      } else {
	tcheck->SetState(kButtonUp);
      }
      string sTitle = tcheck->GetTitle();
      tcheck->Connect("Clicked()", "PixTab", this, "boxChecked()");
      continue;
    }


    hFrame = new TGHorizontalFrame(fV2, 300, 30, kLHintsExpandX); 
    fV2->AddFrame(hFrame, new TGLayoutHints(kLHintsRight | kLHintsTop));
    
    tb = new TGTextBuffer(5); 
    tl = new TGLabel(hFrame, amap[i].first.c_str());
    tl->SetWidth(100);
    hFrame->AddFrame(tl, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN)); 

    te  = new TGTextEntry(hFrame, tb, cnt); te->SetWidth(100); 
    hFrame->AddFrame(te, new TGLayoutHints(kLHintsCenterY | kLHintsCenterX, fBorderN, fBorderN, fBorderN, fBorderN)); 
    fParIds.push_back(amap[i].first); 
    fParTextEntries.insert(make_pair(amap[i].first, te)); 

    te->SetText(amap[i].second.c_str());
    te->Connect("ReturnPressed()", "PixTab", this, "setParameter()");
    te->Connect("TextChanged(const char*)", "PixTab", this, "yellow()"); 
    te->Connect("ShiftTabPressed()", "PixTab", this, "moveUp()"); 
    te->Connect("TabPressed()", "PixTab", this, "moveDown()"); 

    tset = new TGTextButton(hFrame, "Set", cnt);
    tset->Connect("Clicked()", "PixTab", this, "setParameter()");
    hFrame->AddFrame(tset, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN)); 
    tset->SetToolTipText("set the parameter\nor click *return* after changing the numerical value");
    tset->GetToolTip()->SetDelay(2000); // add a bit of delay to ease button hitting

    ++cnt;
  }

  hFrame = new TGHorizontalFrame(fV2); 
  TGTextButton * previous = new TGTextButton(hFrame, "Previous");
  previous->SetToolTipText("display previous histogram in this test's list");
  previous->Connect("Clicked()", "PixTab", this, "previousHistogram()");
  hFrame->AddFrame(previous, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, fBorderN, fBorderN, fBorderN, fBorderN));

  TGTextButton * next = new TGTextButton(hFrame, "Next");
  next->SetToolTipText("display next histogram in this test's list");
  next->Connect("Clicked()", "PixTab", this, "nextHistogram()");
  hFrame->AddFrame(next, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, fBorderN, fBorderN, fBorderN, fBorderN));

  TGTextButton * update = new TGTextButton(hFrame, "Update");
  update->SetToolTipText("update canvas");
  update->Connect("Clicked()", "PixTab", this, "update()");
  hFrame->AddFrame(update, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, fBorderN, fBorderN, fBorderN, fBorderN));

  TGTextButton * clear = new TGTextButton(hFrame, "Clear");
  clear->SetToolTipText("clear canvas and resest histogram list");
  clear->Connect("Clicked()", "PixTab", this, "clearHistList()");
  hFrame->AddFrame(clear, new TGLayoutHints(kLHintsCenterX | kLHintsCenterY, fBorderN, fBorderN, fBorderN, fBorderN));

  fV2->AddFrame(hFrame, new TGLayoutHints(kLHintsLeft | kLHintsBottom, fBorderN, fBorderN, fBorderN, fBorderN));


  hFrame = new TGHorizontalFrame(fV2); 
  // -- create doTest Button
  fbDoTest = new TGTextButton(hFrame, " doTest ", B_DOTEST);
  fbDoTest->ChangeOptions(fbDoTest->GetOptions() | kFixedWidth);
  hFrame->AddFrame(fbDoTest, new TGLayoutHints(kLHintsLeft | kLHintsTop, fBorderN, fBorderN, fBorderN, fBorderN));
  fbDoTest->Connect("Clicked()", "PixTest", test, "doTest()");
  fbDoTest->SetBackgroundColor(fGui->fDarkSalmon);
  
  // -- create stop Button
  TGTextButton *bStop = new TGTextButton(hFrame, " stop ", B_DOSTOP);
  bStop->SetToolTipText(fTest->getStopTip().c_str());
  bStop->ChangeOptions(bStop->GetOptions() | kFixedWidth);
  hFrame->AddFrame(bStop, new TGLayoutHints(kLHintsLeft | kLHintsTop, fBorderN, fBorderN, fBorderN, fBorderN));
  bStop->Connect("Clicked()", "PixTab", this, "handleButtons(Int_t)");

  // -- create print Button
  TGTextButton *bPrint = new TGTextButton(hFrame, " print ", B_PRINT);
  bPrint->SetToolTipText("create a pdf of the canvas");
  bPrint->ChangeOptions(bPrint->GetOptions() | kFixedWidth);
  hFrame->AddFrame(bPrint, new TGLayoutHints(kLHintsLeft | kLHintsTop, fBorderN, fBorderN, fBorderN, fBorderN));
  bPrint->Connect("Clicked()", "PixTab", this, "handleButtons(Int_t)");

  // -- create module map button
  fbModMap = new TGTextButton(hFrame, " summary ", B_MODMAP);
  fbModMap->ChangeOptions(fbModMap->GetOptions() | kFixedWidth);
  hFrame->AddFrame(fbModMap, new TGLayoutHints(kLHintsLeft | kLHintsTop, fBorderN, fBorderN, fBorderN, fBorderN));
  fbModMap->Connect("Clicked()", "PixTab", this, "handleButtons(Int_t)");

  // -- create TBrowser button
  fbBrowser = new TGTextButton(hFrame, " browser ", B_BROWSER);
  fbBrowser->ChangeOptions(fbBrowser->GetOptions() | kFixedWidth);
  fbBrowser->SetToolTipText("open a TBrowser (for easier histogram navigation)");
  hFrame->AddFrame(fbBrowser, new TGLayoutHints(kLHintsLeft | kLHintsTop, fBorderN, fBorderN, fBorderN, fBorderN));
  fbBrowser->Connect("Clicked()", "PixTab", this, "handleButtons(Int_t)");
  
  // -- create close Button
  TGTextButton *bClose = new TGTextButton(hFrame, " close ", B_CLOSETAB);
  bClose->ChangeOptions(bClose->GetOptions() | kFixedWidth);
  bClose->SetToolTipText("close the test tab");
  hFrame->AddFrame(bClose, new TGLayoutHints(kLHintsRight | kLHintsTop, fBorderN, fBorderN, fBorderN, fBorderN));
  bClose->Connect("Clicked()", "PixTab", this, "handleButtons(Int_t)");
  
  fV2->AddFrame(hFrame, new TGLayoutHints(kLHintsLeft | kLHintsBottom, fBorderN, fBorderN, fBorderN, fBorderN));

  updateToolTips();

//   fhFrame->MapSubwindows();
//   fhFrame->Resize(fhFrame->GetDefaultSize());
//   fhFrame->MapWindow();

  fTabFrame->AddFrame(fhFrame, 
		      new TGLayoutHints(kLHintsRight | kLHintsExpandX | kLHintsExpandY, fBorderN, fBorderN, fBorderN, fBorderN));
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
      fTest->runCommand("stop"); 
      LOG(logDEBUG) << "stopping...and now what???";
      break;
    }

    case B_BROWSER: {
      new TBrowser(getName().c_str(), fTest->getDirectory()); 
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
  TGButton *btn = (TGButton*)gTQSender;
  LOG(logDEBUG) << "xxxPressed():  " << btn->GetTitle();
  fTest->runCommand(btn->GetTitle()); 

} 

// ----------------------------------------------------------------------
void PixTab::boxChecked() {
  TGCheckButton *btn = (TGCheckButton*) gTQSender;
  string sTitle = btn->GetTitle();
  LOG(logDEBUG) << "xxxPressed():  " << btn->GetTitle();
  fTest->setParameter(sTitle,string(btn->IsDown()?"1":"0")) ;

}

// ----------------------------------------------------------------------
void PixTab::yellow() {
  TGButton *btn = (TGButton *) gTQSender;
  int id(-1); 
  id = btn->WidgetId();
  if (-1 == id) {
    LOG(logDEBUG) << "ASLFDKHAPIUDF ";
    return; 
  }
  
  string svalue = ((TGTextEntry*)(fParTextEntries[fParIds[id]]))->GetText(); 
  if (fTest->getParameter(fParIds[id]).compare(svalue)) {
    ((TGTextEntry*)(fParTextEntries[fParIds[id]]))->SetBackgroundColor(fGui->fYellow);
  } else {
    ((TGTextEntry*)(fParTextEntries[fParIds[id]]))->SetBackgroundColor(fGui->fWhite);
  }
}


// ----------------------------------------------------------------------
void PixTab::moveUp() {
  TGButton *btn = (TGButton *) gTQSender;
  int id(-1); 
  id = btn->WidgetId();
  if (-1 == id) {
    LOG(logDEBUG) << "ASLFDKHAPIUDF ";
    return; 
  }
  
  if (id > 0) {
    ((TGTextEntry*)(fParTextEntries[fParIds[id-1]]))->SetFocus();
  } else {
    ((TGTextEntry*)(fParTextEntries[fParIds[fParIds.size()-1]]))->SetFocus();
  }
}

// ----------------------------------------------------------------------
void PixTab::moveDown() {
  TGButton *btn = (TGButton *) gTQSender;
  int id(-1); 
  id = btn->WidgetId();
  if (-1 == id) {
    LOG(logDEBUG) << "ASLFDKHAPIUDF ";
    return; 
  }

  if (id < static_cast<int>(fParIds.size()) - 1) {
    ((TGTextEntry*)(fParTextEntries[fParIds[id+1]]))->SetFocus();
  } else {
    ((TGTextEntry*)(fParTextEntries[fParIds[0]]))->SetFocus();
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
    LOG(logDEBUG) << "ASLFDKHAPIUDF ";
    return; 
  }

  string svalue = ((TGTextEntry*)(fParTextEntries[fParIds[id]]))->GetText(); 
  ((TGTextEntry*)(fParTextEntries[fParIds[id]]))->SetBackgroundColor(fGui->fWhite);
  
  LOG(logDEBUG) << "xxxPressed():  ID = " << id 
		<< " -> " << fParIds[id]
	       << " to value " << svalue;

  fTest->setParameter(fParIds[id], svalue); 
  updateToolTips();
} 


// ----------------------------------------------------------------------
void PixTab::clearHistList() {
  clearCanvas();
  fTest->clearHistList();
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
    string option = fTest->getHistOption(h);
    if (string::npos == option.find("same")) clearCanvas();
    if (h->InheritsFrom(TH2::Class())) {
      h->Draw(option.c_str());
    } else {
      //      cout << "h: " << h->GetName() << " option: " << option << endl;
      h->Draw(option.c_str());
    }
    update(); 
  } else {
    LOG(logDEBUG) << "no previous histogram found ";
  }

}


// ----------------------------------------------------------------------
void PixTab::previousHistogram() {
  TH1 *h = fTest->previousHist(); 
  if (h) {
    string option = fTest->getHistOption(h);
    if (string::npos == option.find("same")) clearCanvas();
    if (h->InheritsFrom(TH2::Class())) {
      h->Draw(option.c_str());
    } else {
      //      cout << "h: " << h->GetName() << " option: " << option << endl;
      h->Draw(option.c_str());
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
	break;
      }	else {
	gStyle->SetOptStat(1111111); 
      }
    }
  }
  
  c->cd();
  c->Modified(); 
  c->Update(); 
}


// ----------------------------------------------------------------------
void PixTab::statusBarUpdate(Int_t event, Int_t px, Int_t py, TObject *selected) {
  //  char text0[200], text2[200];
  string text2; 
  const char* text0 = selected->GetName();
  fStatusBar->SetText(text0, 0);

  if (event == kKeyPress) {
    LOG(logDEBUG) << "key pressed?"; 
  }

  //     sprintf(text1, "%d,%d", px, py);
  //   fStatusBar->SetText(text1, 1);
  if (selected->InheritsFrom(TH1::Class())) {
    string trafo = selected->GetObjectInfo(px,py);
    string::size_type s1 = trafo.find("binx"); 
    string trafo1 = trafo.substr(s1); 
    float x, y, val; 
    if (selected->InheritsFrom(TH2::Class())) {
      sscanf(trafo1.c_str(), "binx=%f, biny=%f, binc=%f", &x, &y, &val);
      if (52 == ((TH2D*)selected)->GetNbinsX() && 80 == ((TH2D*)selected)->GetNbinsY()) {
	text2 = Form("c=%.0f, r=%.0f, value=%4.3f", x-1, y-1, val); 
      } else if (160 == ((TH2D*)selected)->GetNbinsX() && 416 == ((TH2D*)selected)->GetNbinsY()) {
	text2 = Form("x=%.0f, y=%.0f, value=%4.3f", x-1, y-1, val); 
      } else {
	text2 = trafo1;
	//	cout << "text2: " << text2 << " trafo1: " << trafo1 << endl;
      }
    } else {
      sscanf(trafo1.c_str(), "binx=%f, binc=%f", &x, &val);
      text2 = Form("x=%.0f, value=%4.3f", x-1, val); 
    }
  } else {
    text2 = selected->GetObjectInfo(px,py);
  }
  fStatusBar->SetText(text2.c_str(), 1);
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
