#include <stdlib.h>  // atof, atoi
#include <algorithm> // std::find
#include <sstream>   // parsing

#include "PixTestPh.hh"
#include "PixUtil.hh"
#include "log.h"

#include <TStyle.h>
#include <TH2.h>

using namespace std;
using namespace pxar;

ClassImp(PixTestPh)

//------------------------------------------------------------------------------
PixTestPh::PixTestPh( PixSetup *a, std::string name ) :  PixTest(a, name),
  fParDAC("nada"),
  fParDacVal(70),
  fVcalLow(-1),
  fVcalHigh(-1),
  fPhScaleMin(-1),
  fPhOffsetMin(-1),
  fPhMin(-1),
  fPhMax(-1)
{
  PixTest::init();
  init();
}


//------------------------------------------------------------------------------
PixTestPh::PixTestPh() : PixTest() {
  //  LOG(logDEBUG) << "PixTestPh ctor()";
}

//------------------------------------------------------------------------------
bool PixTestPh::setParameter(string parName, string sval) {
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
	fNtrig = atoi( sval.c_str() );
	LOG(logDEBUG) << "  setting fNtrig  ->" << fNtrig
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


      if (!parName.compare("vcallow")) {
	fVcalLow = atoi(sval.c_str());
      }

      if (!parName.compare("vcalhigh")) {
	fVcalHigh = atoi(sval.c_str());
      }

      if (!parName.compare("phmin")) {
	fPhMin = atoi(sval.c_str());
      }

      if (!parName.compare("phmax")) {
	fPhMax = atoi(sval.c_str());
      }

      if (!parName.compare("phscalemin")) {
	fPhScaleMin = atoi(sval.c_str());
      }

      if (!parName.compare("phoffsetmin")) {
	fPhOffsetMin = atoi(sval.c_str());
      }

      break;
    }
  }
  return found;
}

//------------------------------------------------------------------------------
void PixTestPh::init() {
  fDirectory = gFile->GetDirectory(fName.c_str());
  if( !fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str());
  }
  fDirectory->cd();
}

// ----------------------------------------------------------------------
void PixTestPh::setToolTips() {
  fTestTip = string( "PH: pulse height studies for single pixels and full DUT") ;
  fSummaryTip = string("summary plot");
}

// ----------------------------------------------------------------------
string PixTestPh::toolTip(string what) {
  if (string::npos != what.find("phmap")) return string("run PH test for complete DUT ");
  if (string::npos != what.find("optimize")) return string("optimize PH scale and offset ");
  return string("nada");
}


//------------------------------------------------------------------------------
void PixTestPh::bookHist(string name) {
  LOG(logDEBUG) << "nothing done with " << name;
}

//------------------------------------------------------------------------------
PixTestPh::~PixTestPh() {
}

// ----------------------------------------------------------------------
void PixTestPh::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;

  if (!command.compare("phmap")) {
    phMap();
    return;
  }
  if (!command.compare("optimize")) {
    optimize();
    return;
  }

  if (!command.compare("dotest")) {
    doTest();
    return;
  }
  if (!command.compare("fulltest")) {
    fullTest();
    return;
  }

  LOG(logDEBUG) << "did not find command ->" << command << "<-";
}


// ----------------------------------------------------------------------
void PixTestPh::phMap() {
  cacheDacs();
  fDirectory->cd();
  PixTest::update();
  banner(Form("PixTestPh::phMap() ntrig = %d, %s = %d",
	      static_cast<int>(fNtrig), fParDAC.c_str(), static_cast<int>(fParDacVal)));

  fApi->setDAC(Form("%s", fParDAC.c_str()), fParDacVal);

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  maskPixels();

  fNDaqErrors = 0;
  vector<uint8_t> v_ctrlreg     = getDacs("ctrlreg");
  vector<TH2D*> test2 = phMaps(Form("PH_VCAL%d_CTRLREG%d", fParDacVal, v_ctrlreg[0]), fNtrig, FLAG_FORCE_MASKED);
  copy(test2.begin(), test2.end(), back_inserter(fHistList));

  TH2D *h = (TH2D*)(fHistList.back());

  if (h) {
    gStyle->SetPalette(1);
    h->Draw(getHistOption(h).c_str());
    fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
    PixTest::update();
  }


  restoreDacs();

  dutCalibrateOff();
}

//------------------------------------------------------------------------------
void PixTestPh::doTest() {
  optimize();
  phMap();
}


//------------------------------------------------------------------------------
void PixTestPh::fullTest() {
  optimize();
}


// ----------------------------------------------------------------------
void PixTestPh::optimize() {

  bigBanner(Form("PixTestPh::optimize() Ntrig = %d, vcal Low/High = %d/%d"
		 , fNtrig, fVcalLow, fVcalHigh));
  banner(Form("phmin = %d, phmax = %d", fPhMin, fPhMax));

  gStyle->SetPalette(1);
  bool verbose(false);
  cacheDacs(verbose);
  fDirectory->cd();
  PixTest::update();

  // -- set a range where we should see something
  int vcalshot(255);
  fApi->setVcalLowRange();
  fApi->setDAC("vcal", vcalshot);

  // -- determine dead pixels with these settings
  LOG(logINFO) << "PixTestPh::optimize() calling deadPixel(5,false,false)";
  vector<vector<pair<int, int> > > dead = deadPixels(5, false, false);
  for (unsigned int ic = 0; ic < dead.size(); ++ic) {
    for (unsigned int ip = 0; ip < dead[ic].size(); ++ip) {
      LOG(logINFO) << "dead pixel ROC = " << ic << ": " << dead[ic][ip].first << "/" << dead[ic][ip].second;
    }
  }
  LOG(logINFO) << "done with dead pixel determination";

  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);

  LOG(logDEBUG) << "start with shot, vcal = " << vcalshot << " (low range)";
  vector<uint8_t> v_ctrlreg = getDacs("ctrlreg");
  vector<TH2D*> shot = phMaps(Form("phshot_VCAL%d_CTRLREG%d", vcalshot, v_ctrlreg[0]), 5);
  LOG(logDEBUG) << "end with shot";
  copy(shot.begin(), shot.end(), back_inserter(fHistList));
  for (unsigned int i = 0; i < shot.size(); ++i)  {
    TH1D *h1 = distribution(shot[i], 256, 0., 256.);
    fHistList.push_back(h1);
  }

  vector<pair<pair<int, int>, double> >  minPixel1 = getMinimumPixelAndValueMinCol(shot);
  vector<pair<pair<int, int>, double> >  maxPixel1 = getMaximumPixelAndValue(shot, 255.);
  vector<pair<pair<int, int>, double> >  minPixel1Old = getMinimumPixelAndValue(shot);

  LOG(logDEBUG) << "minimum: shot " << minPixel1[0].second
		<< " at " << minPixel1[0].first.first << "/" << minPixel1[0].first.second
		<< " old at " << minPixel1Old[0].first.first << "/" << minPixel1Old[0].first.second;

  LOG(logDEBUG) << "maximum: shot " << maxPixel1[0].second
		<< " at " << maxPixel1[0].first.first << "/" << maxPixel1[0].first.second;

  TH2D *h = (TH2D*)(fHistList.back());

  if (h) {
    gStyle->SetPalette(1);
    h->Draw(getHistOption(h).c_str());
    fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
    PixTest::update();
  }

  cacheDacs();
  fDirectory->cd();
  PixTest::update();

  // -- scan for low PH pixel
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  TH1 *hLo(0), *hHi(0);
  TH2D *h2[16];
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  fPIX.clear();
  for (unsigned int i = 0; i < minPixel1.size(); ++i) fPIX.push_back(minPixel1[i].first);
  LOG(logDEBUG) << "minimum pixels for VCAL = " << fVcalLow;
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    LOG(logDEBUG) << "C" << iroc << ": " << fPIX[iroc].first << "/" << fPIX[iroc].second;
    fApi->_dut->testPixel(fPIX[iroc].first, fPIX[iroc].second, true, rocIds[iroc]);
    fApi->_dut->maskPixel(fPIX[iroc].first, fPIX[iroc].second, false, rocIds[iroc]);
  }
  fApi->setVcalLowRange();
  fApi->setDAC("vcal", fVcalLow);
  scan("phLo");

  // -- scan for high PH pixel
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  fPIX.clear();
  for (unsigned int i = 0; i < maxPixel1.size(); ++i) fPIX.push_back(maxPixel1[i].first);
  LOG(logDEBUG) << "maximum pixels for VCAL = " << fVcalHigh;
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    LOG(logDEBUG) << "C" << iroc << ": " << fPIX[iroc].first << "/" << fPIX[iroc].second;
    fApi->_dut->testPixel(fPIX[iroc].first, fPIX[iroc].second, true, rocIds[iroc]);
    fApi->_dut->maskPixel(fPIX[iroc].first, fPIX[iroc].second, false, rocIds[iroc]);
  }
  fApi->setVcalHighRange();
  fApi->setDAC("vcal", fVcalHigh);
  scan("phHi");

  // -- analysis
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    hLo = fMaps[Form("phLo_phoffset_phscale_c%d_r%d_C%d",
		     minPixel1[iroc].first.first, minPixel1[iroc].first.second, rocIds[iroc])];
    hHi = fMaps[Form("phHi_phoffset_phscale_c%d_r%d_C%d",
		     maxPixel1[iroc].first.first, maxPixel1[iroc].first.second, rocIds[iroc])];
    h2[iroc] = bookTH2D(Form("phoffset_phscale_C%d", rocIds[iroc]), Form("phoffset_phscale_C%d", rocIds[iroc]),
			256, 0., 256., 256, 0., 256.);
    setTitles(h2[iroc], "phoffset", "phscale");
    fHistList.push_back(h2[iroc]);
    fHistOptions.insert(make_pair(h2[iroc], "colz"));

    for (int io = fPhOffsetMin; io < 255; ++io) {
      for (int is = fPhScaleMin; is < 255; ++is) {
	if ((hLo->GetBinContent(io+1, is+1) >= fPhMin)
	    && (hHi->GetBinContent(io+1, is+1) <= fPhMax)) {
	  h2[iroc]->SetBinContent(io+1, is+1, hHi->GetBinContent(io+1, is+1) - hLo->GetBinContent(io+1, is+1));
	}
      }
    }

  }

  restoreDacs();

  // -- set phscale and phoffset to values corresponding to maximum range
  int io(-1), is(-1), ibla(-1), ibin(-1), nGood(0);
  double meanOffset(0), meanScale(0);
  vector<int> vBad;
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc) {
    ibin = h2[iroc]->GetMaximumBin(io, is, ibla);
    LOG(logDEBUG) << " roc " << static_cast<int>(rocIds[iroc]) << " ibin = " << ibin << " io = " << io << " is = " << is
		  << " ibla = " << ibla;

    if (io > fPhOffsetMin && is > fPhScaleMin) {
      fApi->setDAC("phscale", is, rocIds[iroc]);
      fApi->setDAC("phoffset", io, rocIds[iroc]);
      meanScale  += is;
      meanOffset += io;
      ++nGood;
    } else {
      vBad.push_back(iroc);
    }
  }

  meanScale  /= nGood;
  meanOffset /= nGood;

  for (unsigned int ibad = 0; ibad < vBad.size(); ++ibad) {
    LOG(logWARNING) << " PH optimization did not converge for ROC " << static_cast<int>(rocIds[vBad[ibad]])
		    << ", setting to average good values (" << meanScale << "," << meanOffset << ")";
    fApi->setDAC("phscale", meanScale, rocIds[vBad[ibad]]);
    fApi->setDAC("phoffset", meanOffset, rocIds[vBad[ibad]]);
  }

  saveDacs();
  cacheDacs();

  // -- validation
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  fApi->setVcalLowRange();
  fApi->setDAC("vcal", fVcalLow);
  string loData("Lo mean/RMS:"), hiData("Hi mean/RMS:");
  v_ctrlreg = getDacs("ctrlreg");
  vector<TH2D*> loAlive = phMaps(Form("phOptValLo_VCAL%d_CTRLREG%d", fVcalLow, v_ctrlreg[0]), 10, FLAG_FORCE_MASKED);
  for (unsigned int i = 0; i < loAlive.size(); ++i) {
    TH1* d1 = distribution(loAlive[i], 256, 0., 256.);
    loData += Form(" %3.0f/%3.0f", d1->GetMean(), d1->GetRMS());
    fHistList.push_back(loAlive[i]);
    fHistList.push_back(d1);
  }
  fApi->setVcalHighRange();
  fApi->setDAC("vcal", fVcalHigh);
  v_ctrlreg = getDacs("ctrlreg");
  vector<TH2D*> hiAlive = phMaps(Form("phOptValHi_VCAL%d_CTRLREG%d", fVcalHigh, v_ctrlreg[0]), 10, FLAG_FORCE_MASKED);
  for (unsigned int i = 0; i < hiAlive.size(); ++i) {
    TH1* d1 = distribution(hiAlive[i], 256, 0., 256.);
    hiData += Form(" %3.0f/%3.0f", d1->GetMean(), d1->GetRMS());
    fHistList.push_back(hiAlive[i]);
    fHistList.push_back(d1);
  }

  restoreDacs();

  TH1 *h1(0);
  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    (*il)->Draw("colz");
    PixTest::update();
  }
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);


  // -- print summary information
  string psString(""), poString("");
  for (unsigned int i = 0; i < rocIds.size(); ++i) {

    psString += Form(" %3d", fApi->_dut->getDAC(rocIds[i], "phscale"));
    poString += Form(" %3d", fApi->_dut->getDAC(rocIds[i], "phoffset"));
  }
  LOG(logINFO) << "PixTestPh::optimize() done";
  LOG(logINFO) << loData;
  LOG(logINFO) << hiData;
  LOG(logINFO) << "PH scale (per ROC):  " << psString;
  LOG(logINFO) << "PH offset (per ROC): " << poString;

  fApi->setVcalLowRange();

}



// ----------------------------------------------------------------------
void PixTestPh::scan(string name) {

  uint16_t FLAGS = FLAG_FORCE_MASKED;

  TH2D *h2(0), *hd(0);
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  int nx = 256;
  int ny = 256;
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    h2 = bookTH2D(Form("%s_phoffset_phscale_c%d_r%d_C%d", name.c_str(), fPIX[iroc].first, fPIX[iroc].second, rocIds[iroc]),
		  Form("%s_phoffset_phscale_c%d_r%d_C%d", name.c_str(), fPIX[iroc].first, fPIX[iroc].second, rocIds[iroc]),
		  nx, 0., static_cast<double>(nx), ny, 0., static_cast<double>(ny));
    hd = h2;
    h2->SetMinimum(0.);
    setTitles(h2, "phoffset", "phscale");
    fHistList.push_back(h2);
    fHistOptions.insert(make_pair(h2, "colz"));
    fMaps.insert(make_pair(Form("%s_phoffset_phscale_c%d_r%d_C%d",
				name.c_str(), fPIX[iroc].first, fPIX[iroc].second, rocIds[iroc]), h2));
  }

  vector<pair<uint8_t, pair<uint8_t, vector<pixel> > > >  results;
  fNDaqErrors = 0;
  bool done = false;
  int cnt(0);
  while (!done) {
    LOG(logDEBUG) << "      attempt #" << cnt;
    try{
      results = fApi->getPulseheightVsDACDAC("phoffset", 0, 255, "phscale", 0, 255, FLAGS, fNtrig);
      fNDaqErrors = fApi->getStatistics().errors_pixel();
      done = true;
    } catch(pxarException &e) {
      fNDaqErrors = 666667;
      ++cnt;
    }
    done = (cnt>2) || done;
 }

  TH2D *h(0);
  int iroc(-1);
  for (unsigned int i = 0; i < results.size(); ++i) {
    pair<uint8_t, pair<uint8_t, vector<pixel> > > v = results[i];
    int idac1 = v.first;
    pair<uint8_t, vector<pixel> > w = v.second;
    int idac2 = w.first;
    vector<pixel> wpix = w.second;
    for (unsigned ipix = 0; ipix < wpix.size(); ++ipix) {
      iroc = wpix[ipix].roc();
      h = fMaps[Form("%s_phoffset_phscale_c%d_r%d_C%d", name.c_str(), wpix[ipix].column(), wpix[ipix].row(), iroc)];
      if (h) {
	h->Fill(idac1, idac2, wpix[ipix].value());
      } else {
	// LOG(logDEBUG) << "wrong pixel "
	// 	      << Form("%d/%d on ROC %d", wpix[ipix].column(), wpix[ipix].row(), rocIds[iroc])
	// 	      << "; not requested, but seen";
      }
    }
  }

  if (hd) hd->Draw("colz");
  PixTest::update();

}



// ----------------------------------------------------------------------
void PixTestPh::adjustVthrComp() {

  int NTRIG(5), RESERVE(30);
  uint16_t FLAGS = FLAG_FORCE_MASKED;

  map<string, TH1D*> hmap;
  string name("adjust_VthrComp");
  string hname;
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  TH1D *h1(0);
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    hname = Form("%s_c%d_r%d_C%d", name.c_str(), fPIX[iroc].first, fPIX[iroc].second, rocIds[iroc]);
    h1 = bookTH1D(hname.c_str(), hname.c_str(), 256, 0., 256.);
    h1->SetMinimum(0.);
    hmap[hname] = h1;
    fHistList.push_back(h1);
  }


  // -- determine VthrComp with this pixel
  int cnt(0);
  bool done(false);
  vector<pair<uint8_t, vector<pixel> > > results;
  while (!done) {
    try{
      results = fApi->getEfficiencyVsDAC("vthrcomp", 0, 200, FLAGS, NTRIG);
      fNDaqErrors = fApi->getStatistics().errors_pixel();
      done = true;
    } catch(pxarException &e) {
      fNDaqErrors = 666667;
      LOG(logCRITICAL) << "pXar exception: "<< e.what();
      ++cnt;
    }
    done = (cnt>2) || done;
  }

  for (unsigned int i = 0; i < results.size(); ++i) {
    int idac1 = results[i].first;
    vector<pixel> wpix = results[i].second;
    for (unsigned ipix = 0; ipix < wpix.size(); ++ipix) {
      h1 = hmap[Form("%s_c%d_r%d_C%d", name.c_str(), wpix[ipix].column(), wpix[ipix].row(), wpix[ipix].roc())];
      if (h1) {
	h1->Fill(idac1, wpix[ipix].value());
      } else {
	LOG(logDEBUG) << "wrong pixel decoded";
      }
    }
  }

  map<string, TH1D*>::iterator hits = hmap.begin();
  map<string, TH1D*>::iterator hite = hmap.end();
  for (map<string, TH1D*>::iterator hit = hits; hit != hite; ++hit) {
    h1 = hit->second;
    string h1name = h1->GetName();
    string::size_type s1 = h1name.rfind("_C");
    string::size_type s2 = h1name.rfind("_V");
    string rocname = h1name.substr(s1+2, s2-s1-2);
    int roc = atoi(rocname.c_str());
    int thn      = h1->FindLastBinAbove(0.5*NTRIG);
    int vthrcomp = thn - RESERVE;
    fApi->setDAC("vthrcomp", vthrcomp, roc);
    LOG(logDEBUG) << "rocname = ->" << rocname << "<- into roc = " << roc << " vthrcomp: " << vthrcomp;

  }

  h1->Draw();
  PixTest::update();

}
