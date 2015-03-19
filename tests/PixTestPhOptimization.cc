#include <stdlib.h>  // atof, atoi
#include <algorithm> // std::find
#include <sstream>   // parsing

#include <TStopwatch.h>
#include "constants.h"

#include "PixTestPhOptimization.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestPhOptimization)

PixTestPhOptimization::PixTestPhOptimization() {}

PixTestPhOptimization::PixTestPhOptimization( PixSetup *a, std::string name ) :  PixTest(a, name), fParNtrig(-1), fSafetyMarginLow(20), fQuantMax(0.98), fVcalMax(100) {
  PixTest::init();
  init();
}

bool PixTestPhOptimization::setParameter(string parName, string sval) {
  bool found(false);
  string str1, str2;
  string::size_type s1;
  int pixc, pixr;
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (!fParameters[i].first.compare(parName)) {
      found = true;
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());
      if (!parName.compare("ntrig")) {
	setTestParameter("ntrig", sval); 
	fParNtrig = atoi( sval.c_str() );
	LOG(logDEBUG) << "  setting fParNtrig  ->" << fParNtrig
		      << "<- from sval = " << sval;
      }
      if (!parName.compare("safetymarginlow")) {
	fSafetyMarginLow = atoi( sval.c_str() );
	LOG(logDEBUG) << "  setting fSafetyMarginLow  ->" << fSafetyMarginLow
		      << "<- from sval = " << sval;
      }

      if (!parName.compare("saturationvcal")) {
	fVcalMax = atoi(sval.c_str());
	LOG(logDEBUG) << "  setting fVcalMax  ->" << fVcalMax
		      << "<- from sval = " << sval;
      }
      if (!parName.compare("quantilesaturation")) {
	fQuantMax = atof(sval.c_str());
	LOG(logDEBUG) << "  setting fQuantMax  ->" << fQuantMax
		      << "<- from sval = " << sval;
      }
      
      if (!parName.compare("pix")) {
        s1 = sval.find(",");
        if (string::npos != s1) {
	  str1 = sval.substr(0, s1);
	  pixc = atoi(str1.c_str());
	  str2 = sval.substr(s1+1);
	  pixr = atoi(str2.c_str());
	  fPIX.push_back(make_pair(pixc, pixr));
	  addSelectedPixels(sval); 
	  LOG(logDEBUG) << "  adding to FPIX ->" << pixc << "/" << pixr << " fPIX.size() = " << fPIX.size() ;
	} else {
	  clearSelectedPixels();
	  LOG(logDEBUG) << "  clear fPIX: " << fPIX.size(); 
	}
      }
      break;
    }
  }
  return found;
}

void PixTestPhOptimization::init() {
  fDirectory = gFile->GetDirectory(fName.c_str());
  if(!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str());
  }
  fDirectory->cd();
}

void PixTestPhOptimization::bookHist(string /*name*/) {}

PixTestPhOptimization::~PixTestPhOptimization() {}

void PixTestPhOptimization::doTest() {

  TStopwatch t;

  cacheDacs();
  bigBanner(Form("PixTestPhOptimization::doTest() Ntrig = %d", fParNtrig ));
  fDirectory->cd();
  PixTest::update();

  TH1D *h1(0); 
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
//  LOG(logDEBUG)<<"Enabled ROCs vector has size: "<<rocIds.size();
//  LOG(logDEBUG)<<"ROC "<<(int)rocIds[0]<<" is enabled";
//  for(unsigned int iroc=0; iroc < rocIds.size(); iroc++){
//    LOG(logDEBUG)<<"ROC "<<(int)rocIds[iroc]<<" is enabled";
//  }
  string name, title;

  //looking for inefficient pixels, so that they can be avoided
  std::vector<std::pair<uint8_t, pair<int,int> > > badPixels;
  BlacklistPixels(badPixels, 10);

  //Set from trim file  minimum vcal where PH is sampled for minphpix search
  SetMinThr();

  //flag allows to choose between PhOpt on single pixel (1) or on the whole roc (0)
  pair<int, pxar::pixel> maxpixel;
  pair<int, pxar::pixel> minpixel;
  map<int, pxar::pixel> maxpixels;
  map<int, pxar::pixel> minpixels;
  map<int, int> minVcal;
  

  LOG(logDEBUG)<<"**********Ph range will be optimised on the whole ROC***********";
  //getting highest ph pixel
  GetMaxPhPixel(maxpixels, badPixels);
  //getting min ph pixel and finding its vcal threshold
  GetMinPhPixel(minpixels, minVcal, badPixels);

  
  for(unsigned int roc_it = 0; roc_it < rocIds.size(); roc_it++){
    LOG(logDEBUG)<<"vcal min "<<minVcal[roc_it]<<" on ROC"<<(int)rocIds[roc_it];
  }

  //scan phoffset and phscale for max and min ph pixels
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > dacdac_max;
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > dacdac_min;
  MaxPhVsDacDac(dacdac_max, maxpixels);
  MinPhVsDacDac(dacdac_min, minpixels, minVcal);


  //search for optimal dac values in 3 steps
  //1. shrinking the PH to be completely inside the ADC range, adjusting phscale
  map<uint8_t, int> ps_opt, po_opt;
  for(unsigned int roc_it = 0; roc_it < rocIds.size(); roc_it++){
    po_opt[rocIds[roc_it]] = 120;
  }
  
  //new approach
  std::vector<TH2D* > th2_max;
  std::vector<TH2D* > th2_min;
  std::vector<TH2D* > th2_sol;
  TH2D* h2(0);
  unsigned int NRocs = fApi->_dut->getNRocs();
  for(unsigned int iroc=0; iroc< NRocs; iroc++){
    th2_max.push_back(h2);	   
    th2_min.push_back(h2);	
    th2_sol.push_back(h2);    
  }
    
  getTH2fromMaps(dacdac_max, dacdac_min, th2_max, th2_min);
  
  for(unsigned int ih2 = 0; ih2< NRocs; ih2++){
    fHistList.push_back(th2_max[ih2]);  
    fHistOptions.insert( make_pair(th2_max[ih2], "colz" ) ); 
    fHistList.push_back(th2_min[ih2]);  
    fHistOptions.insert( make_pair(th2_min[ih2], "colz" ) ); 
  }
  
  optimiseOnMapsNew(po_opt, ps_opt, th2_max, th2_min, th2_sol);
  
  LOG(logDEBUG)<<"optimisation done";
  
  //set optimized dacs and save
  restoreDacs();
  for(unsigned int roc_it = 0; roc_it < rocIds.size(); roc_it++){
    fApi->setDAC("phscale",ps_opt[rocIds[roc_it]], rocIds[roc_it] );
    fApi->setDAC("phoffset",po_opt[rocIds[roc_it]], rocIds[roc_it]);
  }
  saveDacs();
  
  cacheDacs(); 
  //draw figures of merit of optimization
  DrawPhMaps(minVcal, badPixels);
  DrawPhCurves(maxpixels, minpixels, po_opt, ps_opt);
  restoreDacs();
  
  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    (*il)->Draw(getHistOption(*il).c_str());
    PixTest::update();
  }
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);
  
  // -- print summary information
  string psString(""), poString(""); 
  for (unsigned int i = 0; i < rocIds.size(); ++i) {
    psString += Form(" %3d", fApi->_dut->getDAC(rocIds[i], "phscale"));
    poString += Form(" %3d", fApi->_dut->getDAC(rocIds[i], "phoffset"));
  }

  int seconds = t.RealTime(); 
  LOG(logINFO) << "PixTestPhOptimization::doTest() done, duration: " << seconds << " seconds";
  LOG(logINFO) << "PH scale (per ROC):  " << psString;
  LOG(logINFO) << "PH offset (per ROC): " << poString;
  
 }

void PixTestPhOptimization::BlacklistPixels(std::vector<std::pair<uint8_t, pair<int, int> > > &badPixels, int aliveTrig){
  //makes a list of inefficient pixels, to be avoided during optimization
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  vector<uint8_t> vVcal = getDacs("vcal"); 
  vector<uint8_t> vCreg = getDacs("ctrlreg"); 

  vector<TH2D*> testEff = efficiencyMaps("PixelAlive", aliveTrig);
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  std::pair<uint8_t, pair<int, int> > badPix;
  Double_t eff=0.;
  for(uint8_t rocid = 0; rocid<rocIds.size(); rocid++){
    for(int r=0; r<80; r++){
      for(int c=0; c<52; c++){
	eff = testEff[rocid]->GetBinContent( testEff[rocid]->FindFixBin((double)c + 0.5, (double)r+0.5) );
	if(eff<aliveTrig){
	  LOG(logDEBUG)<<"Pixel ["<<(int)rocIds[rocid]<<", "<<(int)c<<", "<<(int)r<<"] has eff "<<eff<<"/"<<aliveTrig;
	  badPix.first = rocIds[rocid];
	  badPix.second.first = c;
	  badPix.second.second = r;
	  LOG(logDEBUG)<<"bad Pixel found and blacklisted: ["<<(int)badPix.first<<", "<<(int)badPix.second.first<<", "<<(int)badPix.second.second<<"]";
	  (badPixels).push_back(badPix);
	}
      }
    }
  }
  setDacs("vcal", vVcal); 
  setDacs("ctrlreg", vCreg); 
  LOG(logDEBUG)<<"Number of bad pixels found: "<<badPixels.size();
}


pxar::pixel* PixTestPhOptimization::RandomPixel(std::vector<std::pair<uint8_t, pair<int, int> > > &badPixels, uint8_t iroc){
  //Returns a random pixel, taking care it is not on the blacklist
  fApi->setDAC("ctrlreg",4);
  bool isPixGood=true;
  pxar::pixel *randPixel= new pixel();
  srand(int(time(NULL)));
  int random_col=-1, random_row=-1;
  do{
    random_col = rand() % 52;
    random_row = rand() % 80;
    LOG(logDEBUG)<<"random pixel: ["<<iroc<<", "<<random_col<<", "<<random_row<<"]";
    isPixGood=true;
    for(std::vector<std::pair<uint8_t, pair<int, int> > >::iterator bad_it = badPixels.begin(); bad_it != badPixels.end(); bad_it++){
      if(bad_it->first == iroc && bad_it->second.first == random_col && bad_it->second.second == random_row){
	isPixGood=false;
      }
    }
    LOG(logDEBUG)<<"is the random pixel good? "<<isPixGood;
  }while(!isPixGood);
  randPixel->setRoc(iroc);
  randPixel->setColumn(random_col);
  randPixel->setRow(random_row);
  LOG(logDEBUG)<<"In RandomPixel(), rocId "<<iroc<<", randomCol "<<(int)randPixel->column()<<", randomRow "<<(int)randPixel->row()<<", pixel "<<randPixel;
  return randPixel;
}

void PixTestPhOptimization::GetMaxPhPixel(map<int, pxar::pixel > &maxpixels,   std::vector<std::pair<uint8_t, pair<int,int> > >  &badPixels){
  //looks for the pixel with the highest Ph at vcal = 255, taking care the pixels are not already saturating (ph=255)
  std::string rocType = fPixSetup->getConfigParameters()->getRocType();
  bool isRoc21 = true;
  if(!(rocType > "psi46digv2")){
    isRoc21 = false;
  }
  if(isRoc21){LOG(logDEBUG)<<"ROC type is newer than digv2";}
  LOG(logDEBUG)<<"ROC type is "<<rocType;
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  bool isPixGood=true;
  int maxph = 255;
  fApi->setDAC("phoffset", 200);
  int init_phScale =200;
  int init_phOffset = 150;
  if(!isRoc21){
    init_phScale = 110;
    init_phOffset = 220;
  }
  int flag_maxPh=0;
  pair<int, pxar::pixel> maxpixel;
  maxpixel.second.setValue(0);
  std::vector<pxar::pixel> result;
  std::vector<TH2D*> maxphmap;
  int xbinmax=0, ybinmax=0, zbinmax=0;
  int colmax=0, rowmax=0;
  while((maxph>254 || maxph==0) && flag_maxPh<52){
    fApi->setDAC("phscale", abs(init_phScale)%255);
    fApi->setDAC("phoffset",init_phOffset);  
    fApi->setDAC("vcal",255);
    fApi->setDAC("ctrlreg",4);
    maxphmap = phMaps("maxphmap", 10, 0);
    
    maxph=0;
    //check that the pixel showing highest PH on the module is not reaching 255
    for( unsigned int ith2 = 0; ith2 < maxphmap.size(); ith2++) {
      isPixGood=true;
      maxphmap[ith2]->GetBinXYZ(maxphmap[ith2]->GetMaximumBin(), xbinmax, ybinmax, zbinmax);
      colmax = maxphmap[ith2]->GetXaxis()->GetBinCenter(xbinmax);
      rowmax = maxphmap[ith2]->GetYaxis()->GetBinCenter(ybinmax);
      for(std::vector<std::pair<uint8_t, pair<int, int> > >::iterator bad_it = badPixels.begin(); bad_it != badPixels.end(); bad_it++){
	if(bad_it->second.first == colmax && bad_it->second.second == rowmax && bad_it->first == getIdFromIdx(ith2)){
	  isPixGood=false;
	}
      }
      if(isPixGood && maxphmap[ith2]->GetBinContent(xbinmax, ybinmax) > maxph){
	maxph = maxphmap[ith2]->GetBinContent(xbinmax, ybinmax);
	}
    }
    //should have flag for v2 or v2.1
//    if(!isRoc21){
//      init_phScale-=5;
//    }
//    else{
      init_phScale+=5;
      //    }
     
    flag_maxPh++;
  }
  
  for( unsigned int ith2 = 0; ith2 < maxphmap.size(); ith2++) {
    fHistList.push_back(maxphmap[ith2]);  
    fHistOptions.insert( make_pair(maxphmap[ith2], "colz" ) ); 
  }
  
  std::map<uint8_t, std::pair<uint8_t, uint8_t> > maxpixs;
  pxar::pixel temp_pix;
  Double_t xq[1]={0};  // position where to compute the quantiles in [0,1]
  xq[0]=fQuantMax;
  Double_t yq[1]={0};  // array to contain the quantiles
  // Look for pixel with max. pulse height on every ROC:
  for(unsigned int ith2 = 0; ith2 < maxphmap.size(); ith2++) {
    TH1D* h_quant = distribution(maxphmap[ith2], 256, 0., 255.);
    fHistList.push_back(h_quant);
    h_quant->GetQuantiles(1, yq, xq);
    LOG(logDEBUG)<<"maxph quantile "<<yq[0];
    int colMargin = 3;
    int rowMargin = 5; 
    bool pix_found = false;
    bool badpix = false;
    //first, pixel search excludes edges (col(row)Margin cols (rows) per side)
    for(int ibinx = 1 + colMargin; ibinx < maxphmap[ith2]->GetNbinsX()+1-colMargin; ibinx++){
      if(pix_found) break;
      for(int ibiny = 1 + rowMargin; ibiny < maxphmap[ith2]->GetNbinsY()+1 - rowMargin; ibiny++){
	//try to avoid picking edge pixels
	ibinx = (ibinx)%maxphmap[ith2]->GetNbinsX();
	ibiny = (ibiny)%maxphmap[ith2]->GetNbinsY();
	if( abs( maxphmap[ith2]->GetBinContent(ibinx, ibiny) - yq[0] ) < 1){
	  temp_pix.setRoc( getIdFromIdx(ith2) );
	  temp_pix.setRow( ibiny - 1 );
	  temp_pix.setColumn( ibinx - 1 );
	  for(std::vector<std::pair<uint8_t, pair<int, int> > >::iterator bad_it = badPixels.begin(); bad_it != badPixels.end(); bad_it++){
	    if(bad_it->second.first == ibinx && bad_it->second.second == ibiny && bad_it->first == getIdFromIdx(ith2)){
	      badpix=true;
	      break;
	    }
	  }
	  if(badpix) continue;
	  temp_pix.setValue( maxphmap[ith2]->GetBinContent(ibinx,ibiny) );
	  LOG(logDEBUG)<<"Max pixel is ["<<(int)temp_pix.column()<<" ,"<<(int)temp_pix.row()<<"]"<<" phvalue "<<maxphmap[ith2]->GetBinContent(ibinx, ibiny);
	  maxpixels.insert(make_pair(getIdFromIdx(ith2), temp_pix));
	  pix_found=true;
	  break;
	}
      }
    }
    //if not found, look outside fiducial region
    if(!pix_found){
      badpix=false;
      LOG(logDEBUG)<<"Search for maxph pixel failed in the fiducial region on chip "<< (int)getIdFromIdx(ith2)<<", looking at the edges";
      for(int ibinx_ex = maxphmap[ith2]->GetNbinsX()+1-colMargin; ibinx_ex < maxphmap[ith2]->GetNbinsX()+1+colMargin; ibinx_ex++){
	if(pix_found) break;
	for(int ibiny_ex = maxphmap[ith2]->GetNbinsY()+1 - rowMargin; ibiny_ex < maxphmap[ith2]->GetNbinsY()+1 + rowMargin; ibiny_ex++){
	  //try to avoid picking edge pixels
	  int ibinx = (ibinx_ex)%maxphmap[ith2]->GetNbinsX();
	  int ibiny = (ibiny_ex)%maxphmap[ith2]->GetNbinsY();
	  if( abs( maxphmap[ith2]->GetBinContent(ibinx, ibiny) - yq[0] ) < 1){
	    temp_pix.setRoc( getIdFromIdx(ith2) );
	    temp_pix.setRow( ibiny - 1 );
	    temp_pix.setColumn( ibinx - 1 );
	    for(std::vector<std::pair<uint8_t, pair<int, int> > >::iterator bad_it = badPixels.begin(); bad_it != badPixels.end(); bad_it++){
	      if(bad_it->second.first == ibinx && bad_it->second.second == ibiny && bad_it->first == getIdFromIdx(ith2)){
		badpix=true;
		break;
	      }
	    }
	    if(badpix) continue;
	    temp_pix.setValue( maxphmap[ith2]->GetBinContent(ibinx,ibiny) );
	    LOG(logDEBUG)<<"Max pixel is ["<<(int)temp_pix.column()<<" ,"<<(int)temp_pix.row()<<"]"<<" phvalue "<<maxphmap[ith2]->GetBinContent(ibinx, ibiny);
	    maxpixels.insert(make_pair(getIdFromIdx(ith2), temp_pix));
	    pix_found=true;
	    break;
	  }
	}
      }
    }
    if(!pix_found){
      LOG(logDEBUG)<<"max ph pixel determination failed on roc "<<getIdFromIdx(ith2)<<", setting pixel 0,0";
      temp_pix.setRoc( getIdFromIdx(ith2) ); 
      temp_pix.setRow( 0 );
      temp_pix.setColumn( 0 );
      temp_pix.setValue( -1 );
      maxpixels.insert(make_pair(getIdFromIdx(ith2), temp_pix));   
    }
  }
}

void PixTestPhOptimization::GetMinPhPixel(map<int, pxar::pixel > &minpixels, map<int, int> &minVcal, std::vector<std::pair<uint8_t, pair<int,int> > >  &badPixels){
  //looks for the pixel with the lowest Ph at vcal = 60 low range, taking care the pixels are correclty responding (ph>0)
  //finds the min vcal at which the minphpixel can be sampled
  std::string rocType = fPixSetup->getConfigParameters()->getRocType();
  bool isRoc21 = true;
  if(!(rocType > "psi46digv2")){
    isRoc21 = false;
  }
  if(isRoc21){LOG(logDEBUG)<<"ROC type is newer than digv2";}
  LOG(logDEBUG)<<"ROC type is "<<rocType;
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  bool isPixGood=true;
  int minph = 0;
  int init_phScale = 150;
  int init_phOffset = 175;
  if(!isRoc21){
    init_phScale = 90;
    init_phOffset = 220;
  } 
  int flag_minPh=0;
  pair<int, pxar::pixel> minpixel;
  minpixel.second.setValue(0);
  std::vector<pxar::pixel> result;
  std::vector<TH2D*> minphmap;
  int xbinmin=0, ybinmin=0, zbinmin=0;
  int colmin=0, rowmin=0;
  int minph_roc = -1;
  while(minph<12 && flag_minPh<52){
    LOG(logDEBUG) << "init_phScale=" << init_phScale << ", flag_minPh = " << flag_minPh << ", minph = " << minph;
    fApi->setDAC("phscale", init_phScale%255);
    fApi->setDAC("phoffset",init_phOffset);  
    fApi->setDAC("ctrlreg",0);
    fApi->setDAC("vcal",60);
    minphmap = phMaps("minphmap", 10, 0);
    minph=255;
    LOG(logDEBUG) << "result size "<<result.size()<<endl;
    //check that the pixel showing lowest PH above 0 on the module
    for(unsigned int ith2 = 0; ith2 < minphmap.size(); ith2++) {
      isPixGood=true;
      minphmap[ith2]->GetBinXYZ(minphmap[ith2]->GetMinimumBin(), xbinmin, ybinmin, zbinmin);
      colmin = minphmap[ith2]->GetXaxis()->GetBinCenter(xbinmin);
      rowmin = minphmap[ith2]->GetYaxis()->GetBinCenter(ybinmin);
      for(std::vector<std::pair<uint8_t, pair<int, int> > >::iterator bad_it = badPixels.begin(); bad_it != badPixels.end(); bad_it++){
	if(bad_it->second.first == colmin && bad_it->second.second == rowmin && bad_it->first == getIdFromIdx(ith2)){
	  isPixGood=false;
	}
      }
      if(isPixGood && minphmap[ith2]->GetBinContent(xbinmin, ybinmin) < minph){
	minph = minphmap[ith2]->GetBinContent(xbinmin, ybinmin);
	minph_roc = ith2;
      }
    }
    //should have flag for v2 or v2.1
//    if(!isRoc21){
//      init_phScale-=5;
//    }
//    else{
      init_phScale+=5;
      //    }
    
    flag_minPh++;
  }
  LOG(logDEBUG) << "done. init_phScale=" << init_phScale << ", flag_minPh = " << flag_minPh << ", minph = " << minph << "minph_roc = " << minph_roc;
  
  for(unsigned int ith2 = 0; ith2 < minphmap.size(); ith2++) {
    fHistList.push_back(minphmap[ith2]);  
    fHistOptions.insert(make_pair(minphmap[ith2], "colz")  ); 
  }
  // Look for pixel with min pulse height on every ROC:
  std::map<uint8_t, std::pair<uint8_t, uint8_t> > minpixs;
  pxar::pixel temp_pix;
  Double_t xq[1];  // position where to compute the quantiles in [0,1]
  xq[0]=0.02;
  Double_t yq[1];  // array to contain the quantiles
  // Look for pixel with min. pulse height on every ROC:
  for(unsigned int ith2 = 0; ith2 < minphmap.size(); ith2++) {
    TH1D* h_quant = distribution(minphmap[ith2], 256, 0., 255.);
    fHistList.push_back(h_quant);
    h_quant->GetQuantiles(1, yq, xq);
    //    yq[0] = h_quant->FindFirstBinAbove(1.) + 1;
    LOG(logDEBUG)<<"minph quantile "<<xq[0]<<" "<<yq[0];
    int colMargin = 3;
    int rowMargin = 5; 
    bool pix_found = false;
    bool badpix = false;
    //first, pixel search excludes edges (col(row)Margin cols (rows) per side)
    for(int ibinx = 1 + colMargin; ibinx < minphmap[ith2]->GetNbinsX()+1-colMargin; ibinx++){
      if(pix_found) break;
      for(int ibiny = 1 + rowMargin; ibiny < minphmap[ith2]->GetNbinsY()+1 - rowMargin; ibiny++){
	//try to avoid picking edge pixels
	ibinx = (ibinx)%minphmap[ith2]->GetNbinsX();
	ibiny = (ibiny)%minphmap[ith2]->GetNbinsY();	  
	if( abs( minphmap[ith2]->GetBinContent(ibinx, ibiny) - yq[0] ) <= 1){
	  temp_pix.setRoc( getIdFromIdx(ith2) );
	  temp_pix.setRow( ibiny - 1 );
	  temp_pix.setColumn( ibinx - 1 );
	  for(std::vector<std::pair<uint8_t, pair<int, int> > >::iterator bad_it = badPixels.begin(); bad_it != badPixels.end(); bad_it++){
	    if(bad_it->second.first == ibinx && bad_it->second.second == ibiny && bad_it->first == getIdFromIdx(ith2)){
	      badpix=true;
	      break;
	    }
	  }
	  if(badpix) continue;
	  LOG(logDEBUG)<<"Min pixel is ["<<(int)temp_pix.column()<<" ,"<<(int)temp_pix.row()<<"]"<<" phvalue "<<minphmap[ith2]->GetBinContent(ibinx, ibiny);
	  temp_pix.setValue( minphmap[ith2]->GetBinContent(ibinx,ibiny) );
	  minpixels.insert(make_pair(getIdFromIdx(ith2), temp_pix));
	  pix_found = true;
	  break;
	}
      }
    }
    
    //if not found, look outside fiducial region
    if(!pix_found){
      badpix = false;
      LOG(logDEBUG)<<"Search for minph pixel failed in the fiducial region for chip "<< (int)getIdFromIdx(ith2)<<", looking at the edges";
      for(int ibinx_ex = minphmap[ith2]->GetNbinsX()+1-colMargin; ibinx_ex < minphmap[ith2]->GetNbinsX()+1+colMargin; ibinx_ex++){
	if(pix_found) break;
	for(int ibiny_ex = minphmap[ith2]->GetNbinsY()+1 - rowMargin; ibiny_ex < minphmap[ith2]->GetNbinsY()+1 + rowMargin; ibiny_ex++){
	  //ibinx_ex, ibiny_ex extend beyond the range so wrap around 
	  int ibinx = (ibinx_ex)%minphmap[ith2]->GetNbinsX();
	  int ibiny = (ibiny_ex)%minphmap[ith2]->GetNbinsY();
	  if( abs( minphmap[ith2]->GetBinContent(ibinx, ibiny) - yq[0] ) <= 1){
	    temp_pix.setRoc( getIdFromIdx(ith2) );
	    temp_pix.setRow( ibiny - 1 );
	    temp_pix.setColumn( ibinx - 1 );
	    for(std::vector<std::pair<uint8_t, pair<int, int> > >::iterator bad_it = badPixels.begin(); bad_it != badPixels.end(); bad_it++){
	      if(bad_it->second.first == ibinx && bad_it->second.second == ibiny && bad_it->first == getIdFromIdx(ith2)){
		badpix=true;
		break;
	      }
	    }
	    if(badpix) continue;
	    LOG(logDEBUG)<<"Min pixel is ["<<(int)temp_pix.column()<<" ,"<<(int)temp_pix.row()<<"]"<<" phvalue "<<minphmap[ith2]->GetBinContent(ibinx, ibiny);
	    temp_pix.setValue( minphmap[ith2]->GetBinContent(ibinx,ibiny) );
	    minpixels.insert(make_pair(getIdFromIdx(ith2), temp_pix));
	    pix_found = true;
	    break;
	  }
	}
      }
    }
    if(!pix_found){
      LOG(logDEBUG)<<"min ph pixel determination failed on roc "<<getIdFromIdx(ith2)<<", setting pixel 0,0";
      temp_pix.setRoc( getIdFromIdx(ith2) ); 
      temp_pix.setRow( 0 );
      temp_pix.setColumn( 0 );
      temp_pix.setValue( -1 );
      minpixels.insert(make_pair(getIdFromIdx(ith2), temp_pix));   
    }
  }
  //finds min vcal
  fApi->setDAC("ctrlreg",0);
  int cnt(0); 
  bool done(false);
  TH1D *h1(0); 
  vector<pair<uint8_t, vector<pixel> > > results;
  pair<int, int> vcalmin;
  int vcalthr = 0;
  h1 = bookTH1D("h1", "h1", 256, 0., 256.);
  unsigned int NRocs = rocIds.size();
  for(unsigned int roc_it = 0; roc_it < NRocs; roc_it++){
    for(unsigned int roc_kt = 0; roc_kt < NRocs; roc_kt++){
      fApi->_dut->setROCEnable(roc_kt, true);
    }
    fApi->_dut->testAllPixels(false);
    fApi->_dut->maskAllPixels(true);
    fApi->_dut->testPixel(minpixels[rocIds[roc_it]].column(), minpixels[rocIds[roc_it]].row(), true);
    fApi->_dut->maskPixel(minpixels[rocIds[roc_it]].column(), minpixels[rocIds[roc_it]].row(), false);
    LOG(logDEBUG)<<"enabling pixels "<<(int)minpixels[roc_it].column()<<", "<<(int)minpixels[roc_it].row()<<", "<<(int)minpixels[roc_it].roc()<<" "<<(int)roc_it;
    for(unsigned int roc_jt = 0; roc_jt < NRocs; roc_jt++){
      if(roc_jt!= roc_it){
	fApi->_dut->setROCEnable(roc_jt, false);
      }
    }
    fApi->_dut->info();
    cnt = 0; 
    done = false;
    while (!done) {
      try {
	results = fApi->getPulseheightVsDAC("vcal", 0, 255, FLAG_FORCE_MASKED, 10);
	done = true;
      } catch(pxarException &e) {
	LOG(logCRITICAL) << "pXar execption: "<< e.what(); 
	++cnt;
      }
      done = (cnt>5) || done;
    }
    
    LOG(logDEBUG)<<"size of results "<<results.size();
    for (unsigned int i = 0; i < results.size(); ++i) {
      pair<uint8_t, vector<pixel> > v = results[i];
      int idac = v.first; 
      vector<pixel> vpix = v.second;
      for (unsigned int ipix = 0; ipix < vpix.size(); ++ipix) {
	h1->Fill(idac, vpix[ipix].value());
      }
    }
    vcalthr = static_cast<int>( h1->GetBinCenter( h1->FindFirstBinAbove(10.) ) );
    vcalmin = make_pair(roc_it, vcalthr);
    minVcal.insert(vcalmin);
    h1->Reset();
  }
  for(unsigned int roc_kt = 0; roc_kt < NRocs; roc_kt++){
    fApi->_dut->setROCEnable(roc_kt, false);
  }
  for(unsigned int roc_kt = 0; roc_kt < NRocs; roc_kt++){
    fApi->_dut->setROCEnable(roc_kt, true);
  }
}


void PixTestPhOptimization::DrawPhMaps(std::map<int, int> &minVcal, std::vector<std::pair<uint8_t, std::pair<int, int> > > &badPixels){
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  string name, title;
  //draw PH maps and extract validation distributions from them
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  
  std::vector<pxar::pixel> result_map;
  map<int, TH2D* > h2_PhMaps;
  map<int, TH1D* > h1_PhMaps;
  TH2D* h2_PhMap(0);
  TH1D* h1_PhMap(0);
  fApi->setDAC("ctrlreg",4);
  fApi->setDAC("vcal",fVcalMax);
  //pulseheight map at vcal=100
  result_map = fApi->getPulseheightMap(0,10);
  //unpacking data from map and filling one histo per ROC
  for(unsigned int roc_it = 0; roc_it < rocIds.size(); roc_it++){
    name  = Form("PH_mapHiVcal_C%d", rocIds[roc_it]);
    title = Form("PH_mapHiVcal_C%d", rocIds[roc_it]);
    h2_PhMap = bookTH2D(name, name, 52, 0., 52., 80, 0., 80.);
    h2_PhMaps.insert(make_pair(rocIds[roc_it], h2_PhMap));
    fHistList.push_back(h2_PhMaps[rocIds[roc_it]]);  
    fHistOptions.insert( make_pair(h2_PhMaps[rocIds[roc_it]], "colz")  ); 
  }
  for (unsigned int i = 0; i < result_map.size(); ++i) {
    h2_PhMaps[ (int) result_map[i].roc() ]->Fill( result_map[i].column(), result_map[i].row(), result_map[i].value());
  }
  
  //PH map for lower vcal sampling point
  for(unsigned int roc_it = 0; roc_it < rocIds.size(); roc_it++){
    fApi->setDAC("ctrlreg",0);
    fApi->setDAC("vcal",minVcal[roc_it]+10, rocIds[roc_it] );
  }
  map<int, TH2D* > h2_PhMapsMin;
  map<int, TH1D* > h1_PhMapsMin;
  //phmap
  result_map = fApi->getPulseheightMap(0,10);
  //unpacking data from map and filling one histo per ROC
  for(unsigned int roc_it = 0; roc_it < rocIds.size(); roc_it++){
    name  = Form("PH_mapLowVcal_C%d", rocIds[roc_it]);
    title = Form("PH_mapLowVcal_C%d", rocIds[roc_it]);
    h2_PhMap = bookTH2D(name, name, 52, 0., 52., 80, 0., 80.);
    h2_PhMapsMin.insert(make_pair(rocIds[roc_it], h2_PhMap));
    fHistList.push_back(h2_PhMapsMin[rocIds[roc_it]]);  
    fHistOptions.insert( make_pair(h2_PhMapsMin[rocIds[roc_it]], "colz")  ); 
  }
  for (unsigned int i = 0; i < result_map.size(); ++i) {
    h2_PhMapsMin[ (int) result_map[i].roc() ]->Fill( result_map[i].column(), result_map[i].row(), result_map[i].value());
  }
  
  
  //PH map for lower vcal sampling point
  for(unsigned int roc_it = 0; roc_it < rocIds.size(); roc_it++){
    fApi->setDAC("ctrlreg",0);
    fApi->setDAC("vcal",50, rocIds[roc_it] );
  }
  map<int, TH2D* > h2_PhMaps50;
  map<int, TH1D* > h1_PhMaps50;
  //phmap
  result_map = fApi->getPulseheightMap(0,10);
  //unpacking data from map and filling one histo per ROC
  for(unsigned int roc_it = 0; roc_it < rocIds.size(); roc_it++){
    name  = Form("PH_mapVcal50_C%d", rocIds[roc_it]);
    title = Form("PH_mapVcal50_C%d", rocIds[roc_it]);
    h2_PhMap = bookTH2D(name, name, 52, 0., 52., 80, 0., 80.);
    h2_PhMaps50.insert(make_pair(rocIds[roc_it], h2_PhMap));
    fHistList.push_back(h2_PhMaps50[rocIds[roc_it]]);  
    fHistOptions.insert( make_pair(h2_PhMaps50[rocIds[roc_it]], "colz")  ); 
  }
  for (unsigned int i = 0; i < result_map.size(); ++i) {
    h2_PhMaps50[ (int) result_map[i].roc() ]->Fill( result_map[i].column(), result_map[i].row(), result_map[i].value());
  }
  
  //removing blacklisted pixels (bincontent = 0) from histos
  for(std::vector<std::pair<uint8_t, pair<int, int> > >::iterator bad_it = badPixels.begin(); bad_it != badPixels.end(); bad_it++){
    h2_PhMaps[ bad_it->first] ->SetBinContent( h2_PhMaps[ bad_it->first] ->FindFixBin(bad_it->second.first, bad_it->second.second), 0);
    h2_PhMapsMin[ bad_it->first] ->SetBinContent( h2_PhMapsMin[ bad_it->first] ->FindFixBin(bad_it->second.first, bad_it->second.second), 0);
    h2_PhMaps50[ bad_it->first] ->SetBinContent( h2_PhMaps50[ bad_it->first] ->FindFixBin(bad_it->second.first, bad_it->second.second), 0);
  }
  
  //adjust z axis range and extract ph distribution
  for(unsigned int roc_it = 0; roc_it < rocIds.size(); roc_it++){
    h2_PhMaps[ (int) rocIds[roc_it] ]->GetZaxis()->SetRangeUser(h2_PhMaps[ (int) rocIds[roc_it] ]->GetMinimum(), 255. );
    h1_PhMap = distribution( h2_PhMaps[ (int) rocIds[roc_it] ], 255, 0., 255.);
    h1_PhMaps.insert( make_pair(rocIds[roc_it], h1_PhMap) );
    fHistList.push_back(h1_PhMaps[rocIds[roc_it]]);  
  }
  
  //adjust z axis range and extract ph distribution
  for(unsigned int roc_it = 0; roc_it < rocIds.size(); roc_it++){
    h2_PhMapsMin[ (int) rocIds[roc_it] ]->GetZaxis()->SetRangeUser(0., h2_PhMapsMin[ (int) rocIds[roc_it] ]->GetMaximum() );
    h1_PhMap = distribution( h2_PhMapsMin[ (int) rocIds[roc_it] ], 255, 0., 255.);
    h1_PhMapsMin.insert( make_pair(rocIds[roc_it], h1_PhMap) );
    fHistList.push_back(h1_PhMapsMin[rocIds[roc_it]]);  
  }
  
  //adjust z axis range and extract ph distribution
  for(unsigned int roc_it = 0; roc_it < rocIds.size(); roc_it++){
    h2_PhMaps50[ (int) rocIds[roc_it] ]->GetZaxis()->SetRangeUser(0., h2_PhMaps50[ (int) rocIds[roc_it] ]->GetMaximum() );
    h1_PhMap = distribution( h2_PhMaps50[ (int) rocIds[roc_it] ], 255, 0., 255.);
    h1_PhMaps50.insert( make_pair(rocIds[roc_it], h1_PhMap) );
    fHistList.push_back(h1_PhMaps50[rocIds[roc_it]]);  
  }
  
}

void PixTestPhOptimization::DrawPhCurves(map<int, pxar::pixel > &maxpixels, map<int, pxar::pixel > &minpixels, std::map<uint8_t, int> &po_opt, std::map<uint8_t, int> &ps_opt){
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  string name, title;
  TH1D *h1(0); 
  for(unsigned int roc_it = 0; roc_it < rocIds.size(); roc_it++){
    fApi->setDAC("ctrlreg",4);
  }
  vector<pair<uint8_t, vector<pixel> > > results;
  for(unsigned int roc_it = 0; roc_it < rocIds.size(); roc_it++){
    //draw PH curve for max and min pixel on every ROC
    results.clear();
    name  = Form("PH_c%d_r%d_C%d", maxpixels[roc_it].column(), maxpixels[roc_it].row(), rocIds[roc_it]);
    title = Form("PH_c%d_r%d_C%d, phscale = %d, phoffset = %d, maxpixel", maxpixels[roc_it].column(), maxpixels[roc_it].row(), rocIds[roc_it], ps_opt[rocIds[roc_it]], po_opt[rocIds[roc_it]]);
    h1 = bookTH1D(name, name, 256, 0., 256.);
    fApi->_dut->testAllPixels(false);
    fApi->_dut->maskAllPixels(true);
    fApi->_dut->testPixel(maxpixels[roc_it].column(), maxpixels[roc_it].row(), true, roc_it);
    fApi->_dut->maskPixel(maxpixels[roc_it].column(), maxpixels[roc_it].row(), false, roc_it);
    int  cnt = 0; 
    bool done = false;
    while (!done) {
      try {
	results = fApi->getPulseheightVsDAC("vcal", 0, 255, FLAG_FORCE_MASKED, 10);
	done = true;
      } catch(pxarException &e) {
	LOG(logCRITICAL) << "pXar execption: "<< e.what(); 
	++cnt;
      }
      done = (cnt>5) || done;
    }
    
    for (unsigned int i = 0; i < results.size(); ++i) {
      pair<uint8_t, vector<pixel> > v = results[i];
      int idac = v.first; 
      vector<pixel> vpix = v.second;
      for (unsigned int ipix = 0; ipix < vpix.size(); ++ipix) {
	h1->Fill(idac, vpix[ipix].value());
      }
    }
    h1->SetMinimum(0);
    setTitles(h1, title.c_str(), "average PH");
    fHistList.push_back(h1);  
    
    results.clear();
    
    name  = Form("PH_c%d_r%d_C%d", minpixels[roc_it].column(), minpixels[roc_it].row(), rocIds[roc_it]);
    title = Form("PH_c%d_r%d_C%d, phscale = %d, phoffset = %d, minpixel", minpixels[roc_it].column(), minpixels[roc_it].row(), rocIds[roc_it], ps_opt[rocIds[roc_it]], po_opt[rocIds[roc_it]]);
    h1 = bookTH1D(name, name, 256, 0., 256.);
    fApi->_dut->testAllPixels(false);
    fApi->_dut->maskAllPixels(true);
    fApi->_dut->testPixel(minpixels[roc_it].column(), minpixels[roc_it].row(), true, roc_it);
    fApi->_dut->maskPixel(minpixels[roc_it].column(), minpixels[roc_it].row(), false, roc_it);
    cnt = 0; 
    done = false;
    while (!done) {
      try {
	results = fApi->getPulseheightVsDAC("vcal", 0, 255, FLAG_FORCE_MASKED, 10);
	done = true;
      } catch(pxarException &e) {
	LOG(logCRITICAL) << "pXar execption: "<< e.what(); 
	++cnt;
      }
      done = (cnt>5) || done;
    }
    
    for (unsigned int i = 0; i < results.size(); ++i) {
      pair<uint8_t, vector<pixel> > v = results[i];
      int idac = v.first; 
      vector<pixel> vpix = v.second;
      for (unsigned int ipix = 0; ipix < vpix.size(); ++ipix) {
	h1->Fill(idac, vpix[ipix].value());
      }
    }
    h1->SetMinimum(0);
    setTitles(h1, title.c_str(), "average PH");
    fHistList.push_back(h1);
  }
}
void PixTestPhOptimization::MaxPhVsDacDac(std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_max, map<int, pxar::pixel> maxpixels){
  
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  for(std::map<int, pxar::pixel>::iterator maxp_it = maxpixels.begin(); maxp_it != maxpixels.end(); maxp_it++){
    fApi->_dut->testPixel(maxp_it->second.column(),maxp_it->second.row(),true, getIdxFromId(maxp_it->second.roc()));
    fApi->_dut->maskPixel(maxp_it->second.column(),maxp_it->second.row(),false, getIdxFromId(maxp_it->second.roc()));
  } 
  //sample pulseheight at 35k e-
  fApi->setDAC("vcal",fVcalMax);
  fApi->setDAC("ctrlreg",4);
  
  //scanning through offset and scale for max pixel (or randpixel)
  int cnt = 0; 
  bool done = false;
  while (!done) {
    try {
      dacdac_max = fApi->getPulseheightVsDACDAC("phoffset",0,255,"phscale",0,255,0,10);
      done = true;
    } catch(pxarException &e) {
      LOG(logCRITICAL) << "pXar execption: "<< e.what(); 
      ++cnt;
    }
    done = (cnt>5) || done;
  }  
}

void PixTestPhOptimization::MinPhVsDacDac(std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_min, map<int, pxar::pixel> minpixels, std::map<int, int> &minVcal){

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  for(std::map<int, pxar::pixel>::iterator minp_it = minpixels.begin(); minp_it != minpixels.end(); minp_it++){
    fApi->_dut->testPixel(minp_it->second.column(),minp_it->second.row(),true, getIdxFromId(minp_it->second.roc()));
    fApi->_dut->maskPixel(minp_it->second.column(),minp_it->second.row(),false, getIdxFromId(minp_it->second.roc()));
  }

  fApi->setDAC("ctrlreg",0);
  for(std::map<int, int>::iterator ivcal = minVcal.begin(); ivcal != minVcal.end(); ivcal++){
    fApi->setDAC("vcal",minVcal[ivcal->first]+10, getIdxFromId(ivcal->first));
  }
    
  //scanning through offset and scale for min pixel (or same randpixel)
  int cnt = 0; 
  bool done = false;
  while (!done) {
    try {
      dacdac_min = fApi->getPulseheightVsDACDAC("phoffset",0,255,"phscale",0,255,0,10);
      done = true;
    } catch(pxarException &e) {
      LOG(logCRITICAL) << "pXar execption: "<< e.what(); 
      ++cnt;
    }
    done = (cnt>5) || done;
  }
}

void PixTestPhOptimization::SetMinThr(){
  fMinThr=0;
  string trimfile = fPixSetup->getConfigParameters()->getTrimParameterFileName() + fPixSetup->getConfigParameters()->getTrimVcalSufix();
  trimfile.erase(0,14); //erases 'trimParamerers' from the name of the file
  if(0!=(trimfile.compare(""))){
    fMinThr = atoi(trimfile.c_str());
  }
  else{
    LOG(logINFO)<<"***::: The test requires a TRIMMED module, but no TrimParameterFile is loaded :::***";
    LOG(logINFO)<<"Vcal lower sample point will be set to 40";
    fMinThr=40;
  }
}

void PixTestPhOptimization::getTH2fromMaps(std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_max,   std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_min, std::vector<TH2D* > &th2_max, std::vector<TH2D* > &th2_min){

  TH2D* h2 = new TH2D("h2", "h2", 256, 0., 255., 256, 0., 255.);
  for(unsigned int i=0; i< fApi->_dut->getNEnabledRocs() ; i++){
    th2_max[i] = (TH2D*)h2->Clone(Form("maxphvsdacdac_th2_C%d", getIdFromIdx(i)));
    th2_max[i]->SetTitle(Form("max PH phscaleVSphoffset, C%d",  getIdFromIdx(i)));
    th2_max[i]->GetXaxis()->SetTitle("phscale");
    th2_max[i]->GetYaxis()->SetTitle("phoffset");
    th2_min[i] = (TH2D*)h2->Clone(Form("minphvsdacdac_th2_C%d", getIdFromIdx(i)));
    th2_min[i]->SetTitle(Form("min PH phscaleVSphoffset, C%d",  getIdFromIdx(i)));
    th2_min[i]->GetXaxis()->SetTitle("phscale");
    th2_min[i]->GetYaxis()->SetTitle("phoffset");
  }
  
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit_max = dacdac_max.begin();
  for(dacit_max = dacdac_max.begin(); dacit_max != dacdac_max.end(); dacit_max++){
    for(unsigned int ipix = 0; ipix< dacit_max->second.second.size(); ipix++){
      th2_max[getIdxFromId(dacit_max->second.second[ipix].roc())]->Fill(dacit_max->second.first, dacit_max->first, dacit_max->second.second[ipix].value());
    }
  } 
  
  
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit_min = dacdac_min.begin();
  for(dacit_min = dacdac_min.begin(); dacit_min != dacdac_min.end(); dacit_min++){
    for(unsigned int ipix = 0; ipix< dacit_min->second.second.size(); ipix++){
      th2_min[getIdxFromId(dacit_min->second.second[ipix].roc())]->Fill(dacit_min->second.first, dacit_min->first, dacit_min->second.second[ipix].value());
    }
  }   
}


void PixTestPhOptimization::optimiseOnMapsNew(std::map<uint8_t, int> &po_opt, std::map<uint8_t, int> &ps_opt,  std::vector<TH2D* > &th2_max, std::vector<TH2D* > &th2_min, std::vector<TH2D* > &th2_sol){
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  TH2D* hmin(0);
  TH2D* hmax(0);
  TH2D* hsol = new TH2D("hsol", "hsol", 256, 0., 255., 256, 0., 255.);
  for(unsigned int i=0; i< fApi->_dut->getNEnabledRocs() ; i++){
    LOG(logDEBUG)<<"before assigning th2_sol to vector component";
    th2_sol[i] = (TH2D*)hsol->Clone(Form("solphvsdacdac_th2_C%d", getIdFromIdx(i)));
    th2_sol[i]->SetTitle(Form("Solution phscaleVSphoffset C%d", getIdFromIdx(i)));
    th2_sol[i]->GetXaxis()->SetTitle("phscale");
    th2_sol[i]->GetYaxis()->SetTitle("phoffset");
    fHistList.push_back(th2_sol[i]);
    fHistOptions.insert( make_pair(th2_sol[i], "colz" ) ); 
    LOG(logDEBUG)<<"after assigning th2_sol to vector component, chip"<<getIdFromIdx(i);
  }
  int goodbinx = 0, goodbiny = 0, goodbinz=0;
  double maxsol=0.;
  double maxPH=0.;
  
  for(unsigned int iroc = 0; iroc < rocIds.size(); iroc++){
    maxsol=0.;
    for(int safetyCorrection = 0; maxsol<1. && safetyCorrection+fSafetyMarginLow<256; safetyCorrection++){
      LOG(logINFO)<<"safety margin for low PH: adding "<<safetyCorrection<<", margin is now "<<safetyCorrection+fSafetyMarginLow;
      // hsol->Reset("M");
      hmin = (TH2D*)th2_min[rocIds[iroc]]->Clone();
      hmax = (TH2D*)th2_max[rocIds[iroc]]->Clone();
      maxPH = hmax->GetMaximum();
      
      for(int ibinx=1; ibinx < hmax->GetNbinsX()+1; ibinx++){
	for(int ibiny=1; ibiny < hmax->GetNbinsY()+1; ibiny++){
	  if(abs(hmax->GetBinContent(ibinx, ibiny) - maxPH + 2) > 1 ){
	    hmax->SetBinContent(ibinx, ibiny, 0);
	  }
	}
      }
      
      for(int ibinx=1; ibinx < hmin->GetNbinsX()+1; ibinx++){
	for(int ibiny=1; ibiny < hmin->GetNbinsY()+1; ibiny++){
	  if(abs(hmin->GetBinContent(ibinx, ibiny) - (fSafetyMarginLow+safetyCorrection)) > 1 ){
	    hmin->SetBinContent(ibinx, ibiny, 0);
	  }
	}
      }
      
      for(int ibinx=1; ibinx < hmin->GetNbinsX()+1; ibinx++){
	for(int ibiny=1; ibiny < hmin->GetNbinsY()+1; ibiny++){
	  if(hmin->GetBinContent(ibinx, ibiny) != 0 && hmax->GetBinContent(ibinx, ibiny) != 0){
	    th2_sol[rocIds[iroc]]->SetBinContent(ibinx, ibiny, 1);
	  }
	}
      }
      maxsol = th2_sol[rocIds[iroc]]->GetMaximum();
      th2_sol[rocIds[iroc]]->GetBinXYZ(th2_sol[rocIds[iroc]]->GetMaximumBin(), goodbinx, goodbiny, goodbinz);
      
    }
    //po_opt[rocIds[iroc]] = (int)th2_sol[rocIds[iroc]]->GetXaxis()->GetBinCenter(goodbinx);
    //ps_opt[rocIds[iroc]] = (int)th2_sol[rocIds[iroc]]->GetYaxis()->GetBinCenter(goodbiny);
    po_opt[rocIds[iroc]] = goodbiny - 1;
    ps_opt[rocIds[iroc]] = goodbinx - 1;
  }

  
  delete hmin;
  delete hmax;
  delete hsol;

}
