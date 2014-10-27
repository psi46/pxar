#include "PixMonitor.hh"
#include "log.h"

#include "PixGui.hh"
#include "PixSetup.hh"

using namespace std;
using namespace pxar; 

// ----------------------------------------------------------------------
PixMonitor::PixMonitor(TGGroupFrame *f, PixGui *pixGui) {
  fGui = pixGui; 
  fMonitorFrame = new TGVerticalFrame(f);
  fHFrame1 = new TGHorizontalFrame(fMonitorFrame);
  fHFrame2 = new TGHorizontalFrame(fMonitorFrame);

  TGString *a = new TGString("I(ana)    ");
  TGString *d = new TGString("I(digi)    ");

  fAna = new TGLabel(fHFrame1, a);
  fDigi = new TGLabel(fHFrame2, d);

  fNmrAna = new TGTextEntry(fHFrame1, fAnaFileBuffer = new TGTextBuffer(50));
  fNmrAna->SetWidth(50);
  fNmrAna->SetToolTipText(Form("Total analog current drawn by %s", (fGui->getPixSetup()->getConfigParameters()->getNrocs()>1?"module":"ROC")));
  fNmrDigi = new TGTextEntry(fHFrame2, fDigiFileBuffer = new TGTextBuffer(50));
  fNmrDigi->SetWidth(50);
  fNmrDigi->SetToolTipText(Form("Total digital current drawn by %s", (fGui->getPixSetup()->getConfigParameters()->getNrocs()>1?"module":"ROC")));

  fAnaButton = new TGTextButton(fHFrame1," Draw ", B_DRAWANA);
  fAnaButton->SetToolTipText("not yet implemented");
  fAnaButton->ChangeOptions(fAnaButton->GetOptions() | kFixedWidth);
  fAnaButton->Connect("Clicked()", "PixMonitor", this, "handleButtons()");

  fDigiButton = new TGTextButton(fHFrame2," Draw ", B_DRAWDIGI);
  fDigiButton->SetToolTipText("not yet implemented");
  fDigiButton->ChangeOptions(fDigiButton->GetOptions() | kFixedWidth);
  fDigiButton->Connect("Clicked()", "PixMonitor", this, "handleButtons()");

  if ("fpix" == fGui->getHdiType()) {
    TGString *temperature_degree = new TGString("T (deg C) ");
    fHFrame_TDegree = new TGHorizontalFrame(fMonitorFrame);
    fTemperatureDegree = new TGLabel(fHFrame_TDegree, temperature_degree);
    fNmrTDegree = new TGTextEntry(fHFrame_TDegree, fTDegreeFileBuffer = new TGTextBuffer(40));
    fNmrTDegree->SetWidth(40);
    fHFrame_TDegree->AddFrame(fTemperatureDegree, new TGLayoutHints(kLHintsTop | kLHintsLeft,2,2,2,2));
    fHFrame_TDegree->AddFrame(fNmrTDegree, new TGLayoutHints(kLHintsTop | kLHintsLeft,2,2,2,2));
  }

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


  if ("fpix" == fGui->getHdiType()) {
    fMonitorFrame->AddFrame(fHFrame_TDegree, new TGLayoutHints(kLHintsTop | kLHintsExpandX,1,1,1,1));
  }

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
    ia = static_cast<float>(fGui->getApi()->getTBia());
    id = static_cast<float>(fGui->getApi()->getTBid());
  } else {
    ia += 1.0; 
    id += 1.0; 
  }

  fNmrAna->SetText(Form("%4.3f", ia));
  fNmrDigi->SetText(Form("%4.3f", id));

  if ("fpix" == fGui->getHdiType()) {
    if (fGui->getApi()) {
      uint16_t v_ref =  fGui->getApi()->GetADC(5);
      uint16_t v_val =  fGui->getApi()->GetADC(4);
      fNmrTDegree->SetText(Form("%3.1f", (-(v_val - v_ref) - 0.92) / 6.55));
    } else {
      fNmrTDegree->SetText(Form("---"));
    }
  }

}


// ----------------------------------------------------------------------
string PixMonitor::stringify(int x) {
  ostringstream o;
  o << x;
  return o.str();

}
