#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include "PixTestSetup.hh"
#include "log.h"

#include <TH2.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestSetup)

// ----------------------------------------------------------------------
PixTestSetup::PixTestSetup(PixSetup *a, std::string name) : PixTest(a, name), fParNtrig(-1), fParVcal(-1) {
  PixTest::init();
  init(); 
  LOG(logDEBUG) << "PixTestSetup ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestSetup::PixTestSetup() : PixTest() {
  LOG(logDEBUG) << "PixTestSetup ctor()";
}

// ----------------------------------------------------------------------
bool PixTestSetup::setParameter(string parName, string sval) {
  bool found(false);
  string::size_type s1;
  string str1, str2;
  int pixc, pixr;
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 

      LOG(logDEBUG) << "  ==> parName: " << parName;
      LOG(logDEBUG) << "  ==> sval:    " << sval;
      if (!parName.compare("Ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  ==> setting fParNtrig to " << fParNtrig; 
	setToolTips();
      }

      if (!parName.compare("Deser160Lo")) {
	fParDeser160Lo = atoi(sval.c_str());
	LOG(logDEBUG) << "  setting fParDeser160Lo  ->" << fParDeser160Lo
		     << "<- from sval = " << sval;
      }
      if (!parName.compare("Deser160Hi")) {
	fParDeser160Hi = atoi(sval.c_str());
	LOG(logDEBUG) << "  setting fParDeser160Hi  ->" << fParDeser160Hi
		     << "<- from sval = " << sval;
      }

      if (!parName.compare("ClkLo")) {
	fParClkLo = atoi(sval.c_str());
	LOG(logDEBUG) << "  setting fParClkLo  ->" << fParClkLo
		     << "<- from sval = " << sval;
      }
      if (!parName.compare("ClkHi")) {
	fParClkHi = atoi(sval.c_str());
	LOG(logDEBUG) << "  setting fParClkHi  ->" << fParClkHi
		     << "<- from sval = " << sval;
      }

      if (!parName.compare("Ntests")) {
	fParNtests = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  ==> setting fParNtests to " << fParNtests; 
	setToolTips();
      }
      if (!parName.compare("Vcal")) {
	fParVcal = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  ==> setting fParVcal to " << fParVcal; 
	setToolTips();
      }
      if (!parName.compare("PIX")) {
	s1 = sval.find( "," );
	if (string::npos != s1) {
	  str1 = sval.substr(0, s1);
	  pixc = atoi(str1.c_str());
	  str2 = sval.substr(s1+1);
	  pixr = atoi(str2.c_str());
	  fPIX.push_back(make_pair(pixc, pixr));
	}
	else {
	  fPIX.push_back( make_pair(-1, -1));
	}
      }
      break;
    }
  }
  return found; 
}


// ----------------------------------------------------------------------
void PixTestSetup::init() {
  LOG(logDEBUG) << "PixTestSetup::init()";

  setToolTips();
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 

}


// ----------------------------------------------------------------------
void PixTestSetup::setToolTips() {
  fTestTip    = string(Form("scan testboard parameter settings and check for valid readout\n")
		       + string("TO BE IMPLEMENTED!!"))
    ;
  fSummaryTip = string("summary plot to be implemented")
    ;
}


// ----------------------------------------------------------------------
void PixTestSetup::bookHist(string name) {

  fDirectory->cd(); 
  LOG(logDEBUG) << "nothing done with " << name;

}


//----------------------------------------------------------
PixTestSetup::~PixTestSetup() {
  LOG(logDEBUG) << "PixTestSetup dtor";
  std::list<TH1*>::iterator il; 
  fDirectory->cd(); 
  for (il = fHistList.begin(); il != fHistList.end(); ++il) {
    LOG(logDEBUG) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory); 
    (*il)->Write(); 
  }
}


// ----------------------------------------------------------------------
void PixTestSetup::doTest() {
  fDirectory->cd();
  LOG(logINFO) << "PixTestSetup::doTest() ntrig = " << fParNtrig << " and ntests = " << fParNtests;
  //FIXME clearHist(); 

  bookHist("bla");
 
  vector<pair<string, double> > power_settings = fPixSetup->getConfigParameters()->getTbPowerSettings();
  vector<pair<uint16_t, uint8_t> > pg_setup = fPixSetup->getConfigParameters()->getTbPgSettings();;


  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);

  vector<uint8_t> vthrcomp, ctrlreg, vcal; 

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    vthrcomp.push_back(fApi->_dut->getDAC(iroc, "vthrcomp")); 
    ctrlreg.push_back(fApi->_dut->getDAC(iroc, "ctrlreg")); 
    vcal.push_back(fApi->_dut->getDAC(iroc, "vcal")); 

    fApi->setDAC("vthrcomp", 20, iroc); 
    fApi->setDAC("ctrlreg", 4, iroc); 
    fApi->setDAC("vcal", 250, iroc); 

    for (unsigned int i = 0; i < fPIX.size(); ++i) {
      if (fPIX[i].first > -1)  {
	fApi->_dut->testPixel(fPIX[i].first, fPIX[i].second, true);
	fApi->_dut->maskPixel(fPIX[i].first, fPIX[i].second, false);
      } 
    }
  }


  int offset[] = {-1, 0, 1};
  int NCLK(fParClkHi-fParClkLo+1), 
    NOFF(3), 
    NDESER(fParDeser160Hi-fParDeser160Lo+1);
  
  vector<TH2D *> h3; 
  TH2D *h2(0); 
  for (int ideser = fParDeser160Lo; ideser < fParDeser160Lo+NDESER; ++ideser) {
    cout << "creating TH2D " << Form("setupEff_deser%d", ideser) << endl;
    h2 = new TH2D(Form("setupEff_deser%d", ideser), Form("setupEff_deser%d", ideser), NOFF, -1., 2., NCLK, fParClkLo, fParClkHi);
    h3.push_back(h2); 
    setTitles(h2, "offset wrt fixed conventions", "CLK"); 

    fHistList.push_back(h3[ideser-fParDeser160Lo]); 
    fHistOptions.insert(make_pair(h3[ideser-fParDeser160Lo], "colz")); 
    
  }

  unsigned int npix(0);
  for (unsigned int iroc = 0; iroc < fApi->_dut->getNEnabledRocs(); ++iroc) {
    npix += fApi->_dut->getNEnabledPixels(iroc); 
  }

  LOG(logINFO) << " total enabled ROCs: " << fApi->_dut->getNEnabledRocs() 
	       << " with total enabled pixels: " << npix;

  vector<pixel> results;
  for (int ideser = fParDeser160Lo; ideser < fParDeser160Lo+NDESER; ++ideser) {
    for (int iclk = fParClkLo; iclk < fParClkLo+NCLK; ++iclk) {
      for (int ioffset = 0; ioffset < NOFF; ++ioffset) {
	cout << "xxx> starting loop point: " << " deser160phase = " << ideser << " iclk = " << iclk << " offset = " << offset[ioffset] << endl;
	fPixSetup->getConfigParameters()->setTbParameter("clk", iclk); 
	fPixSetup->getConfigParameters()->setTbParameter("ctr", iclk); 
	fPixSetup->getConfigParameters()->setTbParameter("sda", iclk+15+offset[ioffset]); 
	fPixSetup->getConfigParameters()->setTbParameter("tin", iclk+5+offset[ioffset]); 
	fPixSetup->getConfigParameters()->setTbParameter("deser160phase", ideser); 
	
	vector<pair<string, uint8_t> > sig_delays = fPixSetup->getConfigParameters()->getTbSigDelays();
	LOG(logDEBUG) << fPixSetup->getConfigParameters()->dumpParameters(sig_delays); 
	
	fApi->initTestboard(sig_delays, power_settings, pg_setup);
	
	for (int i = 0; i < fParNtests; ++i) {
	  results.clear();
	  results = fApi->getEfficiencyMap(0, fParNtrig);
	  cout << " test " << i << " results.size() = " << results.size() << endl;
	  if (npix == results.size()) h3[ideser-fParDeser160Lo]->Fill(offset[ioffset], iclk); 
	  if (0 == results.size()) break; // bail out for failures
	}
      }
    }
  }

  h3[0]->Draw("colz");
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h3[0]);
  PixTest::update(); 

  for (int ideser = 0; ideser < NDESER; ++ideser) {
    for (int ix = 0; ix < h3[ideser]->GetNbinsX(); ++ix) {
      for (int iy = 0; iy < h3[ideser]->GetNbinsY(); ++iy) {
	if (fParNtests == h3[ideser]->GetBinContent(ix+1, iy+1)) {
	  LOG(logINFO) << " Found 100% PixelAlive success rate for deser160phase = " << fParDeser160Lo+ideser 
		       << " CLK = " << h3[ideser]->GetYaxis()->GetBinCenter(iy+1) 
		       << " OFFSET = " << h3[ideser]->GetXaxis()->GetBinCenter(ix+1);
	}
      }
    }
  }

  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    LOG(logDEBUG) << "resetting vthrcomp = " << static_cast<int>(vthrcomp[iroc]) << " for ROC " << iroc; 
    fApi->setDAC("vthrcomp", vthrcomp[iroc], iroc); 
    LOG(logDEBUG) << "resetting ctrlreg = " << static_cast<int>(ctrlreg[iroc]) << " for ROC " << iroc; 
    fApi->setDAC("ctrlreg", ctrlreg[iroc], iroc); 
    LOG(logDEBUG) << "resetting vcal = " << static_cast<int>(vcal[iroc]) << " for ROC " << iroc; 
    fApi->setDAC("vcal", vcal[iroc], iroc); 
  }

  LOG(logINFO) << "PixTestSetup::doTest() done";
}
