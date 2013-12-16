#include "PixGui.hh"
#include "PixTab.hh"

#include "PixTestAlive.hh"
#include "PixTestGainCalibration.hh"
 
using namespace std;

ClassImp(PixGui)

// ----------------------------------------------------------------------
PixGui::PixGui( const TGWindow *p, UInt_t w, UInt_t h, PixSetup *setup) : 
  TGMainFrame(p, w, h, kVerticalFrame) {

  ULong_t red;
  gClient->GetColorByName("red", red);

  fTb = setup->getTBInterface();
  fConfigParameters = setup->getConfigParameters();
  fTestParameters = setup->getPixTestParameters(); 

  fTb->HVoff(); 
  fTb->Poff(); 
  fPower = false;
  fHV = false;

  // create the frames
  fhFrame = new TGVerticalFrame(this, w, h);
  fH1 = new TGHorizontalFrame(fhFrame, 600, 200, kFixedHeight);
  fH2 = new TGHorizontalFrame(fhFrame, 600, 1200);

  TGCompositeFrame *topFrame = new TGCompositeFrame(fH1, w, 0.3*h);
  topFrame->SetName("topFrame");

  TGHorizontalFrame *topFrame1 = new TGHorizontalFrame(topFrame,w,0.1*h);
  topFrame1->SetName("topFrame1");

  TGHorizontalFrame *logFrame = new TGHorizontalFrame(topFrame, w, 0.1*h);
  logFrame->SetName("logFrame");

  TGVerticalFrame *leftFrame = new TGVerticalFrame(logFrame, 250, 0.1*h);
  leftFrame->SetName("leftFrame");

  TGHorizontalFrame *upFrame = new TGHorizontalFrame(leftFrame, 250, 0.05*h);
  TGHorizontalFrame *downFrame = new TGHorizontalFrame(leftFrame, 250, 0.05*h);
  
  // Loggerview
  fLogger = new TGTextView(logFrame, 500, 100, "Welcome to pxar");
  // Consoleview
  fConsole = new TGTextEntry(logFrame, fConsoleBuffer = new TGTextBuffer(100), 450, 100);
  fConsole->SetText("exec module.ini");
  fConsole->Connect("ReturnPressed()", "PixGui", this, "doSetfConsole()");

  // -- create and add widgets
  fcmbTests = new TGComboBox(upFrame, kDoubleBorder);
  fcmbTests->SetWidth(200);
  fcmbTests->SetHeight(20);
  fcmbTests->Connect("Selected(char*)","PixGui",this,"createTab(char*)");

  fcmbTests->AddEntry("Choose a test", 0);
  vector<string> tests = fTestParameters->getTests();
  for (int i = 0; i < tests.size(); ++i) {
    fcmbTests->AddEntry(tests[i].c_str(), i+1);
  }
  fcmbTests->Select(0);

  TGTextButton *exitButton = new TGTextButton(upFrame, "exit", B_EXIT);
  exitButton->ChangeOptions(exitButton->GetOptions() | kFixedWidth);
  exitButton->Connect("Clicked()", "PixGui", this, "handleButtons()");
  exitButton->Resize(70,35);


  // Frames
  TGGroupFrame *indicateFrame = new TGGroupFrame(topFrame1, "Indicators");

  TGHorizontalFrame *powerFrame = new TGHorizontalFrame(indicateFrame, 150, 75);
  powerFrame->SetName("powerFrame");

  TGHorizontalFrame *hvFrame = new TGHorizontalFrame(indicateFrame, 150,75);
  hvFrame->SetName("hvFrame");
  // Labels
  flblPower = new TGLabel(powerFrame, "Power: ");
  powerFrame->AddFrame(flblPower, new TGLayoutHints(kLHintsLeft,5,5,3,4));

  flblHV = new TGLabel(hvFrame, "HV: ");
  hvFrame->AddFrame(flblHV, new TGLayoutHints(kLHintsLeft,5,5,3,4));
  
  // Buttons
  fbtnPower = new TGTextButton(powerFrame, "Off", B_POWER);
  fbtnPower->ChangeOptions(fbtnPower->GetOptions() | kFixedWidth);
  fbtnPower->Resize(70,35);
  fbtnPower->Connect("Clicked()", "PixGui", this, "handleButtons()");
  fbtnPower->ChangeBackground(red);
  powerFrame->AddFrame(fbtnPower, new TGLayoutHints(kLHintsRight,5,5,3,4));
  
  fbtnHV = new TGTextButton(hvFrame, "Off", B_HV);
  fbtnHV->ChangeOptions(fbtnHV->GetOptions() | kFixedWidth);
  fbtnHV->Resize(70,35);
  fbtnHV->Connect("Clicked()", "PixGui", this, "handleButtons()");
  fbtnHV->ChangeBackground(red);
  hvFrame->AddFrame(fbtnHV, new TGLayoutHints(kLHintsRight,22,5,3,7));

  indicateFrame->AddFrame(powerFrame);
  indicateFrame->AddFrame(hvFrame);


  // -- Placeholders --
  // Frames
  TGHorizontalFrame *placeFrame = new TGHorizontalFrame(topFrame1);
  
  // Buttons
  TGTextButton *btnPlaceHolder = new TGTextButton(placeFrame, "PlaceHolder");
  btnPlaceHolder->ChangeOptions(btnPlaceHolder->GetOptions() | kFixedWidth);
  btnPlaceHolder->ChangeOptions(btnPlaceHolder->GetOptions() | kFixedHeight);
  btnPlaceHolder->Resize(200,78);

  placeFrame->AddFrame(btnPlaceHolder, new TGLayoutHints(kLHintsLeft,5,5,8,4));


  // -- rootfile for output --
  TGTextEntry *output = new TGTextEntry(downFrame, fRootFileNameBuffer = new TGTextBuffer(100), B_FILENAME);
  TGLabel *wOutputDirLabel = new TGLabel(downFrame, "root file:");
  wOutputDirLabel->MoveResize(130, 60, 55, output->GetDefaultHeight());

  output->SetText("bla");
  output->MoveResize(200, 60, 200, output->GetDefaultHeight());
  output->Connect("ReturnPressed()", "PixGui", this, "handleButtons()");
  

  // -- frame managing
  upFrame->AddFrame(exitButton, new TGLayoutHints(kLHintsLeft,5,5,3,4));
  upFrame->AddFrame(fcmbTests, new TGLayoutHints(kLHintsLeft,5,5,3,4));

  downFrame->AddFrame(wOutputDirLabel, new TGLayoutHints(kLHintsLeft,5,5,3,4));
  downFrame->AddFrame(output, new TGLayoutHints(kLHintsLeft,5,5,3,4));

  leftFrame->AddFrame(upFrame, new TGLayoutHints(kLHintsTop,5,5,3,4));
  leftFrame->AddFrame(downFrame, new TGLayoutHints(kLHintsTop,5,5,3,4));

  logFrame->AddFrame(leftFrame, new TGLayoutHints(kLHintsTop,2,2,2,2));
  logFrame->AddFrame(fLogger, new TGLayoutHints(kLHintsTop,2,2,2,2));
  logFrame->AddFrame(fConsole, new TGLayoutHints(kLHintsTop | kLHintsExpandX,2,2,2,2));

  topFrame1->AddFrame(indicateFrame, new TGLayoutHints(kLHintsTop,1,1,2,0));
  topFrame1->AddFrame(placeFrame, new TGLayoutHints(kLHintsTop,1,1,2,0));

  // -- tab widget
  fTabs = new TGTab(fH2, 0.7*w, 0.7*h);

  fTabs->SetTab(0);
  
  fTabs->Resize(fTabs->GetDefaultSize());
  fH2->AddFrame(fTabs, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY,2,2,2,2));
  fTabs->MoveResize(16,300,0.7*w,0.7*h);

  // -- organize the frames
  topFrame->Resize(topFrame->GetDefaultSize());
  topFrame1->Resize(topFrame1->GetDefaultSize());
  logFrame->Resize(logFrame->GetDefaultSize());
  topFrame->AddFrame(logFrame, new TGLayoutHints(kLHintsTop | kLHintsExpandX | kLHintsExpandY,2,2,2,2));
  topFrame->AddFrame(topFrame1, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX,2,2,2,2));
  fH1->AddFrame(topFrame, new TGLayoutHints(kLHintsTop | kLHintsExpandX,2,2,2,2));

  topFrame->MoveResize(8,24,696,80);
  topFrame1->MoveResize(8,80,696,136);
  logFrame->MoveResize(8,24,696,80);
  
  // -- splitter frames
  fhFrame->AddFrame(fH1, new TGLayoutHints(kLHintsTop | kLHintsExpandX));
  TGHSplitter *splitter = new TGHSplitter(fhFrame,2,2);
  splitter->SetFrame(fH1, kTRUE);
  fhFrame->AddFrame(splitter, new TGLayoutHints(kLHintsTop | kLHintsExpandX));
  fhFrame->AddFrame(fH2, new TGLayoutHints(kLHintsBottom | kLHintsExpandY | kLHintsExpandX));

  AddFrame(fhFrame, new TGLayoutHints(kLHintsRight | kLHintsExpandX | kLHintsExpandY));
 
  Resize(GetDefaultSize());
  SetWindowName("pix");
  MapSubwindows();
  MapWindow();

  //fTimer->TurnOn();

}
// ----------------------------------------------------------------------
PixGui::~PixGui() {
  cout << "PixGui::destructor" << endl;
}



// ----------------------------------------------------------------------
void PixGui::closeWindow() {
  // Close Window
  if (fTb) {
    fTb->HVoff();
    fTb->Poff();
  }
  if (fTimer) fTimer->TurnOff();
  DestroyWindow();
}
// ----------------------------------------------------------------------
void PixGui::handleButtons(Int_t id) {
  if (id == -1) {
    TGButton *btn = (TGButton *) gTQSender;
    id = btn->WidgetId();
  }
  ULong_t red;
  ULong_t green;

  gClient->GetColorByName("red", red);
  gClient->GetColorByName("green", green);
 
  switch (id) {
  case B_FILENAME: {
    cout << Form("fRootFileNameBuffer: %s", fRootFileNameBuffer->GetString()) << endl;
    break;
  }
  case B_EXIT: {
    cout << "PixGui::exit called" << endl;
    std::vector<PixTest*>::iterator il; 
    for (il = fTestList.begin(); il != fTestList.end(); ++il) {
      delete (*il); 
    } 

    //    delete this; 
    //delete fTb;
    //CloseWindow();
    gApplication->Terminate(0);
  }
  case B_POWER: {
    if(fPower == true) {
      fPower = false;
      fbtnPower->ChangeBackground(red);
      fbtnPower->SetText("Off");
      fTb->Poff(); 
      cout << "Power set Off" << endl;
    } else {
      fPower = true;
      fbtnPower->ChangeBackground(green);
      fbtnPower->SetText("On");
      fTb->Pon(); 
      cout << "Power set On" << endl;
    }
    break;
  }
  case B_HV: {
    if(fHV == true) {
      fHV = false;
      fbtnHV->ChangeBackground(red);
      fbtnHV->SetText("Off");
      fTb->HVoff(); 
      cout << "HV set Off" << endl;
    } else {
      fHV = true;
      fbtnHV->ChangeBackground(green);
      fbtnHV->SetText("On");
      fTb->HVon(); 
      cout << "HV set On" << endl;
    }
    break;  
  }
  default:
    break;
  }
}

// --------------------------------------------------------------------------------
void PixGui::createTab(char*csel) {
  string tname = csel; 
  PixTest *pt = createTest(fTb, string(csel));
  if (0 == pt) {
    cout << "ERROR: " << csel << " not known, nothing created" << endl;
    return;
  }

  fTestList.push_back(pt); 
  PixTab *t = new PixTab(this, pt, string(csel)); 
  pt->Connect("update()", "PixTab", t, "update()"); 
  MapSubwindows();
  fTabs->MoveResize(2, 0, 1500, 800);
  Resize(GetDefaultSize());
  MapWindow();
  
}

// --------------------------------------------------------------------------------
void PixGui::doSetfConsole() {

  char *p = (char*)fConsoleBuffer->GetString();
  if (p) {
      /*
    do {
      if (pSysCommand1->TargetIsTB()) {
        fTB->Execute(*fpSysCommand1);
        cout << Form(" fTB->Execute: ");
      } else  {
        fCN->Execute(*fpSysCommand1);
        cout << Form(" fCN->Execute: ");
      }

      char* s = fpSysCommand1->toString();
      cout << Form(" %s", s) << endl;
      delete s;
    } while (fpSysCommand1->Next());
  */
    cout << p << endl;
    fConsole->Clear();
  }
  // fConsoleBuffer->AddText(0,"");
}


// ----------------------------------------------------------------------
PixTest* PixGui::createTest(TBInterface *tb, string testname) {
  if (!testname.compare("PixelAlive")) return new PixTestAlive(tb, testname, fTestParameters); 
  if (!testname.compare("GainCalibration")) return new PixTestGainCalibration(tb, testname, fTestParameters); 
  return 0; 
}
