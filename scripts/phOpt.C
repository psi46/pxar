// -- Usage:
// ---------
//    ../bin/pXar -c '../scripts/phOpt.C()'

// ----------------------------------------------------------------------
// create PH vs VCal scans for a grid of phscale and phoffset values
void phOpt(string rootfile = "phOpt.root", string cfgdirectory = "testROC") {
  ConfigParameters *configParameters = ConfigParameters::Singleton();
  
  configParameters->setDirectory(cfgdirectory);
  string cfgFile = configParameters->getDirectory() + string("/configParameters.dat");
  configParameters->readConfigParameterFile(cfgFile);

  
  PixTestParameters *ptp = new PixTestParameters(configParameters->getDirectory() + "/" + configParameters->getTestParameterFileName()); 

  PixSetup *ap = new PixSetup("DEBUG", ptp, configParameters);  

  cout << "pxar: dumping results into " << rootfile << endl;
  TFile *rfile = TFile::Open(rootfile.c_str(), "RECREATE"); 
  
  PixTestFactory *factory = PixTestFactory::instance(); 
  
  PixTest *pt = factory->createTest("DacScan", ap); 
  pt->setDAC("ctrlreg", 0); 
  pt->setParameter("PHmap", "1"); 
  pt->setParameter("DAC", "Vcal"); 
  pt->setParameter("DACLO", "0"); 
  pt->setParameter("DACHI", "255"); 

  int cycle(0);
  TH1D *h1(0); 
  for (unsigned int io = 0; io < 52; ++io) {
    for (unsigned int is = 0; is < 52; ++is) {
      pt->setDAC("phoffset", io*5);
      pt->setDAC("phscale", is*5);
      pt->doTest(); 
      h1 = (TH1D*)rfile->Get(Form("DacScan/ph_Vcal_c11_r20_C0_V%d", cycle)); 
      h1->SetTitle(Form("ph_Vcal_c11_r20_C0_V%d phscale=%d phoffset=%d", cycle, is*5, io*5));
      ++cycle;
    }
  }

  delete pt; 

  rfile->Close();

  ap->killApi();

}



// ----------------------------------------------------------------------
void ana(string rootfile = "phOpt.root") {
  TFile *f = TFile::Open(rootfile.c_str()); 

  f->cd("DacScan"); 
  char bla;
  int cnt = 0;
  TH1D *h;

  double x(0.); 
  TIter next(gDirectory->GetListOfKeys(););
  TObject *obj;
  double maxPh(0.), minVcal(999.); 
  TH1D *hmax(0), *hmin(0); 
  TCanvas *c0 = new TCanvas("C0"); 
  while ((obj = (TObject*)next())) {
    cout << obj->GetName() << endl;
    h = (TH1D*)gDirectory->Get(obj->GetName()); 

    if (h->Integral() < 10.) continue;
    
    double plateau = h->GetMaximum();
    if (plateau > maxPh) {
      maxPh = plateau;
      hmax = h; 
    }

    double vcal = h->FindFirstBinAbove(1.); 
    if (vcal > 10 && vcal < minVcal) {
      minVcal = vcal;
      hmin = h; 
    }

    cout << "h = " << h << " obj = " << obj << " name: " << h->GetName() << " integral: " << integral << endl;
    h->Draw("");
    c0->Modified(); 
    c0->Update(); 
  }

  c0->Clear(); 
  c0->Divide(1,2);
  c0->cd(1);
  hmin->Draw();

  c0->cd(2);
  hmax->Draw();
  

}
