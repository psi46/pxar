#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include <TStopwatch.h>
#include <TStyle.h>
#include "PixTestReadback.hh"
#include "log.h"
#include "helper.h"
#include "timer.h"
#include <fstream>
#include "PixUtil.hh"

using namespace std;
using namespace pxar;

ClassImp(PixTestReadback)

// ----------------------------------------------------------------------
PixTestReadback::PixTestReadback(PixSetup *a, std::string name) : PixTest(a, name), fParStretch(0), fParTriggerFrequency(100), fParResetROC(0),  fCalwVd(true), fCalwVa(false) {
  PixTest::init();
  init(); 
  LOG(logDEBUG) << "PixTestReadback ctor(PixSetup &a, string, TGTab *)";
  fTree = 0; 

  vector<vector<pair<string, double> > > iniCal;
  vector<pair<string, double> > prova1;
  fRbCal =  fPixSetup->getConfigParameters()->getReadbackCal();
  prova1 = fRbCal[0];
  LOG(logDEBUG)<<"***???!!! calibration constants for readback !!!???***";
  LOG(logDEBUG)<<"size is "<<prova1.size();
  for(int iv=0; iv<prova1.size(); iv++){
    LOG(logDEBUG)<<prova1[iv].first<<" "<<prova1[iv].second;
  }

  //initialize all calibration factors to 1
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  for(unsigned int iroc=0; iroc < rocIds.size(); iroc++){
    fPar0VdCal.push_back(1.);  
    fPar1VdCal.push_back(1.);  
    fPar0VaCal.push_back(1.);  
    fPar1VaCal.push_back(1.);  
    fPar0RbIaCal.push_back(1.);
    fPar1RbIaCal.push_back(1.);
    fPar0TbIaCal.push_back(1.);
    fPar1TbIaCal.push_back(1.);
    fPar2TbIaCal.push_back(1.);
    fRbVbg.push_back(0.);
  }

  for(unsigned int iroc=0; iroc < rocIds.size(); iroc++){
    for(std::vector<std::pair<std::string, double> >::iterator ical = fRbCal[iroc].begin(); ical != fRbCal[iroc].end(); ical++){
      if(!(ical->first.compare("par0vd"))){
	fPar0VdCal[iroc] = ical->second;
      }
      else if(!(ical->first.compare("par1vd"))){
	fPar1VdCal[iroc] = ical->second;
      }
      else if(!(ical->first.compare("par0va"))){
	fPar0VaCal[iroc] = ical->second;
      }
      else if(!(ical->first.compare("par1va"))){
	fPar1VaCal[iroc] = ical->second;
      }
      else if(!(ical->first.compare("par0rbia"))){
	fPar0RbIaCal[iroc] = ical->second;
      }
      else if(!(ical->first.compare("par1rbia"))){
	fPar1RbIaCal[iroc] = ical->second;
      }
      else if(!(ical->first.compare("par0tbia"))){
	fPar0TbIaCal[iroc] = ical->second;
      }
      else if(!(ical->first.compare("par1tbia"))){
	fPar1TbIaCal[iroc] = ical->second;
      }
      else if(!(ical->first.compare("par2tbia"))){
	fPar2TbIaCal[iroc] = ical->second;
      }
    }
  }

  fPhCal.setPHParameters(fPixSetup->getConfigParameters()->getGainPedestalParameters());
  fPhCalOK = fPhCal.initialized();
}


//----------------------------------------------------------
PixTestReadback::PixTestReadback() : PixTest() {
  LOG(logDEBUG) << "PixTestReadback ctor()";
  fTree = 0; 
}

//----------------------------------------------------------
PixTestReadback::~PixTestReadback() {
	LOG(logDEBUG) << "PixTestReadback dtor, saving tree ... ";
	fDirectory->cd();
	if (fTree && fParFillTree) fTree->Write();
}

// ----------------------------------------------------------------------
void PixTestReadback::init() {
  LOG(logDEBUG) << "PixTestReadback::init()";

  setToolTips();
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 
}

// ----------------------------------------------------------------------
void PixTestReadback::setToolTips() {
  fTestTip    = string("Run DAQ - data from each run will be added to the same histogram.") ;
  fSummaryTip = string("to be implemented") ;
  fStopTip    = string("Stop DAQ and save data.");
}

// ----------------------------------------------------------------------
void PixTestReadback::bookHist(string name) {
	fDirectory->cd();
	LOG(logDEBUG) << "nothing done with " << name;
}

// ----------------------------------------------------------------------
void PixTestReadback::stop(){
	// Interrupt the test 
	fDaq_loop = false;
	LOG(logINFO) << "Stop pressed. Ending test.";
}

// ----------------------------------------------------------------------
void PixTestReadback::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;
  if(!command.compare("stop")){
    stop();
    return;
  }
  if(!command.compare("calibratevd")){
    CalibrateVd();
    return;
  }
  if(!command.compare("calibrateva")){
    CalibrateVa();
    return;
  }
  if(!command.compare("calibrateia")){
    CalibrateIa();
    return;
  }
  if(!command.compare("readbackvbg")){
    readbackVbg();
    getCalibratedVbg();
    return;
  }
  if(!command.compare("getcalibratedvbg")){
    getCalibratedVbg();
    return;
  }
  if(!command.compare("getcalibratedia")){
    getCalibratedIa();
    return;
  }
  if(!command.compare("setvana")){
    setVana();
    return;
  }
  else{
    LOG(logINFO) << "Command " << command << " not implemented.";
  }
}

// ----------------------------------------------------------------------
bool PixTestReadback::setParameter(string parName, string sval) {
  bool found(false);
  fParOutOfRange = false;
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true;
      if (!parName.compare("readback")) {
	fParReadback = atoi(sval.c_str());
	setToolTips();
      }
      if (!parName.compare("usecalvd")) {
	PixUtil::replaceAll(sval, "checkbox(", ""); 
	PixUtil::replaceAll(sval, ")", ""); 
	fCalwVd = atoi(sval.c_str()); 
	LOG(logDEBUG)<<"fCalwVd set to "<<fCalwVd;
	setToolTips();
      }
      if (!parName.compare("usecalva")) {
	PixUtil::replaceAll(sval, "checkbox(", ""); 
	PixUtil::replaceAll(sval, ")", ""); 
	fCalwVa = atoi(sval.c_str()); 
	setToolTips();
      }
      if (!parName.compare("clockstretch")) {
	fParStretch = atoi(sval.c_str());
				setToolTips();
      }
      if (!parName.compare("filltree")) {
	fParFillTree = !(atoi(sval.c_str()) == 0);
	setToolTips();
      }
      if (!parName.compare("trgfrequency(khz)")){   // trigger frequency in kHz.
	fParTriggerFrequency = atoi(sval.c_str());
	if (fParTriggerFrequency == 0) {
	  LOG(logWARNING) << "PixTestReadback::setParameter() trgfrequency must be different from zero";
	  found = false; fParOutOfRange = true;
	}
      }
    }
  }
  return found;
}

//----------------------------------------------------------
bool PixTestReadback::setTrgFrequency(uint8_t TrgTkDel){
  int nDel = 0;
  uint8_t trgtkdel= TrgTkDel;
  double period_ns = 1 / (double)fParTriggerFrequency * 1000000; // trigger frequency in kHz.
  fParPeriod = (uint16_t)period_ns / 25;
  uint16_t ClkDelays = fParPeriod - trgtkdel;
  
  //add right delay between triggers:
  if (fParResetROC) {       //by default not reset (already done before daqstart)
    fPg_setup.push_back(make_pair("resetroc", 15));
    ClkDelays -= 15;
    nDel++;
  }
  while (ClkDelays>255){
    fPg_setup.push_back(make_pair("delay", 255));
    ClkDelays = ClkDelays - 255;
    nDel ++;
  }
  fPg_setup.push_back(make_pair("delay", ClkDelays));
  
  //then send trigger and token:
  fPg_setup.push_back(make_pair("trg", trgtkdel));
  fPg_setup.push_back(make_pair("tok", 0));
  
  fParPeriod = fParPeriod + 4 + nDel; //to align to the new pg minimum (1 additional clk cycle per PG call);
  
  return true;
}

// ----------------------------------------------------------------------
void PixTestReadback::pgToDefault() {
  fPg_setup.clear();
  LOG(logDEBUG) << "PixTestPattern::PG_Setup clean";
  
  fPg_setup = fPixSetup->getConfigParameters()->getTbPgSettings();
  fApi->setPatternGenerator(fPg_setup);
  LOG(logINFO) << "PixTestPattern::       pg_setup set to default.";
}

// ----------------------------------------------------------------------
void PixTestReadback::setHistos(){
  if (fParFillTree) bookTree();
  fHits.clear(); fPhmap.clear(); fPh.clear(); fQmap.clear(); fQ.clear();
  
  std::vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  TH1D *h1(0);
  TH2D *h2(0);
  TProfile2D *p2(0);
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    h2 = bookTH2D(Form("hits_C%d", rocIds[iroc]), Form("hits_C%d", rocIds[iroc]), 52, 0., 52., 80, 0., 80.);
    h2->SetMinimum(0.);
    h2->SetDirectory(fDirectory);
    setTitles(h2, "col", "row");
    fHistOptions.insert(make_pair(h2, "colz"));
    fHits.push_back(h2);
    
    p2 = bookTProfile2D(Form("phMap_C%d", rocIds[iroc]), Form("phMap_C%d", rocIds[iroc]), 52, 0., 52., 80, 0., 80.);
    p2->SetMinimum(0.);
    p2->SetDirectory(fDirectory);
    setTitles(p2, "col", "row");
    fHistOptions.insert(make_pair(p2, "colz"));
    fPhmap.push_back(p2);
    
    h1 = bookTH1D(Form("ph_C%d", rocIds[iroc]), Form("ph_C%d", rocIds[iroc]), 256, 0., 256.);
    h1->SetMinimum(0.);
    h1->SetDirectory(fDirectory);
    setTitles(h1, "ADC", "Entries/bin");
    fPh.push_back(h1);
    
    p2 = bookTProfile2D(Form("qMap_C%d", rocIds[iroc]), Form("qMap_C%d", rocIds[iroc]), 52, 0., 52., 80, 0., 80.);
    p2->SetMinimum(0.);
    p2->SetDirectory(fDirectory);
    setTitles(p2, "col", "row");
    fHistOptions.insert(make_pair(p2, "colz"));
    fQmap.push_back(p2);
    
    h1 = bookTH1D(Form("q_C%d", rocIds[iroc]), Form("q_C%d", rocIds[iroc]), 200, 0., 1000.);
    h1->SetMinimum(0.);
    h1->SetDirectory(fDirectory);
    setTitles(h1, "Q [Vcal]", "Entries/bin");
    fQ.push_back(h1);
  }
}


// ----------------------------------------------------------------------
void PixTestReadback::ProcessData(uint16_t numevents){
  
  LOG(logDEBUG) << "Getting Event Buffer";
  std::vector<pxar::Event> daqdat;
  
  if (numevents > 0) {
    for (unsigned int i = 0; i < numevents; i++) {
      pxar::Event evt = fApi->daqGetEvent();
      //Check if event is empty?
      if (evt.pixels.size() > 0)
	daqdat.push_back(evt);
    }
  }
  else
    daqdat = fApi->daqGetEventBuffer();
  
  LOG(logDEBUG) << "Processing Data: " << daqdat.size() << " events.";
  
  int pixCnt(0);
  int idx(-1);
  uint16_t q;
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  for (std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
    pixCnt += it->pixels.size();
    
    if (fParFillTree) {
      fTreeEvent.header = it->header;
      fTreeEvent.dac = 0;
      fTreeEvent.trailer = it->trailer;
      fTreeEvent.npix = it->pixels.size();
    }
    
    for (unsigned int ipix = 0; ipix < it->pixels.size(); ++ipix) {
      idx = getIdxFromId(it->pixels[ipix].roc());
      if(idx == -1) {
	LOG(logWARNING) << "PixTestReadback::ProcessData() wrong 'idx' value --> return";
	return;    			
      }
      fHits[idx]->Fill(it->pixels[ipix].column(), it->pixels[ipix].row());
      fPhmap[idx]->Fill(it->pixels[ipix].column(), it->pixels[ipix].row(), it->pixels[ipix].value());
      fPh[idx]->Fill(it->pixels[ipix].value());
      
      if (fPhCalOK) {
	q = static_cast<uint16_t>(fPhCal.vcal(it->pixels[ipix].roc(), it->pixels[ipix].column(),	
					      it->pixels[ipix].row(), it->pixels[ipix].value()));
      }
      else {
	q = 0;
      }
      fQ[idx]->Fill(q);
      fQmap[idx]->Fill(it->pixels[ipix].column(), it->pixels[ipix].row(), q);
      if (fParFillTree) {
	fTreeEvent.proc[ipix] = it->pixels[ipix].roc();
	fTreeEvent.pcol[ipix] = it->pixels[ipix].column();
	fTreeEvent.prow[ipix] = it->pixels[ipix].row();
	fTreeEvent.pval[ipix] = it->pixels[ipix].value();
	fTreeEvent.pq[ipix] = q;
      }
    }
    if (fParFillTree) fTree->Fill();
  }
  
  //to draw the hitsmap as 'online' check.
  TH2D* h2 = (TH2D*)(fHits.back());
  h2->Draw(getHistOption(h2).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);
  PixTest::update();
  
  LOG(logINFO) << Form("events read: %6ld, pixels seen: %3d, hist entries: %4d",
		       daqdat.size(), pixCnt,	static_cast<int>(fHits[0]->GetEntries()));	
}

// ----------------------------------------------------------------------
void PixTestReadback::FinalCleaning() {
  
  // Reset the pg_setup to default value.
  pgToDefault();
  //clean local variables:
  fPg_setup.clear();
}

// ----------------------------------------------------------------------
void PixTestReadback::doTest() {
  LOG(logINFO) << "PixTestReadback::doTest() start.";
  
  CalibrateVd();
  CalibrateVa();
  readbackVbg();
  vector<double> VBG =  getCalibratedVbg();
  CalibrateIa();
  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    (*il)->Draw((getHistOption(*il)).c_str()); 
  }
  
//  ofstream myfile;
//  myfile.open ((fPixSetup->getConfigParameters()->getDirectory()+"/ReadbackCal.dat").c_str(), ios::app);
//  myfile << "##Voltages" << endl;
//  myfile <<"#par0Vd    par1Vd    par0Va    par1Va    directory    rbVbg    Vbg    calVd    calVa    "<<endl;
//  myfile<<fPar0VdCal<<"    "<<fPar1VdCal<<"    "<<fPar0VaCal<<"    "<<fPar1VaCal<<"    "<<fPixSetup->getConfigParameters()->getDirectory()<<"    "<<fRbVbg<<"    "<<VBG<<"	"<<fCalwVd<<"    "<<fCalwVa<<endl;
//  myfile << "##Currents " << endl;
//  myfile << "par0IaRb    par1IaRb    par0IaTb    par1IaTb    par2IaTb" << endl;
//  myfile << fPar0RbIaCal << "    " << fPar1RbIaCal << "    " << fPar0TbIaCal << "    " << fPar1TbIaCal << "    " << fPar2TbIaCal << endl;
//  myfile.close();
  
 LOG(logINFO) << "PixTestReadback::doTest() done";

}


void PixTestReadback::CalibrateIa(){
  cacheDacs();
  //readback DAC set to 12 (i.e. Ia)
  fParReadback=12;

  vector<uint8_t> readback;

  string name, title;
  //  TH1D* h_rbIa = new TH1D("rbIa","rbIa", 256, 0., 256.);
  //  TH1D* h_tbIa = new TH1D("tbIa","tbIa", 256, 0., 256.);
  TH1D* hrb(0);
  vector<TH1D*> hs_rbIa, hs_tbIa;
  double tbIa = 0.;
  map<uint8_t, vector<uint8_t > > rbIa;
  while(readback.size()<1){
    readback=daqReadback("vana", (uint8_t)80, fParReadback);  
  }
  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    name = Form("rbIa_C%d", iroc);
    title=name;
    hrb = bookTH1D(name, title, 256, 0., 256);
    hs_rbIa.push_back(hrb);
    name = Form("tbIa_C%d", iroc);
    title=name;
    hrb = bookTH1D(name, title, 256, 0., 256);
    hs_tbIa.push_back(hrb);
  }

  uint8_t vana=0;
  //measuring average current offset from other ROCs
  //  vector<uint8_t> readback_offset;
  double ioff16=0.;
  fApi->setDAC("vana", 0);
  ioff16 = fApi->getTBia()*1E3;
  double avIoff=0;
  avIoff = ioff16*(readback.size()-1)/(readback.size());

  for(unsigned int iroc=0; iroc < readback.size(); iroc++){ 
    fApi->setDAC("vana", 0);
    for(int ivana=0; ivana<52; ivana++){
      vana = (uint8_t)ivana*5;
      do{readback=daqReadback("vana", vana, iroc, fParReadback);
      }  while(readback.size()<1);
      rbIa.insert(make_pair(vana, readback));   
      fApi->setDAC("vana", vana, iroc);
      tbIa = fApi->getTBia()*1E3;
      hs_rbIa[iroc]->Fill(vana, readback[iroc]);//should this be corrected as well?
      hs_tbIa[iroc]->Fill(vana, tbIa-avIoff);//tbIa corrected for offset
    }
  }

//  for(int ivana=0; ivana<52; ivana++){
//    vana = (uint8_t)ivana*5;
//    LOG(logDEBUG)<<"////????**** Vana =  "<<(int)vana<<", rb = "<<(int)rbIa[vana][0]<<"****????////";
//    }
  
  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    hs_rbIa[iroc]->GetXaxis()->SetTitle("Vana [DAC]");
    hs_rbIa[iroc]->GetYaxis()->SetTitle("Ia_rb [ADC]");
    hs_tbIa[iroc]->GetXaxis()->SetTitle("Vana [DAC]");
    hs_tbIa[iroc]->GetYaxis()->SetTitle("Ia_TB [mA]");
  }
  gStyle->SetOptFit(1111);

  vector<double> rb_vanaMax(readback.size(), 0.);
  vector<double> tb_vanaMax(readback.size(), 0.);
   
  //protection to exclude plateu from fit
  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    rb_vanaMax[iroc] = hs_rbIa[iroc]->GetBinCenter(hs_rbIa[iroc]->FindFirstBinAbove(254));

    LOG(logDEBUG)<<"Vana max for fit:"<<endl<<"rb: "<<rb_vanaMax[iroc]<<endl<<"tb :"<<tb_vanaMax[iroc];
  }


  TF1* frb;
  TF1* ftb;
  vector<TF1*> v_frb, v_ftb;
  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    name = Form("lin_rb_C%d", iroc);
    frb = new TF1(name.c_str(), "[0] + x*[1]", 0, rb_vanaMax[iroc]);
    v_frb.push_back(frb);
    name = Form("pol2_ftb_C%d", iroc);
    ftb = new TF1(name.c_str(), "[0] + x*[1] + x*x*[2] ", 0, 255);
    v_ftb.push_back(ftb);
  }

  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    hs_rbIa[iroc]->Fit(v_frb[iroc], "W", "", 0., rb_vanaMax[iroc]);
    hs_tbIa[iroc]->Fit(v_ftb[iroc]);

    //  LOG(logDEBUG)<<"Number of points for rb fit "<<frb->GetNumberFitPoints();

  fPar0RbIaCal[iroc]=v_frb[iroc]->GetParameter(0);
  fPar1RbIaCal[iroc]=v_frb[iroc]->GetParameter(1);
  fPar0TbIaCal[iroc]=v_ftb[iroc]->GetParameter(0);
  fPar1TbIaCal[iroc]=v_ftb[iroc]->GetParameter(1);
  fPar2TbIaCal[iroc]=v_ftb[iroc]->GetParameter(2);
  }


  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    for(std::vector<std::pair<std::string, double> >::iterator ical = fRbCal[iroc].begin(); ical != fRbCal[iroc].end(); ical++){
      if(!(ical->first.compare("par0rbia"))){
	ical->second = fPar0RbIaCal[iroc];
      }
      else if(!(ical->first.compare("par1rbia"))){
	ical->second = fPar1RbIaCal[iroc];
      }
      else if(!(ical->first.compare("par0tbia"))){
	ical->second = fPar0TbIaCal[iroc];
      }
      else if(!(ical->first.compare("par1tbia"))){
	ical->second = fPar1TbIaCal[iroc];
      }
      else if(!(ical->first.compare("par2tbia"))){
	ical->second = fPar2TbIaCal[iroc];
      }
    }
  }
  

  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    for(std::vector<std::pair<std::string, double> >::iterator ical = fRbCal[iroc].begin(); ical != fRbCal[iroc].end(); ical++){
      LOG(logDEBUG)<<"debug: "<<ical->first<<" "<<ical->second;
    }
  }

  TH1D* h_rbIaCal (0);
  vector<TH1D*> hs_rbIaCal;
  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    name = Form("rbIaCal_C%d", iroc);
    title = name;
    h_rbIaCal = bookTH1D(name,title, 256, 0., 256.);
    hs_rbIaCal.push_back(h_rbIaCal);
  }

  for(int ivana=0; ivana<52; ivana++){
    vana = (uint8_t)ivana*5;
    for(unsigned int iroc=0; iroc < rbIa[vana].size(); iroc++){
      LOG(logDEBUG)<<"step ivana = "<<ivana;
      //    h_rbIaCal->Fill(vana, ((tbpar1/rbpar1)*(rbIa[vana]-rbpar0)+tbpar0));
      hs_rbIaCal[iroc]->Fill(vana, (rbIa[vana][iroc]*rbIa[vana][iroc]*fPar2TbIaCal[iroc]/fPar1RbIaCal[iroc]/fPar1RbIaCal[iroc] + rbIa[vana][iroc]/fPar1RbIaCal[iroc]/fPar1RbIaCal[iroc]*(fPar1RbIaCal[iroc]*fPar1TbIaCal[iroc]-2*fPar0RbIaCal[iroc]*fPar2TbIaCal[iroc]) + (fPar0RbIaCal[iroc]*fPar0RbIaCal[iroc]*fPar2TbIaCal[iroc] - fPar0RbIaCal[iroc]*fPar1TbIaCal[iroc])/fPar1RbIaCal[iroc] + fPar0TbIaCal[iroc]));
    }
  }

  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    hs_rbIaCal[iroc]->GetXaxis()->SetTitle("Vana [DAC]");
    hs_rbIaCal[iroc]->GetYaxis()->SetTitle("Ia_rb_cal [mA]");
    hs_rbIaCal[iroc]->SetLineColor(kBlue);
    fHistList.push_back(hs_rbIa[iroc]);
    fHistList.push_back(hs_rbIaCal[iroc]);
    fHistList.push_back(hs_tbIa[iroc]);
  }
  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    fPixSetup->getConfigParameters()->writeReadbackFile(iroc, fRbCal[iroc]);    
  }
  
  restoreDacs();
}

std::vector<double> PixTestReadback::getCalibratedIa(){
  //readback DAC set to 12 (i.e. Ia)
  fParReadback=12;

  vector<uint8_t> readback;

  while(readback.size()<1){
    readback=daqReadbackIa();
  }
  vector<double> calIa(readback.size(), 0.);
  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    //  calIa = ((fPar1TbIaCal/fPar1RbIaCal)*((double)readback-fPar0RbIaCal)+fPar0TbIaCal);
    calIa[iroc] = (((double)readback[iroc])*((double)readback[iroc])*fPar2TbIaCal[iroc]/fPar1RbIaCal[iroc]/fPar1RbIaCal[iroc] + ((double)readback[iroc])/fPar1RbIaCal[iroc]/fPar1RbIaCal[iroc]*(fPar1RbIaCal[iroc]*fPar1TbIaCal[iroc]-2*fPar0RbIaCal[iroc]*fPar2TbIaCal[iroc]) + (fPar0RbIaCal[iroc]*fPar0RbIaCal[iroc]*fPar2TbIaCal[iroc] - fPar0RbIaCal[iroc]*fPar1TbIaCal[iroc])/fPar1RbIaCal[iroc] + fPar0TbIaCal[iroc]);
    LOG(logDEBUG)<<"Calibrated analog current is "<<calIa[iroc];
  }
  return  calIa;
}

double PixTestReadback::getCalibratedIa(unsigned int iroc){
  //readback DAC set to 12 (i.e. Ia)
  fParReadback=12;

  vector<uint8_t> readback;
  
  while(readback.size()<1){
    readback=daqReadbackIa();
  }
  double calIa;
  calIa = (((double)readback[iroc])*((double)readback[iroc])*fPar2TbIaCal[iroc]/fPar1RbIaCal[iroc]/fPar1RbIaCal[iroc] + ((double)readback[iroc])/fPar1RbIaCal[iroc]/fPar1RbIaCal[iroc]*(fPar1RbIaCal[iroc]*fPar1TbIaCal[iroc]-2*fPar0RbIaCal[iroc]*fPar2TbIaCal[iroc]) + (fPar0RbIaCal[iroc]*fPar0RbIaCal[iroc]*fPar2TbIaCal[iroc] - fPar0RbIaCal[iroc]*fPar1TbIaCal[iroc])/fPar1RbIaCal[iroc] + fPar0TbIaCal[iroc]);
  LOG(logDEBUG)<<"Calibrated analog current is "<<calIa;
  
  return  calIa;
}

void PixTestReadback::CalibrateVana(){
//  cacheDacs();
//  //readback DAC set to 11 (i.e. Vana)
//  fParReadback=12;
//
//  int readback=0;
//
//  TH1D* h_rbVana = new TH1D("rbVana","rbVana", 256, 0., 256.);
//  TH1D* h_dacVana = new TH1D("dacVana","dacVana", 256, 0., 256.);
//  vector<double > rbVana;
//  
//  for(uint8_t vana=0; vana<255; vana++){
//    readback=daqReadback("vana", vana, fParReadback);
//    rbVana.push_back(readback);
//    h_rbVana->Fill(vana, readback);
//    h_dacVana->Fill(vana, vana);
//  }
//
//  double rb_vanaMax=0.;
//
//  rb_vanaMax = h_rbVana->GetBinCenter(h_rbVana->FindFirstBinAbove(254));
//
//  LOG(logDEBUG)<<"Vana max for fit:"<<endl<<"rb: "<<rb_vanaMax;
//
//  TF1* frb = new TF1("lin_rb", "[0] + x*[1]", 0, rb_vanaMax);
//  TF1* fdac = new TF1("lin_fdac", "[0] + x*[1]", 0, 255);
//
//  h_rbVana->Fit(frb, "W", "", 0., rb_vanaMax);
//  h_dacVana->Fit(fdac);
//
//  LOG(logDEBUG)<<"Number of points for rb fit "<<frb->GetNumberFitPoints();
//
//  double rbpar0=0., rbpar1=0., dacpar0=0., dacpar1=0.;
//  rbpar0=frb->GetParameter(0);
//  rbpar1=frb->GetParameter(1);
//  dacpar0=fdac->GetParameter(0);
//  dacpar1=fdac->GetParameter(1);
//
//  TH1D* h_rbVanaCal = new TH1D("rbVana","rbVana", 256, 0., 256.);
//  for(int vana=0; vana<256; vana++){
//    h_rbVanaCal->Fill(vana, ((dacpar1/rbpar1)*(rbVana[vana]-rbpar0)+dacpar0));
//  }
//
//  h_rbVanaCal->SetLineColor(kBlue);
//  fHistOptions.insert(make_pair(h_rbVanaCal,"same"));
//
//  fHistList.push_back(h_rbVana);
//  fHistList.push_back(h_rbVanaCal);
//  fHistList.push_back(h_dacVana);
//  restoreDacs();
}

void PixTestReadback::CalibrateVd(){
  cacheDacs();

  //readback DAC set to 8 (i.e. Vd)
  fParReadback=8;

  vector<uint8_t> readback;

  string name, title;
//  TH1D* h_rbVd = new TH1D("rbVd","rbVd", 500, 0., 5.);
//  TH1D* h_dacVd = new TH1D("dacVd","dacVd", 500, 0., 5.);
  TH1D* hrb(0);
  vector<TH1D*> hs_rbVd, hs_dacVd;
  vector<double > rbVd;
  double Vd;

  //dry run to avoid spikes
  while(readback.size()<1){
    readback=daqReadback("vd", 2.1, fParReadback);
  }
  //book histos
  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    name = Form("rbVd_C%d", iroc);
    title = name;
    hrb = bookTH1D(name, title, 500, 0., 5.);
    hs_rbVd.push_back(hrb);
    name = Form("dacVd_C%d", iroc);
    title = name;
    hrb = bookTH1D(name, title, 500, 0., 5.);
    hs_dacVd.push_back(hrb);
  }
  readback.clear();

  for(int iVd=0; iVd<18; iVd++){
    LOG(logDEBUG)<<"/****:::::: CALIBRATE VD :::::****/";
    Vd = 2.1 + iVd*0.05;
    LOG(logDEBUG)<<"Digital voltage will be set to: "<<Vd;
    do{
      readback=daqReadback("vd", Vd, fParReadback);
    }  while(readback.size()<1);
    for(unsigned int iroc=0; iroc < readback.size(); iroc++){
      LOG(logDEBUG)<<"Voltage "<<Vd<<", readback "<<(int)readback[iroc];
    //    rbVd.push_back(readback);
      hs_rbVd[iroc]->Fill(Vd, readback[iroc]);
      hs_dacVd[iroc]->Fill(Vd, fApi->getTBvd());
    }
  }
  
  for(unsigned int iroc=0; iroc < readback.size(); iroc++){  
    hs_rbVd[iroc]->GetXaxis()->SetTitle("Vd [V]");
    hs_rbVd[iroc]->GetYaxis()->SetTitle("Vd_rb [ADC]");
    hs_dacVd[iroc]->GetXaxis()->SetTitle("Vd set [V]");
    hs_dacVd[iroc]->GetYaxis()->SetTitle("Vd TB [V]"); 
  }

  gStyle->SetOptFit(1111);

  TF1* frb (0);
  vector<TF1*> v_frb;
  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    name = Form("lin_vd_C%d", iroc);
    frb = new TF1(name.c_str(), "[0] + x*[1]");
    v_frb.push_back(frb);
  }
  
  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    hs_rbVd[iroc]->Fit(v_frb[iroc], "W", "");

    fPar0VdCal[iroc]=v_frb[iroc]->GetParameter(0);
    fPar1VdCal[iroc]=v_frb[iroc]->GetParameter(1);
    
    fHistList.push_back(hs_rbVd[iroc]);
    fHistList.push_back(hs_dacVd[iroc]);
  }


  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    for(std::vector<std::pair<std::string, double> >::iterator ical = fRbCal[iroc].begin(); ical != fRbCal[iroc].end(); ical++){
      if(!(ical->first.compare("par0vd"))){
	ical->second = fPar0VdCal[iroc];
      }
      else if(!(ical->first.compare("par1vd"))){
	ical->second = fPar1VdCal[iroc];
      }
    }
  }

  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    fPixSetup->getConfigParameters()->writeReadbackFile(iroc, fRbCal[iroc]);    
  }
  
  restoreDacs();
}


void PixTestReadback::readbackVbg(){
  cacheDacs();
  //readback DAC set to 11 (i.e. Vbg)
  fParReadback=11;

  vector<uint8_t> readback;

  while(readback.size()<1){
    readback = daqReadback("vd", 2.5, fParReadback);
  }
  vector<double> avReadback(readback.size(), 0.);

  double Vd;

  int n_meas=0;
  bool okRb=true;

  for(int i=0; i<10; i++){
    LOG(logDEBUG)<<"/****:::::: READBACK VBG :::::****/";
    Vd = 2.5;
    LOG(logDEBUG)<<"Digital voltage will be set to: "<<Vd;

    do{ readback = daqReadback("vd", Vd, fParReadback);
    }  while(readback.size()<1);
    for(unsigned int iroc=0; iroc < readback.size(); iroc++){
      if(0==readback[iroc]){
	okRb=false;
	break;
      }
    }
    for(unsigned int iroc=0; iroc < readback.size(); iroc++){
      if (okRb){
	avReadback[iroc]+=(double)readback[iroc];
	n_meas++;
      }
      LOG(logDEBUG)<<"Voltage "<<Vd<<", average readback "<<(double)readback[iroc]/(i+1);
    }
  }
  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    fRbVbg[iroc] = avReadback[iroc]/n_meas;
  }

  restoreDacs();
}

vector<double> PixTestReadback::getCalibratedVbg(){
  vector<double> calVbg(fRbVbg.size(), 0.);
  //0.5 needed because Vbg rb has twice the sensitivity Vd and Va have
  if(fCalwVd){
    LOG(logDEBUG)<<"Vbg will be calibrated using Vd calibration";
    for(unsigned int iroc=0; iroc < calVbg.size(); iroc++){
      calVbg[iroc]=0.5*(fRbVbg[iroc]-fPar0VdCal[iroc])/fPar1VdCal[iroc];
    }
  }
  else if(fCalwVa){
    LOG(logDEBUG)<<"Vbg will be calibrated using Va calibration";
    for(unsigned int iroc=0; iroc < calVbg.size(); iroc++){
      calVbg[iroc]=0.5*(fRbVbg[iroc]-fPar0VaCal[iroc])/fPar1VaCal[iroc];
    }
  }
  else{
    LOG(logDEBUG)<<"No calibration option specified. Please select one and retry.";
    return calVbg;
  }
   for(unsigned int iroc=0; iroc < calVbg.size(); iroc++){
     LOG(logDEBUG)<<"/*/*/*/*::: ROC "<<iroc<<": calibrated Vbg = "<<calVbg[iroc]<<" :::*/*/*/*/";
   }
  return calVbg;
}

void PixTestReadback::CalibrateVa(){
  cacheDacs();

  //readback DAC set to 9 (i.e. Va)
  fParReadback=9;

  vector<uint8_t> readback;

  string name, title;
  //  TH1D* h_rbVa = new TH1D("rbVa","rbVa", 500, 0., 5.);
  //TH1D* h_dacVa = new TH1D("dacVa","dacVa", 500, 0., 5.);
  TH1D* hrb(0);
  vector<TH1D*> hs_rbVa, hs_dacVa;
  vector<double > rbVa;
  double Va;
  
  //dry run to avoid spikes
  while(readback.size()<1){
    readback=daqReadback("va", 1.5, fParReadback);
  }
  //book histos
  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    name = Form("rbVa_C%d", iroc);
    title = name;
    hrb = bookTH1D(name, title, 500, 0., 5.);
    hs_rbVa.push_back(hrb);
    name = Form("dacVa_C%d", iroc);
    title = name;
    hrb = bookTH1D(name, title, 500, 0., 5.);
    hs_dacVa.push_back(hrb);
  }
  readback.clear();

  for(int iVa=0; iVa<18; iVa++){
    LOG(logDEBUG)<<"/****:::::: CALIBRATE VA FUNCTION :::::****/";
    Va = 1.5 + iVa*0.05;
    LOG(logDEBUG)<<"Analog voltage will be set to: "<<Va;
    do{readback=daqReadback("va", Va, fParReadback);
    }  while(readback.size()<1);
    for(unsigned int iroc=0; iroc < readback.size(); iroc++){
      //      LOG(logDEBUG)<<"Voltage "<<Va<<", readback "<<readback;
      //      rbVa.push_back(readback);
      hs_rbVa[iroc]->Fill(Va, readback[iroc]);
      hs_dacVa[iroc]->Fill(Va, fApi->getTBva());
    }
  }

  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    hs_rbVa[iroc]->GetXaxis()->SetTitle("Va [V]");
    hs_rbVa[iroc]->GetYaxis()->SetTitle("Va_rb [ADC]");
    hs_dacVa[iroc]->GetXaxis()->SetTitle("Va set [V]");
    hs_dacVa[iroc]->GetYaxis()->SetTitle("Va TB [V]");
  }

  gStyle->SetOptFit(1111);

  vector<double> rb_VaMax(readback.size(), 0.);
  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    rb_VaMax[iroc] = hs_rbVa[iroc]->GetBinCenter(hs_rbVa[iroc]->FindFirstBinAbove(254));
    
    LOG(logDEBUG)<<"Va max for fit on ROC "<<iroc<<" : "<<rb_VaMax[iroc];
  }

  
  TF1* frb (0);
  vector<TF1*> v_frb;
  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    name = Form("lin_va_C%d", iroc);
    frb = new TF1(name.c_str(), "[0] + x*[1]", 0, rb_VaMax[iroc]);
    v_frb.push_back(frb);
  }

  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    hs_rbVa[iroc]->Fit(v_frb[iroc], "W", "", 0., rb_VaMax[iroc]);

    //  LOG(logDEBUG)<<"Number of points for rb fit "<<frb->GetNumberFitPoints();
    
    fPar0VaCal[iroc]=v_frb[iroc]->GetParameter(0);
    fPar1VaCal[iroc]=v_frb[iroc]->GetParameter(1);
    
    fHistList.push_back(hs_rbVa[iroc]);
    fHistList.push_back(hs_dacVa[iroc]);
  }
  
  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    for(std::vector<std::pair<std::string, double> >::iterator ical = fRbCal[iroc].begin(); ical != fRbCal[iroc].end(); ical++){
      if(!(ical->first.compare("par0va"))){
	ical->second = fPar0VaCal[iroc];
      }
      else if(!(ical->first.compare("par1va"))){
	ical->second = fPar1VaCal[iroc];
      }
    }
  }

  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    fPixSetup->getConfigParameters()->writeReadbackFile(iroc, fRbCal[iroc]);    
  }

  restoreDacs();
}

vector<uint8_t> PixTestReadback::daqReadback(string dac, double vana, int8_t parReadback){

  PixTest::update();
  fDirectory->cd();
  fPg_setup.clear();

  if (!dac.compare("vana")){
    LOG(logDEBUG)<<"Wrong daqReadback function called!!!";
  }
  else if (!dac.compare("vd")){
    vector<pair<string,double > > powerset = fPixSetup->getConfigParameters()->getTbPowerSettings();
    for(std::vector<std::pair<std::string,double> >::iterator pow_it=powerset.begin(); pow_it!=powerset.end(); pow_it++){
      if( pow_it->first.compare("vd") == 0){
	pow_it->second = vana;
      }
    }
    fApi->setTestboardPower(powerset);
  }
else if (!dac.compare("va")){
    vector<pair<string,double > > powerset = fPixSetup->getConfigParameters()->getTbPowerSettings();
    for(std::vector<std::pair<std::string,double> >::iterator pow_it=powerset.begin(); pow_it!=powerset.end(); pow_it++){
      if( pow_it->first.compare("va") == 0){
	pow_it->second = vana;
      }
    }
    fApi->setTestboardPower(powerset);
  }

  fApi->setDAC("readback", parReadback);

  doDAQ();

  std::vector<std::vector<uint16_t> > rb;
  rb = fApi->daqGetReadback();
  std::vector<uint8_t> rb_val;
//  for(uint8_t i=0; i<rb.size(); i++){
//      for(uint8_t j=0; i<rb[i].size(); j++){
//	LOG(logDEBUG)<<";;;*** DEBUG readback ***;;; "<<i<<" "<<j<<" :"<<rb[i][j];
//      }
//  }
  for(uint8_t i=0; i<rb.size(); i++){
    //    for(uint8_t j=0; j<rb[i].size(); j++){
    // LOG(logDEBUG)<<"Readback values for vana = "<<(int)vana<<" : "<<(int)(rb[i][j]&0xff);
    rb_val.push_back( rb[i][ rb[i].size()-1 ]&0xff ); // read the last (size-1) readback word read out for ROC i
      // }
  }
  
  //::::::::::::::::::::::::::::::
  //DAQ - THE END.

  FinalCleaning();
  fApi->setClockStretch(0, 0, 0); //No Stretch after trigger, 0 delay
  return rb_val;
 }


std::vector<uint8_t> PixTestReadback::daqReadback(string dac, uint8_t vana, int8_t parReadback){

  PixTest::update();
  fDirectory->cd();
  fPg_setup.clear();

  if (!dac.compare("vana")){
    fApi->setDAC(dac.c_str(), vana);
  }
  else {
    LOG(logDEBUG)<<"Wrong daqReadback function called!!!";
    //fApi->_hal->setTBvd(vana);
  }

  fApi->setDAC("readback", parReadback);
  doDAQ();

  std::vector<std::vector<uint16_t> > rb;
  rb = fApi->daqGetReadback();
  std::vector<uint8_t> rb_val;

  for(uint8_t i=0; i<rb.size(); i++){
    //    for(uint8_t j=0; j<rb[i].size(); j++){
    // LOG(logDEBUG)<<"Readback values for vana = "<<(int)vana<<" : "<<(int)(rb[i][j]&0xff);
    rb_val.push_back( rb[i][ rb[i].size()-1 ]&0xff ); // read the last (size-1) readback word read out for ROC i
      // }
  }
  
  //::::::::::::::::::::::::::::::
  //DAQ - THE END.

  FinalCleaning();
  fApi->setClockStretch(0, 0, 0); //No Stretch after trigger, 0 delay
  return rb_val;
 }

std::vector<uint8_t> PixTestReadback::daqReadback(string dac, uint8_t vana, unsigned int roc, int8_t parReadback){

  PixTest::update();
  fDirectory->cd();
  fPg_setup.clear();

  if (!dac.compare("vana")){
    fApi->setDAC(dac.c_str(), vana, roc);
  }
  else {
    LOG(logDEBUG)<<"Wrong daqReadback function called!!!";
    //fApi->_hal->setTBvd(vana);
  }

  fApi->setDAC("readback", parReadback);
  doDAQ();

  std::vector<std::vector<uint16_t> > rb;
  rb = fApi->daqGetReadback();
  std::vector<uint8_t> rb_val;

  for(uint8_t i=0; i<rb.size(); i++){
    //    for(uint8_t j=0; j<rb[i].size(); j++){
    // LOG(logDEBUG)<<"Readback values for vana = "<<(int)vana<<" : "<<(int)(rb[i][j]&0xff);
    rb_val.push_back( rb[i][ rb[i].size()-1 ]&0xff ); // read the last (size-1) readback word read out for ROC i
      // }
  }
  
  //::::::::::::::::::::::::::::::
  //DAQ - THE END.

  FinalCleaning();
  fApi->setClockStretch(0, 0, 0); //No Stretch after trigger, 0 delay
  return rb_val;
 }



std::vector<uint8_t> PixTestReadback::daqReadbackIa(){

  PixTest::update();
  fDirectory->cd();
  fPg_setup.clear();

  fApi->setDAC("readback", 12);

  //Immediately stop if parameters not in range	
  //  if (fParOutOfRange) return 255;

  //Set the ClockStretch
  fApi->setClockStretch(0, 0, fParStretch); //Stretch after trigger, 0 delay
   
  //Set the histograms:
  if(fHistList.size() == 0) setHistos();  //to book histo only for the first 'doTest' (or after Clear).

  //To print on shell the number of masked pixels per ROC:
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  LOG(logINFO) << "PixTestReadback::Number of masked pixels:";
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
	  LOG(logINFO) << "PixTestReadback::    ROC " << static_cast<int>(iroc) << ": " << fApi->_dut->getNMaskedPixels(static_cast<int>(iroc));
  }  
  
  // Start the DAQ:
  //::::::::::::::::::::::::::::::::

  //:::Setting register to read back a given quantity::::://

  //First send only a RES:
  fPg_setup.push_back(make_pair("resetroc", 0));     // PG_RESR b001000 
  uint16_t period = 28;

  //Set the pattern generator:
  fApi->setPatternGenerator(fPg_setup);

  fApi->daqStart();

  //Send only one trigger to reset:
  fApi->daqTrigger(1, period);
  LOG(logINFO) << "PixTestReadback::RES sent once ";

  fApi->daqStop();

  fPg_setup.clear();
  LOG(logINFO) << "PixTestReadback::PG_Setup clean";

  //Set the pattern wrt the trigger frequency:
  LOG(logINFO) << "PG set to have trigger frequency = " << fParTriggerFrequency << " kHz";
  if (!setTrgFrequency(20)){
	  FinalCleaning();
	  //	  return 255;
  }

  //Set pattern generator:
  fApi->setPatternGenerator(fPg_setup);

  fDaq_loop = true;

  //Start the DAQ:
  fApi->daqStart();

  int  Ntrig=32;
  //Send the triggers:
  fApi->daqTrigger(Ntrig, fParPeriod);
  gSystem->ProcessEvents();
  ProcessData(0);
 
  fApi->daqStop(); 

  std::vector<std::vector<uint16_t> > rb;
  rb = fApi->daqGetReadback();
  std::vector<uint8_t> rb_val;

  for(uint8_t i=0; i<rb.size(); i++){
    //    for(uint8_t j=0; j<rb[i].size(); j++){
    // LOG(logDEBUG)<<"Readback values for vana = "<<(int)vana<<" : "<<(int)(rb[i][j]&0xff);
    rb_val.push_back( rb[i][ rb[i].size()-1 ]&0xff ); // read the last (size-1) readback word read out for ROC i
      // }
  } 
  

  //::::::::::::::::::::::::::::::
  //DAQ - THE END.

  FinalCleaning();
  fApi->setClockStretch(0, 0, 0); //No Stretch after trigger, 0 delay
  return rb_val;
 }

void PixTestReadback::setVana() {
  cacheDacs();
  fDirectory->cd();
  PixTest::update(); 
  
  int fTargetIa=24;
  
  banner(Form("PixTestPretest::setVana() target Ia = %d mA/ROC", fTargetIa)); 

  fApi->_dut->testAllPixels(false);

  vector<uint8_t> vanaStart;
  vector<double> rocIana;

  // -- cache setting and switch off all(!) ROCs
  int nRocs = fApi->_dut->getNRocs(); 
  for (int iroc = 0; iroc < nRocs; ++iroc) {
    vanaStart.push_back(fApi->_dut->getDAC(iroc, "vana"));
    rocIana.push_back(0.); 
    fApi->setDAC("vana", 0, iroc);
  }
  
//  double i016 = getCalibratedIa();
//
//  // FIXME this should not be a stopwatch, but a delay
  TStopwatch sw;
  sw.Start(kTRUE); // reset
//  do {
//    sw.Start(kFALSE); // continue
//    i016 = getCalibratedIa();
//  } while (sw.RealTime() < 0.1);
//
//  // subtract one ROC to get the offset from the other Rocs (on average):
//  double i015 = (nRocs-1) * i016 / nRocs; // = 0 for single chip tests
//  LOG(logDEBUG) << "offset current from other " << nRocs-1 << " ROCs is " << i015 << " mA";

  double i015=0.;

  // tune per ROC:

  const double extra = 0.1; // [mA] besser zu viel als zu wenig 
  const double eps = 0.25; // [mA] convergence
  const double slope = 6; // 255 DACs / 40 mA

  for (int roc = 0; roc < nRocs; ++roc) {
    if (!selectedRoc(roc)) {
      LOG(logDEBUG) << "skipping ROC idx = " << roc << " (not selected) for Vana tuning"; 
      continue;
    }
    int vana = vanaStart[roc];
    fApi->setDAC("vana", vana, roc); // start value

    double ia = getCalibratedIa(roc); // [mA], just to be sure to flush usb
    sw.Start(kTRUE); // reset
    do {
      sw.Start(kFALSE); // continue
      ia = getCalibratedIa(roc); // [mA]
    } while (sw.RealTime() < 0.1);

    double diff = fTargetIa + extra - (ia - i015);

    int iter = 0;
    LOG(logDEBUG) << "ROC " << roc << " iter " << iter
		 << " Vana " << vana
		 << " Ia " << ia-i015 << " mA";

    while (TMath::Abs(diff) > eps && iter < 11 && vana > 0 && vana < 255) {

      int stp = static_cast<int>(TMath::Abs(slope*diff));
      if (stp == 0) stp = 1;
      if (diff < 0) stp = -stp;

      vana += stp;

      if (vana < 0) {
	vana = 0;
      } else {
	if (vana > 255) {
	  vana = 255;
	}
      }

      fApi->setDAC("vana", vana, roc);
      iter++;

      sw.Start(kTRUE); // reset
      do {
	sw.Start(kFALSE); // continue
	ia = getCalibratedIa(roc); // [mA]
      }
      while( sw.RealTime() < 0.1 );

      diff = fTargetIa + extra - (ia - i015);

      LOG(logDEBUG) << "ROC " << setw(2) << roc
		   << " iter " << setw(2) << iter
		   << " Vana " << setw(3) << vana
		   << " Ia " << ia-i015 << " mA";
    } // iter

    rocIana[roc] = ia-i015; // more or less identical for all ROCS?!
    vanaStart[roc] = vana; // remember best
    fApi->setDAC( "vana", 0, roc ); // switch off for next ROC

  } // rocs

  TH1D *hsum = bookTH1D("VanaSettings", "Vana per ROC", nRocs, 0., nRocs);
  setTitles(hsum, "ROC", "Vana [DAC]"); 
  hsum->SetStats(0);
  hsum->SetMinimum(0);
  hsum->SetMaximum(256);
  fHistList.push_back(hsum);

  TH1D *hcurr = bookTH1D("Iana", "Iana per ROC", nRocs, 0., nRocs);
  setTitles(hcurr, "ROC", "Vana [DAC]"); 
  hcurr->SetStats(0); // no stats
  hcurr->SetMinimum(0);
  hcurr->SetMaximum(30.0);
  fHistList.push_back(hcurr);


  restoreDacs();
  for (int roc = 0; roc < nRocs; ++roc) {
    // -- reset all ROCs to optimum or cached value
    fApi->setDAC( "vana", vanaStart[roc], roc );
    LOG(logDEBUG) << "ROC " << setw(2) << roc << " Vana " << setw(3) << int(vanaStart[roc]);
    // -- histogramming only for those ROCs that were selected
    if (!selectedRoc(roc)) continue;
    hsum->Fill(roc, vanaStart[roc] );
    hcurr->Fill(roc, rocIana[roc]); 
  }
  
  vector<double> v_ia16 = getCalibratedIa(); // [mA]

  sw.Start(kTRUE); // reset
  do {
    sw.Start(kFALSE); // continue
    v_ia16 = getCalibratedIa(); // [mA]
  }
  while( sw.RealTime() < 0.1 );

  double ia16=0.;
  for(unsigned int iroc = 0; iroc<v_ia16.size(); iroc++){
    ia16 += v_ia16[iroc];
  }

  hsum->Draw();
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), hsum);
  PixTest::update();

  LOG(logINFO) << "PixTestPretest::setVana() done, Module Ia " << ia16 << " mA = " << ia16/nRocs << " mA/ROC";

}


void PixTestReadback::doDAQ(){
  //Immediately stop if parameters not in range	
  if (fParOutOfRange) return;

  //Set the ClockStretch
  fApi->setClockStretch(0, 0, fParStretch); //Stretch after trigger, 0 delay
   
  //Set the histograms:
  if(fHistList.size() == 0) setHistos();  //to book histo only for the first 'doTest' (or after Clear).

  //To print on shell the number of masked pixels per ROC:
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  LOG(logINFO) << "PixTestReadback::Number of masked pixels:";
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
	  LOG(logINFO) << "PixTestReadback::    ROC " << static_cast<int>(iroc) << ": " << fApi->_dut->getNMaskedPixels(static_cast<int>(iroc));
  }  
  
  // Start the DAQ:
  //::::::::::::::::::::::::::::::::

  //:::Setting register to read back a given quantity::::://

  //First send only a RES:
  fPg_setup.push_back(make_pair("resetroc", 0));     // PG_RESR b001000 
  uint16_t period = 28;

  //Set the pattern generator:
  fApi->setPatternGenerator(fPg_setup);

  fApi->daqStart();

  //Send only one trigger to reset:
  fApi->daqTrigger(1, period);
  LOG(logINFO) << "PixTestReadback::RES sent once ";

  fApi->daqStop();

  fPg_setup.clear();
  LOG(logINFO) << "PixTestReadback::PG_Setup clean";

  //Set the pattern wrt the trigger frequency:
  LOG(logINFO) << "PG set to have trigger frequency = " << fParTriggerFrequency << " kHz";
  if (!setTrgFrequency(20)){
	  FinalCleaning();
	  return;
  }

  //Set pattern generator:
  fApi->setPatternGenerator(fPg_setup);

  fDaq_loop = true;

  //Start the DAQ:
  fApi->daqStart();

  int  Ntrig=32;
  //Send the triggers:
  fApi->daqTrigger(Ntrig, fParPeriod);
  gSystem->ProcessEvents();
  ProcessData(0);
 
  fApi->daqStop(); 
}
