// -- Invocation:
// --------------
//    ../bin/pXar -d ../data/defaultParametersRocPSI46digV2 -c '../scripts/singleTest.C("PixelAlive", "pixelalive.root", "../data/defaultParametersRocPSI46digV2")'

void singleTest(string testname = "PixelAlive", string rootfilename = "pixelalive.root", string cfgdirectory = "../data/defaultParametersRocPSI46digV2") {
  ConfigParameters *configParameters = ConfigParameters::Singleton();

  configParameters->setDirectory(cfgdirectory);
  string cfgFile = configParameters->getDirectory() + string("/configParameters.dat");
  configParameters->readConfigParameterFile(cfgFile);

  string rootfile = rootfilename;
  
  PixTestParameters *ptp = new PixTestParameters(configParameters->getDirectory() + "/" + configParameters->getTestParameterFileName()); 

  PixSetup *ap = new PixSetup("DEBUG", ptp, configParameters);  

  cout << "pxar: dumping results into " << rootfile << endl;
  TFile *rfile = TFile::Open(rootfile.c_str(), "RECREATE"); 
  
  PixTestFactory *factory = PixTestFactory::instance(); 
  
  PixTest *pt = factory->createTest(testname, ap); 

  if (!pt->getName().compare("PixelAlive")) {
    pt->setParameter("Ntrig", "10"); 
    pt->doTest();

    pt->setParameter("Ntrig", "20"); 
    pt->doTest();
  }
  
  if (!pt->getName().compare("Ph")) {
    pt->setParameter("Ntrig", "2"); 
    pt->setParameter("DAC", "Vcal"); 
    pt->setParameter("DacVal", "150"); 
    pt->dumpParameters(); 
    pt->doTest();
    
    pt->setParameter("Ntrig", "2"); 
    pt->setParameter("DAC", "Vcal"); 
    pt->setParameter("DacVal", "250"); 
    pt->dumpParameters(); 
    pt->doTest();
  }


  delete pt; 

  rfile->Close();

  ap->killApi();
}
