// -- Invocation:
// --------------
//    ../bin/pXar -d ../data/defaultParametersRocPSI46digV2 -c '../scripts/singleTest.C("PixelAlive", "pixelalive.root")'

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
  
  
  PixTestAlive *pt = factory->createTest(testname, ap); 
  pt->doTest();
  pt->setParameter("Ntrig", "12");
  pt->doTest();

  delete pt; 
  rfile->Write();
  rfile->Close();

  ap->killApi();
}
