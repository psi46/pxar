#include <iostream>
#include <cstdlib>
#include <bitset>

#include <TApplication.h>
#include <TGButton.h>
#include <TGToolTip.h>
#include <TRandom.h>
#include <TSystem.h>
#include <TCanvas.h>
#include <TGTab.h>
#include <TGLabel.h>
#include <TGButtonGroup.h>
#include <TGTableLayout.h>
#include <cstdlib>

#include "PixParTab.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixParTab)

// ----------------------------------------------------------------------
PixParTab::PixParTab(PixGui *p, ConfigParameters *cfg, string tabname) {
  init(p, cfg, tabname);

  fLockClk = true; 

  fBorderR = fBorderL = fBorderT = fBorderB = 2;

  fTabFrame = fGui->getTabs()->AddTab(fTabName.c_str());
  fTabFrame->SetLayoutManager(new TGVerticalLayout(fTabFrame));


  UInt_t w = fGui->getTabs()->GetWidth();
  UInt_t h = fGui->getTabs()->GetHeight();

  fhFrame = new TGCompositeFrame(fTabFrame, w, h, kHorizontalFrame);
  fTabFrame->AddFrame(fhFrame, new TGLayoutHints(kLHintsRight | kLHintsExpandX | kLHintsExpandY, fBorderL, fBorderR, fBorderT, fBorderB));

  TGTabElement *tabel = fGui->getTabs()->GetTabTab(fTabName.c_str());
  tabel->ChangeBackground(fGui->fDarkSeaGreen);

  TGTextEntry *te(0);
  TGLabel *tl(0);
  TGTextBuffer *tb(0);
  TGTextButton *tset(0);

  TGCheckButton *tcb(0);
  TGHorizontalFrame *hFrame(0);
  TGVerticalFrame *vFrame(0);
  TGGroupFrame *g1Frame(0), *g2Frame(0);

  // -- TB Parameters
  vFrame = new TGVerticalFrame(fhFrame);
  fhFrame->AddFrame(vFrame, new TGLayoutHints(kLHintsLeft, fBorderL, fBorderR, fBorderT, fBorderB));
  g1Frame = new TGGroupFrame(vFrame, "Testboard");
  vFrame->AddFrame(g1Frame);

  g1Frame->AddFrame(hFrame = new TGHorizontalFrame(g1Frame, 300, 30, kLHintsExpandX), new TGLayoutHints(kLHintsRight | kLHintsTop));
  tcb = new TGCheckButton(hFrame, "lock parameters to clk");
  tcb->Connect("Clicked()", "PixParTab", this, "lockClk()");
  tcb->SetToolTipText("Enforce the relations ctr = clk; sda = clk+15; tin = clk+5");
  tcb->GetToolTip()->SetDelay(2000); // add a bit of delay to ease button hitting
  tcb->SetState(kButtonDown);
  hFrame->AddFrame(tcb, new TGLayoutHints(kLHintsCenterY | kLHintsRight, fBorderL, fBorderR, fBorderT, fBorderB));

  vector<pair<string, uint8_t> > amap = fConfigParameters->getTbParameters();
  for (unsigned int i = 0; i < amap.size(); ++i) {
    hFrame = new TGHorizontalFrame(g1Frame, 300, 30, kLHintsExpandX);
    g1Frame->AddFrame(hFrame, new TGLayoutHints(kLHintsRight | kLHintsTop));
    tl = new TGLabel(hFrame, amap[i].first.c_str());
    tl->SetWidth(100);
    hFrame->AddFrame(tl, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, fBorderL, fBorderR, fBorderT, 0));

    tb = new TGTextBuffer(5);
    te  = new TGTextEntry(hFrame, tb, i); te->SetWidth(100);
    hFrame->AddFrame(te, new TGLayoutHints(kLHintsCenterY | kLHintsCenterX, fBorderL, fBorderR, fBorderT, 0));
    fTbParIds.push_back(amap[i].first);
    fTbTextEntries.insert(make_pair(amap[i].first, te));

    te->SetText(Form("%d", int(amap[i].second)));
    te->Connect("ReturnPressed()", "PixParTab", this, "setTbParameter()");
    te->Connect("TextChanged(const char *)", "PixParTab", this, "tbYellow()");

    tset = new TGTextButton(hFrame, "Set", i);
    tset->SetToolTipText("set the parameter\nor click *return* after changing the numerical value");
    tset->GetToolTip()->SetDelay(2000); // add a bit of delay to ease button hitting
    tset->Connect("Clicked()", "PixParTab", this, "setTbParameter()");
    hFrame->AddFrame(tset, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, fBorderL, fBorderR, fBorderT, 0));
  }

  vector<pair<string, double> > dmap = fConfigParameters->getTbPowerSettings();
  for (unsigned int i = 0; i < dmap.size(); ++i) {
    hFrame = new TGHorizontalFrame(g1Frame, 300, 30, kLHintsExpandX);
    g1Frame->AddFrame(hFrame, new TGLayoutHints(kLHintsRight | kLHintsTop));
    tb = new TGTextBuffer(5);
    tl = new TGLabel(hFrame, dmap[i].first.c_str());
    hFrame->AddFrame(tl, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, fBorderL, fBorderR, fBorderT, 0));
    tl->SetWidth(100);

    te  = new TGTextEntry(hFrame, tb, i); te->SetWidth(100);
    hFrame->AddFrame(te, new TGLayoutHints(kLHintsCenterY | kLHintsCenterX, fBorderL, fBorderR, fBorderT, 0));
    fPowerParIds.push_back(dmap[i].first);
    fPowerTextEntries.insert(make_pair(dmap[i].first, te));

    te->SetText(Form("%5.3f", float(dmap[i].second)));
    te->Connect("ReturnPressed()", "PixParTab", this, "setPowerSettings()");
    te->Connect("TextChanged(const char *)", "PixParTab", this, "powerYellow()");

    tset = new TGTextButton(hFrame, "Set", i);
    hFrame->AddFrame(tset, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, fBorderL, fBorderR, fBorderT, 0));
    tset->SetToolTipText("set the parameter\nor click *return* after changing the numerical value");
    tset->GetToolTip()->SetDelay(2000); // add a bit of delay to ease button hitting
    tset->Connect("Clicked()", "PixParTab", this, "setPowerSettings()");
  }

  tset = new TGTextButton(g1Frame, "Save Parameters");
  g1Frame->AddFrame(tset, new TGLayoutHints(kLHintsBottom|kLHintsRight, fBorderL, fBorderR, fBorderT, 0));
  tset->SetToolTipText(Form("Write the testboard parameters to file.\nThe output file will overwrite whatever is in the directory \"%s\"\n(change this in the top right part of the GUI)", fConfigParameters->getDirectory().c_str()));
  tset->Connect("Clicked()", "PixParTab", this, "saveTbParameters()");

  // -- TBM Parameters
  TGCompositeFrame *bGroup = new TGCompositeFrame(vFrame, 60, 20, kHorizontalFrame |kSunkenFrame);
  vFrame->AddFrame(bGroup, new TGLayoutHints(kLHintsCenterX|kLHintsCenterY, fBorderL, fBorderR, fBorderT, fBorderB));
  vector<vector<pair<string, uint8_t> > > cmap;
  for (unsigned int i = 0; i < fGui->getApi()->_dut->getNTbms(); ++i) {
    tcb = new TGCheckButton(bGroup, Form("%d", i), i);
    tcb->Connect("Clicked()", "PixParTab", this, "selectTbm()");
    bGroup->AddFrame(tcb, new TGLayoutHints(kLHintsLeft, fBorderL, fBorderR, fBorderT, 0));
    fSelectTbm.push_back(tcb);
    vector<pair<string, uint8_t> > smap = fGui->getApi()->_dut->getTbmDACs(i);
    cmap.push_back(smap);
  }
  if (fSelectTbm.size() > 0) {
    fSelectedTbm = 0;
    // -- by default enable all present
    for (unsigned itbm = 0; itbm < fSelectTbm.size(); ++itbm) {
      fSelectTbm[itbm]->SetState(kButtonDown);
    }
  }

  tset = new TGTextButton(bGroup, "Save");
  bGroup->AddFrame(tset, new TGLayoutHints(kLHintsCenterX|kLHintsCenterY, fBorderL, fBorderR, fBorderT, fBorderB));
  tset->SetToolTipText(Form("Write the TBM parameters of all selected TBMs to file.\nThe output file will overwrite whatever is in the directory \"%s\"\n(change this in the top right part of the GUI)", fConfigParameters->getDirectory().c_str()));
  tset->Connect("Clicked()", "PixParTab", this, "saveTbmParameters()");
  
  updateSelection();

  g2Frame = new TGGroupFrame(vFrame, "DAC of first selected TBM");
  vFrame->AddFrame(g2Frame, new TGLayoutHints(kLHintsRight, fBorderL, fBorderR, fBorderT, 0));

  if (cmap.size() > 0) {
    unsigned int firsttbm(0);
    for (unsigned int i = 0; i < fSelectTbm.size(); ++i) {
      if (kButtonDown == fSelectTbm[i]->GetState()) {
	firsttbm = i;
	break;
      }
    }

    for (unsigned int itbm = 0; itbm < fGui->getApi()->_dut->getNTbms(); ++itbm) {
      map<string, uint8_t>  parids;
      amap = cmap[itbm];

      for (unsigned int i = 0; i < amap.size(); ++i) {
	if (static_cast<unsigned int>(itbm) == firsttbm) {
	  hFrame = new TGHorizontalFrame(g2Frame, 300, 30, kLHintsExpandX);
	  g2Frame->AddFrame(hFrame, new TGLayoutHints(kLHintsRight | kLHintsTop));
	  tb = new TGTextBuffer(5);
	  tl = new TGLabel(hFrame, amap[i].first.c_str());
	  hFrame->AddFrame(tl, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, fBorderL, fBorderR, fBorderT, 0));
	  tl->SetWidth(100);

	  te  = new TGTextEntry(hFrame, tb, i); te->SetWidth(70);
	  te->SetToolTipText("note that the numbers are in binary format (in case this is not obvious)");
	  hFrame->AddFrame(te, new TGLayoutHints(kLHintsCenterY | kLHintsCenterX, fBorderL, fBorderR, fBorderT, 0));
	  std::bitset<8> bits(amap[i].second);
	  te->SetText(Form("%s", bits.to_string().c_str()));
	  te->Connect("ReturnPressed()", "PixParTab", this, "setTbmParameter()");
	  te->Connect("TextChanged(const char *)", "PixParTab", this, "tbmYellow()");

	  tset = new TGTextButton(hFrame, "Set", i);
	  tset->SetToolTipText("set the parameter\nor click *return* after changing the numerical value");
	  tset->GetToolTip()->SetDelay(2000); // add a bit of delay to ease button hitting
	  tset->Connect("Clicked()", "PixParTab", this, "setTbmParameter()");
	  hFrame->AddFrame(tset, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, fBorderL, fBorderR, fBorderT, 0));
	}
	parids.insert(make_pair(amap[i].first, amap[i].second));
	fTbmTextEntries.insert(make_pair(amap[i].first, te));
	fTbmTextMap.insert(make_pair(i, amap[i].first));

      }
      fTbmParIds.push_back(parids);
      //      vFrame->AddFrame(g2Frame);
      g2Frame->SetWidth(g2Frame->GetDefaultWidth());
    }

  }

  // -----------------
  // -- DAC Parameters
  // -----------------
  g1Frame = new TGGroupFrame(fhFrame, "ROCs");
  fhFrame->AddFrame(g1Frame);

  vFrame = new TGVerticalFrame(g1Frame);
  g1Frame->AddFrame(vFrame, new TGLayoutHints(kLHintsLeft, fBorderL, fBorderR, fBorderT, fBorderB));

  TGHorizontalFrame *h1Frame = new TGHorizontalFrame(vFrame, 300, 30, kLHintsExpandX);
  vFrame->AddFrame(h1Frame);

  h1Frame->AddFrame(tset = new TGTextButton(h1Frame, "Select all", B_SELECTALL), 
		    new TGLayoutHints(kLHintsLeft, fBorderL, fBorderR, fBorderT, fBorderB));
  tset->SetToolTipText("Select all ROCs");
  tset->Connect("Clicked()", "PixParTab", this, "handleButtons()");
  h1Frame->AddFrame(tset = new TGTextButton(h1Frame, "Deselect all", B_DESELECTALL), 
		    new TGLayoutHints(kLHintsLeft, fBorderL, fBorderR, fBorderT, fBorderB));
  tset->SetToolTipText("Deselect all ROCs");
  tset->Connect("Clicked()", "PixParTab", this, "handleButtons()");

  h1Frame->AddFrame(tset = new TGTextButton(h1Frame, "Set DAC for ROC"), 
		    new TGLayoutHints(kLHintsLeft, fBorderL, fBorderR, fBorderT, fBorderB));
  tset->SetToolTipText("Change a DAC for one ROC and set this value for this ROC");
  tset->Connect("Clicked()", "PixParTab", this, "setOneRocParameter()");
  h1Frame->AddFrame(tset = new TGTextButton(h1Frame, "Set DAC for all ROCs"), 
		    new TGLayoutHints(kLHintsLeft, fBorderL, fBorderR, fBorderT, fBorderB));
  tset->SetToolTipText("Change a DAC for one ROC and set this value for all ROCs");
  tset->Connect("Clicked()", "PixParTab", this, "setAllRocParameter()");

  h1Frame->AddFrame(tset = new TGTextButton(h1Frame, "Save DAC"), 
		    new TGLayoutHints(kLHintsLeft, fBorderL, fBorderR, fBorderT, fBorderB));
  tset->SetToolTipText(Form("Write the DAC parameters of all selected ROCs to file\n(also the DACs of the righthand box will be written).\nThe output file will overwrite whatever is in the directory \"%s\"\n(change this in the top right part of the GUI)", fConfigParameters->getDirectory().c_str()));
  tset->Connect("Clicked()", "PixParTab", this, "saveDacParameters()");

  
  h1Frame->AddFrame(tset = new TGTextButton(h1Frame, "Save Trim"), 
		    new TGLayoutHints(kLHintsLeft, fBorderL, fBorderR, fBorderT, fBorderB));
  tset->SetToolTipText(Form("Write the trim parameters of all selected ROCs to file.\nThe output file will overwrite whatever is in the directory \"%s\"\n(change this in the top right part of the GUI)", fConfigParameters->getDirectory().c_str()));
  tset->Connect("Clicked()", "PixParTab", this, "saveTrimParameters()");


  cmap.clear();
  for (unsigned int i = 0; i < fGui->getApi()->_dut->getNRocs(); ++i) {
    vector<pair<string, uint8_t> > smap = fGui->getApi()->_dut->getDACs(i);
    cmap.push_back(smap);
  }
  updateSelection();

  if (cmap.size() > 0) {

    TGCompositeFrame *cFrame = new TGCompositeFrame(vFrame, 300, 30, kLHintsExpandX);
    vFrame->AddFrame(cFrame);
    cFrame->SetLayoutManager(new TGTableLayout(cFrame, cmap[0].size()+1, cmap.size()+1, 0, 0));

    // leftmost column with DAC names
    amap = cmap[0];
    for (unsigned int idac = 0; idac < amap.size(); ++idac) {
      cFrame->AddFrame(tl = new TGLabel(cFrame, amap[idac].first.c_str()), 
		       new TGTableLayoutHints(0, 1, idac+1, idac+2, kLHintsCenterX|kLHintsExpandX|kLHintsShrinkX|kLHintsFillX));
    }


    for (unsigned int iroc = 0; iroc < fGui->getApi()->_dut->getNRocs(); ++iroc) {
      map<string, TGTextEntry*>  rocTextEntries;
      amap = cmap[iroc];
      // ROC ID
      cFrame->AddFrame(tcb = new TGCheckButton(cFrame, "XX"), 
		       new TGTableLayoutHints(iroc+1, iroc+2, 0, 1, kFixedWidth));
      tcb->SetText(Form((iroc>10?"%d":" %d"), iroc)); 
      tcb->Connect("Clicked()", "PixParTab", this, "selectRoc()");      
      fSelectRoc.push_back(tcb);
      

      // DACs
      for (unsigned int idac = 0; idac < amap.size(); ++idac) {
	cFrame->AddFrame(te  = new TGTextEntry(cFrame, tb = new TGTextBuffer(3), idac),
			 new TGTableLayoutHints(iroc+1, iroc+2, idac+1, idac+2, kLHintsExpandX|kLHintsShrinkX|kLHintsFillX));
	te->SetAlignment(kTextRight);
	te->SetName(amap[idac].first.c_str()); 
	te->SetWidth(30);
	te->SetText(Form("%3d", int(amap[idac].second)));
	te->Connect("ReturnPressed()", "PixParTab", this, "setOneRocParameter()");
	te->Connect("TextChanged(const char *)", "PixParTab", this, "rocYellow()");
	
	rocTextEntries.insert(make_pair(amap[idac].first, te));
      }
      fRocTextEntries.push_back(rocTextEntries);
    }


    // -- by default enable all present
    if (fSelectRoc.size() > 0) {
      for (unsigned iroc = 0; iroc < fSelectRoc.size(); ++iroc) {
	fSelectRoc[iroc]->SetState(kButtonDown);
      }
    }

  



    /*    
    h1Frame = new TGHorizontalFrame(vFrame, 300, 30, kLHintsExpandX);
    vFrame->AddFrame(h1Frame, new TGLayoutHints(kLHintsBottom, fBorderL, fBorderR, fBorderT, fBorderB));

    TGVerticalFrame *v1Frame(0); 
    h1Frame->AddFrame(v1Frame = new TGVerticalFrame(h1Frame), 
		      new TGLayoutHints(kLHintsLeft, fBorderL, fBorderR, fBorderT, fBorderB));
    
    // -- left column with DAC names
    amap = cmap[0];
    hFrame = new TGHorizontalFrame(v1Frame, 300, 30, kLHintsExpandX);
    v1Frame->AddFrame(hFrame, new TGLayoutHints(kLHintsRight | kLHintsTop));
    tl = new TGLabel(hFrame, " ");
    hFrame->AddFrame(tl, new TGLayoutHints(kLHintsCenterY | kLHintsLeft, fBorderL, fBorderR, fBorderT, fBorderB));
    for (unsigned int idac = 0; idac < amap.size(); ++idac) {
      v1Frame->AddFrame(hFrame = new TGHorizontalFrame(v1Frame, 300, 25, kFixedHeight), 
			new TGLayoutHints(kLHintsRight|kLHintsCenterY));
      hFrame->AddFrame(tl = new TGLabel(hFrame, amap[idac].first.c_str()), 
		       new TGLayoutHints(kLHintsRight|kLHintsCenterY));
    }


    // -- one column per ROC
    for (unsigned int iroc = 0; iroc < fGui->getApi()->_dut->getNRocs(); ++iroc) {
      map<string, TGTextEntry*>  rocTextEntries;
      amap = cmap[iroc];
      h1Frame->AddFrame(v1Frame = new TGVerticalFrame(h1Frame), 
			new TGLayoutHints(kLHintsLeft, fBorderL, fBorderR, fBorderT, fBorderB));
      // ROC ID
      v1Frame->AddFrame(hFrame = new TGHorizontalFrame(v1Frame, 300, 26, kLHintsExpandX),
			new TGLayoutHints(kLHintsCenterY|kLHintsRight|kLHintsTop));

      hFrame->AddFrame(tcb = new TGCheckButton(hFrame, Form("%d", iroc)), 
		       new TGLayoutHints(kLHintsCenterY|kLHintsRight, fBorderL, fBorderR, fBorderT, fBorderB));
      tcb->Connect("Clicked()", "PixParTab", this, "selectRoc()");      
      fSelectRoc.push_back(tcb);

      // DACs
      for (unsigned int idac = 0; idac < amap.size(); ++idac) {
	v1Frame->AddFrame(hFrame = new TGHorizontalFrame(v1Frame, 300, 10), 
			  new TGLayoutHints(kLHintsRight | kLHintsTop));
	hFrame->AddFrame(te  = new TGTextEntry(hFrame, tb = new TGTextBuffer(3), idac),
			 new TGLayoutHints(kLHintsCenterY|kLHintsRight, fBorderL, fBorderR, fBorderT, fBorderB));
	te->SetAlignment(kTextRight);
	te->SetName(amap[idac].first.c_str()); 
	te->SetWidth(30);
	te->SetText(Form("%3d", int(amap[idac].second)));
	te->Connect("ReturnPressed()", "PixParTab", this, "setOneRocParameter()");
	te->Connect("TextChanged(const char *)", "PixParTab", this, "rocYellow()");
	
	rocTextEntries.insert(make_pair(amap[idac].first, te));
      }
      fRocTextEntries.push_back(rocTextEntries);
    }
    
    // -- by default enable all present
    if (fSelectRoc.size() > 0) {
      for (unsigned iroc = 0; iroc < fSelectRoc.size(); ++iroc) {
	fSelectRoc[iroc]->SetState(kButtonDown);
      }
    }


    */
  }


  fTabFrame->Layout();
  fTabFrame->MapSubwindows();
  fTabFrame->Resize(fTabFrame->GetDefaultSize());
  fTabFrame->MapWindow();

}


// ----------------------------------------------------------------------
PixParTab::PixParTab() {
  init(0, 0, "nada");
}

// ----------------------------------------------------------------------
void PixParTab::init(PixGui *p, ConfigParameters *cfg, std::string tabname) {
  fGui = p;
  fConfigParameters = cfg;
  fTabName = tabname;
}

// ----------------------------------------------------------------------
PixParTab::~PixParTab() {
  LOG(logDEBUG) << "PixParTab destructor";
}


// ----------------------------------------------------------------------
void PixParTab::handleButtons(Int_t id) {

  if (!fGui->getTabs()) return;

  if (id == -1) {
    TGButton *btn = (TGButton*)gTQSender;
    id = btn->WidgetId();
  }

  switch (id) {
  case B_SELECTALL: {
    LOG(logDEBUG) << "SELECT ALL";
    for (unsigned int i = 0; i < fSelectRoc.size(); ++i) {
      fSelectRoc[i]->SetState(kButtonDown);
    }
    updateSelection();
    break;
  }

  case B_DESELECTALL: {
    LOG(logDEBUG) << "DESELECT ALL";
    for (unsigned int i = 0; i < fSelectRoc.size(); ++i) {
      fSelectRoc[i]->SetState(kButtonUp);
    }
    updateSelection();
    break;
  }

  }
}


// ----------------------------------------------------------------------
void PixParTab::setTbParameter() {
  if (!fGui->getTabs()) return;

  TGButton *btn = (TGButton *) gTQSender;
  int id(-1);
  id = btn->WidgetId();
  if (-1 == id) {
    LOG(logDEBUG) << "ASLFDKHAPIUDF ";
    return;
  }

  string svalue = ((TGTextEntry*)(fTbTextEntries[fTbParIds[id]]))->GetText();
  uint8_t udac = atoi(svalue.c_str());

  // -- enforce: ctr = clk; sda = clk + 15; tin = clk + 5; 
  if (fLockClk && (
		   (fTbParIds[id] == "clk") 
		   || (fTbParIds[id] == "ctr")
		   || (fTbParIds[id] == "sda")
		   || (fTbParIds[id] == "tin")
		   )
      ) {
    int clk(0); 
    if (fTbParIds[id] == "clk") {
      clk = atoi(((TGTextEntry*)(fTbTextEntries[fTbParIds[id]]))->GetText()); 
    } else if (fTbParIds[id] == "ctr") {
      clk = atoi(((TGTextEntry*)(fTbTextEntries[fTbParIds[id]]))->GetText()); 
    } else if (fTbParIds[id] == "sda") {
      clk = atoi(((TGTextEntry*)(fTbTextEntries[fTbParIds[id]]))->GetText()) - 15; 
    } else if (fTbParIds[id] == "tin") {
      clk = atoi(((TGTextEntry*)(fTbTextEntries[fTbParIds[id]]))->GetText()) - 5; 
    }

    if (clk < 0) clk += 20;

    if (svalue != "clk") {
      LOG(logDEBUG)  << "PixParTab::setTbParameter: " << "clk" << ": " << int(clk);
      fConfigParameters->setTbParameter("clk", clk);
      ((TGTextEntry*)(fTbTextEntries["clk"]))->SetText(Form("%d", clk));
      ((TGTextEntry*)(fTbTextEntries["clk"]))->SetBackgroundColor(fGui->fWhite);
    }
    if (svalue != "ctr") {
      LOG(logDEBUG)  << "PixParTab::setTbParameter: " << "ctr" << ": " << int(clk);
      fConfigParameters->setTbParameter("ctr", clk);
      ((TGTextEntry*)(fTbTextEntries["ctr"]))->SetText(Form("%d", clk));
      ((TGTextEntry*)(fTbTextEntries["ctr"]))->SetBackgroundColor(fGui->fWhite);
    }
    if (svalue != "sda") {
      LOG(logDEBUG)  << "PixParTab::setTbParameter: " << "sda" << ": " << int(clk+15);
      fConfigParameters->setTbParameter("sda", clk+15);
      ((TGTextEntry*)(fTbTextEntries["sda"]))->SetText(Form("%d", clk+15));
      ((TGTextEntry*)(fTbTextEntries["sda"]))->SetBackgroundColor(fGui->fWhite);
    }
    if (svalue != "tin") {
      LOG(logDEBUG)  << "PixParTab::setTbParameter: " << "tin" << ": " << int(clk+5);
      fConfigParameters->setTbParameter("tin", clk+5);
      ((TGTextEntry*)(fTbTextEntries["tin"]))->SetText(Form("%d", clk+5));
      ((TGTextEntry*)(fTbTextEntries["tin"]))->SetBackgroundColor(fGui->fWhite);
    }
  } else {
    LOG(logDEBUG)  << "PixParTab::setTbParameter: " << fTbParIds[id] << ": " << int(udac);
    fConfigParameters->setTbParameter(fTbParIds[id], udac);
    ((TGTextEntry*)(fTbTextEntries[fTbParIds[id]]))->SetBackgroundColor(fGui->fWhite);
  }

 
  initTestboard();
}


// ----------------------------------------------------------------------
void PixParTab::tbYellow() {
  if (!fGui->getTabs()) return;

  TGButton *btn = (TGButton *) gTQSender;
  int id(-1);
  id = btn->WidgetId();
  if (-1 == id) {
    LOG(logDEBUG) << "ASLFDKHAPIUDF ";
    return;
  }

  ((TGTextEntry*)(fTbTextEntries[fTbParIds[id]]))->SetBackgroundColor(fGui->fYellow);

}


// ----------------------------------------------------------------------
void PixParTab::setPgSettings() {
  if (!fGui->getTabs()) return;

  TGButton *btn = (TGButton *) gTQSender;
  int id(-1);
  id = btn->WidgetId();
  if (-1 == id) {
    LOG(logDEBUG) << "ASLFDKHAPIUDF ";
    return;
  }

  // FIXME: currently not controllable from gui
  initTestboard();

}


// ----------------------------------------------------------------------
void PixParTab::setPowerSettings() {
  if (!fGui->getTabs()) return;

  TGButton *btn = (TGButton *) gTQSender;
  int id(-1);
  id = btn->WidgetId();
  if (-1 == id) {
    LOG(logDEBUG) << "ASLFDKHAPIUDF ";
    return;
  }

  string svalue = ((TGTextEntry*)(fPowerTextEntries[fPowerParIds[id]]))->GetText();
  double udac = atof(svalue.c_str());

  LOG(logDEBUG)  << "PixParTab::setPowerSettings: " << fPowerParIds[id] << ": " << udac;
  fConfigParameters->setTbPowerSettings(fPowerParIds[id], udac);
  ((TGTextEntry*)(fPowerTextEntries[fPowerParIds[id]]))->SetBackgroundColor(fGui->fWhite);
  initTestboard();

}


// ----------------------------------------------------------------------
void PixParTab::powerYellow() {
  TGButton *btn = (TGButton *) gTQSender;
  int id(-1);
  id = btn->WidgetId();
  if (-1 == id) {
    LOG(logDEBUG) << "ASLFDKHAPIUDF ";
    return;
  }

  ((TGTextEntry*)(fPowerTextEntries[fPowerParIds[id]]))->SetBackgroundColor(fGui->fYellow);
}


// ----------------------------------------------------------------------
void PixParTab::initTestboard() {

  vector<pair<string, uint8_t> > sig_delays = fConfigParameters->getTbSigDelays();
  vector<pair<string, double> > power_settings = fConfigParameters->getTbPowerSettings();
  vector<pair<std::string, uint8_t> > pg_setup = fConfigParameters->getTbPgSettings();;

  fGui->getApi()->initTestboard(sig_delays, power_settings, pg_setup);

}


// ----------------------------------------------------------------------
void PixParTab::setTbmParameter() {
  if (!fGui->getTabs()) return;
  LOG(logDEBUG)  << "PixParTab::setTbmParameter: ";

  TGButton *btn = (TGButton *) gTQSender;
  int id(-1);
  id = btn->WidgetId();
  if (-1 == id) {
    LOG(logDEBUG) << "ASLFDKHAPIUDF ";
    return;
  }

  string sdac = fTbmTextMap[id];
  string sval = fTbmTextEntries[sdac]->GetText();
  //  uint8_t udac = atoi(sval.c_str());
  bitset<8> bits(sval);
  uint8_t udac = static_cast<uint8_t>(bits.to_ulong());

  int itbm(-1);
  for (unsigned int i = 0; i < fSelectTbm.size(); ++i) {
    if (kButtonDown == fSelectTbm[i]->GetState()) {
      itbm = i;
      LOG(logDEBUG) << "TBM " << itbm << " is selected. id = " << id;
      fTbmParIds[itbm][sdac] = udac;
      LOG(logDEBUG)<< "xxx: ID = " << id << " TBM = " << itbm
		  << " -> " << sdac << " set to int(udac) = " << int(udac);
      fGui->getApi()->setTbmReg(sdac, udac, itbm);
      fTbmTextEntries[sdac]->SetBackgroundColor(fGui->fWhite);
    }
  }

}


// ----------------------------------------------------------------------
void PixParTab::tbmYellow() {
  TGButton *btn = (TGButton *) gTQSender;
  int id(-1);
  id = btn->WidgetId();
  if (-1 == id) {
    LOG(logDEBUG) << "ASLFDKHAPIUDF ";
    return;
  }

  string sdac = fTbmTextMap[id];
  ((TGTextEntry*)(fTbmTextEntries[sdac]))->SetBackgroundColor(fGui->fYellow);
}


// ----------------------------------------------------------------------
void PixParTab::updateParameters() {
  LOG(logDEBUG)  << "PixParTab::updateParameters: ";

  for (unsigned int iroc = 0; iroc < fGui->getApi()->_dut->getNRocs(); ++iroc) {
    map<string, TGTextEntry*> amap = fRocTextEntries[iroc];
    for (map<string, TGTextEntry*>::iterator mapit = amap.begin(); mapit != amap.end(); ++mapit) {
      mapit->second->SetText(Form("%d", (int)(fGui->getApi()->_dut->getDAC(iroc, mapit->first))));
    }
  }

  vector<pair<string, uint8_t> > tbparameters = fConfigParameters->getTbParameters();
  for (vector<pair<string, uint8_t> >::iterator itbparameters = tbparameters.begin(); itbparameters != tbparameters.end(); ++itbparameters)
    ((TGTextEntry*)(fTbTextEntries[itbparameters->first]))->SetText(Form("%d", int(itbparameters->second)));

  for (unsigned int itbm = 0; itbm < fGui->getApi()->_dut->getNTbms(); ++itbm) {
    vector<pair<string, uint8_t> > tbmparameters = fGui->getApi()->_dut->getTbmDACs(itbm);
    for (vector<pair<string, uint8_t> >::iterator itbmparameters = tbmparameters.begin(); itbmparameters != tbmparameters.end(); ++itbmparameters) {
      std::bitset<8> bits(itbmparameters->second);
      ((TGTextEntry*)(fTbmTextEntries[itbmparameters->first]))->SetText(Form("%s", bits.to_string().c_str()));
      fTbmParIds[itbm][itbmparameters->first]=itbmparameters->second;
    }
  }
  updateSelection();

}


// ----------------------------------------------------------------------
void PixParTab::selectRoc(int iroc) {
  bool selected(false);
  if (iroc == -1) {
    TGButton *btn = (TGButton *) gTQSender;
    if (kButtonDown == btn->GetState()) {
      selected = true;
    }
    iroc = btn->WidgetId();
  }

  LOG(logDEBUG) << "selectRoc: iroc = " << iroc << " selected? " << selected;

  if (false == selected) {
    iroc = 0;
    for (unsigned int i = 0; i < fSelectRoc.size(); ++i) {
      if (kButtonDown == fSelectRoc[i]->GetState()) {
	iroc = i;
	break;
      }
    }
  }

  updateSelection();
}


// ----------------------------------------------------------------------
void PixParTab::selectTbm(int id) {

  if (id == -1) {
    TGButton *btn = (TGButton *) gTQSender;
    id = btn->WidgetId();
  }

  LOG(logDEBUG) << "selectTbm: id = " << id;
  fSelectedTbm = id;

  map<string, uint8_t> amap = fTbmParIds[fSelectedTbm];
  for (map<string, uint8_t >::iterator mapit = amap.begin(); mapit != amap.end(); ++mapit) {

    std::bitset<8> bits((*mapit).second);
    fTbmTextEntries[(*mapit).first]->SetText(Form("%s", bits.to_string().c_str()));
  }
  updateSelection();
}


// ----------------------------------------------------------------------
void PixParTab::setAllRocParameter() {
  if (!fGui->getTabs()) return;
  set<TGTextEntry*>::iterator irc = fRocChanges.begin(); 
  set<TGTextEntry*>::iterator ircE = fRocChanges.end(); 
  for (; irc != ircE; ++irc) {
    string sdac = (*irc)->GetName();
    string sval = (*irc)->GetText();
    uint8_t udac = atoi(sval.c_str());
    int iroc(-1);
    for (unsigned int i = 0; i < fGui->getApi()->_dut->getNRocs(); ++i) {
      iroc = i;
      LOG(logDEBUG)<< "roc = " << iroc  << " -> " << sdac << " set to  int(udac) = " << int(udac);
      fGui->getApi()->setDAC(sdac, udac, iroc);
      (*irc)->SetFocus();
      (*irc)->SetBackgroundColor(fGui->fWhite);
      (*irc)->SetAlignment(kTextRight);
    }


  }

  fRocChanges.clear();
  updateParameters();
}

// ----------------------------------------------------------------------
void PixParTab::setOneRocParameter() {
  if (!fGui->getTabs()) return;
  cout << "fRocChanges.size() = " << fRocChanges.size() << endl;
  set<TGTextEntry*>::iterator irc; 
  for (irc = fRocChanges.begin(); irc != fRocChanges.end(); ++irc) {
    string sdac = (*irc)->GetName();
    string sval = (*irc)->GetText();
    uint8_t udac = atoi(sval.c_str());
    int iroc(-1);
    for (unsigned int i = 0; i < fRocTextEntries.size(); ++i) {
      for (map<string, TGTextEntry*>::iterator imap = fRocTextEntries[i].begin(); imap != fRocTextEntries[i].end(); ++imap) {
	if (imap->second == *irc) {
	  iroc = i; 
	  goto found;
	}
      }
    }
    
  found:
    if (iroc < 0) {
      LOG(logDEBUG) << "did not find ROC for TGTextEntry"; 
      continue;
    }
    LOG(logDEBUG)<< "roc = " << iroc << " -> " << sdac << " set to  int(udac) = " << int(udac);
    fGui->getApi()->setDAC(sdac, udac, iroc);
    (*irc)->SetFocus();
    (*irc)->SetBackgroundColor(fGui->fWhite);
    (*irc)->SetAlignment(kTextRight);
  }

  fRocChanges.clear();
  updateParameters();
}

// ----------------------------------------------------------------------
void PixParTab::rocYellow() {
  TGTextEntry *f = (TGTextEntry*) gTQSender; 
  if (f) {
    int iroc(-1);
    for (unsigned int i = 0; i < fRocTextEntries.size(); ++i) {
      for (map<string, TGTextEntry*>::iterator imap = fRocTextEntries[i].begin(); imap != fRocTextEntries[i].end(); ++imap) {
	if (imap->second == f) {
	  iroc = i; 
	  goto found; 
	}
      }
    }
  found:
    if (iroc < 0) {
      LOG(logDEBUG) << "did not find ROC for TGTextEntry"; 
      return;
    }
    string sdac = f->GetName();
    string sval = f->GetText();
    uint8_t udac = atoi(sval.c_str());
    
    if (udac != fGui->getApi()->_dut->getDAC(iroc, sdac)) {
      f->SetBackgroundColor(fGui->fYellow);
      f->SetAlignment(kTextRight);
      fRocChanges.insert(f);
    } else {
      f->SetBackgroundColor(fGui->fWhite);
      f->SetAlignment(kTextRight);
      fRocChanges.erase(f);
    }
  }

}

// ----------------------------------------------------------------------
void PixParTab::setLemo() {
  if (!fGui->getTabs()) return;
  LOG(logDEBUG)  << "PixParTab::setLemo: ";

  TGButton *btn = (TGButton *) gTQSender;
  int id(-1);
  id = btn->WidgetId();
  if (-1 == id) {
    LOG(logDEBUG) << "ASLFDKHAPIUDF ";
    return;
  }

}


// ----------------------------------------------------------------------
void PixParTab::lockClk() {
  if (fLockClk) {
    fLockClk = false;
  } else {
    fLockClk = true;
  }
}


// ----------------------------------------------------------------------
void PixParTab::saveTbParameters() {
  LOG(logDEBUG) << "save Tb parameters";
  fConfigParameters->writeTbParameterFile();
}

// ----------------------------------------------------------------------
void PixParTab::saveTbmParameters() {
  LOG(logDEBUG) << "save Tbm parameters";
  for (unsigned int itbm = 0; itbm < fGui->getApi()->_dut->getNTbms(); itbm += 2) {
    fConfigParameters->writeTbmParameterFile(itbm, fGui->getApi()->_dut->getTbmDACs(itbm), fGui->getApi()->_dut->getTbmDACs(itbm+1));
  }
}

// ----------------------------------------------------------------------
void PixParTab::saveDacParameters() {
  LOG(logDEBUG) << "save DAC parameters";
  vector<uint8_t> rocs = fGui->getApi()->_dut->getEnabledRocIDs();
  LOG(logDEBUG) << " rocs.size() = " << rocs.size();
  for (unsigned int iroc = 0; iroc < rocs.size(); ++iroc) {
    fConfigParameters->writeDacParameterFile(rocs[iroc], fGui->getApi()->_dut->getDACs(rocs[iroc]));
  }
}


// ----------------------------------------------------------------------
void PixParTab::saveTrimParameters() {
  LOG(logDEBUG) << "save Trim parameters";
  vector<uint8_t> rocs = fGui->getApi()->_dut->getEnabledRocIDs();
  for (unsigned int iroc = 0; iroc < rocs.size(); ++iroc) {
    fConfigParameters->writeTrimFile(rocs[iroc], fGui->getApi()->_dut->getEnabledPixels(rocs[iroc]));
  }

}


// ----------------------------------------------------------------------
vector<int> PixParTab::getSelectedTbms() {
  vector<int> result;
  for (unsigned int i = 0; i < fSelectTbm.size(); ++i) {
    if (kButtonDown == fSelectTbm[i]->GetState()) {
      result.push_back(i);
    }
  }
  return result;
}

// ----------------------------------------------------------------------
vector<int> PixParTab::getSelectedRocs() {
  vector<int> result;
  for (unsigned int i = 0; i < fSelectRoc.size(); ++i) {
    if (kButtonDown == fSelectRoc[i]->GetState()) {
      result.push_back(i);
    }
  }
  return result;
}


// ----------------------------------------------------------------------
void PixParTab::updateSelection() {
  map<int, int> id2idx;
  vector<int> selectedRocs = getSelectedRocs();
  for (unsigned int i = 0; i < fGui->getApi()->_dut->getNRocs(); ++i) {
    fGui->getApi()->_dut->setROCEnable(i, false);
  }
  for (unsigned i = 0; i < selectedRocs.size(); ++i) {
    fGui->getApi()->_dut->setROCEnable(selectedRocs[i], true);
    id2idx.insert(make_pair(selectedRocs[i], i));
  }

  fGui->updateSelectedRocs(id2idx);

  vector<int> selectedTbms = getSelectedTbms();
  for (unsigned int i = 0; i < fGui->getApi()->_dut->getNTbms(); ++i) {
    fGui->getApi()->_dut->setTBMEnable(i, false);
  }
  for (unsigned i = 0; i < selectedTbms.size(); ++i) {
    fGui->getApi()->_dut->setTBMEnable(selectedTbms[i], true);
  }

}
