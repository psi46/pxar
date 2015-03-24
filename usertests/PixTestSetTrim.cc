// -- author: Daniel Pitzl
// set Vtrim and trim bits

#include <cmath> // sqrt
#include <stdlib.h>   // atof, atoi
#include <algorithm>  // std::find

#include "PixTestSetTrim.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestSetTrim)

//------------------------------------------------------------------------------
PixTestSetTrim::PixTestSetTrim( PixSetup *a, std::string name )
: PixTest(a, name), fParVcal(-1), fParNtrig(-1)
{
  PixTest::init();
  init();
  LOG(logDEBUG) << "PixTestSetTrim ctor(PixSetup &a, string, TGTab *)";
}

//------------------------------------------------------------------------------
PixTestSetTrim::PixTestSetTrim() : PixTest()
{
  LOG(logDEBUG) << "PixTestSetTrim ctor()";
}

//------------------------------------------------------------------------------
bool PixTestSetTrim::setParameter( string parName, string sval )
{
  bool found(false);

  for( uint32_t i = 0; i < fParameters.size(); ++i ) {

    if( fParameters[i].first == parName ) {

      found = true;

      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());

      if( !parName.compare( "targetvcal" ) )
	fParVcal = atoi( sval.c_str() );

      if( !parName.compare( "ntrig" ) )
	fParNtrig = atoi( sval.c_str() );

      setToolTips();
      break;
    }
  }
  return found;
}

//------------------------------------------------------------------------------
void PixTestSetTrim::init()
{
  LOG(logDEBUG) << "PixTestSetTrim::init()";
  
  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory ) {
    fDirectory = gFile->mkdir( fName.c_str() );
  }
  fDirectory->cd();
}

//------------------------------------------------------------------------------
void PixTestSetTrim::setToolTips()
{
  fTestTip = string( "set Vtrim and trim bits to get TargetVcal threshold")
    ;
  fSummaryTip = string("summary plot to be implemented")
    ;

}

//------------------------------------------------------------------------------
void PixTestSetTrim::bookHist(string name)
{
  LOG(logDEBUG) << "nothing done with " << name;
}

//------------------------------------------------------------------------------
PixTestSetTrim::~PixTestSetTrim()
{
  LOG(logDEBUG) << "PixTestSetTrim dtor";
  std::list<TH1*>::iterator il;
  fDirectory->cd();
  for( il = fHistList.begin(); il != fHistList.end(); ++il ) {
    LOG(logINFO) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory);
    (*il)->Write();
  }
}

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 25.1.2014: measure ROC threshold map vs Vcal

void PixTestSetTrim::RocThrMap( uint8_t roc, uint32_t nTrig,
				bool xtalk, bool cals )
{
  if( roc > 15 ) {
    LOG(logERROR) << "[PixTestSetTrim::RocThrMap] invalid roc " << roc;
    return;
  }

  fApi->_dut->testAllPixels( true, roc );

  for( uint8_t col = 0; col < 52; ++col )
    for( uint8_t row = 0; row < 80; ++row ) {
      uint8_t trim = modtrm[roc][col][row];
      //      fApi->_dut->trimPixel( col, row, trim, roc ); // DP
      fApi->_dut->updateTrimBits( col, row, trim, roc ); 
    }

  // measure:

  uint16_t flags = FLAG_RISING_EDGE;
  if( xtalk ) flags |= FLAG_XTALK;
  if( cals ) flags |= FLAG_CALS;

  vector<pixel> vpix = fApi->getThresholdMap( "Vcal", flags, nTrig );

  LOG(logINFO) << "vpix.size() " << vpix.size();

  // book maps per ROC:

  TH2D *h2(0);
  TH1D *h1(0);

  static uint32_t iter = 0;

  h2 = new TH2D( Form( "Threshold_map_ROC_%d_i%i", int(roc), iter ),
		 Form( "Threshold map ROC %d i%i", int(roc), iter ),
		 52, -0.5, 51.5, 80, -0.5, 79.5 );
  h2->SetMinimum(0);
  h2->SetMaximum(256);
  setTitles( h2, "col", "row" );
  h2->GetZaxis()->SetTitle( "Vcal threshold [DAC]" );
  h2->SetStats(0);
  fHistList.push_back(h2);

  h1 = new TH1D( Form( "ThrDistribution_C%d_i%i", int(roc), iter ),
		 Form( "Threshold distribution ROC %d i%i", int(roc), iter ),
		 256, -0.5, 255.5 );
  setTitles( h1, "Vcal threshold [DAC]", "pixels" );
  h1->SetStats(1);
  fHistList.push_back(h1);

  // data:

  for( size_t ipx = 0; ipx < vpix.size(); ++ipx ) {
    if( vpix[ipx].roc() == roc ) {
      h2->Fill( vpix[ipx].column(), vpix[ipx].row(), vpix[ipx].value());
      h1->Fill( vpix[ipx].value());
      if( vpix[ipx].column() < 52 && vpix[ipx].row() < 80 )
	modthr[roc][vpix[ipx].column()][vpix[ipx].row()] = (uint8_t)vpix[ipx].value();
    }
  }

  h2->Draw("colz");
  PixTest::update();

  fDisplayedHist = find( fHistList.begin(), fHistList.end(), h2 );

  fApi->_dut->testAllPixels( false, roc );

  iter++;

} // RocThrMap

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 25.1.2014: print ROC threshold map

void PixTestSetTrim::printThrMap( uint8_t roc, int &nok )
{
  int sum = 0;
  nok = 0;
  int su2 = 0;
  int vmin = 255;
  int vmax =   0;
  int colmin = -1;
  int rowmin = -1;
  int colmax = -1;
  int rowmax = -1;

  cout << "Threshold map for ROC " << roc << endl;
  for( int row = 79; row >= 0; --row ) {
    cout << setw(2) << row << ":";
    for( int col = 0; col < 52; ++col ) {

      int thr = modthr[roc][col][row];
      cout << " " << thr;
      if( col == 25 ) cout << endl << "   ";
	
      if( thr < 255 ) {
	sum += thr;
	su2 += thr*thr;
	nok++;
      }
      if( thr > 0 && thr < vmin ) {
	vmin = thr;
	colmin = col;
	rowmin = row;
      }
      if( thr > vmax && thr < 255 ) {
	vmax = thr;
	colmax = col;
	rowmax = row;
      }
    } // cols
    cout << endl;
  } // rows

  cout << "valid thresholds " << nok << endl;
  if( nok > 0 ) {
    cout << "min thr " << vmin << " at " << colmin << " " << rowmin << endl;
    cout << "max thr " << vmax << " at " << colmax << " " << rowmax << endl;
    double mid = (double)sum / (double) nok;
    double rms = sqrt( (double)su2 / (double)nok - mid*mid );
    cout << "mean thr " << mid
	 << ", rms " << rms
	 << endl;
  }
  cout.flush();

} // printThrMap

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 25.1.2014: trim bit correction

void PixTestSetTrim::TrimStep( int roc, int target, int correction,
			       int nTrig, int xtalk, int cals )
{
  LOG(logINFO) << "TrimStep for ROC " << roc
	       << " with correction " << correction;

  for( int col = 0; col < 52; ++col ) {
    for( int row = 0; row < 80; ++row ) {

      int thr = modthr[roc][col][row];
      rocthr[col][row] = thr; // remember

      int trim = modtrm[roc][col][row];
      roctrm[col][row] = trim; // remember

      if( thr > target && thr < 255 )
	trim -= correction; // make softer
      else
	trim += correction; // make harder

      if( trim <  0 ) trim = 0;
      if( trim > 15 ) trim = 15;

      modtrm[roc][col][row] = trim;

    } // rows
  } // cols

  // measure new result:

  RocThrMap( roc, nTrig, xtalk, cals ); // fills modthr[roc][][]

  for( int col = 0; col < 52; ++col ) {
    for( int row = 0; row < 80; ++row ) {

      int thr = modthr[roc][col][row];

      int old = rocthr[col][row];

      if( abs( thr - target ) > abs( old - target ) ) {
	modtrm[roc][col][row] = roctrm[col][row]; // go back
	modthr[roc][col][row] = old;
      }

    } // rows
  } // cols

} // TrimStep

//------------------------------------------------------------------------------
void PixTestSetTrim::doTest()
{
  LOG(logINFO) << "PixTestSetTrim::doTest() ntrig = " << fParNtrig;

  fDirectory->cd();
  fHistList.clear();
  PixTest::update();

  // global threshold map:

  for( size_t k = 0; k < 16; ++k )
    for( size_t i = 0; i < 52; ++i )
      for( size_t j = 0; j < 80; ++j ){
	modthr[k][i][j] = 255; // thr, 255 = invalid
	modtrm[k][i][j] =  15; // trim bits, 15 = off
      }

  size_t nRocs = fPixSetup->getConfigParameters()->getNrocs();
  bool cals = 0;
  bool xtalk = 0;
  uint16_t flags = FLAG_RISING_EDGE;
  int nok = 0;

  uint8_t cal = fApi->_dut->getDAC( 0, "Vcal" );
  uint8_t ctl = fApi->_dut->getDAC( 0, "CtrlReg" );

  fApi->setDAC( "CtrlReg", 0 ); // all ROCs small Vcal
  LOG(logINFO) << "CtrlReg 0 (small Vcal)";

  for( size_t roc = 0; roc < nRocs; ++roc ) {

    LOG(logINFO) << "Trimming ROC " << roc;

    // measure un-trimmed threshold map:

    RocThrMap( roc, fParNtrig, xtalk, cals ); // fills modthr[roc][][]

    // find pixel with hardest threshold:

    int vmax = 0;
    int maxcol = -1;
    int maxrow = -1;

    for( int row = 79; row >= 0; --row ) {
      for( int col = 0; col < 52; ++col ) {

	int thr = modthr[roc][col][row];
	if( thr > vmax && thr < 255 ) {
	  vmax = thr;
	  maxcol = col;
	  maxrow = row;
	}
      } // cols
    } // rows

    if( maxcol < 0 ) {
      LOG(logWARNING) << "No valid threshold on ROC " << roc << ": skipped";
      continue;
    }
    LOG(logINFO) << "max thr " << vmax
		 << " at " << maxcol << " " << maxrow;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // setVtrim using max pixel:

    fApi->_dut->testPixel( maxcol, maxrow, true, roc );

    uint8_t trim = 0; // 0 = strongest
    //    fApi->_dut->trimPixel( maxcol, maxrow, trim, roc ); // DP
    fApi->_dut->updateTrimBits( maxcol, maxrow, trim, roc ); 

    int itrim = 1;
    for( ; itrim < 254; itrim += 2 ) {

      fApi->setDAC( "Vtrim", itrim, roc );
      vector<pixel> vpix9 = fApi->getThresholdMap( "Vcal", flags, fParNtrig );
      int thr = 256;
      for( size_t ipx = 0; ipx < vpix9.size(); ++ipx )
	if( vpix9[ipx].roc() == roc &&
	    vpix9[ipx].column() == maxcol &&
	    vpix9[ipx].row() == maxrow )
	  thr = (int)vpix9[ipx].value();

      LOG(logINFO)
	<< "Vtrim " << setw(3) << itrim
	<< ": vpix9.size " << vpix9.size()
	<< ": " << (int) vpix9[0].roc()
	<< " " << (int) vpix9[0].column()
	<< " " << (int) vpix9[0].row()
	<< ", thr " << setw(3) << thr;
      if( thr < fParVcal )
	break;

    } // itrim

    //itrim += 2; // margin
    fApi->setDAC( "Vtrim", itrim, roc );
    LOG(logINFO) << "set Vtrim to " << itrim;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // iterate trim bits:

    for( int row = 79; row >= 0; --row )
      for( int col = 0; col < 52; ++col ) {
	modtrm[roc][col][row] = 7; // half way
	rocthr[col][row] = modtrm[roc][col][row]; // remember
	roctrm[col][row] = 15; // old
      }

    LOG(logINFO) << "trim: measuring Vcal threshold map with trim 7";

    RocThrMap( roc, fParNtrig, xtalk, cals ); // uses modtrm, fills modthr

    printThrMap( roc, nok );

    int correction = 4;

    TrimStep( roc, fParVcal, correction,
	      fParNtrig, xtalk, cals ); // fills modtrm, fills modthr

    printThrMap( roc, nok );

    correction = 2;

    TrimStep( roc, fParVcal, correction,
	      fParNtrig, xtalk, cals ); // fills modtrm, fills modthr

    printThrMap( roc, nok );

    correction = 1;

    TrimStep( roc, fParVcal, correction,
	      fParNtrig, xtalk, cals ); // fills modtrm, fills modthr

    printThrMap( roc, nok );

    correction = 1;

    TrimStep( roc, fParVcal, correction,
	      fParNtrig, xtalk, cals ); // fills modtrm, fills modthr

    printThrMap( roc, nok );

    correction = 1;

    TrimStep( roc, fParVcal, correction,
	      fParNtrig, xtalk, cals ); // fills modtrm, fills modthr

    printThrMap( roc, nok );

    fApi->_dut->testPixel( maxcol, maxrow, false, roc );

  } // rocs

  fApi->setDAC( "Vcal", cal ); // restore on all ROCs
  fApi->setDAC( "CtrlReg", ctl ); // restore
  LOG(logINFO) << "back to Vcal " << int(cal);
  LOG(logINFO) << "back to CtrlReg " << int(ctl);

  LOG(logINFO) << "PixTestSetTrim::doTest() done for " << nRocs << " ROCs";
}
