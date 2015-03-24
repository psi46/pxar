#include "PixGui.hh"

#include <TSystem.h>

#include "log.h"

#include <TGComboBox.h>
#include "PixTab.hh"
#include "PixParTab.hh"
#include "PixTestFactory.hh"
#include "PixUserTestFactory.hh"
#include "PixUtil.hh"

#include "ConfigParameters.hh"
#include "PixTest.hh"
#include "PixTestParameters.hh"
#include "PixSetup.hh"
#include "PixMonitorFrame.hh"

#include "dictionaries.h"

using namespace std;
using namespace pxar;

ClassImp(PixGui)

// ----------------------------------------------------------------------
PixGui::PixGui( const TGWindow *p, UInt_t w, UInt_t h, PixSetup *setup) : 
TGMainFrame(p, 1, 1, kVerticalFrame), fWidth(w), fHeight(h) {
  
  fBorderN = 2; 
  fBorderL = 10; 
  fBorderT = 1;

  SetWindowName("pXar");

  fOldDirectory = "nada"; 

  gClient->GetColorByName("red", fRed);
  gClient->GetColorByName("green", fGreen);
  gClient->GetColorByName("yellow", fYellow);
  gClient->GetColorByName("white", fWhite);
  gClient->GetColorByName("DarkSeaGreen", fDarkSeaGreen);
  gClient->GetColorByName("DarkOrange", fDarkOrange);
  gClient->GetColorByName("DarkGray", fDarkGray);
  gClient->GetColorByName("DarkSalmon", fDarkSalmon);

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

  // ---------------
  // -- left frame
  // ---------------
  TGGroupFrame *pStuff = new TGGroupFrame(h1v1, "lost in space");
  h1v1->AddFrame(pStuff);
  pStuff->SetWidth(500); 
  TGVerticalFrame *FpStuff = new TGVerticalFrame(pStuff); 
  pStuff->AddFrame(FpStuff); 



  // ---------------
  // -- middle frame
  // ---------------
  TGGroupFrame *hwControl = new TGGroupFrame(h1v2, "Hardware control");
  h1v2->AddFrame(hwControl);
  TGHorizontalFrame *FhwControl = new TGHorizontalFrame(hwControl); 
  hwControl->AddFrame(FhwControl);
  TGVerticalFrame *h1v2l = new TGVerticalFrame(FhwControl); 
  FhwControl->AddFrame(h1v2l);
  TGVerticalFrame *h1v2r = new TGVerticalFrame(FhwControl);
  FhwControl->AddFrame(h1v2r);

  // left: power
  TGHorizontalFrame *powerFrame = new TGHorizontalFrame(h1v2l, 150, 75);
  h1v2l->AddFrame(powerFrame);
  powerFrame->SetName("powerFrame");
  powerFrame->AddFrame(new TGLabel(powerFrame, "Power: "), new TGLayoutHints(kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN));

  fbtnPower = new TGTextButton(powerFrame, "Off", B_POWER);
  fbtnPower->SetToolTipText(fConfigParameters->getNrocs()>1?"Turn on/off module low voltage":"Turn on/off ROC low voltage");
  fbtnPower->Resize(70,35);
  fbtnPower->Connect("Clicked()", "PixGui", this, "handleButtons()");
  if (fPower) {
    fbtnPower->ChangeBackground(fGreen);
    fbtnPower->SetText("On");
  }  else {
    fbtnPower->ChangeBackground(fRed);
    fbtnPower->SetText("Off");
  }
  powerFrame->AddFrame(fbtnPower, new TGLayoutHints(kLHintsRight, fBorderN, fBorderN, fBorderN, fBorderN));
  
  // left: HV
  TGHorizontalFrame *hvFrame = new TGHorizontalFrame(h1v2l, 150,75);
  h1v2l->AddFrame(hvFrame);
  hvFrame->SetName("hvFrame");
  hvFrame->AddFrame(new TGLabel(hvFrame, "HV:     "), new TGLayoutHints(kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN));
  
  fbtnHV = new TGTextButton(hvFrame, "Off", B_HV);
  hvFrame->AddFrame(fbtnHV, new TGLayoutHints(kLHintsRight, fBorderN, fBorderN, fBorderN, fBorderN));
  fbtnHV->SetToolTipText(fConfigParameters->getNrocs()>1?"Turn on/off module bias voltage":"Turn on/off ROC bias voltage");
  fbtnHV->Resize(70,35);
  fbtnHV->Connect("Clicked()", "PixGui", this, "handleButtons()");
  if (fHV) {
    hvOn();    
  } else {
    hvOff();
  }

  Connect("PixTest", "hvOn()", "PixGui", this, "hvOn()"); 
  Connect("PixTest", "hvOff()", "PixGui", this, "hvOff()"); 
  Connect("PixTest", "powerOn()", "PixGui", this, "powerOn()"); 
  Connect("PixTest", "powerOff()", "PixGui", this, "powerOff()"); 


  // Left: h/w monitoring 
  fMonitor = new PixMonitorFrame(h1v2l, this);
  fTimer = new TTimer(1000);
  fTimer->Connect("Timeout()", "PixMonitorFrame", fMonitor, "Update()");
  fTimer->TurnOn();


  // Right: scope probes
  TGComboBox *signalBoxA[2];
  TGComboBox *signalBoxD[2];

  TGHorizontalFrame *sigFrame(0); 

  sigFrame = new TGHorizontalFrame(h1v2r);
  h1v2r->AddFrame(sigFrame, new TGLayoutHints(kLHintsTop|kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN));
  sigFrame->AddFrame(new TGLabel(sigFrame, "A1:"), new TGLayoutHints(kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN));
  sigFrame->AddFrame(signalBoxA[0] = new TGComboBox(sigFrame), new TGLayoutHints(kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN));
  signalBoxA[0]->SetName("a1");
   
  sigFrame = new TGHorizontalFrame(h1v2r);
  h1v2r->AddFrame(sigFrame, new TGLayoutHints(kLHintsTop|kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN));
  sigFrame->AddFrame(new TGLabel(sigFrame, "A2:"), new TGLayoutHints(kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN));
  sigFrame->AddFrame(signalBoxA[1] = new TGComboBox(sigFrame), new TGLayoutHints(kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN));
  signalBoxA[1]->SetName("a2");

  sigFrame = new TGHorizontalFrame(h1v2r);
  h1v2r->AddFrame(sigFrame, new TGLayoutHints(kLHintsTop|kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN));
  sigFrame->AddFrame(new TGLabel(sigFrame, "D1:"), new TGLayoutHints(kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN));
  sigFrame->AddFrame(signalBoxD[0] = new TGComboBox(sigFrame), new TGLayoutHints(kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN));
  signalBoxD[0]->SetName("d1");

  sigFrame = new TGHorizontalFrame(h1v2r);
  h1v2r->AddFrame(sigFrame, new TGLayoutHints(kLHintsTop|kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN));
  sigFrame->AddFrame(new TGLabel(sigFrame, "D2:"), new TGLayoutHints(kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN));
  sigFrame->AddFrame(signalBoxD[1] = new TGComboBox(sigFrame), new TGLayoutHints(kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN));
  signalBoxD[1]->SetName("d2");

  signalBoxA[0]->SetWidth(75);
  signalBoxA[0]->SetHeight(20);
  signalBoxA[1]->SetWidth(75);
  signalBoxA[1]->SetHeight(20);
  signalBoxD[0]->SetWidth(75);
  signalBoxD[0]->SetHeight(20);
  signalBoxD[1]->SetWidth(75);
  signalBoxD[1]->SetHeight(20);

  // Get singleton Probe dictionary object:
  ProbeDictionary * _dict = ProbeDictionary::getInstance();

  // Get all analog probes:
  std::vector<std::string> analogsignals = _dict->getAllAnalogNames();
  std::vector<std::string> digitalsignals = _dict->getAllDigitalNames();

  for(std::vector<std::string>::iterator it = analogsignals.begin(); it != analogsignals.end(); it++) {
    signalBoxA[0]->AddEntry(it->c_str(),_dict->getSignal(*it,PROBE_ANALOG));
    signalBoxA[1]->AddEntry(it->c_str(),_dict->getSignal(*it,PROBE_ANALOG));
  }

  for(std::vector<std::string>::iterator it = digitalsignals.begin(); it != digitalsignals.end(); it++) {
    signalBoxD[0]->AddEntry(it->c_str(),_dict->getSignal(*it,PROBE_DIGITAL));
    signalBoxD[1]->AddEntry(it->c_str(),_dict->getSignal(*it,PROBE_DIGITAL));
  }

  for(int i = 0 ; i <= 1 ; i++) {
    signalBoxA[i]->Connect("Selected(Int_t)", "PixGui", this, "selectProbes(Int_t)");
    signalBoxD[i]->Connect("Selected(Int_t)", "PixGui", this, "selectProbes(Int_t)");
  }
  
  signalBoxA[0]->Select(_dict->getSignal(fConfigParameters->getProbe("a1"),PROBE_ANALOG),false);
  signalBoxA[1]->Select(_dict->getSignal(fConfigParameters->getProbe("a2"),PROBE_ANALOG),false);
  signalBoxD[0]->Select(_dict->getSignal(fConfigParameters->getProbe("d1"),PROBE_DIGITAL),false);
  signalBoxD[1]->Select(_dict->getSignal(fConfigParameters->getProbe("d2"),PROBE_DIGITAL),false);



  // --------------
  // -- right frame
  // --------------
  TGGroupFrame *pControl = new TGGroupFrame(h1v3, "pXar control");
  h1v3->AddFrame(pControl);
  TGVerticalFrame *FpControl = new TGVerticalFrame(pControl); 
  pControl->AddFrame(FpControl); 

  TGHorizontalFrame *bFrame = new TGHorizontalFrame(FpControl); 
  FpControl->AddFrame(bFrame, new TGLayoutHints(kLHintsLeft | kLHintsTop, fBorderN, fBorderN, fBorderN, fBorderN)); 

  TGTextButton *exitButton = new TGTextButton(bFrame, "exit", B_EXIT);
  bFrame->AddFrame(exitButton, new TGLayoutHints(kLHintsBottom | kLHintsRight, fBorderN, fBorderN, fBorderN, fBorderN));
  exitButton->SetToolTipText("exit pxar,\nwrite rootfile,\ndo *not* write config files");
  exitButton->ChangeOptions(exitButton->GetOptions() );
  exitButton->Connect("Clicked()", "PixGui", this, "handleButtons()");
  exitButton->Resize(70,35);
  exitButton->ChangeBackground(fRed);


  TGTextButton *writeButton = new TGTextButton(bFrame, "write cfg files", B_WRITEALLFILES);
  bFrame->AddFrame(writeButton, new TGLayoutHints(kLHintsBottom | kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN));
  writeButton->SetToolTipText("write all config files (ROC/TBM DAC, trim bits, DTB setup, config file)");
  writeButton->ChangeOptions(writeButton->GetOptions() );
  writeButton->Connect("Clicked()", "PixGui", this, "handleButtons()");
  writeButton->Resize(70,35);
  writeButton->ChangeBackground(fYellow);


  TGHorizontalFrame *rootfileFrame = new TGHorizontalFrame(FpControl, 150,75);
  FpControl->AddFrame(rootfileFrame, new TGLayoutHints(kLHintsTop|kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN));
  rootfileFrame->SetName("rootfileFrame");

  TGTextButton *rootfileButton = new TGTextButton(rootfileFrame, " Change rootfile ", B_FILENAME);
  rootfileButton->SetToolTipText("change the rootfile name");
  rootfileButton->Connect("Clicked()", "PixGui", this, "handleButtons()");
  rootfileFrame->AddFrame(rootfileButton, new TGLayoutHints(kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN));

  TGTextEntry *output = new TGTextEntry(rootfileFrame, fRootFileNameBuffer = new TGTextBuffer(200), B_FILENAME);
  output->SetText(fConfigParameters->getRootFileName().c_str());
  output->MoveResize(100, 60, 120, output->GetDefaultHeight());
  output->Connect("ReturnPressed()", "PixGui", this, "handleButtons()");
  rootfileFrame->AddFrame(output, new TGLayoutHints(kLHintsRight, fBorderN, fBorderN, fBorderN, fBorderN));
 


  TGHorizontalFrame *dirFrame = new TGHorizontalFrame(FpControl, 150,75);
  FpControl->AddFrame(dirFrame, new TGLayoutHints(kLHintsTop|kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN));
  dirFrame->SetName("dirFrame");

  TGTextButton *dirButton = new TGTextButton(dirFrame, " Change directory ", B_DIRECTORY);
  dirButton->Connect("Clicked()", "PixGui", this, "handleButtons()");
  dirButton->SetToolTipText("change the output directory; will move the rootfile as well");
  dirFrame->AddFrame(dirButton, new TGLayoutHints(kLHintsLeft, fBorderN, fBorderN, fBorderN, fBorderN));

  TGTextEntry *doutput = new TGTextEntry(dirFrame, fDirNameBuffer = new TGTextBuffer(200), B_DIRECTORY);
  doutput->SetText(fConfigParameters->getDirectory().c_str());
  doutput->MoveResize(100, 60, 120, output->GetDefaultHeight());
  doutput->Connect("ReturnPressed()", "PixGui", this, "handleButtons()");
  dirFrame->AddFrame(doutput, new TGLayoutHints(kLHintsRight, fBorderN, fBorderN, fBorderN, fBorderN));
 

  h1v3->SetWidth(fWidth-h1v1->GetWidth()-h1v2->GetWidth());



  // -- tab widget
  fTabs = new TGTab(fH2, fH2->GetDefaultWidth(), fH2->GetDefaultHeight());
  fTabs->SetTab(0);
  fTabs->Connect("Selected(Int_t)", "PixGui", this, "selectedTab(Int_t)");
  
  fH2->AddFrame(fTabs, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, fBorderN, fBorderN, fBorderN, fBorderN));

  if(fApi) fParTab = new PixParTab(this, fConfigParameters, "h/w"); 
  fTabs->SetTab(0);

  if (fApi) fParTab->updateSelection(); // ensure that fId2Idx for all tests is initialized

  vector<string> tests = fTestParameters->getTests();
  for (unsigned int i = 0; i < tests.size(); ++i) {
    createTab(tests[i].c_str()); 
  }

  fH1->AddFrame(h1v1, new TGLayoutHints(kLHintsLeft | kLHintsExpandX | kLHintsExpandY, fBorderN, fBorderN, fBorderN, fBorderN));
  fH1->AddFrame(h1v2, new TGLayoutHints(kLHintsCenterX , fBorderN, fBorderN, fBorderN, fBorderN));
  fH1->AddFrame(h1v3, new TGLayoutHints(kLHintsRight | kLHintsExpandX | kLHintsExpandY, fBorderN, fBorderN, fBorderN, fBorderN));

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
  fPixSetup->getPixMonitor()->dumpSummaries();
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
  fPixSetup->getPixMonitor()->dumpSummaries();

  //  DestroyWindow();
  gApplication->Terminate(0);

}

// ----------------------------------------------------------------------
void PixGui::selectProbes(Int_t /*id*/) {
   TGComboBox *box = (TGComboBox *) gTQSender;
   
   fApi->SignalProbe(box->GetName(),box->GetSelectedEntry()->GetTitle());

   // Write the selected probe to the configParameters.

   fConfigParameters->setProbe(box->GetName(),box->GetSelectedEntry()->GetTitle());

   //   fConfigParameters->writeConfigParameterFile();

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
      powerOff();
    } else {
      powerOn();
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
void PixGui::powerOn() {
  if (fApi) {
    fPower = true;
    fbtnPower->ChangeBackground(fGreen);
    fbtnPower->SetText("On");
    fApi->Pon(); 
    LOG(logDEBUG) << "Power set On";
  }
}

// ----------------------------------------------------------------------
void PixGui::powerOff() {
  if (fApi) {
    fPower = false;
    fbtnPower->ChangeBackground(fRed);
    fbtnPower->SetText("Off");
    fApi->Poff(); 
    LOG(logDEBUG) << "Power set Off";
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



// // --------------------------------------------------------------------------------
// // FIXME needed?
// void PixGui::createParTab() {
//   UInt_t w = 400; 
//   UInt_t h = 400; 

//   fParTab = fTabs->AddTab("hardware parameters");
//   fParTab->SetLayoutManager(new TGVerticalLayout(fParTab));

//   // create the TopFrame
//   fhFrame = new TGHorizontalFrame(fParTab, w, 0.5*h);

// }


// --------------------------------------------------------------------------------
void PixGui::createTab(const char* csel) {

  PixTest *pt = createTest(string(csel));
  if (0 == pt) {
    LOG(logDEBUG) << "ERROR: " << csel << " not known, nothing created";
    return;
  }

  fTestList.push_back(pt); 
  PixTab *t = new PixTab(this, pt, string(csel)); 
  fPixTabList.push_back(t);
  //  LOG(logDEBUG) << "added tab " << t << " to fPixTabList, now size() = " << fPixTabList.size();
  
  pt->Connect("update()", "PixTab", t, "update()"); 
  //  fTabs->Resize(fTabs->GetDefaultSize());
  //  fTabs->MoveResize(0, 0, 800, 800);
  MapSubwindows();
  Resize(GetDefaultSize());
  MapWindow();
  fTabs->SetTab(csel); 
  
}


// ----------------------------------------------------------------------
PixTest* PixGui::createTest(string testname) {
  PixTestFactory *factory = PixTestFactory::instance(); 
  PixUserTestFactory *userfactory = PixUserTestFactory::instance(); 
  PixTest *t =  factory->createTest(testname, fPixSetup);
  if (0 == t) t = userfactory->createTest(testname, fPixSetup);
  return t;
}


// ----------------------------------------------------------------------
void PixGui::selectedTab(int id) {
  fTabs->SetTab(id);
  if (0 == id) {
    fParTab->updateParameters();
    fPixTab = 0;
    return;
  }
  fPixTab = fPixTabList[id-1];
  //  LOG(logDEBUG) << "Switched to tab " << id << " fPixTabList.size() = " << fPixTabList.size() << " fPixTab = " << fPixTab;
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


// ----------------------------------------------------------------------
void PixGui::updateSelectedRocs(map<int, int> a) {

  for (unsigned int i = 0; i < fTestList.size(); ++i) {
    fTestList[i]->setId2Idx(a); 
  }


}

// ----------------------------------------------------------------------
std::string PixGui::getHdiType() {
  return fConfigParameters->getHdiType();
}
