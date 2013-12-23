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
      api = new pxar::api("*","DEBUG");
      std::vector<std::pair<std::string,uint8_t> > sig_delays;
      std::vector<std::pair<std::string,double> > power_settings;
      std::vector<std::pair<std::string,uint8_t> > pg_setup;
      api->initTestboard(sig_delays, power_settings, pg_setup);
    } catch (...) {
      cout << "pxar caught an exception from the board. Exiting." << endl;
      return -1;
    }
  }

  
  std::vector<std::vector<std::pair<std::string,uint8_t> > > rocDACs;
  std::vector<std::pair<std::string,uint8_t> > dacs;
  dacs.push_back(std::make_pair("Vdig",6));
  dacs.push_back(std::make_pair("Vana",84));
  dacs.push_back(std::make_pair("Vsf",30));
  dacs.push_back(std::make_pair("Vcomp",12));
  dacs.push_back(std::make_pair("VwllPr",60));
  dacs.push_back(std::make_pair("VwllSh",60));
  dacs.push_back(std::make_pair("VhldDel",230));
  dacs.push_back(std::make_pair("Vtrim",29));
  dacs.push_back(std::make_pair("VthrComp",86));
  dacs.push_back(std::make_pair("VIBias_Bus",1));
  dacs.push_back(std::make_pair("Vbias_sf",6));
  dacs.push_back(std::make_pair("VoffsetOp",40));
  dacs.push_back(std::make_pair("VOffsetR0",129));
  dacs.push_back(std::make_pair("VIon",120));
  dacs.push_back(std::make_pair("Vcomp_ADC",100));
  dacs.push_back(std::make_pair("VIref_ADC",91));
  dacs.push_back(std::make_pair("VIbias_roc",150));
  dacs.push_back(std::make_pair("VIColOr",50));
  dacs.push_back(std::make_pair("Vcal",200));
  dacs.push_back(std::make_pair("CalDel",122));
  dacs.push_back(std::make_pair("CtrlReg",0));
  dacs.push_back(std::make_pair("WBC",100));
  rocDACs.push_back(dacs);

    // Get some pixelConfigs up and running:
    std::vector<std::vector<pxar::pixelConfig> > rocPixels;
    std::vector<pxar::pixelConfig> pixels;

    for(int col = 0; col < 52; col++) {
      for(int row = 0; row < 80; row++) {
	pxar::pixelConfig newpix;
	newpix.column = col;
	newpix.row = row;
	//Trim: fill random value, should report fail as soon as we have range checks:
	newpix.trim = row;
	newpix.mask = false;
	newpix.enable = true;

	pixels.push_back(newpix);
      }
    }
    rocPixels.push_back(pixels);

  
  // Initialize the DUT (power it up and stuff):
  std::vector<std::vector<std::pair<std::string,uint8_t> > > tbmDACs;
  api->initDUT("", tbmDACs, "psi46digV3", rocDACs, rocPixels);


  api->_dut->setAllPixelEnable(false);
  api->_dut->setPixelEnable(34,12,true);
  api->_dut->setPixelEnable(33,12,true);
  api->_dut->setPixelEnable(34,11,true);
  api->_dut->setPixelEnable(14,12,true);

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
