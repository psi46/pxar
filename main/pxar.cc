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

#include "PixGui.hh"

using namespace std;

// ----------------------------------------------------------------------
int main(int argc, char *argv[]){
  
  cout << "Welcome to pix!" << endl;

  // -- command line arguments
  string dir("."); 
  bool debug(false);
  for (int i = 0; i < argc; i++){
    if (!strcmp(argv[i],"-h")) {
      cout << "List of arguments:" << endl;
      cout << "-D [--dir] path       directory with config files" << endl;
      cout << "-d                    debug -- no testboard connected" << endl;
      return 0;
    }
    if (!strcmp(argv[i],"-d"))                                {debug      = 1; } 
    if (!strcmp(argv[i],"-D") || !strcmp(argv[i],"--dir"))    {dir  = string(argv[++i]); }               
  }

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


  TApplication theApp("App", &argc, argv);
  pixSetup a; 
  a.debug = debug;
  a.aTB = tb;
  //  a.aCN = 0;
  a.aCP = configParameters; 
  a.aPTP = ptp; 

  configParameters->setGuiMode(true);

  TFile *f = TFile::Open("pxar.root", "RECREATE"); 

  PixGui gui(gClient->GetRoot(), 1200, 900, &a);
  theApp.Run();

  // -- clean exit
  tb->HVoff();
  tb->Poff();
  tb->Cleanup();

  f->Write(); 
  f->Close(); 
  
  delete configParameters;
  //  delete controlNetwork;
  delete tb;
  cout << "pix: this is the end, my friend" << endl;

  return 0;
}
#endif
