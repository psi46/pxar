// -- author: Daniel Pitzl
// threshold map

#include <stdlib.h>   // atof, atoi
#include <algorithm>  // std::find

#include "PixTestMapThr.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestMapThr)

//------------------------------------------------------------------------------
PixTestMapThr::PixTestMapThr( PixSetup *a, std::string name )
: PixTest(a, name), fParNtrig(-1)
{
  PixTest::init();
  init();
  LOG(logDEBUG) << "PixTestMapThr ctor(PixSetup &a, string, TGTab *)";
}

//------------------------------------------------------------------------------
PixTestMapThr::PixTestMapThr() : PixTest()
{
  LOG(logDEBUG) << "PixTestMapThr ctor()";
}

//------------------------------------------------------------------------------
bool PixTestMapThr::setParameter( string parName, string sval )
{
  bool found(false);

  for( uint32_t i = 0; i < fParameters.size(); ++i ) {

    if( fParameters[i].first == parName ) {

      found = true;

      if( !parName.compare( "Ntrig" ) )
	fParNtrig = atoi( sval.c_str() );

      break;
    }
  }
  return found;
}

//------------------------------------------------------------------------------
void PixTestMapThr::init()
{
  LOG(logDEBUG) << "PixTestMapThr::init()";
  
  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory ) {
    fDirectory = gFile->mkdir( fName.c_str() );
  }
  fDirectory->cd();
}

// ----------------------------------------------------------------------
void PixTestMapThr::setToolTips()
{
  fTestTip = string( "measure threshold map");
  fSummaryTip = string("summary plot to be implemented");
}

//------------------------------------------------------------------------------
void PixTestMapThr::bookHist(string name)
{
  LOG(logDEBUG) << "nothing done with " << name;
}

//------------------------------------------------------------------------------
PixTestMapThr::~PixTestMapThr()
{
  LOG(logDEBUG) << "PixTestMapThr dtor";
  std::list<TH1*>::iterator il;
  fDirectory->cd();
  for( il = fHistList.begin(); il != fHistList.end(); ++il ) {
    LOG(logINFO) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory);
    (*il)->Write();
  }
}

//------------------------------------------------------------------------------
void PixTestMapThr::doTest()
{
  LOG(logINFO) << "PixTestMapThr::doTest() ntrig = " << fParNtrig;

  fDirectory->cd();
  fHistList.clear();
  PixTest::update();

  if( !fApi ) {
    LOG(logERROR) << "PixTestMapThr::doTest: no fApi ?!? quit";
    return;
  }

  fApi->_dut->testAllPixels(true); // all pix, all rocs

  // measure:

  uint16_t flags = FLAG_RISING_EDGE;
  vector<pixel> vpix =
    fApi->getThresholdMap( "Vcal", flags, fParNtrig ); // all pix, all ROCs

  LOG(logINFO) << "vpix.size() " << vpix.size();

  // book maps per ROC:

  vector<TH2D*> maps;
  TH2D *h2(0);
  vector<TH1D*> hsts;
  TH1D *h1(0);

  size_t nRocs = fPixSetup->getConfigParameters()->getNrocs();

  for( size_t roc = 0; roc < nRocs; ++roc ) {

    h2 = new TH2D( Form( "MapThr_C%d", int(roc) ),
		   Form( "Threshold map ROC %d", int(roc) ),
		   52, -0.5, 51.5, 80, -0.5, 79.5 );
    h2->SetMinimum(0);
    h2->SetMaximum(256);
    setTitles( h2, "col", "row" );
    h2->GetZaxis()->SetTitle( "Vcal threshold [DAC]" );
    h2->SetStats(0);
    maps.push_back(h2);
    fHistList.push_back(h2);

    h1 = new TH1D( Form( "ThrDistribution_C%d", int(roc) ),
		   Form( "Threshold distribution ROC %d", int(roc) ),
		   256, -0.5, 255.5 );
    setTitles( h1, "Vcal threshold [DAC]", "pixels" );
    h1->SetStats(1);
    hsts.push_back(h1);
    fHistList.push_back(h1);

  }

  // data:

  for( size_t ipx = 0; ipx < vpix.size(); ++ipx ) {
    h2 = maps.at(vpix[ipx].roc_id);
    if( h2 ) h2->Fill( vpix[ipx].column, vpix[ipx].row, vpix[ipx].value );
    h1 = hsts.at(vpix[ipx].roc_id);
    if( h1 ) h1->Fill( vpix[ipx].value );
  }

  for( size_t roc = 0; roc < nRocs; ++roc ) {
    h2 = maps[roc];
    h2->Draw("colz");
    PixTest::update();
  }
  fDisplayedHist = find( fHistList.begin(), fHistList.end(), h2 );

  LOG(logINFO) << "PixTestMapThr::doTest() done for " << maps.size() << " ROCs";

}
