// -- author: A.Vargas, D.Pitzl
// Bump Bonding tests, 
// sending test-pulse through the sensor (cals), a scan of vthrcomp for all pixels is performed. 
// The width of the plateau is used to distinguish between 
// good/dead/bad bump-bonding


#include <sstream>   // parsing
#include <algorithm>  // std::find

#include "PixTestBB2Map.hh"
#include "PixUtil.hh"
#include "log.h"
#include "constants.h"   // roctypes

using namespace std;
using namespace pxar;

ClassImp(PixTestBB2Map)

//------------------------------------------------------------------------------
PixTestBB2Map::PixTestBB2Map(PixSetup *a, std::string name): PixTest(a, name), fParNtrig(10), fParVcalS(222), fParPlWidth(35) {
  PixTest::init();
  init();
  LOG(logDEBUG) << "PixTestBB2Map ctor(PixSetup &a, string, TGTab *)";
}


//------------------------------------------------------------------------------
PixTestBB2Map::PixTestBB2Map(): PixTest() {
  LOG(logDEBUG) << "PixTestBB2Map ctor()";
}



//------------------------------------------------------------------------------
bool PixTestBB2Map::setParameter(string parName, string sval) {

  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (uint32_t i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());

      stringstream s(sval);
      if (!parName.compare( "ntrig")) { 
	s >> fParNtrig; 
	setToolTips();
	return true;
      }
      if (!parName.compare( "vcals")) { 
	s >> fParVcalS; 
	setToolTips();
	return true;
      }
      if (!parName.compare( "Plwidth"))  { 
	s >> fParPlWidth; 
	setToolTips();
	return true;
      }
    }
  }
  return false;
}

//------------------------------------------------------------------------------
void PixTestBB2Map::init() {
  LOG(logDEBUG) << "PixTestBB2Map::init()";
  
  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory ) {
    fDirectory = gFile->mkdir( fName.c_str() );
  }
  fDirectory->cd();
}

// ----------------------------------------------------------------------
void PixTestBB2Map::setToolTips() {
  fTestTip = string( "Bump Bonding Test = threshold map for CalS");
  fSummaryTip = string("module summary");
}


//------------------------------------------------------------------------------
PixTestBB2Map::~PixTestBB2Map() {
  LOG(logDEBUG) << "PixTestBB2Map dtor";
}

//------------------------------------------------------------------------------
void PixTestBB2Map::doTest() {

  cacheDacs();
  PixTest::update();
  bigBanner(Form("PixTestBB2Map::doTest() Ntrig = %d, VcalS = %d, PlWidth = %d", fParNtrig, fParVcalS, fParPlWidth));

  fDirectory->cd();

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  int flag(FLAG_CALS);
  fApi->setDAC("ctrlreg", 4);     // high range
  fApi->setDAC("vcal", fParVcalS);    

  //int result(7);

  // enable roc

  uint16_t FLAGS0 = flag | FLAG_FORCE_MASKED | FLAG_FORCE_SERIAL;

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 

  // loop over roc-s
  
  for (unsigned int idx = 0; idx < rocIds.size(); ++idx){

    unsigned int rocId = getIdFromIdx(idx);

    vector<pair<uint8_t, vector<pixel> > > results_bbmap;  
    results_bbmap = fApi->getEfficiencyVsDAC("VthrComp", 0, 250, FLAGS0, fParNtrig); 

    // display the quantities:
    int ic, ir, iroc; 
    double val;

    // Define the 2D-Histogram
    // "N_DAC%02i_CR%i_Vcal%03i_ma
    // Form("%s_%i_%s", "N_DAC12_CR4_Vcal",fParVcalS,"map")

  
    TH2D *h22 = new TH2D(Form("h22_C%d", rocId),Form("h22_C%d",rocId), 4160, -0.5, 4160 - 0.5, 250, -0.5, 249.5 );
    h22->SetTitle(Form("%s_%i_%s", "N_DAC12_CR4_Vcal",fParVcalS,"map"));
    fHistOptions.insert(make_pair(h22, "colz")); 
    setTitles(h22, "Pixel 80*ic+ir", "VthrComp");
    
    for (unsigned int idac = 0; idac < results_bbmap.size(); ++idac) {
      int dac = results_bbmap[idac].first; 
      for (unsigned int ipix = 0; ipix < results_bbmap[idac].second.size(); ++ipix) {
	ic =   results_bbmap[idac].second[ipix].column(); 
	ir =   results_bbmap[idac].second[ipix].row(); 
	iroc = results_bbmap[idac].second[ipix].roc(); 
	if (ic > 51 || ir > 79) {
	  continue;
	}
	val =  results_bbmap[idac].second[ipix].value();
	h22->Fill(80 * ic + ir, dac, val);
      //cout << "dac: " << dac << " ic: " << ic << " ir: " << ir << " iroc: "  << iroc << " val: "  << val << " "<< 80*ic + ir << endl;
      }
    }
  
    fHistList.push_back(h22);

    // Find from the previous scan [h22] the missing bumps
    // output save in:

    TH1D *h12 = new TH1D( Form("CalsVthrPlateauWidth_C%d",rocId),
			  "Width of VthrComp plateau for cals;width of VthrComp plateau for cals [DAC];pixels",
			  101, -0.5, 100.5 );
    
    TH2D *h24 = new TH2D( Form("BBtestMap_C%d",rocId),
			  "BBtest map;col;row;max responses",
			  52, -0.5, 51.5, 80, -0.5, 79.5 );
    fHistOptions.insert(make_pair(h24, "colz"));
    setTitles(h24, "col", "row");
    
    int nbinx = h22->GetNbinsX(  );
    int nbiny = h22->GetNbinsY(  );
    
    int nActive;
    int nMissing;
    int nIneff;

    nActive = 0;
    nMissing = 0;
    nIneff = 0;
  
    int ibinCenter;

    // Find Plateau
  
    for( int ibin = 1; ibin <= nbinx; ibin++ ) {
      
      ibinCenter = h22->GetXaxis(  )->GetBinCenter( ibin );

      // Find first the maximum
      int imax;
      imax = 0;
      for( int j = 1; j <= nbiny; ++j ) {
	int cnt = h22->GetBinContent( ibin, j );
	// Find Maximum
	if( cnt > imax )
	  imax = cnt;
      }
    
      if( imax <  fParNtrig / 2 ) {
	++nIneff;
	cout << "Dead pixel at raw col: " << ibinCenter % 80
	     << " " << ibinCenter / 80 << endl;
      }
      else {
      
	// Search for the Plateau
      
	int iEnd = 0;
	int iBegin = 0;
	
	// use the maximum response to localize the Plateau
	for( int jbin = 0; jbin <= nbiny; jbin++ ) {
	  int cnt = h22->GetBinContent( ibin, jbin );
	  // Find Plateau
	  if( cnt >= imax / 2 ) {
	    iEnd = jbin; // end of plateau
	    if( iBegin == 0 )
	      iBegin = jbin; // begin of plateau
	  }
	}

	// cout << "Bin: " << ibin << " Plateau Begins and End in  " << iBegin << " - " << iEnd << endl;
	// narrow plateau is from noise

	h12->Fill( iEnd - iBegin );
	// plateau Size
	if( iEnd - iBegin < fParPlWidth ) {

	  nMissing++;
	  cout << "[Missing Bump at raw col:] " << ibinCenter% 80 << " " << ibinCenter / 80 << endl;
	  // with weight 2 to draw it as red
	  h24->Fill( ibinCenter / 80, ibinCenter % 80, 2 );
	}
	else {
	  //h24->Fill( ibinCenter / 80, ibinCenter % 80, imax );
	  // draw this pixel green
	  h24->Fill( ibinCenter / 80, ibinCenter % 80, 1 );
	  nActive++;
	} // plateau width
      } // active imax
    } // x-bins

    // define the color
    h24->SetMaximum(2);
    h24->SetMinimum(0);

    fHistList.push_back(h12);
    fHistList.push_back(h24);
  
    h22->Draw(getHistOption(h22).c_str());

    fDisplayedHist = find(fHistList.begin(), fHistList.end(), h22);
  } // end of loop over rocs

  restoreDacs();
  PixTest::update(); 
  
  LOG(logINFO) << "PixTestBB2Map::doTest() done";

}


