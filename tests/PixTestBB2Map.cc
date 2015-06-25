// -- author: A.Vargas, D.Pitzl
// Bump Bonding tests, 
// sending test-pulse through the sensor (cals), a scan of vthrcomp for all pixels is performed. 
// The width of the plateau is used to distinguish between 
// good/dead/bad bump-bonding


#include <sstream>   // parsing
#include <algorithm>  // std::find

#include "TStopwatch.h"
#include <TMarker.h>

#include "PixTestBB2Map.hh"
#include "PixUtil.hh"
#include "log.h"
#include "constants.h"   // roctypes

using namespace std;
using namespace pxar;

ClassImp(PixTestBB2Map)

//------------------------------------------------------------------------------
PixTestBB2Map::PixTestBB2Map(PixSetup *a, std::string name): PixTest(a, name), fTargetIa(-1), fParNtrig(10), fParVcalS(250), fParPlWidth(35) {
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

  string str1, str2; 
  string::size_type s1;
  int pixc, pixr;

  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (uint32_t i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());

      stringstream s(sval);
      
      // target ia

      if (!parName.compare("targetia")) {
	fTargetIa = atoi(sval.c_str());  // [mA/ROC]
	LOG(logDEBUG) << "setting fTargetIa    = " << fTargetIa << " mA/ROC";
      }
      
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
      if (!parName.compare( "plwidth"))  { 
	s >> fParPlWidth; 
	setToolTips();
	return true;
      }

      // pixel
      if (!parName.compare("pix") || !parName.compare("pix1") ) {
	s1 = sval.find(",");
	if (string::npos != s1) {
	  str1 = sval.substr(0, s1);
	  pixc = atoi(str1.c_str());
	  str2 = sval.substr(s1+1);
	  pixr = atoi(str2.c_str());
	  fPIX.push_back( make_pair(pixc, pixr) );
	}
      }
      break;

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

  TStopwatch t;
  cacheDacs();
  
  // Save initial Dac values 

  vector<uint8_t> v_iniVana     = getDacs("vana");
  vector<uint8_t> v_iniVcal     = getDacs("vcal"); 
  vector<uint8_t> v_iniVthrComp = getDacs("vthrcomp"); 
  vector<uint8_t> v_iniCreg     = getDacs("ctrlreg"); 
  vector<uint8_t> v_iniCalDel   = getDacs("caldel"); 
  
  // Begin now the Test

  PixTest::update();
  bigBanner(Form("PixTestBB2Map::doTest() Ntrig = %d, VcalS = %d, PlWidth = %d", fParNtrig, fParVcalS, fParPlWidth));

  fDirectory->cd();
 
  // set setVana

  //  std::cout << "Setting Vana " << std::endl;
  setVana();

  // find working point for tornado distribution but pulsing with cals!
  
  //  std::cout << "Setting caldel & vthrcomp for cals" << std::endl;

  setVthrCompCalDelForCals();

  // BB Threshold maps
  //  std::cout << "Pulsing cals for Bump Bonding Tests " << std::endl;

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  maskPixels();

  int flag(FLAG_CALS);
  fApi->setDAC("ctrlreg", 4);     // high range
  fApi->setDAC("vcal", fParVcalS);    

  //int result(7);

  // enable roc

  uint16_t FLAGS0 = flag | FLAG_FORCE_MASKED;

  // Define the histograms to be stored

  TH2D*              result_h22[16];
  vector<int>        index_h22;
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 

  // step Size

  int step;
  step=2;
  int dacMax = 255;
  int dacMin = 0;

  int nstp = (dacMax - dacMin) / step + 1;


  for (unsigned int idx = 0; idx < rocIds.size(); ++idx){
    unsigned int rocId = getIdFromIdx(idx);

    result_h22[rocId] = bookTH2D(Form("N_VthrComp_C%d", rocId), Form("N_VthrComp_C%d", rocId), 
				 4160, -0.5, 4160 - 0.5, nstp, dacMin - 0.5*step, dacMax + 0.5*step);
    setTitles(result_h22[rocId], "Pixel 80*ic+ir", "VthrComp");
  }

  // get the efficiency map for all available rocs  

  vector<pair<uint8_t, vector<pixel> > > results_bbmap;  
  
  results_bbmap = fApi->getEfficiencyVsDAC("VthrComp",step, dacMin, dacMax, FLAGS0, fParNtrig);
  //results_bbmap = fApi->getEfficiencyVsDAC("VthrComp",step, 0, 250, FLAGS0, fParNtrig); 

  // display the quantities:
  int ic, ir, iroc; 
  double val;

  // store the output in the corresponding roc-histogram    
    
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
      result_h22[iroc]->Fill(80 * ic + ir, dac, val);
      //result_h22[iroc]->Fill(80 * ic + ir, dac, val);
      //cout << "dac: " << dac << " ic: " << ic << " ir: " << ir << " iroc: "  << iroc << " val: "  << val << " "<< 80*ic + ir << endl;
    }    
  }
  
  
  // Find from the previous scan [h22] the missing bumps
  // output save in:

  int nMissingPerRoc[16]={16*0};
  string bb2mapString("");
  
  for (unsigned int idx = 0; idx < rocIds.size(); ++idx){

    unsigned int rocId = idx;
    //    cout << idx << " rocId: " << rocId << endl;
    fHistList.push_back(result_h22[rocId]);

    // correct binning
    int maxPlSize = 110;
    int minPlSize = 0;    
    int nstpPlSize = (maxPlSize - minPlSize) / step + 1;

    TH1D* h12 = bookTH1D(Form("CalsVthrPlateauWidth_C%d",rocId),Form("CalsVthrPlateauWidth_C%d",rocId),nstpPlSize, minPlSize - 0.5*step, maxPlSize + 0.5*step );

    TH2D* h24 = bookTH2D(Form("BBtestMap_C%d",rocId),Form("BBtestMap_C%d",rocId), 52, -0.5, 51.5, 80, -0.5, 79.5 );

    setTitles(h24, "col", "row");
    
    int nbinx = (result_h22[rocId])->GetNbinsX(  ); // pixel direction
    int nbiny = (result_h22[rocId])->GetNbinsY(  ); // dac Scan
    
    int nActive;
    int nMissing;
    int nIneff;
  
    nActive = 0;
    nMissing = 0;
    nIneff = 0;
    
    int ibinCenter;
      
  // Find Plateau
  
    for( int ibin = 1; ibin <= nbinx; ibin++ ) {
    
      ibinCenter = (result_h22[rocId])->GetXaxis(  )->GetBinCenter( ibin );
    
      // Find first the maximum
      int imax;
      imax = 0;
      for( int j = 1; j <= nbiny; ++j ) {
	int cnt = (result_h22[rocId])->GetBinContent( ibin, j );
	// Find Maximum
	if( cnt > imax )
	  imax = cnt;
      }
      
      if( imax <  fParNtrig / 2 ) {
	++nIneff;
	// 	cout << "Dead pixel at raw col: " << ibinCenter % 80
	// 	     << " " << ibinCenter / 80 << endl;
      }
      else {
	
	// Search for the Plateau
	
	int iEnd = 0;
	int iBegin = 0;
      
	float beginCont = 0.;
	float endCont = 0.;	

	// use the maximum response to localize the Plateau
	for( int jbin = 0; jbin <= nbiny; jbin++ ) {
	  int cnt = (result_h22[rocId])->GetBinContent( ibin, jbin );
	  // Find Plateau
	  if( cnt >= imax / 2 ) {
	    iEnd = jbin; // end of plateau
	    if( iBegin == 0 )
	      iBegin = jbin; // begin of plateau
	  }
	}

	endCont = (result_h22[rocId])->GetYaxis()->GetBinUpEdge(iEnd);
	beginCont = (result_h22[rocId])->GetYaxis()->GetBinUpEdge(iBegin);

	// narrow plateau is from noise
	
	//h12->Fill( iEnd - iBegin );
	h12->Fill( endCont- beginCont );
	// plateau Size
	if( endCont- beginCont < fParPlWidth ) {
	
	  nMissing++;
	  //	  cout << "[Missing Bump at raw col:] " << ibinCenter% 80 << " " << ibinCenter / 80 << endl;
	  // with weight 2 to draw it as red
	  h24->Fill( ibinCenter / 80, ibinCenter % 80, 2 );
	  nMissingPerRoc[rocId]++;
	}
	else {
	  //h24->Fill( ibinCenter / 80, ibinCenter % 80, imax );
	  // draw this pixel green
	  h24->Fill( ibinCenter / 80, ibinCenter % 80, 1 );
	  nActive++;
	} // plateau width
	
      } // active imax
    } // x-bins

    // display statistic:    
    //cout << "Roc " << rocId << " has "  << nMissingPerRoc[rocId] <<  " missing bumps" << endl;
    bb2mapString += Form(" %4d", nMissingPerRoc[rocId]);

    // define the color
    h24->SetMaximum(2);
    h24->SetMinimum(0);
    
    fHistList.push_back(h12);
    fHistList.push_back(h24);

    (result_h22[rocId])->Draw(getHistOption(result_h22[rocId]).c_str());

  }
  
  fDisplayedHist = find(fHistList.begin(), fHistList.end(),result_h22[0]);

  // Restore initial DAC values 

  setDacs("vana",v_iniVana);
  setDacs("vcal", v_iniVcal); 
  setDacs("ctrlreg", v_iniCreg); 
  setDacs("caldel", v_iniCalDel); 
  setDacs("vthrcomp", v_iniVthrComp); 

  PixTest::update(); 

  int seconds = t.RealTime();

  LOG(logINFO) << "Missing Bumps:   " << bb2mapString;
  LOG(logINFO) << "PixTestBB2Map::doTest() done"
	       << (fNDaqErrors>0? Form(" with %d decoding errors: ", static_cast<int>(fNDaqErrors)):"") 
	       << ", duration: " << seconds << " seconds";

  dutCalibrateOff();
}

// ----------------------------------------------------------------------


void PixTestBB2Map::setVana() {
  cacheDacs();
  fDirectory->cd();
  PixTest::update(); 
  banner(Form("PixTestBB2Map::setVana() target Ia = %d mA/ROC", fTargetIa)); 

  fApi->_dut->testAllPixels(false);

  vector<uint8_t> vanaStart;
  vector<double> rocIana;

  // -- cache setting and switch off all(!) ROCs
  int nRocs = fApi->_dut->getNRocs(); 
  for (int iroc = 0; iroc < nRocs; ++iroc) {
    vanaStart.push_back(fApi->_dut->getDAC(iroc, "vana"));
    rocIana.push_back(0.); 
    fApi->setDAC("vana", 0, iroc);
  }
  
  double i016 = fApi->getTBia()*1E3;

  // FIXME this should not be a stopwatch, but a delay
  TStopwatch sw;
  sw.Start(kTRUE); // reset
  do {
    sw.Start(kFALSE); // continue
    i016 = fApi->getTBia()*1E3;
  } while (sw.RealTime() < 0.1);

  // subtract one ROC to get the offset from the other Rocs (on average):
  double i015 = (nRocs-1) * i016 / nRocs; // = 0 for single chip tests
  LOG(logDEBUG) << "offset current from other " << nRocs-1 << " ROCs is " << i015 << " mA";

  // tune per ROC:

  const double extra = 0.1; // [mA] besser zu viel als zu wenig 
  const double eps = 0.25; // [mA] convergence
  const double slope = 6; // 255 DACs / 40 mA

  for (int roc = 0; roc < nRocs; ++roc) {
    if (!selectedRoc(roc)) {
      LOG(logDEBUG) << "skipping ROC idx = " << roc << " (not selected) for Vana tuning"; 
      continue;
    }
    int vana = vanaStart[roc];
    fApi->setDAC("vana", vana, roc); // start value

    double ia = fApi->getTBia()*1E3; // [mA], just to be sure to flush usb
    sw.Start(kTRUE); // reset
    do {
      sw.Start(kFALSE); // continue
      ia = fApi->getTBia()*1E3; // [mA]
    } while (sw.RealTime() < 0.1);

    double diff = fTargetIa + extra - (ia - i015);

    int iter = 0;
    LOG(logDEBUG) << "ROC " << roc << " iter " << iter
		 << " Vana " << vana
		 << " Ia " << ia-i015 << " mA";

    while (TMath::Abs(diff) > eps && iter < 11 && vana > 0 && vana < 255) {

      int stp = static_cast<int>(TMath::Abs(slope*diff));
      if (stp == 0) stp = 1;
      if (diff < 0) stp = -stp;

      vana += stp;

      if (vana < 0) {
	vana = 0;
      } else {
	if (vana > 255) {
	  vana = 255;
	}
      }

      fApi->setDAC("vana", vana, roc);
      iter++;

      sw.Start(kTRUE); // reset
      do {
	sw.Start(kFALSE); // continue
	ia = fApi->getTBia()*1E3; // [mA]
      }
      while( sw.RealTime() < 0.1 );

      diff = fTargetIa + extra - (ia - i015);

      LOG(logDEBUG) << "ROC " << setw(2) << roc
		   << " iter " << setw(2) << iter
		   << " Vana " << setw(3) << vana
		   << " Ia " << ia-i015 << " mA";
    } // iter

    rocIana[roc] = ia-i015; // more or less identical for all ROCS?!
    vanaStart[roc] = vana; // remember best
    fApi->setDAC( "vana", 0, roc ); // switch off for next ROC

  } // rocs

  TH1D *hsum = bookTH1D("VanaSettings", "Vana per ROC", nRocs, 0., nRocs);
  setTitles(hsum, "ROC", "Vana [DAC]"); 
  hsum->SetStats(0);
  hsum->SetMinimum(0);
  hsum->SetMaximum(256);
  fHistList.push_back(hsum);

  TH1D *hcurr = bookTH1D("Iana", "Iana per ROC", nRocs, 0., nRocs);
  setTitles(hcurr, "ROC", "Vana [DAC]"); 
  hcurr->SetStats(0); // no stats
  hcurr->SetMinimum(0);
  hcurr->SetMaximum(30.0);
  fHistList.push_back(hcurr);


  restoreDacs();
  for (int roc = 0; roc < nRocs; ++roc) {
    // -- reset all ROCs to optimum or cached value
    fApi->setDAC( "vana", vanaStart[roc], roc );
    LOG(logDEBUG) << "ROC " << setw(2) << roc << " Vana " << setw(3) << int(vanaStart[roc]);
    // -- histogramming only for those ROCs that were selected
    if (!selectedRoc(roc)) continue;
    hsum->Fill(roc, vanaStart[roc] );
    hcurr->Fill(roc, rocIana[roc]); 
  }
  
  double ia16 = fApi->getTBia()*1E3; // [mA]

  sw.Start(kTRUE); // reset
  do {
    sw.Start(kFALSE); // continue
    ia16 = fApi->getTBia()*1E3; // [mA]
  }
  while( sw.RealTime() < 0.1 );


  hsum->Draw();
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), hsum);
  PixTest::update();

  LOG(logINFO) << "PixTestBB2Map::setVana() done, Module Ia " << ia16 << " mA = " << ia16/nRocs << " mA/ROC";

}

// ----------------------------------------------------------------------

void PixTestBB2Map::setVthrCompCalDelForCals() {
  
  uint16_t FLAGS = FLAG_FORCE_MASKED | FLAG_CALS;

  cacheDacs();
  fDirectory->cd();
  PixTest::update(); 
  banner(Form("PixTestBB2Map::setVthrCompCalDel()")); 

  string name1("bb2VthrCompCalDel");

  fApi->setDAC("CtrlReg", 4);   
  fApi->setDAC("Vcal", 250); 

  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  //   std::cout << "at the beginning the trigger: " <<  fParNtrig << std::endl;

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  vector<pair<uint8_t, pair<uint8_t, vector<pixel> > > >  rresults;

  TH1D *h1(0); 
  h1 = bookTH1D(Form("bb2CalDel"), Form("bb2CalDel"), rocIds.size(), 0., rocIds.size()); 
  h1->SetMinimum(0.); 
  h1->SetDirectory(fDirectory); 
  setTitles(h1, "ROC", "CalDel DAC"); 

  TH2D *h2(0); 
  TH1D *h_optimize(0);

  vector<int> calDel(rocIds.size(), -1); 
  vector<int> vthrComp(rocIds.size(), -1); 
  vector<int> calDelE(rocIds.size(), -1);
  
  int ip = 0; 
  fApi->_dut->testPixel(fPIX[ip].first, fPIX[ip].second, true);
  fApi->_dut->maskPixel(fPIX[ip].first, fPIX[ip].second, false);


  map<int, TH2D*> maps;
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    
    h2 = bookTH2D(Form("%s_c%d_r%d_C%d", name1.c_str(), fPIX[ip].first, fPIX[ip].second, rocIds[iroc]), 
		  Form("%s_c%d_r%d_C%d", name1.c_str(), fPIX[ip].first, fPIX[ip].second, rocIds[iroc]), 
		  255, 0., 255., 255, 0., 255.); 
    fHistOptions.insert(make_pair(h2, "colz")); 
    h2->SetMinimum(0.); 
    h2->SetDirectory(fDirectory); 
    setTitles(h2, "CalDel", "VthrComp"); 
    maps.insert(make_pair(rocIds[iroc], h2)); 
  }
   
  bool done = false;
  while (!done) {
    rresults.clear();     
    int cnt(0); 
    try{
      rresults = fApi->getEfficiencyVsDACDAC("caldel", 0, 255, "vthrcomp", 0, 255, FLAGS, fParNtrig);
      done = true;
    } catch(DataMissingEvent &e){
      LOG(logCRITICAL) << "problem with readout: "<< e.what() << " missing " << e.numberMissing << " events"; 
      ++cnt;
      if (e.numberMissing > 10) done = true; 
    } catch(pxarException &e) {
      LOG(logCRITICAL) << "pXar execption: "<< e.what(); 
      ++cnt;
    }
    done = (cnt>5) || done;
  }
    
  fApi->_dut->testPixel(fPIX[ip].first, fPIX[ip].second, false);
  fApi->_dut->maskPixel(fPIX[ip].first, fPIX[ip].second, true);
    
  for (unsigned i = 0; i < rresults.size(); ++i) {
    pair<uint8_t, pair<uint8_t, vector<pixel> > > v = rresults[i];
    int idac1 = v.first; 
    pair<uint8_t, vector<pixel> > w = v.second;      
    int idac2 = w.first;
    vector<pixel> wpix = w.second;
    for (unsigned ipix = 0; ipix < wpix.size(); ++ipix) {
      if (wpix[ipix].value() >= fParNtrig/2){
	maps[wpix[ipix].roc()]->Fill(idac1, idac2, wpix[ipix].value());
      }
    }
  }

  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
  
    h2 = maps[rocIds[iroc]];
    
    // modification for vthrcomp:
    // find efficiency plateau of dac2 at dac1Mean:

    int nm = 0;
    int i0 = 0;
    int i9 = 0;    

    //double dac1Mean =  h2->GetMean( 1 );
    int dac1Mean = int ( h2->GetMean( 1 ) );
    int i1 = h2->GetXaxis()->FindBin( dac1Mean);

    //     cout << "Mean ist at Bin: " << i1 << endl;

    // Optimize for Vthrcomp

    h_optimize =  bookTH1D(Form("VthrComp-Optimized_C%d",iroc), Form("VthrComp-Optimized_C%d",iroc),255, 0., 255); 

    for( int j = 0; j < 255; ++j ) {

      int cnt = h2->GetBinContent( i1, j+1 ); // ROOT bins start at 1
      
      // Fill histogram
      h_optimize->Fill(j, cnt);

      //cout << "j " << j <<  " cnt " << cnt << endl;
      if( cnt > nm ) {
	nm = cnt;
	i0 = j; // begin of plateau
      }
      if( cnt >= nm ) {
	i9 = j; // end of plateau
      }      

    }

    fHistList.push_back(h_optimize);    
    //     std::cout << "Number of bins x- " 
    // 	      << h2->GetXaxis()->GetNbins() 
    // 	      << " mean: " 
    // 	      << h2->GetMean(1) 
    // 	      << " at bin: " << i1 << endl; 
    //     std::cout << "Number of bins y- " 
    // 	      << h2->GetYaxis()->GetNbins() 
    // 	      << "mean: "
    // 	      << h2->GetMean(2) << endl;
    
    //    cout << " plateau from " << i0 << " to " << i9 << endl;

    int dac2Best = i0 + ( i9 - i0 ) / 4;

    // use the result of our test

    calDelE[iroc] = static_cast<int>(0.);
    calDel[iroc] = static_cast<int>(h2->GetMean( 1 ));
    vthrComp[iroc] = dac2Best;
    
    h1->SetBinContent(rocIds[iroc]+1, calDel[iroc]); 
    h1->SetBinError(rocIds[iroc]+1, calDelE[iroc]); 
    LOG(logDEBUG) << "CalDel: " << calDel[iroc] << " +/- " << calDelE[iroc];
    
    h2->Draw(getHistOption(h2).c_str());
    TMarker *tm = new TMarker(); 
    tm->SetMarkerSize(1);
    tm->SetMarkerStyle(21);
    tm->DrawMarker(calDel[iroc], vthrComp[iroc]); 
    PixTest::update(); 
    
    fHistList.push_back(h2); 
  }

  fHistList.push_back(h1); 
  //fHistList.push_back(h_optimize);

  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h2);
  PixTest::update(); 

  restoreDacs();
  string caldelString(""), vthrcompString(""); 

  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    fApi->setDAC("CalDel", calDel[iroc], rocIds[iroc]);
    caldelString += Form(" %4d", calDel[iroc]); 
    fApi->setDAC("VthrComp", vthrComp[iroc], rocIds[iroc]);
    vthrcompString += Form(" %4d", vthrComp[iroc]); 
  }


  // -- summary printout
  LOG(logINFO) << "PixTestBB2Map::setVthrCompCalDel() done";
  LOG(logINFO) << "CalDel:   " << caldelString;
  LOG(logINFO) << "VthrComp: " << vthrcompString;

  // setting back the Flag - no cals!
  //FLAGS = FLAG_FORCE_SERIAL | FLAG_FORCE_MASKED;


}


// ----------------------------------------------------------------------
