#ifndef PIX_H
#define PIX_H

#include <iostream>
#include <unistd.h>
#include <sys/stat.h>

#include <TApplication.h> 
#include <TFile.h> 
#include <TROOT.h> 
#include <TRint.h> 

#include "ConfigParameters.hh"
#include "PixTestParameters.hh"

#include "PixTest.hh"
#include "PixTestFactory.hh"
#include "PixGui.hh"
#include "PixSetup.hh"

#include "api.h"
#include "log.h"


using namespace std;
using namespace pxar; 

void runGui(PixSetup &a, int argc = 0, char *argv[] = 0);
void runTest(PixTest *b);

 

// ----------------------------------------------------------------------
int main(int argc, char *argv[]){
  
  LOG(logINFO) << "*** Welcome to pxar ***";

  // -- command line arguments
  string dir("."), cmdFile("nada"), rootfile("nada.root"), verbosity("INFO"), flashFile("nada"); 
  bool doRunGui(false), 
    doRunScript(false), 
    noAPI(false), 
    doUpdateFlash(false),
    doAnalysisOnly(false),
    doDummyTest(false),
    doUseRootLogon(false)
    ;
  for (int i = 0; i < argc; i++){
    if (!strcmp(argv[i],"-h")) {
      cout << "List of arguments:" << endl;
      cout << "-a                    do not do tests, do not recreate rootfile, but read in existing rootfile" << endl;
      cout << "-b                    do dummyTest only" << endl;
      cout << "-c filename           read in commands from filename" << endl;
      cout << "-d [--dir] path       directory with config files" << endl;
      cout << "-g                    start with GUI" << endl;
      cout << "-n                    no DUT/API initialization" << endl;
      cout << "-r rootfilename       set rootfile name" << endl;
      cout << "-v verbositylevel     set verbosity level: QUIET CRITICAL ERROR WARNING DEBUG DEBUGAPI DEBUGHAL ..." << endl;
      return 0;
    }
    if (!strcmp(argv[i],"-a"))                                {doAnalysisOnly = true;} 
    if (!strcmp(argv[i],"-b"))                                {doDummyTest = true;} 
    if (!strcmp(argv[i],"-c"))                                {cmdFile    = string(argv[++i]); doRunScript = true;} 
    if (!strcmp(argv[i],"-d") || !strcmp(argv[i],"--dir"))    {dir  = string(argv[++i]); }               
    if (!strcmp(argv[i],"-f"))                                {doUpdateFlash = true; flashFile = string(argv[++i]);} 
    if (!strcmp(argv[i],"-g"))                                {doRunGui   = true; } 
    if (!strcmp(argv[i],"-n"))                                {noAPI   = true; } 
    if (!strcmp(argv[i],"-r"))                                {rootfile  = string(argv[++i]); }               
    if (!strcmp(argv[i],"-v"))                                {verbosity  = string(argv[++i]); }               
  }

  struct stat buffer;   
  if (stat("rootlogon.C", &buffer) == 0) {
    LOG(logINFO) << "reading rootlogon.C, will use gStyle settings from there";
    gROOT->Macro("rootlogon.C");
    doUseRootLogon = true; 
  } else {
    LOG(logINFO) << "no rootlogon.C found, live with the defaults provided";
  }

  if (doRunScript) {
    TRint *interpreter = new TRint("pXar", 0, 0, 0, true);
    interpreter->ExecuteFile(cmdFile.c_str());
    interpreter->Terminate(0);
    LOG(logINFO) << "terminate and shut down"; 
  }


  pxar::api *api(0);
  if (doUpdateFlash) {
    api = new pxar::api("*", verbosity);
    struct stat buffer;   
    if (stat(flashFile.c_str(), &buffer) == 0) {
      
      api->flashTB(flashFile);
    } else {
      LOG(logINFO) << "error: File " << flashFile << " not found" << endl;
    }	  
    delete api;
    return 0; 
  }


  ConfigParameters *configParameters = ConfigParameters::Singleton();
  configParameters->setDirectory(dir);
  string cfgFile = configParameters->getDirectory() + string("/configParameters.dat");
  LOG(logINFO) << "pxar: reading config parameters from " << cfgFile;
  if (!configParameters->readConfigParameterFile(cfgFile)) return 1;

  if (!rootfile.compare("nada.root")) {
    rootfile = configParameters->getDirectory() + "/" + configParameters->getRootFileName();
  } else {
    configParameters->setRootFileName(rootfile); 
    rootfile  = configParameters->getDirectory() + "/" + rootfile;
  }
  
  vector<vector<pair<string,uint8_t> > >       rocDACs = configParameters->getRocDacs(); 
  vector<vector<pair<string,uint8_t> > >       tbmDACs = configParameters->getTbmDacs(); 
  vector<vector<pixelConfig> >                 rocPixels = configParameters->getRocPixelConfig();
  vector<pair<string,uint8_t> >                sig_delays = configParameters->getTbSigDelays(); 
  vector<pair<string, double> >                power_settings = configParameters->getTbPowerSettings();
  vector<pair<uint16_t, uint8_t> >             pg_setup = configParameters->getTbPgSettings();

  if (!noAPI) {
    try {
      api = new pxar::api("*", verbosity);

      api->initTestboard(sig_delays, power_settings, pg_setup);
      api->initDUT(configParameters->getTbmType(), tbmDACs, 
		   configParameters->getRocType(), rocDACs, 
		   rocPixels);
      LOG(logINFO) << "DUT info: ";
      api->_dut->info(); 
      
    } catch (...) {
      LOG(logINFO)<< "pxar caught an exception from the board. Exiting.";
      return -1;
    }
  }

  PixTestParameters *ptp = new PixTestParameters(configParameters->getDirectory() + "/" + configParameters->getTestParameterFileName()); 
  PixSetup a(api, ptp, configParameters);  
  a.setDummy(doDummyTest);
  a.setUseRootLogon(doUseRootLogon); 

  LOG(logINFO)<< "pxar: dumping results into " << rootfile;
  TFile *rfile(0); 
  if (doAnalysisOnly) {
    rfile = TFile::Open(rootfile.c_str(), "UPDATE"); 
  } else {
    rfile = TFile::Open(rootfile.c_str(), "RECREATE"); 
  }


  if (doRunGui) {
    runGui(a, argc, argv); 
  } 

  LOG(logINFO) << "closing down 1";
  
  // -- clean exit (however, you should not get here)
  rfile->Write(); 
  rfile->Close(); 
  
  //  delete configParameters;
  //  delete controlNetwork;
  if (!noAPI) {
    delete api;
  }

  LOG(logINFO) << "pXar: this is the end, my friend";

  return 0;
}


// ----------------------------------------------------------------------
void runTest(PixTest *b) {
  if (b) b->doTest();
  else LOG(logINFO) << "test not known";
}

// ----------------------------------------------------------------------
void runGui(PixSetup &a, int argc, char *argv[]) {
  TApplication theApp("App", 0, 0);
  theApp.SetReturnFromRun(true);
  PixGui gui(gClient->GetRoot(), 1300, 800, &a);
  theApp.Run();
  LOG(logINFO) << "closing down 0 ";
}

#endif
