
// pulse each pixel, count responses, plot map

#include <stdlib.h>   // atof, atoi
#include <algorithm>  // std::find

#include "PixTestAlive.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestAlive)

//------------------------------------------------------------------------------
PixTestAlive::PixTestAlive( PixSetup *a, std::string name )
: PixTest(a, name), fParNtrig(-1), fParVcal(-1)
{
  PixTest::init(a, name);
  init();
  LOG(logDEBUG) << "PixTestAlive ctor(PixSetup &a, string, TGTab *)";
}

//------------------------------------------------------------------------------
PixTestAlive::PixTestAlive() : PixTest() {
  LOG(logDEBUG) << "PixTestAlive ctor()";
}

//------------------------------------------------------------------------------
bool PixTestAlive::setParameter(string parName, string sval) {
  bool found(false);
  for (map<string,string>::iterator imap = fParameters.begin(); imap != fParameters.end(); ++imap) {
    LOG(logDEBUG) << "---> " << imap->first;
    if (0 == imap->first.compare(parName)) {
      found = true;

      fParameters[parName] = sval;
      LOG(logDEBUG) << "  ==> parName: " << parName;
      LOG(logDEBUG) << "  ==> sval:    " << sval;
      if (!parName.compare("Ntrig")) {
	fParNtrig = atoi(sval.c_str());
	setToolTips();
      }
      if (!parName.compare("Vcal")) {
	fParVcal = atoi(sval.c_str());
	setToolTips();
      }
      break;
    }
  }
  return found;
}

//------------------------------------------------------------------------------
void PixTestAlive::init()
{
  LOG(logDEBUG) << "PixTestAlive::init()";

  setToolTips();
  fDirectory = gFile->GetDirectory(fName.c_str());
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str());
  } 
  fDirectory->cd();
}

//------------------------------------------------------------------------------
void PixTestAlive::setToolTips()
{
  fTestTip =
    string("send Ntrig calibrates and count how many hits were measured\n");
}

//------------------------------------------------------------------------------
void PixTestAlive::bookHist(string name)
{
  fDirectory->cd();
  LOG(logDEBUG) << "nothing done with " << name;
}

//------------------------------------------------------------------------------
PixTestAlive::~PixTestAlive()
{
  LOG(logDEBUG) << "PixTestAlive dtor";
}

//------------------------------------------------------------------------------
void PixTestAlive::doTest()
{
  fDirectory->cd();
  LOG(logINFO) << "PixTestAlive::doTest() ntrig = " << fParNtrig;
  PixTest::update();

  if (fApi) fApi->_dut->testAllPixels(true);

  vector<TH2D*> test2 = efficiencyMaps( "PixelAlive", fParNtrig );

  copy( test2.begin(), test2.end(), back_inserter(fHistList) );

  TH2D *h = (TH2D*)(*fHistList.begin());
  h->Draw();
  fDisplayedHist = find( fHistList.begin(), fHistList.end(), h );
  PixTest::update();

  // summary:

  TH1D *halive = new TH1D( "AliveCount", "Alive pixels per ROC;ROC;alive pixels",
			 16, -0.5, 15.5 );
  halive->SetStats(0); // no stats
  halive->SetMinimum(0);
  halive->SetMaximum(4200);
  fHistList.push_back(halive);

  TH1D *hperfect = new TH1D( "PerfectCount", "Perfect pixels per ROC;ROC;perfect pixels",
			 16, -0.5, 15.5 );
  hperfect->SetStats(0); // no stats
  hperfect->SetMinimum(0);
  hperfect->SetMaximum(4200);
  fHistList.push_back(hperfect);

  TH1D *hdead = new TH1D( "DeadCount", "Dead pixels per ROC;ROC;dead pixels",
			 16, -0.5, 15.5 );
  hdead->SetStats(0); // no stats
  hdead->SetMinimum(0);
  hdead->SetMaximum(4200);
  fHistList.push_back(hdead);

  for( size_t roc = 0; roc < test2.size(); ++roc ) {

    TH2D * h2 = test2.at(roc);
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
  PixTest::update();

  LOG(logINFO) << "PixTestAlive::doTest() done";
}
