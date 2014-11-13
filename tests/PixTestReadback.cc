#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include <TStopwatch.h>
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
PixTestReadback::PixTestReadback(PixSetup *a, std::string name) : PixTest(a, name), fParStretch(0), fParTriggerFrequency(100), fParResetROC(0), fPar0VdCal(1.), fPar1VdCal(1.), fPar0VaCal(1.), fPar1VaCal(1.), fCalwVd(true), fCalwVa(false) {
  PixTest::init();
  init(); 
  LOG(logDEBUG) << "PixTestReadback ctor(PixSetup &a, string, TGTab *)";
  fTree = 0; 

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
  double VBG =  getCalibratedVbg();
  CalibrateIa();
  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    (*il)->Draw((getHistOption(*il)).c_str()); 
  }
  
  ofstream myfile;
  myfile.open ((fPixSetup->getConfigParameters()->getDirectory()+"/ReadbackCal.dat").c_str(), ios::app);
  myfile << "##Voltages" << endl;
  myfile <<"#par0Vd    par1Vd    par0Va    par1Va    directory    rbVbg    Vbg    calVd    calVa    "<<endl;
  myfile<<fPar0VdCal<<"    "<<fPar1VdCal<<"    "<<fPar0VaCal<<"    "<<fPar1VaCal<<"    "<<fPixSetup->getConfigParameters()->getDirectory()<<"    "<<fRbVbg<<"    "<<VBG<<"	"<<fCalwVd<<"    "<<fCalwVa<<endl;
  myfile << "##Currents " << endl;
  myfile << "par0IaRb    par1IaRb    par0IaTb    par1IaTb    par2IaTb" << endl;
  myfile << fPar0RbIaCal << "    " << fPar1RbIaCal << "    " << fPar0TbIaCal << "    " << fPar1TbIaCal << "    " << fPar2TbIaCal << endl;
  myfile.close();
  
 LOG(logINFO) << "PixTestReadback::doTest() done";

}


void PixTestReadback::CalibrateIa(){
  cacheDacs();
  //readback DAC set to 12 (i.e. Ia)
  fParReadback=12;

  int readback=0;

  TH1D* h_rbIa = new TH1D("rbIa","rbIa", 256, 0., 256.);
  TH1D* h_tbIa = new TH1D("tbIa","tbIa", 256, 0., 256.);
  double tbIa = 0.;
  vector<double > rbIa;
  
  uint8_t vana=0;

  for(int ivana=0; ivana<52; ivana++){
    vana = (uint8_t)ivana*5;
    readback=daqReadback("vana", vana, fParReadback);
    rbIa.push_back(readback);
    fApi->setDAC("vana", vana);
    tbIa = fApi->getTBia()*1E3;
    h_rbIa->Fill(vana, readback);
    h_tbIa->Fill(vana, tbIa);
  }

  double rb_vanaMax=0.;
  double tb_vanaMax=0.;
   
  //protection to exclude plateu from fit
  rb_vanaMax = h_rbIa->GetBinCenter(h_rbIa->FindFirstBinAbove(254));

  LOG(logDEBUG)<<"Vana max for fit:"<<endl<<"rb: "<<rb_vanaMax<<endl<<"tb :"<<tb_vanaMax;

  TF1* frb = new TF1("lin_rb", "[0] + x*[1]", 0, rb_vanaMax);
  TF1* ftb = new TF1("pol2_ftb", "[0] + x*[1] + x*x*[2] ", 0, 255);

  h_rbIa->Fit(frb, "W", "", 0., rb_vanaMax);
  h_tbIa->Fit(ftb);

  LOG(logDEBUG)<<"Number of points for rb fit "<<frb->GetNumberFitPoints();

  fPar0RbIaCal=frb->GetParameter(0);
  fPar1RbIaCal=frb->GetParameter(1);
  fPar0TbIaCal=ftb->GetParameter(0);
  fPar1TbIaCal=ftb->GetParameter(1);
  LOG(logDEBUG)<<"just before parameter 2";
  fPar2TbIaCal=ftb->GetParameter(2);
  LOG(logDEBUG)<<"just after parameter 2";

  TH1D* h_rbIaCal = new TH1D("rbIaCal","rbIaCal", 256, 0., 256.);
  for(int ivana=0; ivana<52; ivana++){
    LOG(logDEBUG)<<"step ivana = "<<ivana;
    vana = (uint8_t)ivana*5;
    //    h_rbIaCal->Fill(vana, ((tbpar1/rbpar1)*(rbIa[vana]-rbpar0)+tbpar0));
    h_rbIaCal->Fill(vana, (rbIa[ivana]*rbIa[ivana]*fPar2TbIaCal/fPar1RbIaCal/fPar1RbIaCal + rbIa[ivana]/fPar1RbIaCal/fPar1RbIaCal*(fPar1RbIaCal*fPar1TbIaCal-2*fPar0RbIaCal*fPar2TbIaCal) + (fPar0RbIaCal*fPar0RbIaCal*fPar2TbIaCal - fPar0RbIaCal*fPar1TbIaCal)/fPar1RbIaCal + fPar0TbIaCal));
  }

  h_rbIaCal->SetLineColor(kBlue);
  fHistList.push_back(h_rbIa);
  fHistList.push_back(h_rbIaCal);
  fHistList.push_back(h_tbIa);
  restoreDacs();
}

double PixTestReadback::getCalibratedIa(){
  //readback DAC set to 12 (i.e. Ia)
  fParReadback=12;

  int readback=0;
  double calIa=0.;

  readback=daqReadbackIa();
  //  calIa = ((fPar1TbIaCal/fPar1RbIaCal)*((double)readback-fPar0RbIaCal)+fPar0TbIaCal);
  calIa = (((double)readback)*((double)readback)*fPar2TbIaCal/fPar1RbIaCal/fPar1RbIaCal + ((double)readback)/fPar1RbIaCal/fPar1RbIaCal*(fPar1RbIaCal*fPar1TbIaCal-2*fPar0RbIaCal*fPar2TbIaCal) + (fPar0RbIaCal*fPar0RbIaCal*fPar2TbIaCal - fPar0RbIaCal*fPar1TbIaCal)/fPar1RbIaCal + fPar0TbIaCal);
  LOG(logDEBUG)<<"Calibrated analog current is "<<calIa;
  return  calIa;
}

void PixTestReadback::CalibrateVana(){
  cacheDacs();
  //readback DAC set to 11 (i.e. Vana)
  fParReadback=12;

  int readback=0;

  TH1D* h_rbVana = new TH1D("rbVana","rbVana", 256, 0., 256.);
  TH1D* h_dacVana = new TH1D("dacVana","dacVana", 256, 0., 256.);
  vector<double > rbVana;
  
  for(uint8_t vana=0; vana<255; vana++){
    readback=daqReadback("vana", vana, fParReadback);
    rbVana.push_back(readback);
    h_rbVana->Fill(vana, readback);
    h_dacVana->Fill(vana, vana);
  }

  double rb_vanaMax=0.;

  rb_vanaMax = h_rbVana->GetBinCenter(h_rbVana->FindFirstBinAbove(254));

  LOG(logDEBUG)<<"Vana max for fit:"<<endl<<"rb: "<<rb_vanaMax;

  TF1* frb = new TF1("lin_rb", "[0] + x*[1]", 0, rb_vanaMax);
  TF1* fdac = new TF1("lin_fdac", "[0] + x*[1]", 0, 255);

  h_rbVana->Fit(frb, "W", "", 0., rb_vanaMax);
  h_dacVana->Fit(fdac);

  LOG(logDEBUG)<<"Number of points for rb fit "<<frb->GetNumberFitPoints();

  double rbpar0=0., rbpar1=0., dacpar0=0., dacpar1=0.;
  rbpar0=frb->GetParameter(0);
  rbpar1=frb->GetParameter(1);
  dacpar0=fdac->GetParameter(0);
  dacpar1=fdac->GetParameter(1);

  TH1D* h_rbVanaCal = new TH1D("rbVana","rbVana", 256, 0., 256.);
  for(int vana=0; vana<256; vana++){
    h_rbVanaCal->Fill(vana, ((dacpar1/rbpar1)*(rbVana[vana]-rbpar0)+dacpar0));
  }

  h_rbVanaCal->SetLineColor(kBlue);
  fHistOptions.insert(make_pair(h_rbVanaCal,"same"));

  fHistList.push_back(h_rbVana);
  fHistList.push_back(h_rbVanaCal);
  fHistList.push_back(h_dacVana);
  restoreDacs();
}

void PixTestReadback::CalibrateVd(){
  cacheDacs();

  //readback DAC set to 8 (i.e. Vd)
  fParReadback=8;

  int readback;

  TH1D* h_rbVd = new TH1D("rbVd","rbVd", 500, 0., 5.);
  TH1D* h_dacVd = new TH1D("dacVd","dacVd", 500, 0., 5.);
  vector<double > rbVd;
  double Vd;

  //dry run to avoid spikes
  readback=daqReadback("vd", 2.1, fParReadback);
  readback=0;

  for(int iVd=0; iVd<18; iVd++){
    LOG(logDEBUG)<<"/****:::::: CALIBRATE VD FUNCTION :::::****/";
    Vd = 2.1 + iVd*0.05;
    LOG(logDEBUG)<<"Digital voltage will be set to: "<<Vd;
    readback=daqReadback("vd", Vd, fParReadback);
    LOG(logDEBUG)<<"Voltage "<<Vd<<", readback "<<readback;
    rbVd.push_back(readback);
    h_rbVd->Fill(Vd, readback);
    h_dacVd->Fill(Vd, Vd);
  }
  
  TF1* frb = new TF1("lin_vd", "[0] + x*[1]");

  h_rbVd->Fit(frb, "W", "");

  fPar0VdCal=frb->GetParameter(0);
  fPar1VdCal=frb->GetParameter(1);

  fHistList.push_back(h_rbVd);
  fHistList.push_back(h_dacVd);
  restoreDacs();
}


void PixTestReadback::readbackVbg(){
  cacheDacs();
  //readback DAC set to 11 (i.e. Vbg)
  fParReadback=11;

  int readback=0;

  TH1D* h_rbVbg = new TH1D("rbVbg","rbVbg", 500, 0., 5.);
  TH1D* h_dacVbg = new TH1D("dacVbg","dacVbg", 500, 0., 5.);
  vector<double > rbVbg;
  double Vd;

  int n_meas=0.;
  double avReadback=0.;

  for(int i=0; i<10; i++){
    LOG(logDEBUG)<<"/****:::::: CALIBRATE VBG FUNCTION :::::****/";
    Vd = 2.5;
    LOG(logDEBUG)<<"Digital voltage will be set to: "<<Vd;
    readback = daqReadback("vd", Vd, fParReadback);
    if (0!=readback){
      avReadback+=(double)readback;
      n_meas++;
    }
    LOG(logDEBUG)<<"Voltage "<<Vd<<", average readback "<<(double)readback/(i+1);
    rbVbg.push_back(readback);
    h_rbVbg->Fill(Vd, readback);
    h_dacVbg->Fill(Vd, Vd);
  }

  fRbVbg = avReadback/n_meas;
 
  fHistList.push_back(h_rbVbg);
  fHistList.push_back(h_dacVbg);
  restoreDacs();
}

double PixTestReadback::getCalibratedVbg(){
  double calVbg=0;
  //0.5 needed because Vbg rb has twice the sensitivity Vd and Va have
  if(fCalwVd){
    LOG(logDEBUG)<<"Vbg will be calibrated using Vd calibration";
    calVbg=0.5*(fRbVbg-fPar0VdCal)/fPar1VdCal;
  }
  else if(fCalwVa){
    LOG(logDEBUG)<<"Vbg will be calibrated using Va calibration";
    calVbg=0.5*(fRbVbg-fPar0VaCal)/fPar1VaCal;
  }
  else{
    LOG(logDEBUG)<<"No calibration option specified. Please select one and retry.";
    return 0.;
  }
  LOG(logDEBUG)<<"/*/*/*/*::: Calibrated Vbg = "<<calVbg<<" :::*/*/*/*/";
  return calVbg;
}

void PixTestReadback::CalibrateVa(){
  //readback DAC set to 9 (i.e. Va)
  fParReadback=9;

  int readback;

  TH1D* h_rbVa = new TH1D("rbVa","rbVa", 500, 0., 5.);
  TH1D* h_dacVa = new TH1D("dacVa","dacVa", 500, 0., 5.);
  vector<double > rbVa;
  double Va;
  
  //dry run to avoid spikes
  readback=daqReadback("va", 1.5, fParReadback);
  readback=0;

  for(int iVa=0; iVa<18; iVa++){
    LOG(logDEBUG)<<"/****:::::: CALIBRATE VA FUNCTION :::::****/";
    Va = 1.5 + iVa*0.05;
    LOG(logDEBUG)<<"Analog voltage will be set to: "<<Va;
    readback=daqReadback("va", Va, fParReadback);
    LOG(logDEBUG)<<"Voltage "<<Va<<", readback "<<readback;
    rbVa.push_back(readback);
    h_rbVa->Fill(Va, readback);
    h_dacVa->Fill(Va, Va);
  }

  double rb_VaMax=0.;
  
  rb_VaMax = h_rbVa->GetBinCenter(h_rbVa->FindFirstBinAbove(254));
    
  LOG(logDEBUG)<<"Va max for fit:"<<endl<<"rb: "<<rb_VaMax;
  
  TF1* frb = new TF1("lin_rb", "[0] + x*[1]", 0, rb_VaMax);

  h_rbVa->Fit(frb, "W", "", 0., rb_VaMax);

  LOG(logDEBUG)<<"Number of points for rb fit "<<frb->GetNumberFitPoints();

  fPar0VaCal=frb->GetParameter(0);
  fPar1VaCal=frb->GetParameter(1);

  fHistList.push_back(h_rbVa);
  fHistList.push_back(h_dacVa);
}

uint8_t PixTestReadback::daqReadback(string dac, double vana, int8_t parReadback){

  PixTest::update();
  fDirectory->cd();
  fPg_setup.clear();

  if (!dac.compare("vana")){
    LOG(logDEBUG)<<"Wrong daqReadback function called!!!";
  }
  else if (!dac.compare("vd")){
    vector<pair<string,double > > powerset;
    //using some standard values, should be restored after
    powerset.push_back(make_pair("ia", 1.19));
    powerset.push_back(make_pair("id", 1.10));
    powerset.push_back(make_pair("va", 1.9));
    powerset.push_back(make_pair("vd", vana));
    fApi->setTestboardPower(powerset);
  }
else if (!dac.compare("va")){
    //using some standard values, should be restored after
    vector<pair<string,double > > powerset;
    powerset.push_back(make_pair("ia", 1.19));
    powerset.push_back(make_pair("id", 1.10));
    powerset.push_back(make_pair("va", vana));
    powerset.push_back(make_pair("vd", 2.6));
    fApi->setTestboardPower(powerset);
  }

  fApi->setDAC("readback", parReadback);

  doDAQ();

  std::vector<std::vector<uint16_t> > rb;
  rb = fApi->daqGetReadback();
  uint8_t rb_val=0;

  for(uint i=0; i<rb.size(); i++){
    for(uint j=0; j<rb[i].size(); j++){
      LOG(logDEBUG)<<"Readback values for vana = "<<(int)vana<<" : "<<(int)(rb[i][j]&0xff);
      rb_val=(rb[i][j]&0xff);
    }
  }
  
  //::::::::::::::::::::::::::::::
  //DAQ - THE END.

  FinalCleaning();
  fApi->setClockStretch(0, 0, 0); //No Stretch after trigger, 0 delay
  return rb_val;
 }


uint8_t PixTestReadback::daqReadback(string dac, uint8_t vana, int8_t parReadback){

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
  uint8_t rb_val=0;

  for(uint i=0; i<rb.size(); i++){
    for(uint j=0; j<rb[i].size(); j++){
      LOG(logDEBUG)<<"Readback values for vana = "<<(int)vana<<" : "<<(int)(rb[i][j]&0xff);
      rb_val=(rb[i][j]&0xff);
    }
  }
  
  //::::::::::::::::::::::::::::::
  //DAQ - THE END.

  FinalCleaning();
  fApi->setClockStretch(0, 0, 0); //No Stretch after trigger, 0 delay
  return rb_val;
 }


uint8_t PixTestReadback::daqReadbackIa(){

  PixTest::update();
  fDirectory->cd();
  fPg_setup.clear();

  fApi->setDAC("readback", 12);

  //Immediately stop if parameters not in range	
  if (fParOutOfRange) return 255;

  //Set the ClockStretch
  fApi->setClockStretch(0, 0, fParStretch); //Stretch after trigger, 0 delay
   
  //Set the histograms:
  //  if(fHistList.size() == 0) setHistos();  //to book histo only for the first 'doTest' (or after Clear).

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
	  return 255;
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
  uint8_t rb_val=0;

  for(uint i=0; i<rb.size(); i++){
    for(uint j=0; j<rb[i].size(); j++){
      LOG(logDEBUG)<<"Readback values : "<<(int)(rb[i][j]&0xff);
      rb_val=(rb[i][j]&0xff);
    }
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
  
  double i016 = getCalibratedIa();

  // FIXME this should not be a stopwatch, but a delay
  TStopwatch sw;
  sw.Start(kTRUE); // reset
  do {
    sw.Start(kFALSE); // continue
    i016 = getCalibratedIa();
  } while (sw.RealTime() < 0.1);

  // subtract one ROC to get the offset from the other Rocs (on average):
  double i015 = (nRocs-1) * i016 / nRocs; // = 0 for single chip tests
  LOG(logDEBUG) << "offset current from other " << nRocs-1 << " ROCs is " << i015 << " mA";

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

    double ia = getCalibratedIa(); // [mA], just to be sure to flush usb
    sw.Start(kTRUE); // reset
    do {
      sw.Start(kFALSE); // continue
      ia = getCalibratedIa(); // [mA]
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
	ia = getCalibratedIa(); // [mA]
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
  
  double ia16 = getCalibratedIa(); // [mA]

  sw.Start(kTRUE); // reset
  do {
    sw.Start(kFALSE); // continue
    ia16 = getCalibratedIa(); // [mA]
  }
  while( sw.RealTime() < 0.1 );


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
