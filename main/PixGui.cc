#include "PixGui.hh"
#include "log.h"

#include "PixTab.hh"
#include "PixParTab.hh"
#include "PixTestFactory.hh"

// #include "PixTestAlive.hh"
// #include "PixTestDacScan.hh"
 
using namespace std;
using namespace pxar;

ClassImp(PixGui)

// ----------------------------------------------------------------------
PixGui::PixGui( const TGWindow *p, UInt_t w, UInt_t h, PixSetup *setup) : 
TGMainFrame(p, 1, 1, kVerticalFrame), fWidth(w), fHeight(h) {
  

  SetWindowName("pXar");

  ULong_t red;  gClient->GetColorByName("red", red);
  ULong_t green;  gClient->GetColorByName("green", green);
  ULong_t yellow;  gClient->GetColorByName("yellow", yellow);
  
  fPixSetup = setup;
  fApi = fPixSetup->getApi();
  fConfigParameters = fPixSetup->getConfigParameters();
  fTestParameters = fPixSetup->getPixTestParameters(); 
  
  fPower = true;
  fHV = false;
  
  // -- create the main frames: fH1 for top stuff and fH2 for tabs
  fH1 = new TGHorizontalFrame(this, fWidth, static_cast<int>(fHeight*0.2), kFixedHeight);
  fH2 = new TGHorizontalFrame(this, fWidth, static_cast<int>(fHeight*0.8), kFixedHeight);

  TGVerticalFrame *h1v1 = new TGVerticalFrame(fH1); 
  TGVerticalFrame *h1v2 = new TGVerticalFrame(fH1);
  TGVerticalFrame *h1v3 = new TGVerticalFrame(fH1);

  // -- left frame
  TGGroupFrame *tabControl = new TGGroupFrame(h1v1, "Open tabs");

  TGHorizontalFrame *testChooser = new TGHorizontalFrame(tabControl);
  testChooser->SetName("testChooser");
  testChooser->AddFrame(new TGLabel(testChooser, "Choose test "), new TGLayoutHints(kLHintsLeft, 5, 5, 3, 4));

  fcmbTests = new TGComboBox(testChooser);
  fcmbTests->SetWidth(150);
  fcmbTests->SetHeight(20);
  fcmbTests->Connect("Selected(char*)", "PixGui", this, "selectedTab(int)");

  tabControl->AddFrame(testChooser);
  testChooser->AddFrame(fcmbTests, new TGLayoutHints(kLHintsRight, 5, 50, 3, 4));
  testChooser->SetWidth(tabControl->GetWidth());

  h1v1->AddFrame(tabControl, new TGLayoutHints(kLHintsLeft, 5, 5, 3, 4));
  h1v1->SetWidth(800);

  // -- middle frame
  TGGroupFrame *hwControl = new TGGroupFrame(h1v2, "Hardware control");

  // power
  TGHorizontalFrame *powerFrame = new TGHorizontalFrame(hwControl, 150, 75);
  powerFrame->SetName("powerFrame");
  powerFrame->AddFrame(new TGLabel(powerFrame, "Power: "), new TGLayoutHints(kLHintsLeft, 5, 5, 3, 4));

  fbtnPower = new TGTextButton(powerFrame, "Off", B_POWER);
  fbtnPower->Resize(70,35);
  fbtnPower->Connect("Clicked()", "PixGui", this, "handleButtons()");
  if (fPower) {
    fbtnPower->ChangeBackground(green);
    fbtnPower->SetText("On");
  }  else {
    fbtnPower->ChangeBackground(red);
    fbtnPower->SetText("Off");
  }
  powerFrame->AddFrame(fbtnPower, new TGLayoutHints(kLHintsRight, 5, 5, 3, 4));
  hwControl->AddFrame(powerFrame);
  
  // HV
  TGHorizontalFrame *hvFrame = new TGHorizontalFrame(hwControl, 150,75);
  hvFrame->SetName("hvFrame");
  hvFrame->AddFrame(new TGLabel(hvFrame, "HV: "), new TGLayoutHints(kLHintsLeft, 5, 5, 3, 4));
  
  fbtnHV = new TGTextButton(hvFrame, "Off", B_HV);
  fbtnHV->Resize(70,35);
  fbtnHV->Connect("Clicked()", "PixGui", this, "handleButtons()");
  if (fHV) {
    fbtnHV->ChangeBackground(green);
    fbtnHV->SetText("On");
  } else {
    fbtnHV->ChangeBackground(red);
    fbtnHV->SetText("Off");
  }

  hvFrame->AddFrame(fbtnHV, new TGLayoutHints(kLHintsRight, 22, 5, 3, 7));

  hwControl->AddFrame(hvFrame);

  // monitoring 
  fMonitor = new PixMonitor(hwControl, this);
  fTimer = new TTimer(1000);
  fTimer->Connect("Timeout()", "PixMonitor", fMonitor, "Update()");
  fTimer->TurnOn();
    
  h1v2->AddFrame(hwControl, new TGLayoutHints(kLHintsLeft,5,5,3,4));

  // -- right frame
  TGTextButton *exitButton = new TGTextButton(h1v3, "exit", B_EXIT);
  exitButton->ChangeOptions(exitButton->GetOptions() );
  exitButton->Connect("Clicked()", "PixGui", this, "handleButtons()");
  exitButton->Resize(70,35);
  exitButton->ChangeBackground(red);


  h1v3->AddFrame(new TGLabel(h1v3, "root file:"), new TGLayoutHints(kLHintsLeft,5,5,3,4));
  TGTextEntry *output = new TGTextEntry(h1v3, fRootFileNameBuffer = new TGTextBuffer(200), B_FILENAME);
  output->SetText("bla");
  output->MoveResize(200, 60, 200, output->GetDefaultHeight());
  output->Connect("ReturnPressed()", "PixGui", this, "handleButtons()");
  h1v3->AddFrame(output, new TGLayoutHints(kLHintsLeft,5,5,3,4));
  h1v3->AddFrame(exitButton, new TGLayoutHints(kLHintsLeft,5,5,3,4));

  // -- tab widget
  fTabs = new TGTab(fH2, fH2->GetDefaultWidth(), fH2->GetDefaultHeight());
  fTabs->SetTab(0);
  fTabs->Connect("Selected(Int_t)", "PixGui", this, "selectedTab(Int_t)");
  
  fH2->AddFrame(fTabs, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY,2,2,2,2));

  PixParTab *t = new PixParTab(this, fConfigParameters, "h/w"); 

  fcmbTests->AddEntry("Ignore this ...", 0);
  vector<string> tests = fTestParameters->getTests();
  for (int i = 0; i < tests.size(); ++i) {
    fcmbTests->AddEntry(tests[i].c_str(), i+1);
    cout << "CREATE TAB FOR TEST " << i << endl;
    createTab(tests[i].c_str()); 
  }
  fcmbTests->Select(0);


  fH1->AddFrame(h1v1, new TGLayoutHints(kLHintsLeft | kLHintsExpandX | kLHintsExpandY, 2, 20, 2, 2));
  fH1->AddFrame(h1v2, new TGLayoutHints(kLHintsCenterX | kLHintsExpandX | kLHintsExpandY, 20, 20, 2, 2));
  fH1->AddFrame(h1v3, new TGLayoutHints(kLHintsRight | kLHintsExpandX | kLHintsExpandY, 20, 2, 2, 2));

  AddFrame(fH1, new TGLayoutHints(kLHintsTop | kLHintsExpandX));
  AddFrame(fH2, new TGLayoutHints(kLHintsBottom | kLHintsExpandY | kLHintsExpandX));
 
  MapSubwindows();
  Resize(GetDefaultSize());
  MapWindow();

}

// ----------------------------------------------------------------------
PixGui::~PixGui() {
  LOG(logINFO) << "PixGui::destructor";
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
    LOG(logINFO) << Form("fRootFileNameBuffer: %s", fRootFileNameBuffer->GetString());
    break;
  }
  case B_EXIT: {
    LOG(logINFO) << "PixGui::exit called";
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
      LOG(logINFO) << "Power set Off";
    } else {
      fPower = true;
      fbtnPower->ChangeBackground(green);
      fbtnPower->SetText("On");
      fApi->Pon(); 
      LOG(logINFO) << "Power set On";
    }
    break;
  }
  case B_HV: {
    if(fHV == true) {
      fHV = false;
      fbtnHV->ChangeBackground(red);
      fbtnHV->SetText("Off");
      fApi->HVoff(); 
      LOG(logINFO) << "HV set Off";
    } else {
      fHV = true;
      fbtnHV->ChangeBackground(green);
      fbtnHV->SetText("On");
      fApi->HVon(); 
      LOG(logINFO) << "HV set On";
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
void PixGui::createTab(const char*csel) {
  string tname = csel; 

  PixTest *pt = createTest(string(csel));
  if (0 == pt) {
    LOG(logINFO) << "ERROR: " << csel << " not known, nothing created";
    return;
  }

  fTestList.push_back(pt); 
  PixTab *t = new PixTab(this, pt, string(csel)); 
  pt->Connect("update()", "PixTab", t, "update()"); 
  //  fTabs->Resize(fTabs->GetDefaultSize());
  //  fTabs->MoveResize(0, 0, 800, 800);
  MapSubwindows();
  Resize(GetDefaultSize());
  MapWindow();
  LOG(logINFO) << "csel = " << csel;
  fTabs->SetTab(csel); 
  
}


// ----------------------------------------------------------------------
PixTest* PixGui::createTest(string testname) {
  PixTestFactory *factory = PixTestFactory::instance(); 
  return factory->createTest(testname, fPixSetup);
}


// ----------------------------------------------------------------------
void PixGui::selectedTab(int id) {
    LOG(logINFO) << "Switched to tab " << id;
    fTabs->SetTab(id); 
}
