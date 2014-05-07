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
  pt->setDAC("ctrlreg", 4); 
  pt->setParameter("PHmap", "1"); 
  pt->setParameter("DAC", "Vcal"); 
  pt->setParameter("DACLO", "0"); 
  pt->setParameter("DACHI", "255"); 

  int cycle(0);
  TH1D *h1(0); 
  for (unsigned int io = 0; io < 26; ++io) {
    for (unsigned int is = 0; is < 52; ++is) {
      pt->setDAC("phoffset", io*10);
      pt->setDAC("phscale", is*5);
      pt->doTest(); 
      h1 = (TH1D*)rfile->Get(Form("DacScan/ph_Vcal_c11_r20_C0_V%d", cycle)); 
      h1->SetTitle(Form("ph_Vcal_c11_r20_C0_V%d phscale=%d phoffset=%d", cycle, is*5, io*10));
      ++cycle;
    }
  }
  rfile->Print();
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
  double maxPh(0.), minVcal(60); 
  TH1D *hmax(0), *hmin(0); 
  TCanvas *c0 = new TCanvas("C0"); 
  c0->Clear(); 
  while ((obj = (TObject*)next())) {
    h = (TH1D*)gDirectory->Get(obj->GetName()); 

    if (!h) {
      cout << "problem with " << obj->GetName() << endl;
      continue;
    }

    double integral = h->Integral();
    if (integral < 100.) continue;
    
    double plateau = h->GetMaximum();
    double vcal = h->FindFirstBinAbove(1.); 

    if (plateau > maxPh && vcal < minVcal+10) {
      maxPh = plateau;
      hmax = h; 
    }

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

void Opt_fit(string rootfile = "phOpt.root") {
  TFile *f = TFile::Open(rootfile.c_str()); 

  TH1D *h;
  double x(0.); 
  int OptDistance(255);
  double maxPh(0.); 
  TH1D *hopt(0); 
  TCanvas *c0 = new TCanvas("C0"); 
  c0->Clear();
  unsigned int io, is, is_opt=999;
  int safety_margin=0;
  int bin_min;
  double bestDist=255;
  double dist =255;
  double upEd_dist=255, lowEd_dist=255;
  unsigned int io_opt=999;

  //set midrange value for offset and scan scale, until PH curve is well inside the range
  io=20;
  io_opt=io;
  safety_margin=50;
  for(is=0; is<52; is++){
    h = (TH1D*)gDirectory->Get(Form("DacScan/ph_Vcal_c11_r20_C0_V%d", 52*io+is));
    cout<<h->GetTitle()<<endl;
    h->Draw();
    //erasing first garbage bin
    h->SetBinContent(1,0.);
    bin_min = h->FindFirstBinAbove(1.);
    upEd_dist = abs(h->GetBinContent(h->FindFixBin(255)) - (255 - safety_margin));
    lowEd_dist = abs(h->GetBinContent(bin_min) - safety_margin);
    dist = (upEd_dist > lowEd_dist ) ? (upEd_dist) : (lowEd_dist);
    if(dist < bestDist){
      is_opt = is;
      bestDist=dist;
    }
  }

  if(is_opt==999){
    cout<<"PH optimization failed"<<endl;
    return;
    }

  cout<<"PH Scale value chosen: " << is_opt << "with distance " << bestDist << endl;

  //centring PH curve adjusting ph offset
  bestDist=255;
  io_opt=999;
  for(io=0; io<26; io++){
    h = (TH1D*)gDirectory->Get(Form("DacScan/ph_Vcal_c11_r20_C0_V%d", 52*io+is_opt));
    cout<<h->GetTitle()<<endl;
    //erasing first garbage bin
    h->SetBinContent(1,0.);
    double plateau = h->GetMaximum();
    bin_min = h->FindFirstBinAbove(1.);
    double minPH = h->GetBinContent(bin_min);

    dist = abs(minPH - (255 - plateau));

    cout<<"offset = " << io << ", plateau = " << plateau << ", minPH = " << minPH << ", distance = " << dist << endl;

    if (dist < bestDist){
      io_opt = io;
      bestDist = dist;
    }
  }
  
  //stretching curve to maximally exploit range
  is_opt=999;
  safety_margin=15;
  bestDist=255;
  dist =0;
  upEd_dist=255;
  lowEd_dist=255;
  for(is=0; is<52; is++){
    h = (TH1D*)gDirectory->Get(Form("DacScan/ph_Vcal_c11_r20_C0_V%d", 52*io_opt+is));
    cout<<h->GetTitle()<<endl;
    h->Draw();
    //erasing first garbage bin
    h->SetBinContent(1,0.);
    bin_min = h->FindFirstBinAbove(1.);
    upEd_dist = abs(h->GetBinContent(h->FindFixBin(255)) - (255 - safety_margin));
    lowEd_dist = abs(h->GetBinContent(bin_min) - safety_margin);
    dist = (upEd_dist < lowEd_dist ) ? (upEd_dist) : (lowEd_dist);
    if(dist<bestDist){
      is_opt = is;
      bestDist=dist;
      //break;
    }
  }

  if(is_opt==999){
    cout<<"PH optimization failed (stretching)"<<endl;
    return;
  }


  hopt = (TH1D*)gDirectory->Get(Form("DacScan/ph_Vcal_c11_r20_C0_V%d", 52*io_opt+is_opt));
  cout << "h = " << hopt << " name: " << h->GetName() <<  endl;
  cout << "optimal PH parameters:" << endl << "PH scale = " << is_opt << endl   << "PH offset = " << io_opt << endl << "Best Distance " << bestDist << endl  ;
  hopt->Draw("");
  
}

void riseLengthMap(string rootfile = "phOpt.root") {
  TFile *f = TFile::Open(rootfile.c_str()); 

 
  TH1D *h;
  TH2F *hmap = new TH2F("hmap", "PH rise lenght map", 26, -5., 255, 52, -2.5, 257.5);
  TCanvas *c0 = new TCanvas("C0"); 
  c0->Clear();
  unsigned int io, is;
  int bin_min, bin_max;
  double lenght, plateau, minPH;
  
  for(io=0; io<26; io++){
    for(is=0; is<52; is++){
      h = (TH1D*)gDirectory->Get(Form("DacScan/ph_Vcal_c11_r20_C0_V%d", 52*io+is));
      cout<<h->GetTitle()<<endl;
      h->Draw();
    //erasing first garbage bin
      h->SetBinContent(1,0.);
      bin_min = h->FindFirstBinAbove(1.);
      plateau = h->GetMaximum();
      bin_min = h->FindFirstBinAbove(1.);
      bin_max = h->FindFirstBinAbove(0.9*plateau);
      double minPH = h->GetBinContent(bin_min);

      lenght = sqrt( pow((h->GetBinCenter(bin_max) - h->GetBinCenter(bin_min) ),2) + pow((plateau*0.9 - h->GetBinContent(bin_min) ),2) );
      
      hmap->Fill(io*10, is*5, lenght);
    }
  }
  hmap->GetXaxis()->SetTitle("PH offset [DAC]");
  hmap->GetYaxis()->SetTitle("PH scale [DAC]");
  gStyle->SetPalette(1);
  set_plot_style();
  gPad->SetTickx();
  gPad->SetTicky();
  hmap->Draw("colz");
    
}


void
set_plot_style()
{
  const Int_t NRGBs = 5;
  const Int_t NCont = 255;

  Double_t stops[NRGBs] = { 0.00, 0.34, 0.61, 0.84, 1.00 };
  Double_t red[NRGBs]   = { 0.00, 0.00, 0.87, 1.00, 0.51 };
  Double_t green[NRGBs] = { 0.00, 0.81, 1.00, 0.20, 0.00 };
  Double_t blue[NRGBs]  = { 0.51, 1.00, 0.12, 0.00, 0.00 };
  TColor::CreateGradientColorTable(NRGBs, stops, red, green, blue, NCont);
  gStyle->SetNumberContours(NCont);
}
