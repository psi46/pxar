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

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  int ntriggers = 10;

  fApi->setDAC("vcal",255);
  fApi->setDAC("ctrlreg",4);
  fApi->setDAC("phoffset",150);  
  int maxph = 255;
  int init_phScale =30;
  pxar::pixel maxpixel;
  maxpixel.value = 0;
  while(maxph>254){
    fApi->setDAC("phscale", init_phScale);
    
    std::vector<pxar::pixel> result = fApi->getPulseheightMap(0, 10);
    
   
    // Look for pixel with max. pulse height:
    
    LOG(logDEBUG) << "result size "<<result.size()<<endl;

    for(std::vector<pxar::pixel>::iterator px = result.begin(); px != result.end(); px++) {
      LOG(logDEBUG) << " pixel col "<<(int)px->column <<", row "<<(int)px->row<<", PH "<<px->value<<endl;
      if(px->value > maxpixel.value){
	maxpixel = *px;
	maxph = px->value;
      }
    }
    
    LOG(logDEBUG) << "maxPh " << maxph <<" "<<maxpixel << endl ;

    //should have flag for v2 or v2.1
    init_phScale-=5;
  }

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  fApi->_dut->testPixel(maxpixel.column,maxpixel.row,true);
  fApi->_dut->maskPixel(maxpixel.column,maxpixel.row,false);

  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > dacdac_max = fApi->getPulseheightVsDACDAC("phoffset",0,255,"phscale",0,255,0,10);




 // Find min. pixel:
  pxar::pixel minpixel;
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  fApi->setDAC("ctrlreg",0);

  std::vector<pxar::pixel> thrmap = fApi->getThresholdMap("vcal", 0, 100, FLAG_RISING_EDGE, 10);
  int minthr=0;
  LOG(logDEBUG) << "thr map size "<<thrmap.size()<<endl;

  for(std::vector<pxar::pixel>::iterator thrit = thrmap.begin(); thrit != thrmap.end(); thrit++){
    LOG(logDEBUG)<<"threshold map "<<*thrit<<endl;
    if(thrit->value > minthr) {
      minpixel = *thrit;
      minthr = minpixel.value;
    }
  }

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  LOG(logDEBUG) << "min vcal thr " << minthr << endl ;

  fApi->_dut->testPixel(minpixel.column,minpixel.row,true);
  fApi->_dut->maskPixel(minpixel.column,minpixel.row,false);
  fApi->setDAC("vcal",minthr+10);

  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > dacdac_min = fApi->getPulseheightVsDACDAC("phoffset",0,255,"phscale",0,255,0,10);



  int po=200;
  int ps_opt = 999, po_opt = 999;
  int maxPh(0.);
  int minPh(0.);
  bool lowEd=false, upEd=false;
  double upEd_dist=255, lowEd_dist=255;
  unsigned int io_opt=999;
  int safetyMargin = 40;
  int dist = 255;
  int bestDist = 255;
  // Do some magic here...
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit_max = dacdac_max.begin();
  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit_min = dacdac_min.begin();
  //or two for cycles??
  while(dacit_max != dacdac_max.end() || dacit_min != dacdac_min.end()){
    if(dacit_max->first == po && dacit_min->first == po) {
      maxPh=dacit_max->second.second.at(0).value;
      minPh=dacit_min->second.second.at(0).value;
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
    if(dacit_max->second.first == ps_opt && dacit_min->second.first == ps_opt) {
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
    if(dacit_max->first == po_opt && dacit_min->first == po_opt) {
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
  }


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
