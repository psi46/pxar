#ifndef PIX_H
#define PIX_H

#include <iostream>
#include <algorithm>
#include <fstream>
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
  gSystem->Exec("git status");

  // -- command line arguments
  string dir("."), cmdFile("nada"), rootfile("nada.root"), logfile("nada.log"), 
    verbosity("INFO"), flashFile("nada"), runtest("fulltest"), trimVcal(""), testParameters("nada"); 
  bool doRunGui(false), 
    doRunScript(false), 
    doRunSingleTest(false), 
    doUpdateFlash(false),
    doUpdateRootFile(false),
    doMoreWebCloning(false), 
    doUseRootLogon(false)
    ;
  for (int i = 0; i < argc; i++){
    if (!strcmp(argv[i],"-h")) {
      cout << "List of arguments:" << endl;
      cout << "-a                    do not do tests, do not recreate rootfile, but read in existing rootfile" << endl;
      cout << "-c filename           read in commands from filename" << endl;
      cout << "-d [--dir] path       directory with config files" << endl;
      cout << "-g                    start with GUI" << endl;
      cout << "-m                    clone pxar histograms into the histograms expected by moreweb" << endl;
      cout << "-p \"p1=v1[;p2=v2]\"  set parameters for test" << endl;
      cout << "-r rootfilename       set rootfile (and logfile) name" << endl;
      cout << "-t test               run test" << endl;
      cout << "-T [--vcal] XX        read in DAC and Trim parameter files corresponding to trim VCAL = XX" << endl;
      cout << "-v verbositylevel     set verbosity level: QUIET CRITICAL ERROR WARNING DEBUG DEBUGAPI DEBUGHAL ..." << endl;
      return 0;
    }
    if (!strcmp(argv[i],"-c"))                                {cmdFile    = string(argv[++i]); doRunScript = true;} 
    if (!strcmp(argv[i],"-d") || !strcmp(argv[i], "--dir"))   {dir  = string(argv[++i]); }               
    if (!strcmp(argv[i],"-f"))                                {doUpdateFlash = true; flashFile = string(argv[++i]);} 
    if (!strcmp(argv[i],"-g"))                                {doRunGui   = true; } 
    if (!strcmp(argv[i],"-m"))                                {doMoreWebCloning = true; } 
    if (!strcmp(argv[i],"-p"))                                {testParameters  = string(argv[++i]); }               
    if (!strcmp(argv[i],"-r"))                                {rootfile  = string(argv[++i]); }               
    if (!strcmp(argv[i],"-t"))                                {doRunSingleTest = true; runtest  = string(argv[++i]); }
    if (!strcmp(argv[i],"-T") || !strcmp(argv[i], "--vcal"))  {trimVcal = string(argv[++i]); }
    if (!strcmp(argv[i],"-u"))                                {doUpdateRootFile = true;} 
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


  pxar::pxarCore *api(0);
  if (doUpdateFlash) {
    api = new pxar::pxarCore("*", verbosity);
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

  if (trimVcal.compare("")) {
    configParameters->setTrimVcalSuffix(trimVcal); 
  }
  
  logfile = rootfile; 
  PixUtil::replaceAll(logfile, ".root", ".log");
  
  LOG(logINFO)<< "pxar: dumping results into " << rootfile << " logfile = " << logfile;
  TFile *rfile(0); 
  FILE* lfile;
  if (doUpdateRootFile) {
    rfile = TFile::Open(rootfile.c_str(), "UPDATE"); 
    lfile = fopen(logfile.c_str(), "a");
    SetLogOutput::Stream() = lfile;
    SetLogOutput::Duplicate() = true;
  } else {
    createBackup(rootfile, logfile); 
    rfile = TFile::Open(rootfile.c_str(), "RECREATE"); 
    lfile = fopen(logfile.c_str(), "a");
    SetLogOutput::Stream() = lfile;
    SetLogOutput::Duplicate() = true;
  }

  vector<vector<pair<string,uint8_t> > >       rocDACs = configParameters->getRocDacs(); 
  vector<vector<pair<string,uint8_t> > >       tbmDACs = configParameters->getTbmDacs(); 
  vector<vector<pixelConfig> >                 rocPixels = configParameters->getRocPixelConfig();
  vector<pair<string,uint8_t> >                sig_delays = configParameters->getTbSigDelays(); 
  vector<pair<string, double> >                power_settings = configParameters->getTbPowerSettings();
  vector<pair<std::string, uint8_t> >          pg_setup = configParameters->getTbPgSettings();
  string tbname = "*";
  if (configParameters->getTbName() != "")
    tbname = configParameters->getTbName();

  try {
    api = new pxar::pxarCore(tbname, verbosity);
    
    api->initTestboard(sig_delays, power_settings, pg_setup);
    if (configParameters->customI2cAddresses()) {
      string i2cstring("");
      vector<uint8_t> i2cAddr = configParameters->getI2cAddresses(); 
      for (unsigned int i = 0; i < i2cAddr.size(); ++i) i2cstring += Form(" %d", (int)i2cAddr[i]); 
      LOG(logINFO) << "custom i2c addresses: " << i2cstring; 
      api->initDUT(configParameters->getHubId(),
		   configParameters->getTbmType(), tbmDACs, 
		   configParameters->getRocType(), rocDACs, 
		   rocPixels, 
		   i2cAddr);
    } else {
      api->initDUT(configParameters->getHubId(),
		   configParameters->getTbmType(), tbmDACs, 
		   configParameters->getRocType(), rocDACs, 
		   rocPixels);
    }

    // -- Set up the four signal probe outputs:
    api->SignalProbe("a1", configParameters->getProbe("a1"));
    api->SignalProbe("a2", configParameters->getProbe("a2"));
    api->SignalProbe("d1", configParameters->getProbe("d1"));
    api->SignalProbe("d2", configParameters->getProbe("d2"));

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
						 , true); 
  PixSetup a(api, ptp, configParameters);  
  a.setUseRootLogon(doUseRootLogon); 
  a.setMoreWebCloning(doMoreWebCloning); 
  a.setRootFileUpdate(doUpdateRootFile);

  if (doRunGui) {
    runGui(a, argc, argv); 
  } else if (doRunSingleTest) {
    PixTestFactory *factory = PixTestFactory::instance(); 
    if (configParameters->getHvOn()) api->HVon(); 

    // -- search for subtest 
    string::size_type m0 = runtest.find(":"); 
    string subtest("nada"); 
    if (m0 != string::npos) {
      subtest = runtest.substr(m0+1); 
      runtest = runtest.substr(0, m0); 
    }
    
    if (testParameters.compare("nada")) {
      ptp->setTestParameters(runtest, testParameters); 
    }
    PixTest *t = factory->createTest(runtest, &a);
    if (subtest.compare("nada")) {
      t->runCommand(subtest); 
    } else {
      t->doTest();
    }
    delete t; 
  } else {
    string input; 
    bool stop(false);
    PixTestFactory *factory = PixTestFactory::instance(); 
    if (configParameters->getHvOn()) api->HVon(); 
    LOG(logINFO) << "enter 'restricted' command line mode";
    do {
      LOG(logINFO) << "enter test to run";
      cout << "pxar> "; 
      string input;
      std::getline(cin, input);
      if (input.size() == 0) stop = true;
      string parameters("nada"), subtest("nada");
      // -- split input with space into testname(s) and parameters
      string::size_type m1 = input.find(" "); 
      if (m1 != string::npos) {
	parameters = input.substr(m1+1); 
	input = input.substr(0, m1);
	cout << "parameters: ->" << parameters << "<- input: ->" << input << "<-" << endl;
      }
      // -- find subtest
      string::size_type m0 = input.find(":"); 
      if (m0 != string::npos) {
	subtest = input.substr(m0+1); 
	input = input.substr(0, m0); 
	cout << "subtest: ->" << subtest << "<- input: ->" << input << "<-" << endl;
      }


      if (!parameters.compare("nada")) {
	LOG(logINFO) << "  test: " << input << " no parameter change"; 
      } else {
	LOG(logINFO) << "  test: " << input << " setting parameters: ->" << parameters << "<-"; 
	ptp->setTestParameters(input, parameters); 
      }

      std::transform(subtest.begin(), subtest.end(), subtest.begin(), ::tolower);
      std::transform(input.begin(), input.end(), input.begin(), ::tolower);
      
      if (!input.compare("gui"))  runGui(a, argc, argv); 
      if (!input.compare("exit")) stop = true; 
      if (!input.compare("quit")) stop = true; 
      if (!input.compare("q")) stop = true; 

      if (stop) break;
      LOG(logINFO) << "  running: " << input; 
      PixTest *t = factory->createTest(input, &a);
      if (t) {
	if (subtest.compare("nada")) {
	  t->runCommand(subtest); 
	} else {
	  t->doTest();
	}
	delete t;
      } else {
	LOG(logINFO) << "command ->" << input << "<- not known, ignored";
      }
    } while (!stop);
    

  }
  
  // -- clean exit (however, you should not get here when running with the GUI)
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
