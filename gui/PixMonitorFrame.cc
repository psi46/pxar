#include "PixMonitorFrame.hh"
#include "log.h"

#include "PixGui.hh"
#include "PixTab.hh"
#include "PixSetup.hh"
#include "PixMonitor.hh"

using namespace std;
using namespace pxar; 

// ----------------------------------------------------------------------
PixMonitorFrame::PixMonitorFrame(TGVerticalFrame *f, PixGui *pixGui) {
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
  fAnaButton->SetToolTipText("draw analog current measurements vs time");
  fAnaButton->ChangeOptions(fAnaButton->GetOptions() | kFixedWidth);
  fAnaButton->Connect("Clicked()", "PixMonitorFrame", this, "handleButtons()");

  fDigiButton = new TGTextButton(fHFrame2," Draw ", B_DRAWDIGI);
  fDigiButton->SetToolTipText("draw digital current measurements vs time");
  fDigiButton->ChangeOptions(fDigiButton->GetOptions() | kFixedWidth);
  fDigiButton->Connect("Clicked()", "PixMonitorFrame", this, "handleButtons()");

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
PixMonitorFrame::~PixMonitorFrame() {
}


// ----------------------------------------------------------------------
void PixMonitorFrame::handleButtons(Int_t id) {
  if(id == -1) {
    TGButton *btn = (TGButton *) gTQSender;
    id = btn->WidgetId();
  }

  // timer initializing
  fActTime = time(NULL);
  fTimeinfo = gmtime (&fActTime);

  PixMonitor *a = fGui->getPixSetup()->getPixMonitor();
  switch(id) {
  case B_DRAWANA:
    a->drawHist("iana");
    if (fGui->getPixTab()) fGui->getPixTab()->update();
    //    fGui->getTabs()->SetTab(fGui->getSelectedTab()); 
    
    break;
  case B_DRAWDIGI:
    a->drawHist("idig");
    if (fGui->getPixTab()) fGui->getPixTab()->update();
    break;
  default:
    LOG(logINFO) << "Something went wrong in the PixMonitorFrame::handleButons method!";
   break;
  }
}

// ----------------------------------------------------------------------
void PixMonitorFrame::Update() {
  static float ia(0.), id(0.); 
  PixMonitor *a = fGui->getPixSetup()->getPixMonitor();
  if (fGui->getApi()) {
    a->update();
//     ia = static_cast<float>(fGui->getApi()->getTBia());
//     id = static_cast<float>(fGui->getApi()->getTBid());
    ia = static_cast<float>(a->getIana());
    id = static_cast<float>(a->getIdig());
    if (!fGui->isPowerOff()) {
      if (ia < 1e-4) {
	LOG(logERROR) << "analog current reading unphysical";
      }
      if (id < 1e-4) {
	LOG(logERROR) << "digital current reading unphysical";
      }
    }
  } else {
    ia += 1.0; 
    id += 1.0; 
  }

  fNmrAna->SetText(Form("%4.3f", ia));
  fNmrDigi->SetText(Form("%4.3f", id));

  if ("fpix" == fGui->getHdiType()) {
    if (fGui->getApi()) fNmrTDegree->SetText(Form("%3.1f", fGui->getApi()->getTemp()));
    else fNmrTDegree->SetText(Form("---"));
  }

}


// // ----------------------------------------------------------------------
// string PixMonitorFrame::stringify(int x) {
//   ostringstream o;
//   o << x;
//   return o.str();

// }
