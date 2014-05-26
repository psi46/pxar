#include <stdlib.h>  // atof, atoi
#include <algorithm> // std::find
#include <sstream>   // parsing

#include "PixTestPhOptimization.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestPhOptimization)

PixTestPhOptimization::PixTestPhOptimization() {}

PixTestPhOptimization::PixTestPhOptimization( PixSetup *a, std::string name ) :  PixTest(a, name),
  fParNtrig(-1), fParDAC("nada"), fParDacVal(100) {
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
      if (!parName.compare("singlePix")) {
	setTestParameter("singlePix", sval); 
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

void PixTestPhOptimization::bookHist(string name) {}

PixTestPhOptimization::~PixTestPhOptimization() {}

void PixTestPhOptimization::doTest() {

  cacheDacs();
  fDirectory->cd();
  PixTest::update();

  TH1D *h1(0); 
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  map<string, TH1D*> hists; 
  string name;
//  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
//    for (unsigned int i = 0; i < fPIX.size(); ++i) {
//      if (fPIX[i].first > -1)  {
//	name = Form("PH_c%d_r%d_C%d", fPIX[i].first, fPIX[i].second, rocIds[iroc]); 
//	h1 = bookTH1D(name, name, 256, 0., 256.);
//	h1->SetMinimum(0);
//	setTitles(h1, Form("PH [ADC] for %s = %d", fParDAC.c_str(), fParDacVal), "Entries/bin"); 
//	hists.insert(make_pair(name, h1)); 
//	fHistList.push_back(h1);
//      }
//    }
//  }

//  fApi->setDAC(fParDAC, fParDacVal);



  //pixelalive test to discard not fully efficient pixels
  std::vector<std::pair<int, int> > badPixels;
  BlacklistPixels(badPixels, 10);


  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  std::vector<pxar::pixel> thrmap = fApi->getThresholdMap("vcal", 0, 255, FLAG_RISING_EDGE, 10);
  int minthr=0;
  LOG(logDEBUG) << "thr map size "<<thrmap.size()<<endl;

  fFlagSinglePix=true;


  bool isPixGood=true;
  pxar::pixel maxpixel;
  pxar::pixel minpixel;

  if(fFlagSinglePix){
    LOG(logDEBUG)<<"**********Ph range will be optimised on a single random pixel***********";
    pxar::pixel randomPix;
    randomPix= *(RandomPixel(badPixels));
    LOG(logDEBUG)<<"In doTest(), randomCol "<<randomPix.column<<", randomRow "<<randomPix.row<<", pixel "<<randomPix;
    maxpixel.column = randomPix.column;
    maxpixel.row    = randomPix.row;
    minpixel.column = randomPix.column;
    minpixel.row    = randomPix.row;
    LOG(logDEBUG)<<"random pixel: "<<maxpixel<<", "<<minpixel<<"is not on the blacklist";

    for(std::vector<pxar::pixel>::iterator thrit = thrmap.begin(); thrit != thrmap.end(); thrit++){
      if(thrit->column == randomPix.column && thrit->row == randomPix.row){
	minthr=thrit->value;
      }
    }
  }
  

  //phopt performed on all pixels simultaneously (looking for extreme cases)
  else{
    LOG(logDEBUG)<<"**********Ph range will be optimised on the whole ROC***********";
    fApi->_dut->testAllPixels(true);
    fApi->_dut->maskAllPixels(false);
    int ntriggers = 10;
    isPixGood=true;
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
	//      LOG(logDEBUG) << " pixel col "<<(int)px->column <<", row "<<(int)px->row<<", PH "<<px->value<<endl;
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

    // Find min. pixel:
    isPixGood=true;
    fApi->_dut->testAllPixels(true);
    fApi->_dut->maskAllPixels(false);
    fApi->setDAC("ctrlreg",4);
  

    for(std::vector<pxar::pixel>::iterator thrit = thrmap.begin(); thrit != thrmap.end(); thrit++){
      isPixGood=true;
      // LOG(logDEBUG)<<"threshold map "<<*thrit<<endl;
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


  //  maxpixel.column = 11;
  // maxpixel.row = 20;
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  fApi->_dut->testPixel(maxpixel.column,maxpixel.row,true);
  fApi->_dut->maskPixel(maxpixel.column,maxpixel.row,false);

  fApi->setDAC("vcal",255);
  fApi->setDAC("ctrlreg",4);

  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > dacdac_max = fApi->getPulseheightVsDACDAC("phoffset",0,255,"phscale",0,255,0,10);



  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);


  //minpixel.column = 11;
  //minpixel.row = 20;
  fApi->_dut->testPixel(minpixel.column,minpixel.row,true);
  fApi->_dut->maskPixel(minpixel.column,minpixel.row,false);
  //  fApi->setDAC("vcal",40);
  minthr = (minthr==255) ? (minthr) : (minthr + 10); 
  //minthr = 50;
  fApi->setDAC("ctrlreg",4);
  fApi->setDAC("vcal",minthr);

  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > dacdac_min = fApi->getPulseheightVsDACDAC("phoffset",0,255,"phscale",0,255,0,10);



  int po=200;
  int ps_opt = 999, po_opt = 999;
  int maxPh(0.);
  int minPh(0.);
  bool lowEd=false, upEd=false;
  double upEd_dist=255, lowEd_dist=255;
  unsigned int io_opt=999;
  int safetyMargin = 50;
  int dist = 255;
  int bestDist = 255;
  // Do some magic here...
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit_max = dacdac_max.begin();
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit_min = dacdac_min.begin();
  //or two for cycles??
  LOG(logDEBUG) << "dacdac at max vcal has size "<<dacdac_max.size()<<endl;
  LOG(logDEBUG) << "dacdac at min vcal has size "<<dacdac_min.size()<<endl;
  while(dacit_max != dacdac_max.end() || dacit_min != dacdac_min.end()){
    //LOG(logDEBUG) << "max Ph for scale "<<(int)dacit_max->second.first<<" and offset "<<(int)dacit_max->first<<" is "<<dacit_max->second.second[0].value<<", size "<<dacit_max->second.second.size();
    //LOG(logDEBUG) << "min Ph for scale "<<(int)dacit_min->second.first<<" and offset "<<(int)dacit_min->first<<", size "<<dacit_min->second.second.size();
    //    LOG(logDEBUG) << "min Ph for scale "<<(int)dacit_min->second.first<<" and offset "<<(int)dacit_min->first˚<<" is "<<dacit_min->second.second[0].value;
    if(dacit_max->first == po && dacit_min->first == po && dacit_min->second.second.size() && dacit_max->second.second.size()) {
      //      LOG(logDEBUG)<<"entered the IF condition";
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

  LOG(logDEBUG)<<"opt step 1: po fixed to"<<po<<" and scale adjusted to "<<ps_opt<<", with distance "<<bestDist;

//  for(std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit = dacdac_max.begin(); dacit != dacdac_max.end();dacit++ ){
//    if(dacit->first == po) {
//      dacit->second.first ;
//      dacit->second.second.at(0).value;
//    }
//}

  if(ps_opt==999){
    cout<<"PH optimization failed"<<endl;
    return;
  }
  

  //centring PH curve adjusting ph offset                                                                                                                                                                          
  bestDist=255;
  po_opt=999;
  dacit_max = dacdac_max.begin();
  dacit_min = dacdac_min.begin();
  dist=255;
  bestDist=255;
  //or two for cycles??
  while(dacit_max != dacdac_max.end() || dacit_min != dacdac_min.end()){
    if(dacit_max->second.first == ps_opt && dacit_min->second.first == ps_opt && dacit_min->second.second.size() && dacit_max->second.second.size()) {
      maxPh=dacit_max->second.second.at(0).value;
      minPh=dacit_min->second.second.at(0).value;
      dist = abs(minPh - (255 - maxPh));
      //cout<<"offset = " << io << ", plateau = " << maxPh << ", minPh = " << minPh << ", distance = " << dist << endl;
      if (dist < bestDist){
	po_opt = dacit_max->first;
	bestDist = dist;
      } 
    }
    dacit_max++;
    dacit_min++;
  }

  LOG(logDEBUG)<<"opt centring step: po "<<po_opt<<" and scale "<<ps_opt<<", with distance "<<bestDist;
 
  io_opt=999;
  safetyMargin=10;
  bestDist=255;
  dist =255;
  upEd_dist=255;
  lowEd_dist=255;
  lowEd=false;
  upEd=false;
  dacit_max = dacdac_max.begin();
  dacit_min = dacdac_min.begin();
  while(dacit_max != dacdac_max.end() || dacit_min != dacdac_min.end()){
    if(dacit_max->first == po_opt && dacit_min->first == po_opt && dacit_min->second.second.size() && dacit_max->second.second.size()) {
      maxPh=dacit_max->second.second.at(0).value;
      minPh=dacit_min->second.second.at(0).value;

      lowEd = (minPh > safetyMargin);
      upEd = (maxPh < 255 - safetyMargin);
      upEd_dist = abs(maxPh - (255 - safetyMargin));
      lowEd_dist = abs(minPh - safetyMargin);
      dist = (upEd_dist < lowEd_dist ) ? (upEd_dist) : (lowEd_dist);
      //      cout<<"scale = " << is <<", maxPh "<<maxPh<<", minPh"<<minPh<< ", distance = " << dist << endl;
      if(dist<bestDist && lowEd && upEd){
	ps_opt = dacit_max->second.first;
	bestDist=dist;
      }
    }
    dacit_max++;
    dacit_min++;
  }

  LOG(logDEBUG)<<"opt final step: po fixed to"<<po_opt<<" and scale adjusted to "<<ps_opt<<", with distance "<<bestDist;

  //draw PH curve for min and max pixel
  string title;
  name  = Form("PH_c%d_r%d_C%d", minpixel.column, minpixel.row, 0);
  //WARNING: roc number is hardcoded
  title = Form("PH_c%d_r%d_C%d, phscale = %d, phoffset = % d, worst minPh", minpixel.column, minpixel.row, 0, ps_opt, po_opt);
  h1 = bookTH1D(name, name, 256, 0., 256.);
  h1->SetMinimum(0);
  setTitles(h1, title.c_str(), "Entries/bin");
  hists.insert(make_pair(name, h1));
  fHistList.push_back(h1);  

  name  = Form("PH_c%d_r%d_C%d", maxpixel.column, maxpixel.row, 0);
  title = Form("PH_c%d_r%d_C%d, phscale = %d, phoffset = % d, worst maxPh", maxpixel.column, maxpixel.row, 0, ps_opt, po_opt);
  h1 = bookTH1D(name, name, 256, 0., 256.);
  h1->SetMinimum(0);
  setTitles(h1, title.c_str(), "Entries/bin");
  hists.insert(make_pair(name, h1));
  fHistList.push_back(h1);  


  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    (*il)->Draw();
    PixTest::update();
  }
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);

  restoreDacs(); 
}

void PixTestPhOptimization::BlacklistPixels(std::vector<std::pair<int, int> > &badPixels, int aliveTrig){


  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  vector<TH2D*> testEff = efficiencyMaps("PixelAlive", aliveTrig);
  LOG(logDEBUG)<<"size of testEff vector "<<testEff.size();
  LOG(logDEBUG)<<"size of pixelalive map "<<testEff[0]->GetNbinsX()<<" X "<<testEff[0]->GetNbinsY();
  //  std::vector<std::pair<int, int> > badPixels;
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

  bool isPixGood=true;
  pxar::pixel randPixel;
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
  
  randPixel.column = random_col;
  randPixel.row    = random_row;
  LOG(logDEBUG)<<"In RandomPixel(), randomCol "<<randPixel.column<<", randomRow "<<randPixel.row<<", pixel "<<randPixel;
  return &randPixel;
  

}
