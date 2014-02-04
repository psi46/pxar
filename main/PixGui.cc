#include "PixGui.hh"

#include <TSystem.h>

#include "log.h"

#include "PixTab.hh"
#include "PixParTab.hh"
#include "PixTestFactory.hh"
#include "PixUtil.hh"



using namespace std;
using namespace pxar;

ClassImp(PixGui)

// ----------------------------------------------------------------------
PixGui::PixGui( const TGWindow *p, UInt_t w, UInt_t h, PixSetup *setup) : 
TGMainFrame(p, 1, 1, kVerticalFrame), fWidth(w), fHeight(h) {
  

  SetWindowName("pXar");

  fOldDirectory = "nada"; 

  gClient->GetColorByName("red", fRed);
  gClient->GetColorByName("green", fGreen);
  gClient->GetColorByName("yellow", fYellow);
  
  fPixSetup = setup;
  fApi = fPixSetup->getApi();
  fConfigParameters = fPixSetup->getConfigParameters();
  fTestParameters = fPixSetup->getPixTestParameters(); 
  
  fPower = true;
  if (fConfigParameters->getHvOn()) {
    fHV = true; 
  } else {
    fHV = false;
  }
  
  // -- create the main frames: fH1 for top stuff and fH2 for tabs
  fH1 = new TGHorizontalFrame(this, fWidth, static_cast<int>(fHeight*0.2), kFixedHeight);
  fH2 = new TGHorizontalFrame(this, fWidth, static_cast<int>(fHeight*0.8), kFixedHeight);

  TGVerticalFrame *h1v1 = new TGVerticalFrame(fH1); 
  TGVerticalFrame *h1v2 = new TGVerticalFrame(fH1);
  TGVerticalFrame *h1v3 = new TGVerticalFrame(fH1);

  // -- left frame
  TGGroupFrame *tabControl = new TGGroupFrame(h1v1, "Open test tabs");

  TGHorizontalFrame *testChooser = new TGHorizontalFrame(tabControl);
  testChooser->SetName("testChooser");
  testChooser->AddFrame(new TGLabel(testChooser, "Choose "), new TGLayoutHints(kLHintsLeft, 5, 5, 3, 4));

  fcmbTests = new TGComboBox(testChooser);
  fcmbTests->SetWidth(150);
  fcmbTests->SetHeight(20);
  fcmbTests->Connect("Selected(char*)", "PixGui", this, "selectedTab(int)");

  tabControl->AddFrame(testChooser);
  testChooser->AddFrame(fcmbTests, new TGLayoutHints(kLHintsRight, 5, 5, 3, 4));
  testChooser->SetWidth(tabControl->GetWidth());

  h1v1->AddFrame(tabControl, new TGLayoutHints(kLHintsLeft, 5, 5, 3, 4));
  h1v1->SetWidth(testChooser->GetWidth());

  // -- middle frame
  TGGroupFrame *hwControl = new TGGroupFrame(h1v2, "Hardware control");

  // power
  TGHorizontalFrame *powerFrame = new TGHorizontalFrame(hwControl, 150, 75);
  powerFrame->SetName("powerFrame");
  powerFrame->AddFrame(new TGLabel(powerFrame, "Power: "), new TGLayoutHints(kLHintsLeft, 5, 5, 3, 4));

  fbtnPower = new TGTextButton(powerFrame, "Off", B_POWER);
  fbtnPower->SetToolTipText("Turn on/off power of DTB");
  fbtnPower->Resize(70,35);
  fbtnPower->Connect("Clicked()", "PixGui", this, "handleButtons()");
  if (fPower) {
    fbtnPower->ChangeBackground(fGreen);
    fbtnPower->SetText("On");
  }  else {
    fbtnPower->ChangeBackground(fRed);
    fbtnPower->SetText("Off");
  }
  powerFrame->AddFrame(fbtnPower, new TGLayoutHints(kLHintsRight, 5, 5, 3, 4));
  hwControl->AddFrame(powerFrame);
  
  // HV
  TGHorizontalFrame *hvFrame = new TGHorizontalFrame(hwControl, 150,75);
  hvFrame->SetName("hvFrame");
  hvFrame->AddFrame(new TGLabel(hvFrame, "HV:     "), new TGLayoutHints(kLHintsLeft, 5, 5, 3, 4));
  
  fbtnHV = new TGTextButton(hvFrame, "Off", B_HV);
  fbtnHV->SetToolTipText(fConfigParameters->getNrocs()>1?"Turn on/off HV for module":"Turn on/off HV for ROC");
  fbtnHV->Resize(70,35);
  fbtnHV->Connect("Clicked()", "PixGui", this, "handleButtons()");
  if (fHV) {
    hvOn();    
  } else {
    hvOff();
  }

  hvFrame->AddFrame(fbtnHV, new TGLayoutHints(kLHintsRight, 5, 5, 3, 4));

  hwControl->AddFrame(hvFrame);

  // h/w monitoring 
  fMonitor = new PixMonitor(hwControl, this);
  fTimer = new TTimer(1000);
  fTimer->Connect("Timeout()", "PixMonitor", fMonitor, "Update()");
  fTimer->TurnOn();
    
  h1v2->AddFrame(hwControl, new TGLayoutHints(kLHintsLeft,5,5,3,4));
  h1v1->SetWidth(400);

  // -- right frame
  TGHorizontalFrame *bFrame = new TGHorizontalFrame(h1v3); 
  h1v3->AddFrame(bFrame, new TGLayoutHints(kLHintsLeft | kLHintsBottom, 5, 5, 3, 4)); 
  TGTextButton *exitButton = new TGTextButton(bFrame, "exit", B_EXIT);
  exitButton->SetToolTipText("exit pxar,\nwrite rootfile,\ndo *not* write config files");
  exitButton->ChangeOptions(exitButton->GetOptions() );
  exitButton->Connect("Clicked()", "PixGui", this, "handleButtons()");
  exitButton->Resize(70,35);
  exitButton->ChangeBackground(fRed);
  bFrame->AddFrame(exitButton, new TGLayoutHints(kLHintsBottom | kLHintsRight,5,5,3,4));


  TGTextButton *writeButton = new TGTextButton(bFrame, "write cfg files", B_WRITEALLFILES);
  writeButton->SetToolTipText("write all config files (ROC/TBM DAC, trim bits, DTB setup, config file)");
  writeButton->ChangeOptions(writeButton->GetOptions() );
  writeButton->Connect("Clicked()", "PixGui", this, "handleButtons()");
  writeButton->Resize(70,35);
  writeButton->ChangeBackground(fYellow);
  bFrame->AddFrame(writeButton, new TGLayoutHints(kLHintsBottom | kLHintsLeft,5,5,3,4));


  TGHorizontalFrame *rootfileFrame = new TGHorizontalFrame(h1v3, 150,75);
  rootfileFrame->SetName("rootfileFrame");

  TGTextButton *rootfileButton = new TGTextButton(rootfileFrame, " Change rootfile ", B_FILENAME);
  rootfileButton->SetToolTipText("change the rootfile name");
  rootfileButton->Connect("Clicked()", "PixGui", this, "handleButtons()");
  rootfileFrame->AddFrame(rootfileButton, new TGLayoutHints(kLHintsLeft,5,5,3,4));

  TGTextEntry *output = new TGTextEntry(rootfileFrame, fRootFileNameBuffer = new TGTextBuffer(200), B_FILENAME);
  output->SetText(fConfigParameters->getRootFileName().c_str());
  output->MoveResize(100, 60, 120, output->GetDefaultHeight());
  output->Connect("ReturnPressed()", "PixGui", this, "handleButtons()");
  rootfileFrame->AddFrame(output, new TGLayoutHints(kLHintsRight,5,5,3,4));
 
  h1v3->AddFrame(rootfileFrame, new TGLayoutHints(kLHintsTop|kLHintsLeft,5,5,3,4));


  TGHorizontalFrame *dirFrame = new TGHorizontalFrame(h1v3, 150,75);
  dirFrame->SetName("dirFrame");

  TGTextButton *dirButton = new TGTextButton(dirFrame, " Change directory ", B_DIRECTORY);
  dirButton->Connect("Clicked()", "PixGui", this, "handleButtons()");
  dirButton->SetToolTipText("change the output directory; will move the rootfile as well");
  dirFrame->AddFrame(dirButton, new TGLayoutHints(kLHintsLeft,5,5,3,4));

  TGTextEntry *doutput = new TGTextEntry(dirFrame, fDirNameBuffer = new TGTextBuffer(200), B_DIRECTORY);
  doutput->SetText(fConfigParameters->getDirectory().c_str());
  doutput->MoveResize(100, 60, 120, output->GetDefaultHeight());
  doutput->Connect("ReturnPressed()", "PixGui", this, "handleButtons()");
  dirFrame->AddFrame(doutput, new TGLayoutHints(kLHintsRight,5,5,3,4));
 
  h1v3->AddFrame(dirFrame, new TGLayoutHints(kLHintsTop|kLHintsLeft,5,5,3,4));



  h1v3->SetWidth(fWidth-h1v1->GetWidth()-h1v2->GetWidth());


  // -- tab widget
  fTabs = new TGTab(fH2, fH2->GetDefaultWidth(), fH2->GetDefaultHeight());
  fTabs->SetTab(0);
  fTabs->Connect("Selected(Int_t)", "PixGui", this, "selectedTab(Int_t)");
  
  fH2->AddFrame(fTabs, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY,2,2,2,2));

  PixParTab *t = new PixParTab(this, fConfigParameters, "h/w"); 
  (void)t;

  fcmbTests->AddEntry("Ignore this ...", 0);
  vector<string> tests = fTestParameters->getTests();
  for (unsigned int i = 0; i < tests.size(); ++i) {
    fcmbTests->AddEntry(tests[i].c_str(), i+1);
    createTab(tests[i].c_str()); 
  }
  fcmbTests->Select(0);

  fH1->AddFrame(h1v1, new TGLayoutHints(kLHintsLeft | kLHintsExpandX | kLHintsExpandY, 2, 2, 2, 2));
  fH1->AddFrame(h1v2, new TGLayoutHints(kLHintsCenterX , 2, 2, 2, 2));
  fH1->AddFrame(h1v3, new TGLayoutHints(kLHintsRight | kLHintsExpandX | kLHintsExpandY, 2, 2, 2, 2));

  AddFrame(fH1, new TGLayoutHints(kLHintsTop | kLHintsExpandX));
  AddFrame(fH2, new TGLayoutHints(kLHintsBottom | kLHintsExpandY | kLHintsExpandX));
 
  MapSubwindows();
  Resize(GetDefaultSize());
  MapWindow();

}

// ----------------------------------------------------------------------
PixGui::~PixGui() {
  LOG(logDEBUG) << "PixGui::destructor";
  delete fTimer;
  delete fMonitor; 
  delete fcmbTests;
  delete fTabs;
  delete fParTab;
  delete fRootFileNameBuffer;
  delete fbtnPower;
  delete fbtnHV;
  delete fpowerSlider;
  delete fhvSlider;
  delete flblPower;
  delete flblHV;
  delete fH1;
  delete fH2;

}


// ----------------------------------------------------------------------
void PixGui::Cleanup() {
    gApplication->Terminate(0);
}


// ----------------------------------------------------------------------
void PixGui::CloseWindow() {
  std::vector<PixTest*>::iterator il; 
  for (il = fTestList.begin(); il != fTestList.end(); ++il) {
    delete (*il); 
  } 
  
  if (fTimer) fTimer->TurnOff();
  if (fApi) delete fApi; 
  
  DestroyWindow();
  gApplication->Terminate(0);

}
// ----------------------------------------------------------------------
void PixGui::handleButtons(Int_t id) {
  if (id == -1) {
    TGButton *btn = (TGButton *) gTQSender;
    id = btn->WidgetId();
  }
 
  switch (id) {
  case B_DIRECTORY: {
    LOG(logDEBUG) << Form("changing base directory: %s", fDirNameBuffer->GetString());
    fOldDirectory = fConfigParameters->getDirectory(); 
    if (0 == gSystem->OpenDirectory(fDirNameBuffer->GetString())) {
      LOG(logINFO) << "directory " << fDirNameBuffer->GetString() << " does not exist, creating it"; 
      int bla = gSystem->MakeDirectory(fDirNameBuffer->GetString()); 
      if (bla < 0)  {
	LOG(logWARNING) << " failed to create directory " << fDirNameBuffer->GetString();
	break;
      }
    } 
    
    fConfigParameters->setDirectory(fDirNameBuffer->GetString()); 
    changeRootFile();
    break;
  }
  case B_FILENAME: {
    LOG(logINFO) << Form("changing rootfilenme: %s", fRootFileNameBuffer->GetString());
    changeRootFile(); 
    break;
  }
  case B_EXIT: {
    LOG(logDEBUG) << "PixGui::exit called";
    CloseWindow();
    break;
  }

  case B_WRITEALLFILES: {
    LOG(logDEBUG) << "PixGui::writeAllFiles called";
    fConfigParameters->writeAllFiles();
    break;
  }
  case B_POWER: {
    if(fPower == true) {
      fPower = false;
      fbtnPower->ChangeBackground(fRed);
      fbtnPower->SetText("Off");
      fApi->Poff(); 
      LOG(logDEBUG) << "Power set Off";
    } else {
      fPower = true;
      fbtnPower->ChangeBackground(fGreen);
      fbtnPower->SetText("On");
      fApi->Pon(); 
      LOG(logDEBUG) << "Power set On";
    }
    break;
  }
  case B_HV: {
    if(fHV == true) {
      hvOff();
    } else {
      hvOn();
    }
    break;  
  }
  default:
    break;
  }
}

// ----------------------------------------------------------------------
void PixGui::hvOn() {
  if (fApi) {
    fHV = true;
    fbtnHV->ChangeBackground(fGreen);
    fbtnHV->SetText("On");
    fApi->HVon(); 
    LOG(logDEBUG) << "HV set On";
  }
}


// ----------------------------------------------------------------------
void PixGui::hvOff() {
  if (fApi) {
    fHV = false;
    fbtnHV->ChangeBackground(fRed);
    fbtnHV->SetText("Off");
    fApi->HVoff(); 
    LOG(logDEBUG) << "HV set Off";
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
    LOG(logDEBUG) << "ERROR: " << csel << " not known, nothing created";
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
  LOG(logDEBUG) << "csel = " << csel;
  fTabs->SetTab(csel); 
  
}


// ----------------------------------------------------------------------
PixTest* PixGui::createTest(string testname) {
  PixTestFactory *factory = PixTestFactory::instance(); 
  return factory->createTest(testname, fPixSetup);
}


// ----------------------------------------------------------------------
void PixGui::selectedTab(int id) {
  LOG(logDEBUG) << "Switched to tab " << id;
  fTabs->SetTab(id); 
}


// ----------------------------------------------------------------------
void PixGui::changeRootFile() {
  string oldRootFilePath = gFile->GetName();
  gFile->Close();

  string newRootFilePath = fConfigParameters->getDirectory() + "/" + fRootFileNameBuffer->GetString();

  gSystem->Rename(oldRootFilePath.c_str(), newRootFilePath.c_str()); 
  TFile *f = TFile::Open(newRootFilePath.c_str(), "UPDATE"); 
  (void)f;
  std::vector<PixTest*>::iterator il; 
  for (il = fTestList.begin(); il != fTestList.end(); ++il) {
    (*il)->resetDirectory();
  } 
}
