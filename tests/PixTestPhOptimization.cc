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
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    for (unsigned int i = 0; i < fPIX.size(); ++i) {
      if (fPIX[i].first > -1)  {
	name = Form("PH_c%d_r%d_C%d", fPIX[i].first, fPIX[i].second, rocIds[iroc]); 
	h1 = bookTH1D(name, name, 256, 0., 256.);
	h1->SetMinimum(0);
	setTitles(h1, Form("PH [ADC] for %s = %d", fParDAC.c_str(), fParDacVal), "Entries/bin"); 
	hists.insert(make_pair(name, h1)); 
	fHistList.push_back(h1);
      }
    }
  }

  fApi->setDAC(fParDAC, fParDacVal);

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  int ntriggers = 10;
  fApi->setDAC("vcal",255);
  fApi->setDAC("ctrlreg",4);

  std::vector<pxar::pixel> result = fApi->getPulseheightMap(0, 10);

  pxar::pixel maxpixel;
  // Look for pixel with max. pulse height:
  for(std::vector<pxar::pixel>::iterator px = result.begin(); px != result.end(); px++) {
    if(px->value > maxpixel.value) maxpixel = *px;
  }

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  fApi->_dut->testPixel(maxpixel.column,maxpixel.row,true);
  fApi->_dut->maskPixel(maxpixel.column,maxpixel.row,false);

  std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > dacdac_max = fApi->getPulseheightVsDACDAC("phoffset",0,255,"phscale",0,255,0,10);

  // Do some magic here...

  
  // Find min. pixel:
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  fApi->setDAC("ctrlreg",0);

  std::vector<pxar::pixel> thrmap = fApi->getThresholdMap("vcal", 0, 100, FLAG_RISING_EDGE, 10);





  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    (*il)->Draw();
    PixTest::update();
  }
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);

  restoreDacs(); 
}
