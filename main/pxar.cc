#ifndef PIX_H
#define PIX_H

#include <iostream>

#include <TApplication.h> 
#include <TFile.h> 

#include "TBInterface.hh"
#include "TBVirtualInterface.hh"

#include "SysCommand.hh"
#include "ConfigParameters.hh"
#include "PixTestParameters.hh"

#include "PixTest.hh"
#include "PixTestFactory.hh"
#include "PixGui.hh"
#include "PixSetup.hh"
#include "getLine.icc"

#include "../core/api/api.h"


using namespace std;

void runFile(PixSetup &a, string cmdFile);
void execute(PixSetup &a, SysCommand *command);
void runGui(PixSetup &a, int argc = 0, char *argv[] = 0);
void runTest(PixTest *b);


// ----------------------------------------------------------------------
int main(int argc, char *argv[]){
  
  cout << "Welcome to pix!" << endl;

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

  TFile *rfile = TFile::Open(rootfile.c_str(), "RECREATE"); 

  ConfigParameters *configParameters = ConfigParameters::Singleton();
  configParameters->setDirectory(dir);
  string cfgFile = configParameters->getDirectory() + string("/configParameters.dat");
  cout << cfgFile << endl;
  if (!configParameters->readConfigParameterFile(cfgFile))
    return 1;
  
  cout << "pxar>  Setting up the testboard interface ..." << endl;
  TBInterface *tb = new TBVirtualInterface(configParameters);
  if (!tb->IsPresent()) return -1;

  PixTestParameters *ptp = new PixTestParameters(configParameters->getTestParametersFileName()); 

//   cout << "[pix] Setting up the testboard interface ..." << endl;
//   TestControlNetwork *controlNetwork = new TestControlNetwork(tb, configParameters);

  SysCommand sysCommand;
  
  PixSetup a(tb, ptp, configParameters, &sysCommand);  

  if (doRunGui) {
    configParameters->setGuiMode(true);
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
  //   tb->HVoff();
  //   tb->Poff();
  //   tb->Cleanup();

  rfile->Write(); 
  rfile->Close(); 
  
  //  delete configParameters;
  //  delete controlNetwork;
  //  delete tb;
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
    if (sysCommand->Keyword("pixelalive"))  runTest(factory->createTest("pixelalive", a)); 
    if (sysCommand->Keyword("gaincalibration"))  runTest(factory->createTest("gaincalibration", a)); 
    if (sysCommand->Keyword("gui")) runGui(a, 0, 0);
    //     else if (a.TargetIsTB()) {tbInterface -> Execute(command);}
    //     else  {controlNetwork->Execute(command);}
  } while (sysCommand->Next());
  //  tbInterface->Flush();
}

// ----------------------------------------------------------------------
void runGui(PixSetup &a, int argc, char *argv[]) {
  TApplication theApp("App", &argc, argv);
  theApp.SetReturnFromRun(true);
  PixGui gui(gClient->GetRoot(), 1200, 900, &a);
  theApp.Run();
  cout << "closing down 0 " << endl;

//   TApplication * application = new TApplication("App", 0, 0, 0, -1);
//   MainFrame MainFrame(gClient->GetRoot(), 400, 400, tbInterface, controlNetwork, configParameters);
//   application->Run();
}

#endif
