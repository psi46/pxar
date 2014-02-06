void script(string testname = "PixelAlive", string rootfilename = "PixelAlive.root") {
  ConfigParameters *configParameters = ConfigParameters::Singleton();
  configParameters->setDirectory("testROC");
  string cfgFile = configParameters->getDirectory() + string("/configParameters.dat");
  configParameters->readConfigParameterFile(cfgFile);

  string rootfile = configParameters->getDirectory() + "/" + rootfilename;
  
  PixTestParameters *ptp = new PixTestParameters(configParameters->getDirectory() + "/" + configParameters->getTestParameterFileName()); 

  PixSetup *ap = new PixSetup("DEBUG", ptp, configParameters);  

  cout << "pxar: dumping results into " << rootfile << endl;
  TFile *rfile = TFile::Open(rootfile.c_str(), "RECREATE"); 
  
  PixTestFactory *factory = PixTestFactory::instance(); 
  
  
  PixTestAlive *pt = factory->createTest(testname, ap); 
  pt->doTest();

  delete pt; 
  rfile->Write();
  rfile->Close();

  ap->killApi();
}
