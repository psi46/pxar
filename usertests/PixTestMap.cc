// -- author: Daniel Pitzl
// pulse each pixel, count responses, plot map, (aka Alive)

#include <stdlib.h>   // atof, atoi
#include <algorithm>  // std::find

#include "PixTestMap.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestMap)

//------------------------------------------------------------------------------
PixTestMap::PixTestMap( PixSetup *a, std::string name ) :
PixTest(a, name), fParNtrig(-1), fParVcal(-1)
{
  PixTest::init();
  init();
  LOG(logDEBUG) << "PixTestMap ctor(PixSetup &a, string, TGTab *)";
}

//------------------------------------------------------------------------------
PixTestMap::PixTestMap() : PixTest()
{
  LOG(logDEBUG) << "PixTestMap ctor()";
}

//------------------------------------------------------------------------------
bool PixTestMap::setParameter( string parName, string sval )
{
  bool found(false);

  for( uint32_t i = 0; i < fParameters.size(); ++i ) {

    if( fParameters[i].first == parName ) {

      found = true;

      if( !parName.compare("Ntrig") ) {
	fParNtrig = atoi(sval.c_str() );
	setToolTips();
      }

      if( !parName.compare("Vcal") ) {
	fParVcal = atoi(sval.c_str() );
	setToolTips();
      }
      break;
    }
  }
  return found;
}

//------------------------------------------------------------------------------
void PixTestMap::init()
{
  LOG(logDEBUG) << "PixTestMap::init()";

  setToolTips();
  fDirectory = gFile->GetDirectory(fName.c_str() );
  if( !fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str() );
  } 
  fDirectory->cd();
}

//------------------------------------------------------------------------------
void PixTestMap::setToolTips()
{
  fTestTip =
    string("send Ntrig pulses to each pixel and count readouts\n")
    + string("the result is a hitmap, not an efficiency map")
    ;
  fSummaryTip =
    string("all ROCs are displayed side-by-side. Note the orientation:\n")
    + string("the canvas bottom corresponds to the narrow module side with the cable")
    ;
}

//------------------------------------------------------------------------------
void PixTestMap::bookHist(string name)
{
  fDirectory->cd();
  LOG(logDEBUG) << "nothing done with " << name;
}

//------------------------------------------------------------------------------
PixTestMap::~PixTestMap()
{
  LOG(logDEBUG) << "PixTestMap dtor";
}

//------------------------------------------------------------------------------
void PixTestMap::doTest()
{
  fDirectory->cd();
  LOG(logINFO) << "PixTestMap::doTest() ntrig = " << fParNtrig;
  PixTest::update();

  if( fApi) fApi->_dut->testAllPixels(true);

  vector<TH2D*> test2 = efficiencyMaps( "PixelMap", fParNtrig ); // PixTest.cc

  copy( test2.begin(), test2.end(), back_inserter(fHistList) );

  // ROC summary:

  TH1D *halive = new TH1D( "MapCount",
			   "Map pixels per ROC;ROC;alive pixels",
			   16, -0.5, 15.5 );
  halive->SetStats(0); // no stats
  halive->SetMinimum(0);
  halive->SetMaximum(4200);
  fHistList.push_back(halive);

  TH1D *hperfect = new TH1D( "PerfectCount",
			     "Perfect pixels per ROC;ROC;perfect pixels",
			     16, -0.5, 15.5 );
  hperfect->SetStats(0); // no stats
  hperfect->SetMinimum(0);
  hperfect->SetMaximum(4200);
  fHistList.push_back(hperfect);

  TH1D *hdead = new TH1D( "DeadCount",
			  "Dead pixels per ROC;ROC;dead pixels",
			  16, -0.5, 15.5 );
  hdead->SetStats(0); // no stats
  hdead->SetMinimum(0);
  hdead->SetMaximum(4200);
  fHistList.push_back(hdead);

  TH2D * h2;

  for( size_t roc = 0; roc < test2.size(); ++roc ) {

    h2 = test2.at(roc);
    h2->Draw("colz");
    PixTest::update();

    int dead = 0;
    int alive = 0;
    int perfect = 0;

    for( int ix = 0; ix < h2->GetNbinsX(); ++ix ) {
      for( int iy = 0; iy < h2->GetNbinsY(); ++iy ) {
	int nn = int( h2->GetBinContent( ix+1, iy+1 ) + 0.1 );
	if( nn == 0 ) dead++;
	if( nn >= fParNtrig/2 ) alive++;
	if( nn == fParNtrig ) perfect++;
      }
    }

    LOG(logINFO) << "ROC " << setw(2) << roc
		 << " dead " << setw(4) << dead
		 << ", alive " << setw(4) << alive
		 << ", perfect " << setw(4) << perfect;

    hdead->Fill( roc, dead );
    halive->Fill( roc, alive );
    hperfect->Fill( roc, perfect );

  } // rocs

  hdead->Draw();
  halive->Draw();
  hperfect->Draw();

  h2 = (TH2D*)(*fHistList.begin() );
  h2->Draw("colz");
  fDisplayedHist = find( fHistList.begin(), fHistList.end(), h2 );
  PixTest::update();

  LOG(logINFO) << "PixTestMap::doTest() done";
}
