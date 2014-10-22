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


//  // - - - - - Satoshi - - - - - -
//  TGString *temperature_ref = new TGString("T_ref");
//  TGString *temperature_val = new TGString("T_val");
//
//  fHFrame_Tref = new TGHorizontalFrame(fMonitorFrame) ;
//  fHFrame_Tval = new TGHorizontalFrame(fMonitorFrame) ;
//
//  fTemperatureRef = new TGLabel(fHFrame_Tref, temperature_ref);
//  fTemperatureVal = new TGLabel(fHFrame_Tval, temperature_val);
//
//  fNmrTRef = new TGTextEntry(fHFrame_Tref, fTRefFileBuffer = new TGTextBuffer(100));
//  fNmrTRef->SetWidth(100);
//  fNmrTVal = new TGTextEntry(fHFrame_Tval, fTValFileBuffer = new TGTextBuffer(100));
//  fNmrTVal->SetWidth(100);
//
//  fHFrame_Tref -> AddFrame( fTemperatureRef , new TGLayoutHints(kLHintsTop | kLHintsLeft,2,2,2,2) );
//  fHFrame_Tref -> AddFrame( fNmrTRef , new TGLayoutHints(kLHintsTop | kLHintsLeft,2,2,2,2) );
//  fHFrame_Tval -> AddFrame( fTemperatureVal , new TGLayoutHints(kLHintsTop | kLHintsLeft,2,2,2,2) );
//  fHFrame_Tval -> AddFrame( fNmrTVal , new TGLayoutHints(kLHintsTop | kLHintsLeft,2,2,2,2) );
//
//  TGString *temperature_diff = new TGString("ADC diff");
//  fHFrame_TDiff = new TGHorizontalFrame(fMonitorFrame) ;
//  fTemperatureDiff = new TGLabel(fHFrame_TDiff, temperature_diff );
//  fNmrTDiff = new TGTextEntry(fHFrame_TDiff, fTDiffFileBuffer = new TGTextBuffer(100));
//  fNmrTDiff->SetWidth(100);
//  fHFrame_TDiff -> AddFrame( fTemperatureDiff , new TGLayoutHints(kLHintsTop | kLHintsLeft,2,2,2,2) );
//  fHFrame_TDiff -> AddFrame( fNmrTDiff , new TGLayoutHints(kLHintsTop | kLHintsLeft,2,2,2,2) );


  if( fGui -> GetHdiType() == "fpix" ){
    printf("this is fpixe");
  TGString *temperature_degree = new TGString("Temperature");
  fHFrame_TDegree = new TGHorizontalFrame(fMonitorFrame) ;
  fTemperatureDegree = new TGLabel(fHFrame_TDegree, temperature_degree );
  fNmrTDegree = new TGTextEntry(fHFrame_TDegree, fTDegreeFileBuffer = new TGTextBuffer(100));
  fNmrTDegree->SetWidth(100);
  fHFrame_TDegree -> AddFrame( fTemperatureDegree , new TGLayoutHints(kLHintsTop | kLHintsLeft,2,2,2,2) );
  fHFrame_TDegree -> AddFrame( fNmrTDegree , new TGLayoutHints(kLHintsTop | kLHintsLeft,2,2,2,2) );
//  /// - - -- - - - - - - satoshi - - - - - - - - - 
  }else{
      printf("this is NOT fpixe");
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


  /// - -- satoshi 
//  fMonitorFrame->AddFrame(fHFrame_Tref , new TGLayoutHints(kLHintsTop | kLHintsExpandX,1,1,1,1));
//  fMonitorFrame->AddFrame(fHFrame_Tval , new TGLayoutHints(kLHintsTop | kLHintsExpandX,1,1,1,1));
//  fMonitorFrame->AddFrame(fHFrame_TDiff, new TGLayoutHints(kLHintsTop | kLHintsExpandX,1,1,1,1));
  if( fGui -> GetHdiType() == "fpix" ){
    fMonitorFrame->AddFrame(fHFrame_TDegree, new TGLayoutHints(kLHintsTop | kLHintsExpandX,1,1,1,1));
  }
  // - - - -satoshi - - -  

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

  fNmrAna->SetText(Form("%4.3f",ia));
  fNmrDigi->SetText(Form("%4.3f",id));


  if( fGui -> GetHdiType() == "fpix" ){
    LOG(logINFO) << "this ls fpix";
  }else{
    LOG(logINFO) << "this ls not fpix";
  }


  if( fGui -> GetHdiType() == "fpix" ){
  if (fGui->getApi()) {

    LOG(logINFO) << "in if condition";

    uint16_t v_ref =  fGui->getApi()->GetADC( 5 ) ;
    uint16_t v_val =  fGui->getApi()->GetADC( 4 ) ;

//     fNmrTRef ->SetText(Form("%4u" ,v_ref ) );
//     fNmrTVal ->SetText(Form("%4u" ,v_val ) );
//     fNmrTDiff->SetText(Form("%4i" ,(int)v_val - v_ref ) );
    fNmrTDegree->SetText(Form("%4f" , ( - ( v_val - v_ref ) - 0.92 ) / 6.55 ) ) ;
    

  } else {
//    fNmrTRef ->SetText(Form("---"));
//    fNmrTVal ->SetText(Form("---"));
//    fNmrTDiff->SetText(Form("---"));
    fNmrTDegree->SetText(Form("---"));
  }
  } // end of if "HDI == FPIX"

  LOG(logINFO) << "satoshi update - done";

}


// ----------------------------------------------------------------------
string PixMonitor::stringify(int x) {
  ostringstream o;
  o << x;
  return o.str();

}
