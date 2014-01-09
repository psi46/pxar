#include "PixGui.hh"
#include "PixTab.hh"
#include "PixParTab.hh"

#include "PixTestAlive.hh"
#include "PixTestGainCalibration.hh"
 
using namespace std;

ClassImp(PixGui)

// ----------------------------------------------------------------------
PixGui::PixGui( const TGWindow *p, UInt_t w, UInt_t h, PixSetup *setup) : 
TGMainFrame(p, w, h, kVerticalFrame), fWidth(w), fHeight(h) {

  ULong_t red;
  gClient->GetColorByName("red", red);

  fPixSetup = setup;
  fApi = fPixSetup->getApi();
  fConfigParameters = fPixSetup->getConfigParameters();
  fTestParameters = fPixSetup->getPixTestParameters(); 

//   fApi->HVoff(); 
//   fApi->Poff(); 
  fPower = true;
  fHV = true;

  // -- create the main frames: fH1 for top stuff and fH2 for tabs
  fhFrame = new TGVerticalFrame(this, w, h);
  fH1 = new TGHorizontalFrame(fhFrame, w, static_cast<int>(h*0.2), kFixedHeight);
  fH2 = new TGHorizontalFrame(fhFrame, w, static_cast<int>(h*0.75), kFixedHeight);

  TGVerticalFrame *h1v1 = new TGVerticalFrame(fH1, 0.33*w, 0.1*h);
  TGVerticalFrame *h1v2 = new TGVerticalFrame(fH1, 0.33*w, 0.1*h);
  TGVerticalFrame *h1v3 = new TGVerticalFrame(fH1, 0.33*w, 0.1*h);

  // -- create and add widgets
  fcmbTests = new TGComboBox(h1v1, kDoubleBorder);
  fcmbTests->SetWidth(200);
  fcmbTests->SetHeight(20);
  fcmbTests->Connect("Selected(char*)","PixGui",this,"createTab(char*)");

  fcmbTests->AddEntry("Choose a test", 0);
  vector<string> tests = fTestParameters->getTests();
  for (int i = 0; i < tests.size(); ++i) {
    fcmbTests->AddEntry(tests[i].c_str(), i+1);
  }
  fcmbTests->Select(0);

  TGTextButton *exitButton = new TGTextButton(h1v3, "exit", B_EXIT);
  exitButton->ChangeOptions(exitButton->GetOptions() );
  exitButton->Connect("Clicked()", "PixGui", this, "handleButtons()");
  exitButton->Resize(70,35);
  exitButton->ChangeBackground(red);


  // Frames
  TGGroupFrame *indicateFrame = new TGGroupFrame(h1v2, "Hardware control");
  
  TGHorizontalFrame *powerFrame = new TGHorizontalFrame(indicateFrame, 150, 75);
  powerFrame->SetName("powerFrame");

  TGHorizontalFrame *hvFrame = new TGHorizontalFrame(indicateFrame, 150,75);
  hvFrame->SetName("hvFrame");

  // -- power
  flblPower = new TGLabel(powerFrame, "Power: ");
  powerFrame->AddFrame(flblPower, new TGLayoutHints(kLHintsLeft,5,5,3,4));

  fbtnPower = new TGTextButton(powerFrame, "Off", B_POWER);
  //  fbtnPower->ChangeOptions(fbtnPower->GetOptions() | kFixedWidth);
  fbtnPower->Resize(70,35);
  fbtnPower->Connect("Clicked()", "PixGui", this, "handleButtons()");
  fbtnPower->ChangeBackground(red);
  powerFrame->AddFrame(fbtnPower, new TGLayoutHints(kLHintsRight,5,5,3,4));
  indicateFrame->AddFrame(powerFrame);
  
  // -- HV
  flblHV = new TGLabel(hvFrame, "HV: ");
  hvFrame->AddFrame(flblHV, new TGLayoutHints(kLHintsLeft,5,5,3,4));
  
  fbtnHV = new TGTextButton(hvFrame, "Off", B_HV);
  //  fbtnHV->ChangeOptions(fbtnHV->GetOptions() | kFixedWidth);
  fbtnHV->Resize(70,35);
  fbtnHV->Connect("Clicked()", "PixGui", this, "handleButtons()");
  fbtnHV->ChangeBackground(red);
  hvFrame->AddFrame(fbtnHV, new TGLayoutHints(kLHintsRight,22,5,3,7));

  indicateFrame->AddFrame(hvFrame);

  // FIXME add self-updating IA and ID readings

  // -- rootfile for output --
  TGTextEntry *output = new TGTextEntry(h1v1, fRootFileNameBuffer = new TGTextBuffer(100), B_FILENAME);
  TGLabel *wOutputDirLabel = new TGLabel(h1v1, "root file:");
  wOutputDirLabel->MoveResize(130, 60, 55, output->GetDefaultHeight());

  output->SetText("bla");
  output->MoveResize(200, 60, 200, output->GetDefaultHeight());
  output->Connect("ReturnPressed()", "PixGui", this, "handleButtons()");
  
  h1v1->AddFrame(wOutputDirLabel, new TGLayoutHints(kLHintsLeft,5,5,3,4));
  h1v1->AddFrame(output, new TGLayoutHints(kLHintsLeft,5,5,3,4));

  // -- frame managing
  h1v1->AddFrame(fcmbTests, new TGLayoutHints(kLHintsLeft,5,5,3,4));
  h1v2->AddFrame(indicateFrame, new TGLayoutHints(kLHintsLeft,5,5,3,4));
  h1v3->AddFrame(exitButton, new TGLayoutHints(kLHintsLeft,5,5,3,4));

//   downFrame->AddFrame(wOutputDirLabel, new TGLayoutHints(kLHintsLeft,5,5,3,4));
//   downFrame->AddFrame(output, new TGLayoutHints(kLHintsLeft,5,5,3,4));

//   leftFrame->AddFrame(upFrame, new TGLayoutHints(kLHintsTop,5,5,3,4));
//   leftFrame->AddFrame(downFrame, new TGLayoutHints(kLHintsTop,5,5,3,4));

//   logFrame->AddFrame(leftFrame, new TGLayoutHints(kLHintsTop,2,2,2,2));
//   logFrame->AddFrame(fLogger, new TGLayoutHints(kLHintsTop,2,2,2,2));
//   logFrame->AddFrame(fConsole, new TGLayoutHints(kLHintsTop | kLHintsExpandX,2,2,2,2));

//   topFrame1->AddFrame(indicateFrame, new TGLayoutHints(kLHintsTop,1,1,2,0));

  // -- tab widget
  fTabs = new TGTab(fH2, 0.7*w, 0.7*h);
  fTabs->SetTab(0);
  
  fTabs->Resize(fTabs->GetDefaultSize());
  fH2->AddFrame(fTabs, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY,2,2,2,2));
  fTabs->MoveResize(16,300,0.7*w,2.7*h);


  PixParTab *t = new PixParTab(this, "hardware parameters"); 
  MapSubwindows();
  fTabs->MoveResize(800, 800);
  MapWindow(); 

  if (0) {
    TGCompositeFrame *ctab = t->getCompositeFrame(); 
    ctab->Resize(800, 800);
    TGCompositeFrame *htab = t->getHorizontalFrame(); 
    htab->Resize(800, 800);
  }

  // -- organize the frames
//   topFrame->Resize(topFrame->GetDefaultSize());
//   topFrame1->Resize(topFrame1->GetDefaultSize());
//   logFrame->Resize(logFrame->GetDefaultSize());

//   topFrame->MoveResize(8,24,696,80);
//   topFrame1->MoveResize(8,80,696,136);
//   logFrame->MoveResize(8,24,696,80);

  fH1->AddFrame(h1v1, new TGLayoutHints(kLHintsTop | kLHintsExpandX,2,2,2,2));
  fH1->AddFrame(h1v2, new TGLayoutHints(kLHintsTop | kLHintsExpandX,2,2,2,2));
  fH1->AddFrame(h1v3, new TGLayoutHints(kLHintsTop | kLHintsExpandX,2,2,2,2));

  fhFrame->AddFrame(fH1, new TGLayoutHints(kLHintsTop | kLHintsExpandX));
  fhFrame->AddFrame(fH2, new TGLayoutHints(kLHintsBottom | kLHintsExpandY | kLHintsExpandX));

  AddFrame(fhFrame, new TGLayoutHints(kLHintsRight | kLHintsExpandX | kLHintsExpandY));
 
  Resize(GetDefaultSize());
  SetWindowName("pXar");
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
  if (fApi) {
    fApi->HVoff();
    fApi->Poff();
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
    if (fApi) delete fApi; 
    gApplication->Terminate(0);
  }
  case B_POWER: {
    if(fPower == true) {
      fPower = false;
      fbtnPower->ChangeBackground(red);
      fbtnPower->SetText("Off");
      fApi->Poff(); 
      cout << "Power set Off" << endl;
    } else {
      fPower = true;
      fbtnPower->ChangeBackground(green);
      fbtnPower->SetText("On");
      fApi->Pon(); 
      cout << "Power set On" << endl;
    }
    break;
  }
  case B_HV: {
    if(fHV == true) {
      fHV = false;
      fbtnHV->ChangeBackground(red);
      fbtnHV->SetText("Off");
      fApi->HVoff(); 
      cout << "HV set Off" << endl;
    } else {
      fHV = true;
      fbtnHV->ChangeBackground(green);
      fbtnHV->SetText("On");
      fApi->HVon(); 
      cout << "HV set On" << endl;
    }
    break;  
  }
  default:
    break;
  }
}


// --------------------------------------------------------------------------------
// FIXME needed?
void PixGui::createParTab() {
  UInt_t w = 400; 
  UInt_t h = 400; 

  fParTab = fTabs->AddTab("hardware parameters");
  fParTab->SetLayoutManager(new TGVerticalLayout(fParTab));

  // create the TopFrame
  fhFrame = new TGHorizontalFrame(fParTab, w, 0.5*h);

}


// --------------------------------------------------------------------------------
void PixGui::createTab(char*csel) {
  string tname = csel; 

  PixTest *pt = createTest(string(csel));
  if (0 == pt) {
    cout << "ERROR: " << csel << " not known, nothing created" << endl;
    return;
  }

  fTestList.push_back(pt); 
  PixTab *t = new PixTab(this, pt, string(csel)); 
  pt->Connect("update()", "PixTab", t, "update()"); 
  MapSubwindows();
  fTabs->MoveResize(2, 0, 800, 800);
  Resize(GetDefaultSize());
  MapWindow();
  cout << "csel = " << csel << endl;
  fTabs->SetTab(csel); 
  
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
PixTest* PixGui::createTest(string testname) {
  if (!testname.compare("PixelAlive")) return new PixTestAlive(fPixSetup, testname); 
  if (!testname.compare("GainCalibration")) return new PixTestGainCalibration(fPixSetup, testname); 
  return 0; 
}
