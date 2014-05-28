#include <stdlib.h>  // atof, atoi
#include <algorithm> // std::find
#include <sstream>   // parsing

#include "PixTestPhOptimization.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestPhOptimization)

PixTestPhOptimization::PixTestPhOptimization() {}

PixTestPhOptimization::PixTestPhOptimization( PixSetup *a, std::string name ) :  PixTest(a, name), fParNtrig(-1), fParDAC("nada"), fParDacVal(100),   fFlagSinglePix(true) {
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
  map<string, TH1D*> hists; 
  string name, title;

  //looking for inefficient pixels, so that they can be avoided
  std::vector<std::pair<int, int> > badPixels;
  BlacklistPixels(badPixels, 10);

  //vcal threshold map in order to choose the low-vcal value the PH will be sampled at
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  fApi->setDAC("ctrlreg",4);
  std::vector<pxar::pixel> thrmap = fApi->getThresholdMap("vcal", 0, 255, FLAG_RISING_EDGE, 10);
  int minthr=0;
  LOG(logDEBUG) << "thr map size "<<thrmap.size()<<endl;

  //flag allows to choose between PhOpt on single pixel (1) or on the whole roc (0)
  pxar::pixel maxpixel;
  pxar::pixel minpixel;

  if(fFlagSinglePix){
    LOG(logDEBUG)<<"**********Ph range will be optimised on a single random pixel***********";
    pxar::pixel randomPix;
    randomPix= *(RandomPixel(badPixels));
    LOG(logDEBUG)<<"In doTest(), randomCol "<<(int)randomPix.column<<", randomRow "<<(int)randomPix.row<<", pixel "<<randomPix;
    maxpixel.column = randomPix.column;
    maxpixel.row    = randomPix.row;
    minpixel.column = randomPix.column;
    minpixel.row    = randomPix.row;
    LOG(logDEBUG)<<"random pixel: "<<maxpixel<<", "<<minpixel<<"is not on the blacklist";
    //retrieving info from the vcal thr map for THIS random pixel
    for(std::vector<pxar::pixel>::iterator thrit = thrmap.begin(); thrit != thrmap.end(); thrit++){
      if(thrit->column == randomPix.column && thrit->row == randomPix.row){
	minthr=thrit->value;
      }
    }
  }
  else{
    LOG(logDEBUG)<<"**********Ph range will be optimised on the whole ROC***********";
    //getting highest ph pixel
    GetMaxPhPixel(maxpixel, badPixels);
    // getting pixel showing the largest vcal threshold (i.e., all other pixels are responding)
    GetMinPixel(minpixel, thrmap, badPixels);
  }
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  fApi->_dut->testPixel(maxpixel.column,maxpixel.row,true);
  fApi->_dut->maskPixel(maxpixel.column,maxpixel.row,false);
  fApi->setDAC("vcal",255);
  fApi->setDAC("ctrlreg",4);
  //scanning through offset and scale for max pixel (or randpixel)
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > dacdac_max = fApi->getPulseheightVsDACDAC("phoffset",0,255,"phscale",0,255,0,10);
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  fApi->_dut->testPixel(minpixel.column,minpixel.row,true);
  fApi->_dut->maskPixel(minpixel.column,minpixel.row,false);
  minthr = (minthr==255) ? (minthr) : (minthr + 5); 
  //minthr = 50;
  fApi->setDAC("ctrlreg",4);
  fApi->setDAC("vcal",minthr);
  //scanning through offset and scale for min pixel (or same randpixel)
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > dacdac_min = fApi->getPulseheightVsDACDAC("phoffset",0,255,"phscale",0,255,0,10);
  //search for optimal dac values in 3 steps
  //1. shrinking the PH to be completely inside the ADC range, adjusting phscale
  int ps_opt = 999, po_opt = 999;
  po_opt=200;
  ps_opt = InsideRangePH(po_opt, dacdac_max, dacdac_min);
  //check for opt failing
  if(ps_opt==999){
    LOG(logDEBUG)<<"PH optimization failed"<<endl<<"Please run PreTest or try PhOptimization on a random pixel";
    return;
  }
  //2. centring PH curve adjusting phoffset
  po_opt = CentrePhRange(po_opt, ps_opt, dacdac_max, dacdac_min);
  //3. stretching curve adjusting phscale
  ps_opt = StretchPH(po_opt, ps_opt, dacdac_max, dacdac_min);


  fApi->setDAC("ctrlreg",4);
  fApi->setDAC("phscale",ps_opt);
  fApi->setDAC("phoffset",po_opt);
  //draw PH curve for max and min pixel
  name  = Form("PH_c%d_r%d_C%d", maxpixel.column, maxpixel.row, 0);
  title = Form("PH_c%d_r%d_C%d, phscale = %d, phoffset = %d, maxpixel", maxpixel.column, maxpixel.row, 0, ps_opt, po_opt);
  h1 = bookTH1D(name, name, 256, 0., 256.);
  vector<pair<uint8_t, vector<pixel> > > results;
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  fApi->_dut->testPixel(maxpixel.column, maxpixel.row, true);
  fApi->_dut->maskPixel(maxpixel.column, maxpixel.row, false);
  results = fApi->getPulseheightVsDAC("vcal", 0, 255, FLAG_FORCE_MASKED, 10);
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    for (unsigned int i = 0; i < results.size(); ++i) {
      pair<uint8_t, vector<pixel> > v = results[i];
      int idac = v.first; 
      vector<pixel> vpix = v.second;
      for (unsigned int ipix = 0; ipix < vpix.size(); ++ipix) {
	if (vpix[ipix].roc_id == rocIds[iroc]) {
	  h1->Fill(idac, vpix[ipix].value);
	}
      }
    }
  }
  h1->SetMinimum(0);
  setTitles(h1, title.c_str(), "average PH");
  hists.insert(make_pair(name, h1));
  fHistList.push_back(h1);  

  results.clear();

  name  = Form("PH_c%d_r%d_C%d", minpixel.column, minpixel.row, 0);
  title = Form("PH_c%d_r%d_C%d, phscale = %d, phoffset = %d, minpixel", minpixel.column, minpixel.row, 0, ps_opt, po_opt);
  h1 = bookTH1D(name, name, 256, 0., 256.);
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  fApi->_dut->testPixel(minpixel.column, minpixel.row, true);
  fApi->_dut->maskPixel(minpixel.column, minpixel.row, false);
  results = fApi->getPulseheightVsDAC("vcal", 0, 255, FLAG_FORCE_MASKED, 10);
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    for (unsigned int i = 0; i < results.size(); ++i) {
      pair<uint8_t, vector<pixel> > v = results[i];
      int idac = v.first; 
      vector<pixel> vpix = v.second;
      for (unsigned int ipix = 0; ipix < vpix.size(); ++ipix) {
	if (vpix[ipix].roc_id == rocIds[iroc]) {
	  h1->Fill(idac, vpix[ipix].value);
	}
      }
    }
  }
  h1->SetMinimum(0);
  setTitles(h1, title.c_str(), "average PH");
  hists.insert(make_pair(name, h1));
  fHistList.push_back(h1);  


  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    (*il)->Draw();
    PixTest::update();
  }
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);
  restoreDacs(); 

  // -- FIXME: should be ROC specific! Must come after restoreDacs()!
  fApi->setDAC("phscale",ps_opt);
  fApi->setDAC("phoffset",po_opt);
  saveDacs();

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

void PixTestPhOptimization::BlacklistPixels(std::vector<std::pair<int, int> > &badPixels, int aliveTrig){
  //makes a list of inefficient pixels, to be avoided during optimization
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  vector<TH2D*> testEff = efficiencyMaps("PixelAlive", aliveTrig);
  std::pair<int, int> badPix;
  int eff=0;
  for (unsigned int i = 0; i < testEff.size(); ++i) {
    for(int r=0; r<80; r++){
      for(int c=0; c<52; c++){
	eff = testEff[i]->GetBinContent( testEff[i]->FindFixBin((double)c + 0.5, (double)r+0.5) );
	if(eff<aliveTrig){
	  LOG(logDEBUG)<<"Pixel "<<c<<", "<<r<<" has eff "<<eff<<"/"<<aliveTrig;
	  badPix.first = c;
	  badPix.second = r;
	  LOG(logDEBUG)<<"bad Pixel found and blacklisted: "<<badPix.first<<", "<<badPix.second;
	  (badPixels).push_back(badPix);
	}
      }
    }
  }
}


pxar::pixel* PixTestPhOptimization::RandomPixel(std::vector<std::pair<int, int> > &badPixels){
  //Returns a random pixel, taking care it is not on the blacklist
  fApi->setDAC("ctrlreg",4);
  bool isPixGood=true;
  pxar::pixel *randPixel= new pixel();
  srand(time(NULL));
  int random_col=-1, random_row=-1;
  do{
    random_col = rand() % 52;
    random_row = rand() % 80;
    LOG(logDEBUG)<<"random pixel: ["<<random_col<<", "<<random_row<<"]";
    isPixGood=true;
    for(std::vector<std::pair<int, int> >::iterator bad_it = badPixels.begin(); bad_it != badPixels.end(); bad_it++){
      if(bad_it->first == random_col && bad_it->second == random_row){
	isPixGood=false;
      }
    }
    LOG(logDEBUG)<<"is the random pixel good? "<<isPixGood;
  }while(!isPixGood);
  randPixel->column = random_col;
  randPixel->row    = random_row;
  LOG(logDEBUG)<<"In RandomPixel(), randomCol "<<(int)randPixel->column<<", randomRow "<<(int)randPixel->row<<", pixel "<<randPixel;
  return randPixel;
}


void PixTestPhOptimization::GetMaxPhPixel(pxar::pixel &maxpixel, std::vector<std::pair<int, int> > &badPixels){
  //looks for the pixel with the highest Ph at vcal = 255, taking care the pixels are not already saturating (ph=255)
    fApi->_dut->testAllPixels(true);
    fApi->_dut->maskAllPixels(false);
    bool isPixGood=true;
    int maxph = 255;
    int init_phScale =255;
    int flag_maxPh=0;
    maxpixel.value = 0;
    while(maxph>254 && flag_maxPh<52){
      fApi->setDAC("phscale", init_phScale);
      fApi->setDAC("vcal",255);
      fApi->setDAC("ctrlreg",4);
      fApi->setDAC("phoffset",200);  
      std::vector<pxar::pixel> result = fApi->getPulseheightMap(0, 10);
      // Look for pixel with max. pulse height:
      LOG(logDEBUG) << "result size "<<result.size()<<endl;
      for(std::vector<pxar::pixel>::iterator px = result.begin(); px != result.end(); px++) {
	isPixGood=true;
	if(px->value > maxpixel.value){
	  for(std::vector<std::pair<int, int> >::iterator bad_it = badPixels.begin(); bad_it != badPixels.end(); bad_it++){
	    if(bad_it->first == px->column && bad_it->second == px->row){
	      isPixGood=false;
	    }
	  }
	  if(isPixGood){
	    maxpixel = *px;
	    maxph = px->value;
	  }
	}
      }
      LOG(logDEBUG) << "maxPh " << maxph <<" "<<maxpixel << endl ;      
      //should have flag for v2 or v2.1
      init_phScale-=5;
      flag_maxPh++;
    }
}




void PixTestPhOptimization::GetMinPixel(pxar::pixel &minpixel, std::vector<pxar::pixel> &thrmap, std::vector<std::pair<int, int> > &badPixels){
  //the minimum pixel is the pixel showing the largest vcal threshold: it is the smallest vcal we can probe the Ph for all pixels, and this pix has the smallest ph for this vcal
  int minthr=0;
  bool  isPixGood=true;
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);  
  for(std::vector<pxar::pixel>::iterator thrit = thrmap.begin(); thrit != thrmap.end(); thrit++){
    isPixGood=true;
    if(thrit->value > minthr) {
      for(std::vector<std::pair<int, int> >::iterator bad_it = badPixels.begin(); bad_it != badPixels.end(); bad_it++){
	  if(bad_it->first == thrit->column && bad_it->second == thrit->row){
	    isPixGood=false;
	  }
      }
      if(isPixGood){
	minpixel = *thrit;
	  minthr = minpixel.value;
      }
    }
  }
  LOG(logDEBUG) << "min vcal thr " << minthr << "found for pixel "<<minpixel<<endl ;
}


int PixTestPhOptimization::InsideRangePH(int po_opt,  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_max,   std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_min){
  //adjusting phscale so that the PH curve is fully inside the ADC range
  int ps_opt = 999;
  int maxPh(0.);
  int minPh(0.);
  bool lowEd=false, upEd=false;
  double upEd_dist=255, lowEd_dist=255;
  int safetyMargin = 50;
  int dist = 255;
  int bestDist = 255;
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit_max = dacdac_max.begin();
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit_min = dacdac_min.begin();
  //or two for cycles??
  LOG(logDEBUG) << "dacdac at max vcal has size "<<dacdac_max.size()<<endl;
  LOG(logDEBUG) << "dacdac at min vcal has size "<<dacdac_min.size()<<endl;
  while(dacit_max != dacdac_max.end() || dacit_min != dacdac_min.end()){
    if(dacit_max->first == po_opt && dacit_min->first == po_opt && dacit_min->second.second.size() && dacit_max->second.second.size()) {
      maxPh=dacit_max->second.second[0].value;
      minPh=dacit_min->second.second[0].value;
      lowEd = (minPh > safetyMargin);
      upEd = (maxPh < 255 - safetyMargin);
      upEd_dist = abs(maxPh - (255 - safetyMargin));
      lowEd_dist = abs(minPh - safetyMargin);
      dist = (upEd_dist > lowEd_dist ) ? (upEd_dist) : (lowEd_dist);
      if(dist < bestDist && upEd && lowEd){
	ps_opt = dacit_max->second.first;
	bestDist=dist;
      }
    }
    dacit_max++;
    dacit_min++;
  }
  LOG(logDEBUG)<<"opt step 1: po fixed to"<<po_opt<<" and scale adjusted to "<<ps_opt<<", with distance "<<bestDist;
  return ps_opt;
}



int PixTestPhOptimization::CentrePhRange(int po_opt_in, int ps_opt,  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_max,   std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_min){
  //centring PH curve adjusting phoffset   
  int po_opt_out = po_opt_in;
  int maxPh(0.);
  int minPh(0.);
  int dist = 255;
  int bestDist = 255;
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit_max = dacdac_max.begin();
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit_min = dacdac_min.begin();
  //or two for cycles??
  while(dacit_max != dacdac_max.end() || dacit_min != dacdac_min.end()){
    if(dacit_max->second.first == ps_opt && dacit_min->second.first == ps_opt && dacit_min->second.second.size() && dacit_max->second.second.size()) {
      maxPh=dacit_max->second.second.at(0).value;
      minPh=dacit_min->second.second.at(0).value;
      dist = abs(minPh - (255 - maxPh));
      if (dist < bestDist){
	po_opt_out = dacit_max->first;
	bestDist = dist;
      } 
    }
    dacit_max++;
    dacit_min++;
  }
  LOG(logDEBUG)<<"opt centring step: po "<<po_opt_out<<" and scale "<<ps_opt<<", with distance "<<bestDist;
  return po_opt_out;
}



int PixTestPhOptimization::StretchPH(int po_opt, int ps_opt_in,  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_max,   std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > &dacdac_min){
  //stretching PH curve to exploit the full ADC range, adjusting phscale             
  int ps_opt_out = ps_opt_in;
  int maxPh(0.);
  int minPh(0.);
  bool lowEd=false, upEd=false;
  double upEd_dist=255, lowEd_dist=255;
  int safetyMargin = 10;
  int dist = 255;
  int bestDist = 255;
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit_max = dacdac_max.begin();
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit_min = dacdac_min.begin();
  while(dacit_max != dacdac_max.end() || dacit_min != dacdac_min.end()){
    if(dacit_max->first == po_opt && dacit_min->first == po_opt && dacit_min->second.second.size() && dacit_max->second.second.size()) {
      maxPh=dacit_max->second.second.at(0).value;
      minPh=dacit_min->second.second.at(0).value;
      lowEd = (minPh > safetyMargin);
      upEd = (maxPh < 255 - safetyMargin);
      upEd_dist = abs(maxPh - (255 - safetyMargin));
      lowEd_dist = abs(minPh - safetyMargin);
      dist = (upEd_dist < lowEd_dist ) ? (upEd_dist) : (lowEd_dist);
      if(dist<bestDist && lowEd && upEd){
	ps_opt_out = dacit_max->second.first;
	bestDist=dist;
      }
    }
    dacit_max++;
    dacit_min++;
  }
  LOG(logDEBUG)<<"opt final step: po fixed to"<<po_opt<<" and scale adjusted to "<<ps_opt_out<<", with distance "<<bestDist;
  return ps_opt_out;
}
