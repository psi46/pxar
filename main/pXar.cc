#ifndef PIX_H
#define PIX_H

#include <iostream>
#include <unistd.h>

#include <TApplication.h> 
#include <TFile.h> 

#include "SysCommand.hh"
#include "ConfigParameters.hh"
#include "PixTestParameters.hh"

#include "PixTest.hh"
#include "PixTestFactory.hh"
#include "PixGui.hh"
#include "PixSetup.hh"
#include "getLine.icc"

#include "api.h"


using namespace std;

void runFile(PixSetup &a, string cmdFile);
void execute(PixSetup &a, SysCommand *command);
void runGui(PixSetup &a, int argc = 0, char *argv[] = 0);
void runTest(PixTest *b);


// ----------------------------------------------------------------------
int main(int argc, char *argv[]){
  
  //  LOG(logINFO) << "Welcome to pxar!";
  cout << "welcome to pxar" << endl;

  // -- command line arguments
  string dir("."), cmdFile("cal.sys"), rootfile("pxar.root"); 
  bool debug(false), doRunGui(false), doRunScript(false);
  for (int i = 0; i < argc; i++){
    if (!strcmp(argv[i],"-h")) {
      cout << "List of arguments:" << endl;
      cout << "-c filename           read in commands from filename" << endl;
      cout << "-D [--dir] path       directory with config files" << endl;
      cout << "-d                    more debug printout (to be implemented)" << endl;
      cout << "-g                    start with GUI" << endl;
      cout << "-r rootfilename       set rootfile name" << endl;
      return 0;
    }
    if (!strcmp(argv[i],"-c"))                                {cmdFile    = string(argv[++i]); doRunScript = true;} 
    if (!strcmp(argv[i],"-D") || !strcmp(argv[i],"--dir"))    {dir  = string(argv[++i]); }               
    if (!strcmp(argv[i],"-d"))                                {debug      = true; } 
    if (!strcmp(argv[i],"-g"))                                {doRunGui   = true; } 
    if (!strcmp(argv[i],"-r"))                                {rootfile  = string(argv[++i]); }               
  }

  pxar::api *api = 0;
  if (1) {
    try {
      api = new pxar::api();
      api->initTestboard();
    } catch (...) {
      cout << "pxar caught an exception from the board. Exiting." << endl;
      return -1;
    }
  }
  
  
  TFile *rfile = TFile::Open(rootfile.c_str(), "RECREATE"); 
  
  ConfigParameters *configParameters = ConfigParameters::Singleton();
  configParameters->setDirectory(dir);
  string cfgFile = configParameters->getDirectory() + string("/configParameters.dat");
  cout << cfgFile << endl;
  if (!configParameters->readConfigParameterFile(cfgFile))
    return 1;
  
  cout << "pxar>  Setting up the testboard interface ..." << endl;

  PixTestParameters *ptp = new PixTestParameters(configParameters->getTestParametersFileName()); 

//   cout << "[pix] Setting up the testboard interface ..." << endl;
//   TestControlNetwork *controlNetwork = new TestControlNetwork(tb, configParameters);

  SysCommand sysCommand;
  
  PixSetup a(api, ptp, configParameters, &sysCommand);  

  if (doRunGui) {
    runGui(a, argc, argv); 
  } else if (doRunScript) {
    runFile(a, cmdFile);    
  } else {
    char * p;
    do {
      p = Getline("psi46expert> ");
      if (sysCommand.Parse(p)) execute(a, &sysCommand);
    } while ((strcmp(p,"exit\n") != 0) && (strcmp(p,"q\n") != 0));

  }

  cout << "closing down 1" << endl;
  
  // -- clean exit

  rfile->Write(); 
  rfile->Close(); 
  
  //  delete configParameters;
  //  delete controlNetwork;
  cout << "pixar: this is the end, my friend" << endl;

  return 0;
}

// ----------------------------------------------------------------------
void runFile(PixSetup &a, string cmdFile) {
  cout << "Executing file " << cmdFile << endl;
  a.getSysCommand()->Read(cmdFile.c_str());
  execute(a, a.getSysCommand());
}


// ----------------------------------------------------------------------
void runTest(PixTest *b) {
  if (b) b->doTest();
  else cout << "test not known" << endl;
}


// ----------------------------------------------------------------------
void execute(PixSetup &a, SysCommand *sysCommand) {
  PixTestFactory *factory = PixTestFactory::instance(); 
  do {
    cout << "sysCommand.toString(): " << sysCommand->toString() << endl;
    if (sysCommand->TargetIsTest()) 
      runTest(factory->createTest(sysCommand->toString(), a)); 
    else if (sysCommand->Keyword("gui")) 
      runGui(a, 0, 0);
    else if (sysCommand->TargetIsTB()) 
      cout << "FIXME  a.getTBInterface()->Execute(sysCommand);" << endl;
      //FIXME  a.getTBInterface()->Execute(sysCommand);
    else if (sysCommand->TargetIsROC()) 
      cout << "FIXME  a.getTBInterface()->Execute(sysCommand);" << endl;
      //FIXME      a.getTBInterface()->Execute(sysCommand);
    else 
      cout << "dunno what to do" << endl;
    //    else if (sysCommand->Keyword("gaincalibration"))  runTest(factory->createTest("gaincalibration", a)); 
  } while (sysCommand->Next());
  //  tbInterface->Flush();
}

// ----------------------------------------------------------------------
void runGui(PixSetup &a, int argc, char *argv[]) {
  TApplication theApp("App", &argc, argv);
  theApp.SetReturnFromRun(true);
  PixGui gui(gClient->GetRoot(), 800, 500, &a);
  theApp.Run();
  cout << "closing down 0 " << endl;
}

#endif
