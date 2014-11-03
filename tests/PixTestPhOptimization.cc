#include <stdlib.h>  // atof, atoi
#include <algorithm> // std::find
#include <sstream>   // parsing

#include "PixTestPhOptimization.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestPhOptimization)

PixTestPhOptimization::PixTestPhOptimization() {}

PixTestPhOptimization::PixTestPhOptimization( PixSetup *a, std::string name ) :  PixTest(a, name), fParNtrig(-1), fParDAC("nada"), fParDacVal(100),   fFlagSinglePix(true), fSafetyMarginUp(10), fSafetyMarginLow(30) {
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
      if (!parName.compare("safetymarginup")) {
	fSafetyMarginUp = atoi( sval.c_str() );
	LOG(logDEBUG) << "  setting fSafetyMarginUp  ->" << fSafetyMarginUp
		      << "<- from sval = " << sval;
      }
      if (!parName.compare("safetymarginlow")) {
	fSafetyMarginLow = atoi( sval.c_str() );
	LOG(logDEBUG) << "  setting fSafetyMarginLow  ->" << fSafetyMarginLow
		      << "<- from sval = " << sval;
      }
      if (!parName.compare("singlepix")) {
	fFlagSinglePix = atoi( sval.c_str() );
	LOG(logDEBUG) << "  setting fFlagSinglePix  ->" << fFlagSinglePix
		      << "<- from sval = " << sval;
      }
      if (!parName.compare("dac")) {
	setTestParameter("dac", sval); 
	fParDAC = sval;
	LOG(logDEBUG) << "  setting fParDAC  ->" << fParDAC
		      << "<- from sval = " << sval;
      }

      if (!parName.compare("dacval")) {
	setTestParameter("dacval", sval); 
	fParDacVal = atoi(sval.c_str());
	LOG(logDEBUG) << "  setting fParDacVal  ->" << fParDacVal
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

  cacheDacs();
  bigBanner(Form("PixTestPhOptimization::doTest() Ntrig = %d, singlePix = %d", fParNtrig, (fFlagSinglePix?1:0)));
  fDirectory->cd();
  PixTest::update();

  TH1D *h1(0); 
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  LOG(logDEBUG)<<"Enabled ROCs vector has size: "<<rocIds.size();
  LOG(logDEBUG)<<"ROC "<<(int)rocIds[0]<<" is enabled";
  for(unsigned int iroc=0; iroc < rocIds.size(); iroc++){
    LOG(logDEBUG)<<"ROC "<<(int)rocIds[iroc]<<" is enabled";
  }
  map<string, TH1D*> hists; 
  string name, title;

  //looking for inefficient pixels, so that they can be avoided
  std::vector<std::pair<uint8_t, pair<int,int> > > badPixels;
  BlacklistPixels(badPixels, 10);

  //vcal threshold map in order to choose the low-vcal value the PH will be sampled at
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  fApi->setDAC("ctrlreg",4);
  //  std::vector<pxar::pixel> thrmap;
  int cnt(0); 
  bool done(false);

  int minthr=40;

  //flag allows to choose between PhOpt on single pixel (1) or on the whole roc (0)
  pair<int, pxar::pixel> maxpixel;
  pair<int, pxar::pixel> minpixel;
  map<int, pxar::pixel> maxpixels;
  map<int, pxar::pixel> minpixels;

  
  if(fFlagSinglePix){
    for (uint iroc = 0; iroc < rocIds.size(); ++iroc){
      LOG(logDEBUG)<<"**********Ph range will be optimised on a single random pixel***********";
      pxar::pixel randomPix;
      randomPix= *(RandomPixel(badPixels, rocIds[iroc]));
      LOG(logDEBUG)<<"In doTest(), randomCol "<<(int)randomPix.column()<<", randomRow "<<(int)randomPix.row()<<", pixel "<<randomPix;
      maxpixel.first = rocIds[iroc]; 
      maxpixel.second.setRoc(rocIds[iroc]);
      maxpixel.second.setColumn(randomPix.column());
      maxpixel.second.setRow(randomPix.row());
      minpixel.first = rocIds[iroc]; 
      minpixel.second.setRoc(rocIds[iroc]);
      minpixel.second.setColumn(randomPix.column());
      minpixel.second.setRow(randomPix.row());
      LOG(logDEBUG)<<"random pixel: "<<maxpixel.second<<", "<<minpixel.second<<"is not on the blacklist";
      maxpixels.insert(maxpixel);
      minpixels.insert(minpixel);
      //retrieving info from the vcal thr map for THIS random pixel
    }
  }
  else{
    LOG(logDEBUG)<<"**********Ph range will be optimised on the whole ROC***********";
    //getting highest ph pixel
    GetMaxPhPixel(maxpixels, badPixels);
    // getting pixel showing the largest vcal threshold (i.e., all other pixels are responding)
    GetMinPhPixel(minpixels, badPixels);
  }

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  for(std::map<int, pxar::pixel>::iterator maxp_it = maxpixels.begin(); maxp_it != maxpixels.end(); maxp_it++){
    fApi->_dut->testPixel(maxp_it->second.column(),maxp_it->second.row(),true, maxp_it->first);
    fApi->_dut->maskPixel(maxp_it->second.column(),maxp_it->second.row(),false, maxp_it->first);
  } 
  fApi->setDAC("vcal",255);
  fApi->setDAC("ctrlreg",4);
  //scanning through offset and scale for max pixel (or randpixel)
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > dacdac_max;
 
  cnt = 0; 
  done = false;
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

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  for(std::map<int, pxar::pixel>::iterator minp_it = minpixels.begin(); minp_it != minpixels.end(); minp_it++){
    fApi->_dut->testPixel(minp_it->second.column(),minp_it->second.row(),true, minp_it->first);
    fApi->_dut->maskPixel(minp_it->second.column(),minp_it->second.row(),false, minp_it->first);
  }

  fApi->setDAC("ctrlreg",4);
  //40 should be changed to the trim target value
  fApi->setDAC("vcal",minthr+5);
  fApi->setDAC("ctrlreg",4);
  //scanning through offset and scale for min pixel (or same randpixel)
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > dacdac_min;
  cnt = 0; 
  done = false;
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

  //search for optimal dac values in 3 steps
  //1. shrinking the PH to be completely inside the ADC range, adjusting phscale
  map<uint8_t, int> ps_opt, po_opt;
  for(uint roc_it = 0; roc_it < rocIds.size(); roc_it++){
    po_opt[rocIds[roc_it]] = 190;
  }
  ps_opt = InsideRangePH(po_opt, dacdac_max, dacdac_min);
  //check for opt failing
  for(uint roc_it = 0; roc_it < rocIds.size(); roc_it++){
    if(ps_opt[rocIds[roc_it]]==999){
      LOG(logDEBUG)<<"PH optimization failed on ROC "<<(int)rocIds[roc_it]<<endl<<"Please run PreTest or try PhOptimization on a random pixel";
    }
  }
  //2. centring PH curve adjusting phoffset
  po_opt = CentrePhRange(po_opt, ps_opt, dacdac_max, dacdac_min);
  
  //3. stretching curve adjusting phscale
  ps_opt = StretchPH(po_opt, ps_opt, dacdac_max, dacdac_min);
  
  for(uint roc_it = 0; roc_it < rocIds.size(); roc_it++){
    fApi->setDAC("ctrlreg",4);
    fApi->setDAC("phscale",ps_opt[rocIds[roc_it]], rocIds[roc_it] );
    fApi->setDAC("phoffset",po_opt[rocIds[roc_it]], rocIds[roc_it]);
    saveDacs();
  }

  for(uint roc_it = 0; roc_it < rocIds.size(); roc_it++){
  //draw PH curve for max and min pixel on every ROC
    name  = Form("PH_ROC%d_c%d_r%d_C%d", rocIds[roc_it], maxpixels[rocIds[roc_it]].column(), maxpixels[rocIds[roc_it]].row(), 0);
    title = Form("PH_ROC%d_c%d_r%d_C%d, phscale = %d, phoffset = %d, maxpixel", rocIds[roc_it], maxpixels[rocIds[roc_it]].column(), maxpixels[rocIds[roc_it]].row(), 0, ps_opt[rocIds[roc_it]], po_opt[rocIds[roc_it]]);
  h1 = bookTH1D(name, name, 256, 0., 256.);
  vector<pair<uint8_t, vector<pixel> > > results;
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  fApi->_dut->testPixel(maxpixels[rocIds[roc_it]].column(), maxpixels[rocIds[roc_it]].row(), true, rocIds[roc_it]);
  fApi->_dut->maskPixel(maxpixels[rocIds[roc_it]].column(), maxpixels[rocIds[roc_it]].row(), false, rocIds[roc_it]);
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
  hists.insert(make_pair(name, h1));
  fHistList.push_back(h1);  

  results.clear();
  
  name  = Form("PH_ROC%d_c%d_r%d_C%d", rocIds[roc_it], minpixels[rocIds[roc_it]].column(), minpixels[rocIds[roc_it]].row(), 0);
  title = Form("PH_ROC%d_c%d_r%d_C%d, phscale = %d, phoffset = %d, minpixel", rocIds[roc_it], minpixels[rocIds[roc_it]].column(), minpixels[rocIds[roc_it]].row(), 0, ps_opt[rocIds[roc_it]], po_opt[rocIds[roc_it]]);
  h1 = bookTH1D(name, name, 256, 0., 256.);
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  fApi->_dut->testPixel(minpixels[rocIds[roc_it]].column(), minpixels[rocIds[roc_it]].row(), true, rocIds[roc_it]);
  fApi->_dut->maskPixel(minpixels[rocIds[roc_it]].column(), minpixels[rocIds[roc_it]].row(), false, rocIds[roc_it]);
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
  hists.insert(make_pair(name, h1));
  fHistList.push_back(h1);  
  }

  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    (*il)->Draw(getHistOption(*il).c_str());
    PixTest::update();
  }
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);
    restoreDacs(); 

  // -- FIXME: should be ROC specific! Must come after restoreDacs()!

  // -- print summary information
  string psString(""), poString(""); 
  for (unsigned int i = 0; i < rocIds.size(); ++i) {
    psString += Form(" %3d", fApi->_dut->getDAC(rocIds[i], "phscale"));
    poString += Form(" %3d", fApi->_dut->getDAC(rocIds[i], "phoffset"));
  }
  LOG(logINFO) << "PixTestPhOptimization::doTest() done";
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
	eff = testEff[rocIds[rocid]]->GetBinContent( testEff[rocIds[rocid]]->FindFixBin((double)c + 0.5, (double)r+0.5) );
	if(eff<aliveTrig){
	  LOG(logDEBUG)<<"Pixel ["<<(int)rocIds[rocid]<<", "<<c<<", "<<r<<"] has eff "<<eff<<"/"<<aliveTrig;
	  badPix.first = rocIds[rocid];
	  badPix.second.first = c;
	  badPix.second.second = r;
	  LOG(logDEBUG)<<"bad Pixel found and blacklisted: ["<<(int)badPix.first<<", "<<badPix.second.first<<", "<<badPix.second.second;
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
    fApi->_dut->testAllPixels(true);
    fApi->_dut->maskAllPixels(false);
    vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
    bool isPixGood=true;
    int maxph = 255;
    fApi->setDAC("phoffset", 200);
    /*********changed to test module w v2 chips***** put it back at 255 later********/
    int init_phScale =100;
    int flag_maxPh=0;
    pair<int, pxar::pixel> maxpixel;
    maxpixel.second.setValue(0);
    std::vector<pxar::pixel> result;
    while((maxph>254 || maxph==0) && flag_maxPh<52){
      result.clear();
      fApi->setDAC("phscale", init_phScale);
      fApi->setDAC("vcal",255);
      fApi->setDAC("ctrlreg",4);
      fApi->setDAC("phoffset",200);  
      int cnt(0); 
      bool done(false);
      while (!done) {
	try {
	  result = fApi->getPulseheightMap(0, 10);
	  done = true;
	} catch(pxarException &e) {
	  LOG(logCRITICAL) << "pXar execption: "<< e.what(); 
	  ++cnt;
	}
	done = (cnt>5) || done;
      }
      
      maxph=0;
      LOG(logDEBUG) << "result size "<<result.size()<<endl;
      //check that the pixel showing highest PH on the module is not reaching 255
      for(std::vector<pxar::pixel>::iterator px = result.begin(); px != result.end(); px++) {
	isPixGood=true;
	for(std::vector<std::pair<uint8_t, pair<int, int> > >::iterator bad_it = badPixels.begin(); bad_it != badPixels.end(); bad_it++){
	  if(bad_it->second.first == px->column() && bad_it->second.second == px->row() && bad_it->first == px->roc()){
	    isPixGood=false;
	  }
	}
	if(isPixGood && px->value() > maxph){
	  maxph = px->value();
	}
      }
      //should have flag for v2 or v2.1
      /*********changed to test module w v2 chips***** put it back at -=5 later********/
      init_phScale+=5;
      flag_maxPh++;
    }
    // Look for pixel with max. pulse height on every ROC:
    for(uint iroc=0; iroc< rocIds.size(); iroc++){
      maxph=0;
      for(std::vector<pxar::pixel>::iterator px = result.begin(); px != result.end(); px++) {
	isPixGood=true;
	if(px->value() > maxph && px->roc() == rocIds[iroc]){
	  for(std::vector<std::pair<uint8_t, pair<int, int> > >::iterator bad_it = badPixels.begin(); bad_it != badPixels.end(); bad_it++){
	    if(bad_it->second.first == px->column() && bad_it->second.second == px->row() && bad_it->first == px->roc()){
	      isPixGood=false;
	      break;
	    }
	  }
	  if(isPixGood){
	    maxpixel = make_pair(rocIds[iroc],*px);
	    maxph = px->value();
	    }
	}
      }
      maxpixels.insert(maxpixel);
      LOG(logDEBUG) << "maxPh " << maxph <<" for ROC "<<maxpixel.first<<" on pixel "<<maxpixel.second << endl ;      
    }
}


void PixTestPhOptimization::GetMinPhPixel(map<int, pxar::pixel > &minpixels,   std::vector<std::pair<uint8_t, pair<int,int> > >  &badPixels){
  //looks for the pixel with the lowest Ph at vcal = trimValue, taking care the pixels are correclty responding (ph>0)
    fApi->_dut->testAllPixels(true);
    fApi->_dut->maskAllPixels(false);
    vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
    bool isPixGood=true;
    int minph = 0;
    /*********changed to test module w v2 chips***** put it back at 255 later********/
    int init_phScale =100;
    int flag_minPh=0;
    pair<int, pxar::pixel> minpixel;
    minpixel.second.setValue(0);
    std::vector<pxar::pixel> result;
    while(minph<1 && flag_minPh<52){
      result.clear();
      fApi->setDAC("phscale", init_phScale);
      //40 should be changed with the trim target value
      fApi->setDAC("vcal",40+5);
      fApi->setDAC("ctrlreg",4);
      fApi->setDAC("phoffset",200);  
      int cnt(0); 
      bool done(false);
      while (!done) {
	try {
	  result = fApi->getPulseheightMap(0, 10);
	  done = true;
	} catch(pxarException &e) {
	  LOG(logCRITICAL) << "pXar execption: "<< e.what(); 
	  ++cnt;
	}
	done = (cnt>5) || done;
      }
      minph=255;
      LOG(logDEBUG) << "result size "<<result.size()<<endl;
      //check that the pixel showing lowest PH above 0 on the module
      for(std::vector<pxar::pixel>::iterator px = result.begin(); px != result.end(); px++) {
	isPixGood=true;
	for(std::vector<std::pair<uint8_t, pair<int, int> > >::iterator bad_it = badPixels.begin(); bad_it != badPixels.end(); bad_it++){
	  if(bad_it->second.first == px->column() && bad_it->second.second == px->row() && bad_it->first == px->roc()){
	    isPixGood=false;
	    break;
	  }
	}
	if(isPixGood && px->value() < minph){
	  minph = px->value();
	}
      }
      //should have flag for v2 or v2.1
      /*********changed to test module w v2 chips***** put it back at -=5 later********/
      init_phScale+=5;
      flag_minPh++;
    }
    // Look for pixel with max. pulse height on every ROC:
    for(uint iroc=0; iroc< rocIds.size(); iroc++){
      minph=255;
      for(std::vector<pxar::pixel>::iterator px = result.begin(); px != result.end(); px++) {
	isPixGood=true;
	if(px->value() < minph && px->roc() == rocIds[iroc]){
	  for(std::vector<std::pair<uint8_t, pair<int, int> > >::iterator bad_it = badPixels.begin(); bad_it != badPixels.end(); bad_it++){
	    if(bad_it->second.first == px->column() && bad_it->second.second == px->row() && bad_it->first == px->roc()){
	      isPixGood=false;
	      break;
	    }
	  }
	  if(isPixGood){
	    minpixel = make_pair(rocIds[iroc],*px);
	    minph = px->value();
	  }
	}
      }
      minpixels.insert(minpixel);
      LOG(logDEBUG) << "minPh " << minph <<" for ROC "<<minpixel.first<<" on pixel "<<minpixel.second << endl ;      
    }
}

map<uint8_t, int> PixTestPhOptimization::InsideRangePH(map<uint8_t,int> &po_opt,  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_max,   std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_min){
  //adjusting phscale so that the PH curve is fully inside the ADC range
  map<uint8_t, int> ps_opt;
  int maxPh(0);
  int minPh(0);
  bool lowEd=false, upEd=false;
  int upEd_dist=255, lowEd_dist=255;
  int safetyMargin = 50;
  int dist = 255;
  map<uint8_t, int> bestDist;
  LOG(logDEBUG) << "dacdac at max vcal has size "<<dacdac_max.size()<<endl;
  LOG(logDEBUG) << "dacdac at min vcal has size "<<dacdac_min.size()<<endl;
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for(uint roc_it = 0; roc_it < rocIds.size(); roc_it++){
    bestDist[rocIds[roc_it]] = 255;
    LOG(logDEBUG)<<"Bestdist at roc_it "<<roc_it<<" initialized with "<<bestDist[roc_it]<<" "<<bestDist[rocIds[roc_it]];
    ps_opt[rocIds[roc_it]] = 999;
  }
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit_max = dacdac_max.begin();
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit_min = dacdac_min.begin();
  int pixsize_max=0;
  int pixsize_min=0;
  LOG(logDEBUG)<<"InsideRange() subtest";
  for(uint roc_it = 0; roc_it < rocIds.size(); roc_it++){
    for(dacit_max = dacdac_max.begin(); dacit_max != dacdac_max.end(); dacit_max++){
      if(dacit_max->first == po_opt[rocIds[roc_it]]){
	for(dacit_min = dacdac_min.begin(); dacit_min != dacdac_min.end(); dacit_min++)
	  if(dacit_min->first == po_opt[rocIds[roc_it]] && dacit_min->second.first == dacit_max->second.first){
    	    pixsize_max = dacit_max->second.second.size();
	    pixsize_min = dacit_min->second.second.size();
    	    for(int pix=0; pix < pixsize_max; pix++){
	      if(dacit_max->second.second[pix].roc()!=rocIds[roc_it] || dacit_min->second.second[pix].roc()!=rocIds[roc_it]) continue;
	      maxPh=dacit_max->second.second[pix].value();
	      minPh=dacit_min->second.second[pix].value();
	      if(dacit_max->second.second[pix].roc() != dacit_min->second.second[pix].roc()){
		LOG(logDEBUG) << "InsideRangePH: ROC ids do not correspond";
	      }
	      lowEd = (minPh > safetyMargin);
	      upEd = (maxPh < 255 - safetyMargin);
	      lowEd_dist = abs(minPh - safetyMargin);
	      upEd_dist = abs(maxPh - (255 - safetyMargin));
	      dist = (upEd_dist > lowEd_dist ) ? (upEd_dist) : (lowEd_dist);
	      if(dist < bestDist[dacit_max->second.second[pix].roc()] && upEd && lowEd){
		LOG(logDEBUG)<<"New distance "<<dist<<" is smaller than previous bestDist "<<bestDist[dacit_max->second.second[pix].roc()]<<" and edges are ok, so... ";
		ps_opt[dacit_max->second.second[pix].roc()] = dacit_max->second.first;
		bestDist[dacit_max->second.second[pix].roc()]=dist;
		LOG(logDEBUG)<<"... new bestDist is "<<bestDist[dacit_max->second.second[pix].roc()]<<" for ps_opt = "<<ps_opt[dacit_max->second.second[pix].roc()];
	      }
	    }
	  }
      }
    }
  }
  
  for(uint roc_it = 0; roc_it < rocIds.size(); roc_it++){
    LOG(logDEBUG)<<"opt step 1: po fixed to"<<po_opt[rocIds[roc_it]]<<" and scale adjusted to "<<ps_opt[rocIds[roc_it]]<<" for ROC "<<(int)rocIds[roc_it]<<", with distance "<<bestDist[rocIds[roc_it]];
  }
  return ps_opt;
}



 map<uint8_t, int> PixTestPhOptimization::CentrePhRange(map<uint8_t, int> &po_opt, map<uint8_t, int> &ps_opt,  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_max,   std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_min){
  //centring PH curve adjusting phoffset   
   LOG(logDEBUG)<<"Welcome to CentrePhRange()"; 
  int maxPh(0);
  int minPh(0);
  int dist = 255;
  map<uint8_t, int> bestDist;
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for(uint roc_it = 0; roc_it < rocIds.size(); roc_it++){
    bestDist[rocIds[roc_it]] = 255;
  }
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit_max = dacdac_max.begin();
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit_min = dacdac_min.begin();
  int pixsize_max=0;
  int pixsize_min=0;
  for(uint roc_it = 0; roc_it < rocIds.size(); roc_it++){
    dist = 255;
    for(dacit_max = dacdac_max.begin(); dacit_max != dacdac_max.end(); dacit_max++){
      if(dacit_max->second.first != ps_opt[rocIds[roc_it]])continue;
      for(dacit_min = dacdac_min.begin(); dacit_min != dacdac_min.end(); dacit_min++){
	if(dacit_min->second.first == ps_opt[rocIds[roc_it]] && dacit_min->first == dacit_max->first){
	  pixsize_max = dacit_max->second.second.size();
	  pixsize_min = dacit_min->second.second.size();
	  for(int pix=0; pix < pixsize_max; pix++){
	    if(dacit_max->second.second[pix].roc()!=rocIds[roc_it] || dacit_min->second.second[pix].roc()!=rocIds[roc_it]) continue;
	    maxPh=dacit_max->second.second[pix].value();
	    minPh=dacit_min->second.second[pix].value();
	    if(dacit_max->second.second[pix].roc() != dacit_min->second.second[pix].roc()){
	      LOG(logDEBUG) << "CentrePhRange: ROC ids do not correspond";
	    }
	    dist = abs(minPh - (255 - maxPh));
	    if (dist < bestDist[dacit_max->second.second[pix].roc()]){
	      po_opt[dacit_max->second.second[pix].roc()] = dacit_max->first;
	      bestDist[dacit_max->second.second[pix].roc()] = dist;
	    } 
	  }
	}
      }
    }
  }
  for(uint roc_it = 0; roc_it < rocIds.size(); roc_it++){
    LOG(logDEBUG)<<"opt centring step: po "<<po_opt[rocIds[roc_it]]<<" and scale "<<ps_opt[rocIds[roc_it]]<<", with distance "<<bestDist[rocIds[roc_it]]<<" on ROC "<<(int)rocIds[roc_it];
  }
  return po_opt;
}

map<uint8_t, int> PixTestPhOptimization::StretchPH(map<uint8_t, int> &po_opt, map<uint8_t, int> &ps_opt,  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_max,   std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_min){
  //stretching PH curve to exploit the full ADC range, adjusting phscale             
  int maxPh(0);
  int minPh(0);
  bool lowEd=false, upEd=false;
  int upEd_dist=255, lowEd_dist=255;
  int safetyMarginUp = fSafetyMarginUp;
  int safetyMarginLow = fSafetyMarginLow;
  LOG(logDEBUG)<<"safety margin for stretching set to "<<fSafetyMarginLow<<" (lower edge) and "<<fSafetyMarginUp<<"(upper edge)";
  int dist = 255;
  map<uint8_t, int> bestDist;
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  for(uint roc_it = 0; roc_it < rocIds.size(); roc_it++){
    bestDist[rocIds[roc_it]] = 255;
  }
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit_max = dacdac_max.begin();
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit_min = dacdac_min.begin();
  while(dacit_max != dacdac_max.end() || dacit_min != dacdac_min.end()){
    for(uint pix=0; pix < dacit_min->second.second.size() && pix < dacit_max->second.second.size(); pix++){
      if(dacit_max->first == po_opt[dacit_max->second.second[pix].roc()] && dacit_min->first == po_opt[dacit_min->second.second[pix].roc()]){
	maxPh=dacit_max->second.second[pix].value();
	minPh=dacit_min->second.second[pix].value();
	if(dacit_max->second.second[pix].roc() != dacit_min->second.second[pix].roc()){
	  LOG(logDEBUG) << "CentrePhRange: ROC ids do not correspond";
	}
	lowEd = (minPh > safetyMarginLow);
	upEd = (maxPh < 255 - safetyMarginUp);
	upEd_dist = abs(maxPh - (255 - safetyMarginUp));
	lowEd_dist = abs(minPh - safetyMarginLow);
	dist = (upEd_dist < lowEd_dist ) ? (upEd_dist) : (lowEd_dist);
	if(dist < bestDist[dacit_max->second.second[pix].roc()] && lowEd && upEd){
	  ps_opt[dacit_max->second.second[pix].roc()] = dacit_max->second.first;
	  bestDist[dacit_max->second.second[pix].roc()]=dist;
	}
      }
    }
    dacit_max++;
    dacit_min++;
  }
  for(uint roc_it = 0; roc_it < rocIds.size(); roc_it++){
    LOG(logDEBUG)<<"opt final step: po fixed to"<<po_opt[rocIds[roc_it]]<<" and scale adjusted to "<<ps_opt[rocIds[roc_it]]<<", with distance "<<bestDist[rocIds[roc_it]]<<" on ROC "<<(int)rocIds[roc_it];
  }
  return ps_opt;
}
