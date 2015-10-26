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
PixTestReadback::PixTestReadback(PixSetup *a, std::string name) : PixTest(a, name), fCalwVd(true), fCalwVa(false) {
  PixTest::init();
  init(); 
  LOG(logDEBUG) << "PixTestReadback ctor(PixSetup &a, string, TGTab *)";
  fTree = 0; 

  vector<vector<pair<string, double> > > iniCal;
  vector<pair<string, double> > prova1;
  fRbCal =  fPixSetup->getConfigParameters()->getReadbackCal();

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
void PixTestReadback::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;
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
  if(!command.compare("getcalibratedvbg")){
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
    }
  }
  return found;
}

//----------------------------------------------------------
bool PixTestReadback::setTrgFrequency(uint8_t TrgTkDel){
  int nDel = 0;
  uint8_t trgtkdel= TrgTkDel;
  double  triggerFreq=100.;
  double period_ns = 1 / (double)triggerFreq * 1000000; // trigger frequency in kHz.
  fParPeriod = (uint16_t)period_ns / 25;
  uint16_t ClkDelays = fParPeriod - trgtkdel;

  while (ClkDelays>255){
    fPg_setup.push_back(make_pair("delay", 255));
    ClkDelays = ClkDelays - 255;
    nDel ++;
  }
  fPg_setup.push_back(make_pair("delay", ClkDelays));

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
      pxar::Event evt;
      try { evt = fApi->daqGetEvent(); }
      catch(pxar::DataNoEvent &) {}
      //Check if event is empty?
      if (evt.pixels.size() > 0)
	daqdat.push_back(evt);
    }
  }
  else
    try { daqdat = fApi->daqGetEventBuffer(); }
    catch(pxar::DataNoEvent &) {}
  
  LOG(logDEBUG) << "Processing Data: " << daqdat.size() << " events.";
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
  bigBanner(Form("PixTestReadback::doTest()"));
  
  CalibrateVd();
  CalibrateVa();
  readbackVbg();
  vector<double> VBG =  getCalibratedVbg();
  CalibrateIa();
  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    (*il)->Draw((getHistOption(*il)).c_str()); 
  }
  
 LOG(logINFO) << "PixTestReadback::doTest() done";
 dutCalibrateOff();
}


void PixTestReadback::CalibrateIa(){
  LOG(logINFO)<<"*******************************************************";
  LOG(logINFO)<<"Running CalibrateIa()";
  prepareDAQ();
  cacheDacs();
  //readback DAC set to 12 (i.e. Ia)
  fParReadback=12;
  int npoints = 10;
  int pace = 25;

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 

  vector<uint8_t> readback;
  vector<uint8_t> readback_allRocs;
  for(unsigned int i=0; i< fApi->_dut->getNRocs(); i++){
    readback_allRocs.push_back(0);
  }

  string name, title;
  TH1D* hrb(0);
  vector<TH1D*> hs_rbIa, hs_tbIa;
  double tbIa = 0.;
  map<uint8_t, vector<uint8_t > > rbIa;
  uint8_t vana=0;
  for(int ivana=0; ivana<npoints; ivana++){
    vana = (uint8_t)ivana*pace;
    rbIa.insert(make_pair(vana, readback_allRocs));   
  }
  int count=0;
  while(readback.size()<1 && count<10){
    readback=daqReadback("vana", (uint8_t)80, fParReadback);  
    LOG(logDEBUG)<<"CalibrateIa: daqReadback attempt #"<<count++;
  }
  if(10==count){
    LOG(logINFO)<<"ERROR: no readback data received after "<<count<<" attempts. Aborting readback calibration";
    return;
  }
  for(unsigned int iroc = 0; iroc < rocIds.size(); iroc++){
    name = Form("rbIa_C%d", getIdFromIdx(iroc));
    title=name;
    hrb = bookTH1D(name, title, 256, 0., 256);
    hs_rbIa.push_back(hrb);
    name = Form("tbIa_C%d", getIdFromIdx(iroc));
    title=name;
    hrb = bookTH1D(name, title, 256, 0., 256);
    hs_tbIa.push_back(hrb);
  }

  vana=0;
  //measuring average current offset from other ROCs
  double ioff16=0.;
  fApi->setDAC("vana", 0);
  TStopwatch sw;
  sw.Start(kTRUE); // reset
  do {
    sw.Start(kFALSE); // continue
    ioff16 = fApi->getTBia()*1E3; // [mA]
  } while (sw.RealTime() < 0.5);
  //  ioff16 = fApi->getTBia()*1E3;
  LOG(logDEBUG)<<"I offset on 16 ROCs is "<<ioff16;
  double avIoff=0;
  avIoff = ioff16*(readback.size()-1)/(readback.size());
  LOG(logDEBUG)<<"Average current offset is "<<avIoff<<", readbacksize "<<(int)readback.size();



  for(int ivana=0; ivana<npoints; ivana++){
    vana = (uint8_t)ivana*pace;
    for(unsigned int iroc = 0; iroc < rocIds.size(); iroc++){
      LOG(logDEBUG)<<"Vana scan for ROC "<<getIdFromIdx(iroc);
      fApi->setDAC("vana", 0);
      tbIa = fApi->getTBia()*1E3; // [mA]
      count=0;
      do{
	readback=daqReadback("vana", vana, getIdFromIdx(iroc), fParReadback);
	LOG(logDEBUG)<<"CalibrateIa: daqReadback attempt #"<<count++<<", ROC "<<iroc<<", vana "<<(int)vana<<", readback "<<(int)readback[getIdFromIdx(iroc)];
      }  while(readback.size()<1 && count<10);
      if(10==count){
	LOG(logINFO)<<"ERROR: no readback data received after "<<count<<" attempts. Aborting readback calibration";
	return;
      }
      rbIa[vana][getIdFromIdx(iroc)]=readback[getIdFromIdx(iroc)];
      fApi->setDAC("vana", vana, getIdFromIdx(iroc));
      sw.Start(kTRUE); // reset
      count=0;
      do {
	sw.Start(kFALSE); // continue
	tbIa = fApi->getTBia()*1E3; // [mA]
      } while (sw.RealTime() < 0.3);
      hs_rbIa[iroc]->Fill(vana, readback[getIdFromIdx(iroc)]);//should this be corrected as well?
      hs_tbIa[iroc]->Fill(vana, tbIa-avIoff);//tbIa corrected for offset
      LOG(logDEBUG)<<"vana "<<(int)vana<<", rbIa "<<(int)readback[getIdFromIdx(iroc)]<<", tbIa "<<(int)(tbIa-avIoff);
    }
  }

  for(unsigned int iroc = 0; iroc < rocIds.size(); iroc++){
    hs_rbIa[iroc]->GetXaxis()->SetTitle("Vana [DAC]");
    hs_rbIa[iroc]->GetYaxis()->SetTitle("Ia_rb [ADC]");
    hs_tbIa[iroc]->GetXaxis()->SetTitle("Vana [DAC]");
    hs_tbIa[iroc]->GetYaxis()->SetTitle("Ia_TB [mA]");
  }
  gStyle->SetOptFit(1111);

  vector<double> rb_vanaMax(rocIds.size(), 0.);
   
  //protection to exclude plateu from fit
  for(unsigned int iroc = 0; iroc < rocIds.size(); iroc++){
    rb_vanaMax[iroc] = hs_rbIa[iroc]->GetBinCenter(hs_rbIa[iroc]->FindFirstBinAbove(254));

    // LOG(logDEBUG)<<"Rb max for fit:rb: "<<rb_vanaMax[iroc]<<endl;
  }

  TF1* frb;
  TF1* ftb;
  vector<TF1*> v_frb, v_ftb;
  for(unsigned int iroc=0; iroc < rocIds.size(); iroc++){
    name = Form("lin_rb_C%d", getIdFromIdx(iroc));
    frb = new TF1(name.c_str(), "[0] + x*[1]", 0, rb_vanaMax[iroc]);
    v_frb.push_back(frb);
    name = Form("pol2_ftb_C%d", getIdFromIdx(iroc));
    ftb = new TF1(name.c_str(), "[0] + x*[1] + x*x*[2] ", 0, 255);
    v_ftb.push_back(ftb);
  }

  for(unsigned int iroc = 0; iroc < rocIds.size(); iroc++){
    hs_rbIa[iroc]->Fit(v_frb[iroc], "WS", "", 0., rb_vanaMax[iroc]);
    hs_tbIa[iroc]->Fit(v_ftb[iroc], "WS");
    fPar0RbIaCal[getIdFromIdx(iroc)]=v_frb[iroc]->GetParameter(0);
    fPar1RbIaCal[getIdFromIdx(iroc)]=v_frb[iroc]->GetParameter(1);
    fPar0TbIaCal[getIdFromIdx(iroc)]=v_ftb[iroc]->GetParameter(0);
    fPar1TbIaCal[getIdFromIdx(iroc)]=v_ftb[iroc]->GetParameter(1);
    fPar2TbIaCal[getIdFromIdx(iroc)]=v_ftb[iroc]->GetParameter(2);
  }


  for(unsigned int iroc = 0; iroc < rocIds.size(); iroc++){
    for(std::vector<std::pair<std::string, double> >::iterator ical = fRbCal[getIdFromIdx(iroc)].begin(); ical != fRbCal[getIdFromIdx(iroc)].end(); ical++){
      if(!(ical->first.compare("par0rbia"))){
	ical->second = fPar0RbIaCal[getIdFromIdx(iroc)];
      }
      else if(!(ical->first.compare("par1rbia"))){
	ical->second = fPar1RbIaCal[getIdFromIdx(iroc)];
      }
      else if(!(ical->first.compare("par0tbia"))){
	ical->second = fPar0TbIaCal[getIdFromIdx(iroc)];
      }
      else if(!(ical->first.compare("par1tbia"))){
	ical->second = fPar1TbIaCal[getIdFromIdx(iroc)];
      }
      else if(!(ical->first.compare("par2tbia"))){
	ical->second = fPar2TbIaCal[getIdFromIdx(iroc)];
      }
    }
  }
  for(unsigned int iroc = 0; iroc < rocIds.size(); iroc++){
    for(std::vector<std::pair<std::string, double> >::iterator ical = fRbCal[getIdFromIdx(iroc)].begin(); ical != fRbCal[getIdFromIdx(iroc)].end(); ical++){
      LOG(logDEBUG)<<"debug: "<<ical->first<<" "<<ical->second;
    }
  }

  TH1D* h_rbIaCal (0);
  vector<TH1D*> hs_rbIaCal;
  for(unsigned int iroc = 0; iroc < rocIds.size(); iroc++){
    name = Form("rbIaCal_C%d", getIdFromIdx(iroc));
    title = name;
    h_rbIaCal = bookTH1D(name,title, 256, 0., 256.);
    hs_rbIaCal.push_back(h_rbIaCal);
  }

  for(int ivana=0; ivana<npoints; ivana++){
    vana = (uint8_t)ivana*pace;
    for(unsigned int iroc = 0; iroc < rocIds.size(); iroc++){
      LOG(logDEBUG)<<"step ivana = "<<ivana;
      LOG(logDEBUG)<<"Vana"<<(int)vana<<" Rbiana "<<(int)rbIa[vana][getIdFromIdx(iroc)]<<" Calibrated ia_rb = "<<(double)(rbIa[vana][getIdFromIdx(iroc)]*rbIa[vana][getIdFromIdx(iroc)]*fPar2TbIaCal[getIdFromIdx(iroc)]/fPar1RbIaCal[getIdFromIdx(iroc)]/fPar1RbIaCal[getIdFromIdx(iroc)] + rbIa[vana][getIdFromIdx(iroc)]/fPar1RbIaCal[getIdFromIdx(iroc)]/fPar1RbIaCal[getIdFromIdx(iroc)]*(fPar1RbIaCal[getIdFromIdx(iroc)]*fPar1TbIaCal[getIdFromIdx(iroc)]-2*fPar0RbIaCal[getIdFromIdx(iroc)]*fPar2TbIaCal[getIdFromIdx(iroc)]) + (fPar0RbIaCal[getIdFromIdx(iroc)]*fPar0RbIaCal[getIdFromIdx(iroc)]*fPar2TbIaCal[getIdFromIdx(iroc)] - fPar0RbIaCal[getIdFromIdx(iroc)]*fPar1TbIaCal[getIdFromIdx(iroc)])/fPar1RbIaCal[getIdFromIdx(iroc)] + fPar0TbIaCal[getIdFromIdx(iroc)]);
      hs_rbIaCal[iroc]->Fill(vana, (rbIa[vana][getIdFromIdx(iroc)]*rbIa[vana][getIdFromIdx(iroc)]*fPar2TbIaCal[getIdFromIdx(iroc)]/fPar1RbIaCal[getIdFromIdx(iroc)]/fPar1RbIaCal[getIdFromIdx(iroc)] + rbIa[vana][getIdFromIdx(iroc)]/fPar1RbIaCal[getIdFromIdx(iroc)]/fPar1RbIaCal[getIdFromIdx(iroc)]*(fPar1RbIaCal[getIdFromIdx(iroc)]*fPar1TbIaCal[getIdFromIdx(iroc)]-2*fPar0RbIaCal[getIdFromIdx(iroc)]*fPar2TbIaCal[getIdFromIdx(iroc)]) + (fPar0RbIaCal[getIdFromIdx(iroc)]*fPar0RbIaCal[getIdFromIdx(iroc)]*fPar2TbIaCal[getIdFromIdx(iroc)] - fPar0RbIaCal[getIdFromIdx(iroc)]*fPar1TbIaCal[getIdFromIdx(iroc)])/fPar1RbIaCal[getIdFromIdx(iroc)] + fPar0TbIaCal[getIdFromIdx(iroc)]));
    }
  }

  for(unsigned int iroc = 0; iroc < rocIds.size(); iroc++){
    hs_rbIaCal[iroc]->GetXaxis()->SetTitle("Vana [DAC]");
    hs_rbIaCal[iroc]->GetYaxis()->SetTitle("Ia_rb_cal [mA]");
    hs_rbIaCal[iroc]->SetLineColor(kBlue);
    fHistList.push_back(hs_rbIa[iroc]);
    fHistList.push_back(hs_rbIaCal[iroc]);
    fHistList.push_back(hs_tbIa[iroc]);
  }
  for(unsigned int iroc = 0; iroc < rocIds.size(); iroc++){
    fPixSetup->getConfigParameters()->writeReadbackFile(getIdFromIdx(iroc), fRbCal[getIdFromIdx(iroc)]);    
  }
  
  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    (*il)->Draw((getHistOption(*il)).c_str()); 
  }

  restoreDacs();
  FinalCleaning();
}

std::vector<double> PixTestReadback::getCalibratedIa(){
  //readback DAC set to 12 (i.e. Ia)
  prepareDAQ();
  fParReadback=12;

  vector<uint8_t> readback;

  int count=0;

  while(readback.size()<1 && count<10){
    readback=daqReadbackIa();
  }
  if(10==count){
    LOG(logINFO)<<"ERROR: no readback data received after "<<count<<" attempts. Aborting readback calibration";
    return vector<double>();
  }
  vector<double> calIa(readback.size(), 0.);

  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    calIa[iroc] = (((double)readback[iroc])*((double)readback[iroc])*fPar2TbIaCal[iroc]/fPar1RbIaCal[iroc]/fPar1RbIaCal[iroc] + ((double)readback[iroc])/fPar1RbIaCal[iroc]/fPar1RbIaCal[iroc]*(fPar1RbIaCal[iroc]*fPar1TbIaCal[iroc]-2*fPar0RbIaCal[iroc]*fPar2TbIaCal[iroc]) + (fPar0RbIaCal[iroc]*fPar0RbIaCal[iroc]*fPar2TbIaCal[iroc] - fPar0RbIaCal[iroc]*fPar1TbIaCal[iroc])/fPar1RbIaCal[iroc] + fPar0TbIaCal[iroc]);
    LOG(logDEBUG)<<"Calibrated analog current is "<<calIa[iroc];
  }
  FinalCleaning();
  return  calIa;
}

double PixTestReadback::getCalibratedIa(unsigned int iroc){
  //readback DAC set to 12 (i.e. Ia)
  prepareDAQ();
  fParReadback=12;
  vector<uint8_t> readback;
  int count=0;

  while(readback.size()<1 && count<10){
    readback=daqReadbackIa();
  }
  if(10==count){
    LOG(logINFO)<<"ERROR: no readback data received after "<<count<<" attempts. Aborting readback calibration";
    return 0.;
  }
  double calIa;
  calIa = (((double)readback[iroc])*((double)readback[iroc])*fPar2TbIaCal[iroc]/fPar1RbIaCal[iroc]/fPar1RbIaCal[iroc] + ((double)readback[iroc])/fPar1RbIaCal[iroc]/fPar1RbIaCal[iroc]*(fPar1RbIaCal[iroc]*fPar1TbIaCal[iroc]-2*fPar0RbIaCal[iroc]*fPar2TbIaCal[iroc]) + (fPar0RbIaCal[iroc]*fPar0RbIaCal[iroc]*fPar2TbIaCal[iroc] - fPar0RbIaCal[iroc]*fPar1TbIaCal[iroc])/fPar1RbIaCal[iroc] + fPar0TbIaCal[iroc]);
  LOG(logDEBUG)<<"Calibrated analog current is "<<calIa;
  
  FinalCleaning();
  return  calIa;
}

void PixTestReadback::CalibrateVd(){
  LOG(logINFO)<<"*******************************************************";
  LOG(logINFO)<<"Running CalibrateVd()";
  prepareDAQ();
  cacheDacs();
  cachePowerSettings();

  //readback DAC set to 8 (i.e. Vd)
  fParReadback=8;

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  LOG(logDEBUG)<<"Number of enabled rocs: "<<(int)rocIds.size();

  vector<uint8_t> readback;

  string name, title;
  TH1D* hrb(0);
  vector<TH1D*> hs_rbVd, hs_dacVd;
  vector<double > rbVd;
  double Vd;
  double Vd_mod;

  int nTbms = fApi->_dut->getNTbms();

  double R_vd, DeltaGND;
  //if this is a module, take into account voltage drops
  if(nTbms>0){
    R_vd=0.338;
    DeltaGND=0.1815;
  }
  else{
    R_vd=0;
    DeltaGND=0;
  }

  double VdMin, pace;
  int Npoints;
  //on a module, from vd=2.7 to vd=3.0 in 13 steps of 0.025
  if(nTbms>0){
    VdMin=2.7;
  }
  //on a single chip, from vd=2.5 to vd 2.8 in 13 steps of 0.025
  else{
    VdMin=2.5;
  }
    pace=0.025;
    Npoints=13;

  int count=0;
  //dry run to avoid spikes
  while(readback.size()<1 && count<10){
    readback=daqReadback("vd", VdMin, fParReadback);
    LOG(logDEBUG)<<"readback size is "<<readback.size();
    count++;
  }
  if(10==count){
    LOG(logINFO)<<"ERROR: no readback data received after "<<count<<" attempts. Aborting readback calibration";
    return;
  }

  //book histos
  for(unsigned int iroc = 0; iroc < rocIds.size(); iroc++){
    name = Form("rbVd_C%d", getIdFromIdx(iroc));
    title = name;
    hrb = bookTH1D(name, title, 500, 0., 5.);
    hs_rbVd.push_back(hrb);
    name = Form("dacVd_C%d", getIdFromIdx(iroc));
    title = name;
    hrb = bookTH1D(name, title, 500, 0., 5.);
    hs_dacVd.push_back(hrb);
  }
  readback.clear();


  for(int iVd=0; iVd<Npoints; iVd++){
    LOG(logDEBUG)<<"/****:::::: CALIBRATE VD :::::****/";
    Vd = VdMin + iVd*pace;
    LOG(logDEBUG)<<"Digital voltage will be set to: "<<Vd;
    count=0;
    do{
      readback=daqReadback("vd", Vd, fParReadback);
      count++;
    }  while(readback.size()<1 && count<10);
    if(10==count){
      LOG(logINFO)<<"ERROR: no readback data received after "<<count<<" attempts. Aborting readback calibration";
      return;
    }
    LOG(logDEBUG)<<"Readback size :"<<(int)readback.size();
    for(unsigned int iroc = 0; iroc < rocIds.size(); iroc++){
      Vd_mod = Vd - R_vd*fApi->getTBid() - DeltaGND;//values measured for 15cm molex cable
      //      LOG(logDEBUG)<<"Filling histo for roc idx "<<(int)iroc<<" id "<<(int)getIdFromIdx(iroc);
      hs_rbVd[iroc]->Fill(Vd_mod, readback[getIdFromIdx(iroc)]);
      hs_dacVd[iroc]->Fill(Vd, fApi->getTBvd());
    }
  }
  
  for(unsigned int iroc = 0; iroc < rocIds.size(); iroc++){
    hs_rbVd[iroc]->GetXaxis()->SetTitle("Vd [V]");
    hs_rbVd[iroc]->GetYaxis()->SetTitle("Vd_rb [ADC]");
    hs_dacVd[iroc]->GetXaxis()->SetTitle("Vd set [V]");
    hs_dacVd[iroc]->GetYaxis()->SetTitle("Vd TB [V]"); 
  }

  gStyle->SetOptFit(1111);

  //excluding possible plateaus
  vector<double> rb_VdMax(rocIds.size(), 0.);
  for(unsigned int iroc=0; iroc < rocIds.size(); iroc++){
    rb_VdMax[iroc] = hs_rbVd[iroc]->GetBinCenter(hs_rbVd[iroc]->FindFirstBinAbove(254));
    
    LOG(logDEBUG)<<"Vd max for fit on ROC "<<getIdFromIdx(iroc)<<" : "<<rb_VdMax[iroc];
  }

  TF1* frb (0);
  vector<TF1*> v_frb;
  for(unsigned int iroc = 0; iroc < rocIds.size(); iroc++){
    name = Form("lin_vd_C%d", getIdFromIdx(iroc));
    frb = new TF1(name.c_str(), "[0] + x*[1]", 0, rb_VdMax[iroc]);
    v_frb.push_back(frb);
  }
  
  for(unsigned int iroc = 0; iroc < rocIds.size(); iroc++){
    hs_rbVd[iroc]->Fit(v_frb[iroc], "WS", "", 0., rb_VdMax[iroc]);

    fPar0VdCal[iroc]=v_frb[iroc]->GetParameter(0);
    fPar1VdCal[iroc]=v_frb[iroc]->GetParameter(1);
    
    fHistList.push_back(hs_rbVd[iroc]);
    fHistList.push_back(hs_dacVd[iroc]);
  }

  for(unsigned int iroc = 0; iroc < rocIds.size(); iroc++){
    for(std::vector<std::pair<std::string, double> >::iterator ical = fRbCal[iroc].begin(); ical != fRbCal[iroc].end(); ical++){
      if(!(ical->first.compare("par0vd"))){
	ical->second = fPar0VdCal[iroc];
      }
      else if(!(ical->first.compare("par1vd"))){
	ical->second = fPar1VdCal[iroc];
      }
    }
  }

  for(unsigned int iroc = 0; iroc < rocIds.size(); iroc++){
    fPixSetup->getConfigParameters()->writeReadbackFile(getIdFromIdx(iroc), fRbCal[iroc]);    
  }

  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    //    LOG(logDEBUG)<<"Drawing histo "<<(*il)->GetName();
    (*il)->Draw((getHistOption(*il)).c_str()); 

  }
  
  fDisplayedHist = fHistList.begin();
  PixTest::update();

  restoreDacs();
  restorePowerSettings();
  FinalCleaning();
}


void PixTestReadback::readbackVbg(){
  LOG(logINFO)<<"*******************************************************";
  LOG(logINFO)<<"Running readbackVbg())";
  prepareDAQ();
  cacheDacs();
  cachePowerSettings();

  //readback DAC set to 11 (i.e. Vbg)
  fParReadback=11;

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 

  vector<uint8_t> readback;

  double VdMin;
  int nTbms = fApi->_dut->getNTbms();
  //on a module, vd=2.8 
  if(nTbms>0){
    VdMin=2.8;
  }
  else{
    VdMin=2.6;
  }

  int count=0;
  while(readback.size()<1 && count<10){
    readback = daqReadback("vd", VdMin, fParReadback);
    count++;
  }
  if(10==count){
    LOG(logINFO)<<"ERROR: no readback data received after "<<count<<" attempts. Aborting readback of Vbg";
    return;
  }

  vector<double> avReadback(readback.size(), 0.);

  int n_meas=0;
  bool okRb=true;
  int sumRb=0;
  
  for(int i=0; i<10; i++){
    sumRb=0;
    LOG(logDEBUG)<<"/****:::::: READBACK VBG :::::****/";
    LOG(logDEBUG)<<"Digital voltage will be set to: "<<VdMin;
    count=0;
    do{ 
      readback = daqReadback("vd", VdMin, fParReadback);
      count++;
    }  while(readback.size()<1 && count<10);
    if(10==count){
      LOG(logINFO)<<"ERROR: no readback data received after "<<count<<" attempts. Aborting readback calibration";
      return;
    }
    for(unsigned int iroc=0; iroc < rocIds.size(); iroc++){
      sumRb+=readback[getIdFromIdx(iroc)];
    }
    if(0==sumRb){
      okRb=false;
      LOG(logDEBUG)<<"Readback measurement #"<<i<<"/10 of Vbg failed, discarding measurement";
    }
    if (okRb){
      for(unsigned int iroc=0; iroc < rocIds.size(); iroc++){
	avReadback[iroc]+=(double)readback[iroc];
	LOG(logDEBUG)<<"Average Vbg readback on roc "<<(int)getIdFromIdx(iroc)<<" is "<<(double)avReadback[getIdFromIdx(iroc)]/(i+1)<<" ADCs";
      }
      n_meas++;
    }
  }

  if(0==n_meas){
    LOG(logINFO)<<"Measurements of Vbg failed. Aborting.";
    return;
  }
  
  for(unsigned int iroc=0; iroc < rocIds.size(); iroc++){
    fRbVbg[iroc] = avReadback[getIdFromIdx(iroc)]/n_meas;
    
  }

  restoreDacs();
  restorePowerSettings();
  FinalCleaning();
}

vector<double> PixTestReadback::getCalibratedVbg(){
  LOG(logINFO)<<"*******************************************************";
  LOG(logINFO)<<"Running getCalibratedVbg()";
  vector<double> calVbg(fRbVbg.size(), 0.);
  string name="";
  if(fCalwVd){
    name = "Vbg_readback_VdCal";
  }
  if(fCalwVa){
    name = "Vbg_readback_VaCal";
  }
  TH1D* h_vbg = bookTH1D(name.c_str(), name.c_str(), 16, 0., 16.);
  name = "Vbg_readback";
  TH1D* h_vbg_rb = bookTH1D(name.c_str(), name.c_str(), 16, 0., 16.);
  //0.5 needed because Vbg rb has twice the sensitivity Vd and Va have
  if(fCalwVd){
    LOG(logINFO)<<"Vbg will be calibrated using Vd calibration";
    for(unsigned int iroc=0; iroc < calVbg.size(); iroc++){
      calVbg[iroc]=0.5*(fRbVbg[iroc]-fPar0VdCal[iroc])/fPar1VdCal[iroc];
    }
  }
  else if(fCalwVa){
    LOG(logINFO)<<"Vbg will be calibrated using Va calibration";
    for(unsigned int iroc=0; iroc < calVbg.size(); iroc++){
      calVbg[iroc]=0.5*(fRbVbg[iroc]-fPar0VaCal[iroc])/fPar1VaCal[iroc];
    }
  }
  else{
    LOG(logDEBUG)<<"No calibration option specified. Please select one and retry.";
    return calVbg;
  }
  for(unsigned int iroc=0; iroc < calVbg.size(); iroc++){
    LOG(logINFO)<<"/*/*/*/*::: ROC "<<iroc<<": uncalibrated Vbg = "<<fRbVbg[iroc]<<"calibrated Vbg = "<<calVbg[iroc]<<" :::*/*/*/*/";
    h_vbg->Fill(getIdFromIdx(iroc), calVbg[getIdFromIdx(iroc)]);
    h_vbg_rb->Fill(getIdFromIdx(iroc), fRbVbg[getIdFromIdx(iroc)]);
  }
  h_vbg->GetXaxis()->SetTitle("#roc");
  h_vbg->GetYaxis()->SetTitle("Vbg [V]");
  h_vbg_rb->GetXaxis()->SetTitle("#roc");
  h_vbg_rb->GetYaxis()->SetTitle("Vbg [ADC]");
  fHistList.push_back(h_vbg);
  fHistList.push_back(h_vbg_rb);
  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    //    LOG(logDEBUG)<<"Drawing histo "<<(*il)->GetName();
    (*il)->Draw((getHistOption(*il)).c_str()); 
  }
  
  fDisplayedHist = fHistList.begin();
  PixTest::update();
  return calVbg;
}

void PixTestReadback::CalibrateVa(){
  LOG(logINFO)<<"*******************************************************";
  LOG(logINFO)<<"Running CalibrateVa()";
  prepareDAQ();
  cacheDacs();
  cachePowerSettings();

  //readback DAC set to 9 (i.e. Va)
  fParReadback=9;

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 

  vector<uint8_t> readback;

  string name, title;
  TH1D* hrb(0);
  vector<TH1D*> hs_rbVa, hs_dacVa;
  vector<double > rbVa;
  double Va;
  double Va_mod;
  int count=0;

  int nTbms = fApi->_dut->getNTbms();

  double R_va, DeltaGND;
  if(nTbms>0){
    R_va=0.6082;
    DeltaGND=0.1815;
  }
  else{
    R_va=0;
    DeltaGND=0;
  }

  double VaMin, pace;
  int Npoints;
  //on a module, from vd=2.7 to vd=3.0 in 13 steps of 0.025
  if(nTbms>0){
    VaMin=1.9;
  }
  //on a single chip, from vd=2.5 to vd 2.8 in 13 steps of 0.025
  else{
    VaMin=1.8;
  }
  pace=0.025;
  Npoints=13;
  
  //dry run to avoid spikes
  while(readback.size()<1 && count<10){
    readback=daqReadback("va", VaMin, fParReadback);
    LOG(logDEBUG)<<"readback size is "<<readback.size();
    count++;
  }
  if(10==count){
    LOG(logINFO)<<"ERROR: no readback data received after "<<count<<" attempts. Aborting readback calibration";
    return;
  }

  //book histos
  for(unsigned int iroc = 0; iroc < rocIds.size(); iroc++){
    name = Form("rbVa_C%d", getIdFromIdx(iroc));
    title = name;
    hrb = bookTH1D(name, title, 500, 0., 5.);
    hs_rbVa.push_back(hrb);
    name = Form("dacVa_C%d", getIdFromIdx(iroc));
    title = name;
    hrb = bookTH1D(name, title, 500, 0., 5.);
    hs_dacVa.push_back(hrb);
  }
  readback.clear();

  for(int iVa=0; iVa<Npoints; iVa++){
    LOG(logDEBUG)<<"/****:::::: CALIBRATE VA :::::****/";
    Va = VaMin + iVa*pace;
    LOG(logDEBUG)<<"Analog voltage will be set to: "<<Va;
    count=0;
    do{
      readback=daqReadback("va", Va, fParReadback);
      count++;
    }  while(readback.size()<1 && count<10);
    if(10==count){
      LOG(logINFO)<<"ERROR: no readback data received after "<<count<<" attempts. Aborting readback calibration";
      return;
    } 
    for(unsigned int iroc=0; iroc < rocIds.size(); iroc++){
      Va_mod = Va - R_va*fApi->getTBia() - DeltaGND; //values measured for 15cm molex cable
      hs_rbVa[iroc]->Fill(Va_mod, readback[getIdFromIdx(iroc)]);
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

  //excluding possible plateaus
  vector<double> rb_VaMax(readback.size(), 0.);
  for(unsigned int iroc=0; iroc < readback.size(); iroc++){
    rb_VaMax[iroc] = hs_rbVa[iroc]->GetBinCenter(hs_rbVa[iroc]->FindFirstBinAbove(254));
    
    LOG(logDEBUG)<<"Va max for fit on ROC "<<iroc<<" : "<<rb_VaMax[iroc];
  }

  TF1* frb (0);
  vector<TF1*> v_frb;
  for(unsigned int iroc=0; iroc < rocIds.size(); iroc++){
    name = Form("lin_va_C%d", getIdFromIdx(iroc));
    frb = new TF1(name.c_str(), "[0] + x*[1]", 0, rb_VaMax[iroc]);
    v_frb.push_back(frb);
  }

  for(unsigned int iroc=0; iroc < rocIds.size(); iroc++){
    hs_rbVa[iroc]->Fit(v_frb[iroc], "WS", "", 0., rb_VaMax[iroc]);
    fPar0VaCal[iroc]=v_frb[iroc]->GetParameter(0);
    fPar1VaCal[iroc]=v_frb[iroc]->GetParameter(1);
    fHistList.push_back(hs_rbVa[iroc]);
    fHistList.push_back(hs_dacVa[iroc]);
  }
  
  for(unsigned int iroc=0; iroc < rocIds.size(); iroc++){
    for(std::vector<std::pair<std::string, double> >::iterator ical = fRbCal[iroc].begin(); ical != fRbCal[iroc].end(); ical++){
      if(!(ical->first.compare("par0va"))){
	ical->second = fPar0VaCal[iroc];
      }
      else if(!(ical->first.compare("par1va"))){
	ical->second = fPar1VaCal[iroc];
      }
    }
  }

  for(unsigned int iroc=0; iroc < rocIds.size(); iroc++){
    fPixSetup->getConfigParameters()->writeReadbackFile(getIdFromIdx(iroc), fRbCal[iroc]);    
  }

  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    //    LOG(logDEBUG)<<"Drawing histo "<<(*il)->GetName();
    (*il)->Draw((getHistOption(*il)).c_str()); 

  }
  
  fDisplayedHist = fHistList.begin();
  PixTest::update();

  restoreDacs();
  restorePowerSettings();
  FinalCleaning();
}

void PixTestReadback::cachePowerSettings(){

  fPowerSet = fPixSetup->getConfigParameters()->getTbPowerSettings();

}

void PixTestReadback::restorePowerSettings(){

   fApi->setTestboardPower(fPowerSet);

}

vector<uint8_t> PixTestReadback::daqReadback(string dac, double vana, int8_t parReadback){

  PixTest::update();
  fDirectory->cd();
  
  if (!dac.compare("vana")){
    LOG(logDEBUG)<<"Wrong daqReadback function called!!!";
  }
  else {
    vector<pair<string,double > > powerset;
    powerset.push_back(std::make_pair(dac,vana));
    fApi->setTestboardPower(powerset);
  }

  fApi->setDAC("readback", parReadback);
  
  doDAQ();

  std::vector<std::vector<uint16_t> > rb;
  rb = fApi->daqGetReadback();
  std::vector<uint8_t> rb_val;

  for(uint8_t i=0; i<rb.size(); i++){
    if(!rb.at(i).empty()) {
      // read the last readback word read out for ROC i
      rb_val.push_back( rb.at(i).back()&0xff ); 
    }
  }

  return rb_val;
 }


std::vector<uint8_t> PixTestReadback::daqReadback(string dac, uint8_t vana, int8_t parReadback){

  PixTest::update();
  fDirectory->cd();

  if (!dac.compare("vana")){
    fApi->setDAC(dac.c_str(), vana);
  }
  else {
    LOG(logDEBUG)<<"Wrong daqReadback function called!!!";
  }

  fApi->setDAC("readback", parReadback);
  doDAQ();

  std::vector<std::vector<uint16_t> > rb;
  rb = fApi->daqGetReadback();
  std::vector<uint8_t> rb_val;

  for(uint8_t i=0; i<rb.size(); i++){
    if(!rb.at(i).empty()) {
      // read the last readback word read out for ROC i
      rb_val.push_back( rb.at(i).back()&0xff ); 
    }
  }

  return rb_val;
}

std::vector<uint8_t> PixTestReadback::daqReadback(string dac, uint8_t vana, unsigned int roc, int8_t parReadback){

  PixTest::update();
  fDirectory->cd();

  if (!dac.compare("vana")){
    fApi->setDAC(dac.c_str(), vana, roc);
  }
  else {
    LOG(logDEBUG)<<"Wrong daqReadback function called!!!";
  }

  fApi->setDAC("readback", parReadback);
  doDAQ();

  std::vector<std::vector<uint16_t> > rb;
  rb = fApi->daqGetReadback();
  std::vector<uint8_t> rb_val;

  for(uint8_t i=0; i<rb.size(); i++){
    if(!rb.at(i).empty()) {
      // read the last readback word read out for ROC i
      rb_val.push_back( rb.at(i).back()&0xff ); 
    }
  }
  
  return rb_val;
 }

std::vector<uint8_t> PixTestReadback::daqReadbackIa(){

  PixTest::update();
  fDirectory->cd();

  fApi->setDAC("readback", 12);
  doDAQ();

  std::vector<std::vector<uint16_t> > rb;
  rb = fApi->daqGetReadback();
  std::vector<uint8_t> rb_val;

  for(uint8_t i=0; i<rb.size(); i++){
    if(!rb.at(i).empty()) {
      // read the last readback word read out for ROC i
      rb_val.push_back( rb.at(i).back()&0xff ); 
    }
  }
 
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
  
  //readback already provides ia value corrected for offset, no need for further correction
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
    TStopwatch sw;
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
  
  vector<double> v_ia16; // [mA]
  
  TStopwatch sw;
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


void PixTestReadback::prepareDAQ(){
  fPg_setup.clear();

  LOG(logDEBUG)<<"preparing DAQ";

  //Set the ClockStretch
  fApi->setClockStretch(0, 0, 0); //Stretch after trigger, 0 delay

  // FIXME - issuing a ROC reset should not be necessary anymore since
  // pxarCore automatically resets the ROC when WBC is changed.
  fApi->daqSingleSignal("resetroc");
  LOG(logDEBUG) << "PixTestReadback::RES sent once ";

  //adding triggers to pg
  PreparePG();

  //set Pattern Generator
  fApi->setPatternGenerator(fPg_setup);
 
}

void PixTestReadback::doDAQ(){
  LOG(logDEBUG)<<"doDAQ";

  //Set the histograms:
  if(fHistList.size() == 0) setHistos();  //to book histo only for the first 'doTest' (or after Clear).

  int  Ntrig=32;
  //Send the triggers:
  fApi->daqStart(); 
  fApi->daqTrigger(Ntrig, fParPeriod);
  fApi->daqStop(); 
  LOG(logDEBUG)<<Ntrig<<" triggers sent";
  gSystem->ProcessEvents();
  ProcessData(0);
}

void PixTestReadback::PreparePG(){

  LOG(logDEBUG)<<"begin PreparePG()";

  //adding delays to the pattern generator, assuming a trigger frequency of 100 kHz, giving a pattern length of 400.
  int nDel = 0;
  double  triggerFreq=100.;
  double period_ns = 1 / (double)triggerFreq * 1000000; // trigger frequency in kHz.
  uint16_t Period = (uint16_t)period_ns / 25;
  uint16_t ClkDelays = Period; // subtracting trigger token delay.

  //for ROCs: subtract "resetroc"
  int nTbms = fApi->_dut->getNTbms();
  vector<pair<string, uint8_t> > pgtmp = fPixSetup->getConfigParameters()->getTbPgSettings();
  
  //for debugging: showing the pattern to be generated
  LOG(logDEBUG) << "********** The Pattern Generator will be set as following:";
  if (1) for (unsigned int i = 0; i < pgtmp.size(); ++i){
    LOG(logDEBUG) << "********** "<<pgtmp[i].first << ": " << (int)pgtmp[i].second;
  }

  for (unsigned i = 0; i < pgtmp.size(); ++i) {
    if(nTbms==0){
      if (string::npos != pgtmp[i].first.find("resetroc")){
        LOG(logDEBUG)<<"Considering "<<pgtmp[i].first<<" by subtracting ("<< (uint16_t)pgtmp[i].second <<" + 3) from "<<ClkDelays;
        ClkDelays -= pgtmp[i].second+3;
      }
      if (string::npos != pgtmp[i].first.find("trigger")){
        LOG(logDEBUG)<<"Considering "<<pgtmp[i].first<<" by subtracting ("<< (uint16_t)pgtmp[i].second <<" + 2) from "<<ClkDelays;
        ClkDelays -= pgtmp[i].second+2;
      }
      if (string::npos != pgtmp[i].first.find("token")){
      LOG(logDEBUG)<<"Considering "<<pgtmp[i].first<<" by subtracting ("<< (uint16_t)pgtmp[i].second <<" + 1) from "<<ClkDelays;
      ClkDelays -= pgtmp[i].second+1;
      }
      if (string::npos != pgtmp[i].first.find("trigger;sinc")){
      LOG(logDEBUG)<<"Considering "<<pgtmp[i].first<<" by subtracting ("<< (uint16_t)pgtmp[i].second <<" + 2) from "<<ClkDelays;
      ClkDelays -= pgtmp[i].second+1;
      }
    }
  }

  //filling the rest of the clock cycles with "delay", taking account, that each delay needs one Clkcycle to be inititated.
  while (ClkDelays>256){ //Considering the additional Clkcycle to perform the delay.
    fPg_setup.push_back(make_pair("delay", 255));
    ClkDelays = ClkDelays - 256;
    nDel ++;
  }
  if(ClkDelays>1){
    fPg_setup.push_back(make_pair("delay", ClkDelays-1));
  }

  //remove "resetroc" (if module), "resettbm" and "calibrate" from fPg_setup.
  for (unsigned i = 0; i < pgtmp.size(); ++i) {
    //remove roc resets (not needed) if this is a module
    if(nTbms>0){
      if (string::npos != pgtmp[i].first.find("resetroc")) continue;
    }
    if (string::npos != pgtmp[i].first.find("resettbm")) continue;
    if (string::npos != pgtmp[i].first.find("calibrate")) continue;
    fPg_setup.push_back(pgtmp[i]);
  }
  
  //for debugging: showing the pattern to be generated
  LOG(logDEBUG) << "********** The Pattern Generator will be set as following:";
  if (1) for (unsigned int i = 0; i < fPg_setup.size(); ++i){
    LOG(logDEBUG) << "********** "<<fPg_setup[i].first << ": " << (int)fPg_setup[i].second;
  }

  //Calculate the needed period, which is the sum of delays plus one clock cycle for each of the commands (3 cycles for "resetroc" and 2 cycles for "trigger")
  LOG(logDEBUG) << "********** Calculating the Pattern Period:";
  fParPeriod=0;
  for(std::vector<std::pair<std::string,uint8_t> >::iterator it = fPg_setup.begin(); it != fPg_setup.end(); ++it){
     if((string)(*it).first==("resetroc")){fParPeriod += (*it).second + 3; LOG(logDEBUG)<<"Adding "<<(*it).first<<": "<<(uint16_t)(*it).second<<" + 3";}
     if((string)(*it).first==("trigger")){fParPeriod += (*it).second + 2;LOG(logDEBUG)<<"Adding "<<(*it).first<<": "<<(uint16_t)(*it).second<<" + 2";}
     if((string)(*it).first==("trigger;sync")){fParPeriod += (*it).second + 2;LOG(logDEBUG)<<"Adding "<<(*it).first<<": "<<(uint16_t)(*it).second<<" + 2";}
     if((string)(*it).first==("delay")){fParPeriod += (*it).second + 1;LOG(logDEBUG)<<"Adding "<<(*it).first<<": "<<(uint16_t)(*it).second<<" + 1";}
     if((string)(*it).first==("token")){fParPeriod += (*it).second + 1;LOG(logDEBUG)<<"Adding "<<(*it).first<<": "<<(uint16_t)(*it).second<<" + 1";}
  }

  //for debugging: showing the calculated period to be sent to the PG.
  LOG(logDEBUG) << "********** The pattern period is "<<fParPeriod;
}
