
// pulse each pixel, plot PH map

#include <stdlib.h>   // atof, atoi
#include <algorithm>  // std::find

#include "PixTestMapPh.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestMapPh)

//------------------------------------------------------------------------------
PixTestMapPh::PixTestMapPh( PixSetup *a, std::string name )
: PixTest(a, name), fParNtrig(-1), fParCals(0)
{
  PixTest::init( a, name );
  init();
  LOG(logDEBUG) << "PixTestMapPh ctor(PixSetup &a, string, TGTab *)";
}

//------------------------------------------------------------------------------
PixTestMapPh::PixTestMapPh() : PixTest()
{
  LOG(logDEBUG) << "PixTestMapPh ctor()";
}

//------------------------------------------------------------------------------
bool PixTestMapPh::setParameter( string parName, string sval )
{
  bool found(false);
  for( map<string,string>::iterator imap = fParameters.begin();
       imap != fParameters.end(); ++imap ) {

    LOG(logDEBUG) << "---> " << imap->first;

    if( 0 == imap->first.compare(parName) ) {
      found = true;

      fParameters[parName] = sval;
      LOG(logDEBUG) << "  ==> parName: " << parName;
      LOG(logDEBUG) << "  ==> sval:    " << sval;

      if( !parName.compare( "Ntrig" ) )
	fParNtrig = atoi( sval.c_str() );

      if( !parName.compare( "CALS" ) )
	fParCals = atoi( sval.c_str() );

      break;
    }
  }
  return found;
}

//------------------------------------------------------------------------------
void PixTestMapPh::init()
{
  LOG(logDEBUG) << "PixTestMapPh::init()";

  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory) {
    fDirectory = gFile->mkdir( fName.c_str() );
  }
  fDirectory->cd();
}

//------------------------------------------------------------------------------
void PixTestMapPh::bookHist(string name)
{
  LOG(logDEBUG) << "nothing done with " << name;
}

//------------------------------------------------------------------------------
PixTestMapPh::~PixTestMapPh()
{
  LOG(logDEBUG) << "PixTestMapPh dtor";
  std::list<TH1*>::iterator il;
  fDirectory->cd();
  for( il = fHistList.begin(); il != fHistList.end(); ++il ) {
    LOG(logINFO) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory);
    (*il)->Write();
  }
}

//------------------------------------------------------------------------------
void PixTestMapPh::doTest()
{
  LOG(logINFO) << "PixTestMapPh::doTest() ntrig = " << fParNtrig;

  fDirectory->cd();
  fHistList.clear();
  PixTest::update();

  if( fApi ) fApi->_dut->testAllPixels(true);

  // measure:

  uint16_t flags = 0;
  if( fParCals ) flags = FLAG_USE_CALS;
  LOG(logINFO) << "flag " << FLAG_USE_CALS;

  uint8_t ctl = fApi->_dut->getDAC( 0, "CtrlReg" );
  if( fParCals ) {
    fApi->setDAC( "CtrlReg", 4 ); // all ROCs large Vcal
    LOG(logINFO) << "CtrlReg 4 (large Vcal)";
  }

  vector<pixel> vpix; // all pixels, all ROCs
  if( fApi ) vpix = fApi->getPulseheightMap( flags, fParNtrig );

  if( fParCals ) {
    fApi->setDAC( "CtrlReg", ctl ); // restore
    LOG(logINFO) << "back to CtrlReg " << ctl;
  }

  LOG(logINFO) << "vpix.size() " << vpix.size();

  // book maps per ROC:

  vector<TH2D*> maps;
  TH2D *h2(0);
  vector<TH1D*> hsts;
  TH1D *h1(0);

  size_t nRocs = fPixSetup->getConfigParameters()->getNrocs();

  for( size_t roc = 0; roc < nRocs; ++roc ) {

    h2 = new TH2D( Form( "MapPh_C%d", int(roc) ),
		   Form( "PH map ROC %d", int(roc) ),
		   52, -0.5, 51.5, 80, -0.5, 79.5 );
    h2->SetMinimum(0);
    h2->SetMaximum(256);
    setTitles( h2, "col", "row" );
    h2->GetZaxis()->SetTitle( "<PH> [ADC]" );
    h2->SetStats(0);
    maps.push_back(h2);
    fHistList.push_back(h2);

    h1 = new TH1D( Form( "PhDistribution_C%d", int(roc) ),
		   Form( "Pulse height distribution ROC %d", int(roc) ),
		   256, -0.5, 255.5 );
    setTitles( h1, "<PH> [ADC]", "pixels" );
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

  LOG(logINFO) << "PixTestMapPh::doTest() done for " << maps.size() << " ROCs";

}
