#ifndef PIX_H
#define PIX_H

#include <iostream>
#include <sys/stat.h>

#if (defined WIN32)
#include <Windows4Root.h>
#endif

#include <TApplication.h> 
#include <TFile.h> 
#include <TROOT.h> 
#include <TRint.h> 
#include <TSystem.h>
#include <TDatime.h>

#include "ConfigParameters.hh"
#include "PixTestParameters.hh"

#include "PixTest.hh"
#include "PixTestFactory.hh"
#include "PixGui.hh"
#include "PixSetup.hh"
#include "PixUtil.hh"

#include "api.h"
#include "log.h"


using namespace std;
using namespace pxar; 

void runGui(PixSetup &a, int argc = 0, char *argv[] = 0);
void createBackup(string a, string b);  

// ----------------------------------------------------------------------
int main(int argc, char *argv[]){
  
  LOG(logINFO) << "*** Welcome to pxar ***";

  // -- command line arguments
  string dir("."), cmdFile("nada"), rootfile("nada.root"), logfile("nada.log"), 
    verbosity("INFO"), flashFile("nada"), runtest("fulltest"); 
  bool doRunGui(false), 
    doRunScript(false), 
    doRunSingleTest(false), 
    doUpdateFlash(false),
    doAnalysisOnly(false),
    doDummyTest(false),
    doMoreWebCloning(false), 
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
      cout << "-m                    clone pxar histograms into the histograms expected by moreweb" << endl;
      cout << "-r rootfilename       set rootfile (and logfile) name" << endl;
      cout << "-t test               run test" << endl;
      cout << "-v verbositylevel     set verbosity level: QUIET CRITICAL ERROR WARNING DEBUG DEBUGAPI DEBUGHAL ..." << endl;
      return 0;
    }
    if (!strcmp(argv[i],"-a"))                                {doAnalysisOnly = true;} 
    if (!strcmp(argv[i],"-b"))                                {doDummyTest = true;} 
    if (!strcmp(argv[i],"-c"))                                {cmdFile    = string(argv[++i]); doRunScript = true;} 
    if (!strcmp(argv[i],"-d") || !strcmp(argv[i],"--dir"))    {dir  = string(argv[++i]); }               
    if (!strcmp(argv[i],"-f"))                                {doUpdateFlash = true; flashFile = string(argv[++i]);} 
    if (!strcmp(argv[i],"-g"))                                {doRunGui   = true; } 
    if (!strcmp(argv[i],"-m"))                                {doMoreWebCloning = true; } 
    if (!strcmp(argv[i],"-r"))                                {rootfile  = string(argv[++i]); }               
    if (!strcmp(argv[i],"-t"))                                {doRunSingleTest = true; runtest  = string(argv[++i]); }               
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

  logfile = rootfile; 
  PixUtil::replaceAll(logfile, ".root", ".log");
  createBackup(rootfile, logfile); 
  
  LOG(logINFO)<< "pxar: dumping results into " << rootfile << " logfile = " << logfile;
  TFile *rfile(0); 
  FILE* lfile;
  if (doAnalysisOnly) {
    rfile = TFile::Open(rootfile.c_str(), "UPDATE"); 
  } else {
    rfile = TFile::Open(rootfile.c_str(), "RECREATE"); 
    lfile = fopen(logfile.c_str(), "a");
    SetLogOutput::Stream() = lfile;
  }

  vector<vector<pair<string,uint8_t> > >       rocDACs = configParameters->getRocDacs(); 
  vector<vector<pair<string,uint8_t> > >       tbmDACs = configParameters->getTbmDacs(); 
  vector<vector<pixelConfig> >                 rocPixels = configParameters->getRocPixelConfig();
  vector<pair<string,uint8_t> >                sig_delays = configParameters->getTbSigDelays(); 
  vector<pair<string, double> >                power_settings = configParameters->getTbPowerSettings();
  vector<pair<uint16_t, uint8_t> >             pg_setup = configParameters->getTbPgSettings();

  try {
    api = new pxar::api("*", verbosity);
    
    api->initTestboard(sig_delays, power_settings, pg_setup);
    api->initDUT(configParameters->getHubId(),
		 configParameters->getTbmType(), tbmDACs, 
		 configParameters->getRocType(), rocDACs, 
		 rocPixels);
    LOG(logINFO) << "DUT info: ";
    api->_dut->info(); 
    
  } 
  catch (pxar::InvalidConfig &e){
    std::cout << "pxar caught an exception due to invalid configuration settings: " << e.what() << std::endl;
    delete api;
    return -1;    
  }
  catch (pxar::pxarException &e){
    std::cout << "pxar caught an internal exception: " << e.what() << std::endl;
    delete api;
    return -1;    
  }
  catch (...) {
    std::cout << "pxar caught an unknown exception. Exiting." << std::endl;
    delete api;
    return -1;
  }

  PixTestParameters *ptp = new PixTestParameters(configParameters->getDirectory() + "/" 
						 + configParameters->getTestParameterFileName()
						 ); 
  PixSetup a(api, ptp, configParameters);  
  a.setDummy(doDummyTest);
  a.setUseRootLogon(doUseRootLogon); 
  a.setMoreWebCloning(doMoreWebCloning); 

  if (doRunGui) {
    runGui(a, argc, argv); 
  } else if (doRunSingleTest) {
    PixTestFactory *factory = PixTestFactory::instance(); 
    PixTest *t = factory->createTest(runtest, &a);
    t->doTest();
    delete t; 
    delete api; 
    return 0;
  } else {
    string input; 
    bool stop(false);
    PixTestFactory *factory = PixTestFactory::instance(); 
    LOG(logINFO) << "enter restricted command line mode";
    do {
      LOG(logINFO) << "enter test to run";
      cout << "pxar> "; 
      cin >> input; 
      LOG(logINFO) << "  running: " << input; 

      if (!input.compare("exit")) stop = true; 
      if (!input.compare("quit")) stop = true; 
      if (!input.compare("q")) stop = true; 

      PixTest *t = factory->createTest(input, &a);
      if (t) {
	t->doTest();
	delete t;
      }
    } while (!stop);
    

  }

  LOG(logINFO) << "closing down 1";
  
  // -- clean exit (however, you should not get here)
  rfile->Write(); 
  rfile->Close(); 
  
  if (api) delete api;

  LOG(logINFO) << "pXar: this is the end, my friend";

  return 0;
}


// ----------------------------------------------------------------------
void runGui(PixSetup &a, int /*argc*/, char ** /*argv[]*/) {
  TApplication theApp("App", 0, 0);
  theApp.SetReturnFromRun(true);
  PixGui gui(gClient->GetRoot(), 1300, 800, &a);
  theApp.Run();
  LOG(logINFO) << "closing down 0 ";
}


// ----------------------------------------------------------------------
void createBackup(string rootfile, string logfile) {
  
  Long_t id, flags, modtime; 
  Long64_t size; 

  string nrootfile(rootfile), nlogfile(logfile); 
  const char *path = rootfile.c_str(); 
  int result = gSystem->GetPathInfo(path, &id, &size, &flags, &modtime);
  if (1 == result) return;

  TDatime d(modtime);
  string tstamp = Form("_%d%02d%02d_%02d%02d%02d", d.GetYear(), d.GetMonth(), d.GetDay(), d.GetHour(), d.GetMinute(), d.GetSecond()); 
  PixUtil::replaceAll(nrootfile, ".root", tstamp+".root");
  PixUtil::replaceAll(nlogfile, ".log", tstamp+".log");

  LOG(logINFO) << "creating backup files for previous run: " << nrootfile << " and " << nlogfile; 
  if (!gSystem->AccessPathName(rootfile.c_str())) gSystem->Rename(rootfile.c_str(), nrootfile.c_str()); 
  if (!gSystem->AccessPathName(logfile.c_str())) gSystem->Rename(logfile.c_str(), nlogfile.c_str()); 
  
}

#endif
