#include "PixMonitor.hh"
#include "log.h"

#include "PixGui.hh"

using namespace std;
using namespace pxar; 

// ----------------------------------------------------------------------
PixMonitor::PixMonitor(TGGroupFrame *f, PixGui *pixGui) {
  fGui = pixGui; 
  fMonitorFrame = new TGVerticalFrame(f);
  fHFrame1 = new TGHorizontalFrame(fMonitorFrame);
  fHFrame2 = new TGHorizontalFrame(fMonitorFrame);

  TGString *a = new TGString("I(ana)");
  TGString *d = new TGString("I(digi)");

  fAna = new TGLabel(fHFrame1, a);
  fDigi = new TGLabel(fHFrame2, d);

  fNmrAna = new TGTextEntry(fHFrame1, fAnaFileBuffer = new TGTextBuffer(100));
  fNmrAna->SetToolTipText(Form("Total analog current drawn by %s", (fGui->getPixSetup()->getConfigParameters()->getNrocs()>1?"module":"ROC")));
  fNmrDigi = new TGTextEntry(fHFrame2, fDigiFileBuffer = new TGTextBuffer(100));
  fNmrAna->SetWidth(100);
  fNmrDigi->SetWidth(100);
  fNmrDigi->SetToolTipText(Form("Total digital current drawn by %s", (fGui->getPixSetup()->getConfigParameters()->getNrocs()>1?"module":"ROC")));

  fAnaButton = new TGTextButton(fHFrame1," Draw ", B_DRAWANA);
  fAnaButton->SetToolTipText("not yet implemented");
  fAnaButton->ChangeOptions(fAnaButton->GetOptions() | kFixedWidth);
  fAnaButton->Connect("Clicked()", "PixMonitor", this, "handleButtons()");

  fDigiButton = new TGTextButton(fHFrame2," Draw ", B_DRAWDIGI);
  fDigiButton->SetToolTipText("not yet implemented");
  fDigiButton->ChangeOptions(fDigiButton->GetOptions() | kFixedWidth);
  fDigiButton->Connect("Clicked()", "PixMonitor", this, "handleButtons()");

  fActTime = time(NULL);
  fTimeinfo = localtime(&fActTime);
  
  fHFrame1->AddFrame(fAna, new TGLayoutHints(kLHintsTop | kLHintsLeft,2,2,2,2));
  fHFrame1->AddFrame(fNmrAna, new TGLayoutHints(kLHintsTop | kLHintsLeft,2,2,2,2));
  fHFrame1->AddFrame(fAnaButton, new TGLayoutHints(kLHintsTop | kLHintsLeft,2,2,2,2));
  fHFrame2->AddFrame(fDigi, new TGLayoutHints(kLHintsTop | kLHintsLeft,2,2,2));
  fHFrame2->AddFrame(fNmrDigi, new TGLayoutHints(kLHintsTop | kLHintsLeft,2,2,2));
  fHFrame2->AddFrame(fDigiButton, new TGLayoutHints(kLHintsTop | kLHintsLeft,2,2,2));
 
  fMonitorFrame->AddFrame(fHFrame1, new TGLayoutHints(kLHintsTop | kLHintsExpandX,1,1,1,1));
  fMonitorFrame->AddFrame(fHFrame2, new TGLayoutHints(kLHintsTop | kLHintsExpandX,1,1,1,1));

  f->AddFrame(fMonitorFrame, new TGLayoutHints(kLHintsTop,2,2,2,2));

}


// ----------------------------------------------------------------------
PixMonitor::~PixMonitor() {
}


// ----------------------------------------------------------------------
void PixMonitor::handleButtons(Int_t id) {
  if(id == -1) {
    TGButton *btn = (TGButton *) gTQSender;
    id = btn->WidgetId();
  }

  // timer initializing
  fActTime = time(NULL);
  fTimeinfo = gmtime (&fActTime);

  switch(id) {
  case B_DRAWANA:
    LOG(logINFO) << "Draw ana XXX FIXME IMPLEMENT THIS XXX";
    break;
  case B_DRAWDIGI:
    LOG(logINFO) << "Draw digi XXX FIXME IMPLEMENT THIS XXX";
    break;
  default:
    LOG(logINFO) << "Something went wrong in the PixMonitor::handleButons method!";
   break;
  }
}

// ----------------------------------------------------------------------
void PixMonitor::Update() {
  static float ia(0.), id(0.); 
  if (fGui->getApi()) {
    ia = fGui->getApi()->getTBia();
    id = fGui->getApi()->getTBid();
  } else {
    ia += 1.0; 
    id += 1.0; 
  }

  fNmrAna->SetText(Form("%4.3f",ia));
  fNmrDigi->SetText(Form("%4.3f",id));

}


// ----------------------------------------------------------------------
string PixMonitor::stringify(int x) {
  ostringstream o;
  o << x;
  return o.str();

}
