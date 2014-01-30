#ifndef PIX_H
#define PIX_H

#include <iostream>
#include <unistd.h>
#include <sys/stat.h>

#include <TApplication.h> 
#include <TFile.h> 
#include <TStyle.h> // DP

#include "SysCommand.hh"
#include "ConfigParameters.hh"
#include "PixTestParameters.hh"

#include "PixTest.hh"
#include "PixTestFactory.hh"
#include "PixGui.hh"
#include "PixSetup.hh"
#include "getLine.icc"

#include "api.h"
#include "log.h"


using namespace std;
using namespace pxar; 

void runFile(PixSetup &a, string cmdFile);
void execute(PixSetup &a, SysCommand *command);
void runGui(PixSetup &a, int argc = 0, char *argv[] = 0);
void runTest(PixTest *b);



// ----------------------------------------------------------------------
int main(int argc, char *argv[]){
  
  LOG(logINFO) << "=== Welcome to pxar ===";

  if (0) {
    uint16_t x = 0; 
    int32_t y = 1; 
    y |= x; 
    cout << "y = " << y << " int(y) = " << int(y) << endl;
    return 0; 
  }


  // -- command line arguments
  string dir("."), cmdFile("cal.sys"), rootfile("nada.root"), verbosity("INFO"), flashFile("nada"); 
  bool doRunGui(false), 
    doRunScript(false), 
    noAPI(false), 
    doUpdateFlash(false),
    doAnalysisOnly(false);
  for (int i = 0; i < argc; i++){
    if (!strcmp(argv[i],"-h")) {
      cout << "List of arguments:" << endl;
      cout << "-a                    do not do tests, do not recreate rootfile, but read in existing rootfile" << endl;
      cout << "-c filename           read in commands from filename" << endl;
      cout << "-d [--dir] path       directory with config files" << endl;
      cout << "-g                    start with GUI" << endl;
      cout << "-n                    no DUT/API initialization" << endl;
      cout << "-r rootfilename       set rootfile name" << endl;
      cout << "-v verbositylevel     set verbosity level: QUIET CRITICAL ERROR WARNING DEBUG DEBUGAPI DEBUGHAL ..." << endl;
      return 0;
    }
    if (!strcmp(argv[i],"-a"))                                {doAnalysisOnly = true;} 
    if (!strcmp(argv[i],"-c"))                                {cmdFile    = string(argv[++i]); doRunScript = true;} 
    if (!strcmp(argv[i],"-d") || !strcmp(argv[i],"--dir"))    {dir  = string(argv[++i]); }               
    if (!strcmp(argv[i],"-f"))                                {doUpdateFlash = true; flashFile = string(argv[++i]);} 
    if (!strcmp(argv[i],"-g"))                                {doRunGui   = true; } 
    if (!strcmp(argv[i],"-n"))                                {noAPI   = true; } 
    if (!strcmp(argv[i],"-r"))                                {rootfile  = string(argv[++i]); }               
    if (!strcmp(argv[i],"-v"))                                {verbosity  = string(argv[++i]); }               
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
  SysCommand sysCommand;
  PixSetup a(api, ptp, configParameters, &sysCommand);  

  LOG(logINFO)<< "pxar: dumping results into " << rootfile;
  TFile *rfile(0); 
  if (doAnalysisOnly) {
    rfile = TFile::Open(rootfile.c_str(), "UPDATE"); 
  } else {
    rfile = TFile::Open(rootfile.c_str(), "RECREATE"); 
  }

  if (doRunGui) {
    runGui(a, argc, argv); 
  } else if (doRunScript) {
    runFile(a, cmdFile);    
  } else {
    char * p;
    do {
      p = Getline("pxar> ");
      if (sysCommand.Parse(p)) execute(a, &sysCommand);
    } while ((strcmp(p,"exit\n") != 0) && (strcmp(p,"q\n") != 0));

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
void runFile(PixSetup &a, string cmdFile) {
  LOG(logINFO) << "Executing file " << cmdFile;
  a.getSysCommand()->Read(cmdFile.c_str());
  execute(a, a.getSysCommand());
}


// ----------------------------------------------------------------------
void runTest(PixTest *b) {
  if (b) b->doTest();
  else LOG(logINFO) << "test not known";
}


// ----------------------------------------------------------------------
void execute(PixSetup &a, SysCommand *sysCommand) {
  PixTestFactory *factory = PixTestFactory::instance(); 
  LOG(logINFO) << "sysCommand.toString(): " << sysCommand->toString();
  if (sysCommand->TargetIsTest()) 
    runTest(factory->createTest(sysCommand->toString(), &a)); 
  else if (sysCommand->Keyword("gui")) 
    runGui(a, 0, 0);
  else if (sysCommand->TargetIsTB()) 
    LOG(logINFO) << "FIXME  a.getTBInterface()->Execute(sysCommand);";
  //FIXME  a.getTBInterface()->Execute(sysCommand);
  else if (sysCommand->TargetIsROC()) 
    LOG(logINFO) << "FIXME  a.getTBInterface()->Execute(sysCommand);";
  //FIXME      a.getTBInterface()->Execute(sysCommand);
  else 
    LOG(logINFO) << "dunno what to do";
  //    else if (sysCommand->Keyword("gaincalibration"))  runTest(factory->createTest("gaincalibration", a)); 
}

// ----------------------------------------------------------------------
void runGui(PixSetup &a, int argc, char *argv[]) {
  TApplication theApp("App", &argc, argv);
  theApp.SetReturnFromRun(true);

  //DP:

  gStyle->SetTickLength( -0.02, "x" ); // tick marks outside
  gStyle->SetTickLength( -0.02, "y");
  gStyle->SetTickLength( -0.01, "z");

  gStyle->SetLabelOffset( 0.022, "x" );
  gStyle->SetLabelOffset( 0.022, "y" );
  gStyle->SetLabelOffset( 0.022, "z" );

  gStyle->SetTitleOffset( 1.3, "x" );
  gStyle->SetTitleOffset( 1.6, "y" );
  gStyle->SetTitleOffset( 1.7, "z" );

  gStyle->SetLabelFont( 62, "X" );
  gStyle->SetLabelFont( 62, "Y" );
  gStyle->SetLabelFont( 62, "z" );

  gStyle->SetTitleFont( 62, "X" );
  gStyle->SetTitleFont( 62, "Y" );
  gStyle->SetTitleFont( 62, "z" );

  gStyle->SetLineWidth(1);// frames
  gStyle->SetHistLineColor(4); // 4=blau
  gStyle->SetHistLineWidth(3);
  gStyle->SetHistFillColor(5); // 5=gelb
  //  gStyle->SetHistFillStyle(4050); // 4050 = half transparent
  gStyle->SetHistFillStyle(1001); // 1001 = solid

  gStyle->SetFrameLineWidth(2);

  gStyle->SetPalette(1); // rainbow colors

  gStyle->SetHistMinimumZero(); // no zero suppression

  PixGui gui(gClient->GetRoot(), 1300, 800, &a);
  theApp.Run();
  LOG(logINFO) << "closing down 0 ";
}

#endif
