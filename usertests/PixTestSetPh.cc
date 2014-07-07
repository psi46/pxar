// -- author: Daniel Pitzl
// set offset and gain for PH into ADC range

#include <stdlib.h>   // atof, atoi
#include <algorithm>  // std::find

#include "PixTestSetPh.hh"
#include "log.h"
#include "constants.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestSetPh)

//------------------------------------------------------------------------------
PixTestSetPh::PixTestSetPh( PixSetup *a, std::string name )
: PixTest(a, name), fParNtrig(-1)
{
  PixTest::init();
  init();
}

//------------------------------------------------------------------------------
PixTestSetPh::PixTestSetPh() : PixTest()
{
}

//------------------------------------------------------------------------------
bool PixTestSetPh::setParameter( string parName, string sval )
{
  bool found(false);

  for( uint32_t i = 0; i < fParameters.size(); ++i ) {

    if( fParameters[i].first == parName ) {

      found = true;

      if( !parName.compare( "ntrig" ) )
	fParNtrig = atoi( sval.c_str() );

      if( !parName.compare( "pix1" ) ) {
	string::size_type s1 = sval.find( "," );
	if( string::npos != s1 ) {
	  string str1 = sval.substr(0, s1);
	  uint16_t pixc = atoi( str1.c_str() );
	  string str2 = sval.substr(s1+1);
	  uint16_t pixr = atoi( str2.c_str() );
	  fPIX.push_back( make_pair( pixc, pixr ) );
	}
	else {
	  fPIX.push_back( make_pair( -1, -1 ) );
	}
      }
      break;
    }
  }
  return found;
}

//------------------------------------------------------------------------------
void PixTestSetPh::init()
{
  LOG(logINFO) << "PixTestSetPh::init()";

  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory )
    fDirectory = gFile->mkdir( fName.c_str() );
  fDirectory->cd();
}

// ----------------------------------------------------------------------
void PixTestSetPh::setToolTips()
{
  fTestTip = string( "tune PH into ADC range using VOffsetRO and VIref_ADC");
  fSummaryTip = string("summary plot to be implemented");
}

//------------------------------------------------------------------------------
void PixTestSetPh::bookHist(string name)
{
  LOG(logDEBUG) << "nothing done with " << name;
}

//------------------------------------------------------------------------------
PixTestSetPh::~PixTestSetPh()
{
  LOG(logDEBUG) << "PixTestSetPh dtor";
  std::list<TH1*>::iterator il;
  fDirectory->cd();
  for( il = fHistList.begin(); il != fHistList.end(); ++il ) {
    LOG(logINFO) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory);
    (*il)->Write();
  }
}

//------------------------------------------------------------------------------
void PixTestSetPh::doTest()
{
  LOG(logINFO) << "PixTestSetPh::doTest() ntrig = " << fParNtrig;

  fDirectory->cd();
  fHistList.clear();
  PixTest::update();

  if( fPIX.size() < 1 ) {
    LOG(logWARNING) << "PixTestSetCalDel: no pixel defined, return";
    return;
  }

  if( fPIX[fPIX.size()-1].first < 0 ) {
    LOG(logWARNING) << "PixTestSetCalDel: no valid pixel defined, return";
    return;
  }

  uint16_t flags = 0;

  uint8_t cal = fApi->_dut->getDAC( 0, "Vcal" );
  uint8_t ctl = fApi->_dut->getDAC( 0, "CtrlReg" );

  fApi->setDAC( "CtrlReg", 4 ); // all ROCs large Vcal
  LOG(logINFO) << "CtrlReg 4 (large Vcal)";

  // Minimize ADC gain for finding the midpoint
  // This avoids PH clipping

  fApi->setDAC( "VIref_ADC", 255 ); // 255 = minimal gain

  const int max_vcal = 255;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // search for min Vcal using one pixel:

  fApi->_dut->testAllPixels(false);

  //coordinates of the last pair = presently set pixel
  int32_t col = fPIX[fPIX.size()-1].first;		
  int32_t row = fPIX[fPIX.size()-1].second;

  if( col > -1)
    fApi->_dut->testPixel( col, row, true );

  size_t nRocs = fPixSetup->getConfigParameters()->getNrocs();

  int pix_ph[16];
  int min_vcal[16]; // large Vcal, after trimming
  bool not_enough = 0;
  bool too_much = 0;

  for( size_t i = 0; i < 16; ++i ) {
    pix_ph[i] = -1;
    min_vcal[i] = 8;
  }

  // measure:

  do {

    for( size_t roc = 0; roc < nRocs; ++roc )
      fApi->setDAC( "Vcal", min_vcal[roc], roc );

    // measure:

    std::vector<pixel> vpix =
      fApi->getPulseheightMap( flags, fParNtrig );

    LOG(logINFO) << "vpix size " << vpix.size();

    // unpack:

    for( size_t ipx = 0; ipx < vpix.size(); ++ipx ) {
      uint8_t roc = vpix[ipx].roc_id;
      if( roc < 16 )
	pix_ph[roc] = vpix[ipx].getValue();
      LOG(logINFO) << "ipx " << setw(2) << ipx
		   << ": ROC " << setw(2) << (int) vpix[ipx].roc_id
		   << " col " << setw(2) << (int) vpix[ipx].column
		   << " row " << setw(2) << (int) vpix[ipx].row
		   << " val " << setw(3) << (int) vpix[ipx].getValue();
    }

    // check:
    not_enough = 0;
    too_much = 0;
    for( size_t roc = 0; roc < nRocs; ++roc ) {
      if( pix_ph[roc] <= 0 ) {
	min_vcal[roc] += 2;
	not_enough = 1;
      }
      if( min_vcal[roc] > 120 ) too_much = 1;
    }
  }
  while( not_enough && !too_much );

  if( too_much ) {
    LOG(logINFO)
      << "[SetPh] Cannot find working Vcal for pixel "
      << fPIX[fPIX.size()-1].first << "," << fPIX[fPIX.size()-1].second
      << ". Please use other pixel or re-trim to lower threshold";
    return;
  }

  for( size_t roc = 0; roc < nRocs; ++roc ) {
    min_vcal[roc] += 2; // safety
    fApi->setDAC( "Vcal", min_vcal[roc], roc );
    LOG(logINFO) << "ROC " << setw(2) << roc
		 << " min Vcal " << setw(2) << min_vcal[roc]
		 << " has Ph " << setw(3) << pix_ph[roc];
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // scan PH vs VOffsetRO for one pixel per ROC at min Vcal

  for( size_t roc = 0; roc < nRocs; ++roc )
    fApi->setDAC( "Vcal", min_vcal[roc], roc );

  string dacName = "VOffsetRO";
  vector<pair<uint8_t, vector<pixel> > > // dac and pix
    result = fApi->getPulseheightVsDAC( dacName, 0, 255, flags, fParNtrig );

  // plot:

  vector<TH1D*> hmin;
  TH1D *h1(0);

  for( size_t roc = 0; roc < nRocs; ++roc ) {

    h1 = new TH1D( Form( "PH_vs_%s_at_Vcal_%2d_c%02d_r%02d_C%02d",
			 dacName.c_str(), min_vcal[roc],
			 col, row, int(roc) ),
		   Form( "PH vs %s at Vcal %2d c%02d r%02d C%02d",
			 dacName.c_str(), min_vcal[roc],
			 col, row, int(roc) ),
		   256, -0.5, 255.5 );
    h1->SetMinimum(0);
    h1->SetMaximum(256);
    setTitles( h1, Form( "%s [DAC]", dacName.c_str() ),
	       Form( "Vcal %2d <PH> [ADC]", min_vcal[roc] ) );
    hmin.push_back(h1);
    fHistList.push_back(h1);

  } // rocs

  // data:

  for( size_t i = 0; i < result.size(); ++i ) {

    int idac = result[i].first;

    vector<pixel> vpix = result[i].second;

    for( size_t ipx = 0; ipx < vpix.size(); ++ipx ) {

      uint8_t roc = vpix[ipx].roc_id;

      if( roc < nRocs &&
	  vpix[ipx].column == col &&
	  vpix[ipx].row == row ) {
	h1 = hmin.at(roc);
	h1->Fill( idac, vpix[ipx].getValue()); // already averaged
      } // valid

    } // pix

  } // dac values

  for( size_t roc = 0; roc < nRocs; ++roc ) {
    hmin[roc]->Draw();
    PixTest::update();
  }
  fDisplayedHist = find( fHistList.begin(), fHistList.end(), hmin[nRocs-1] );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // scan PH vs VOffsetRO for one pixel per ROC at max Vcal

  fApi->setDAC( "Vcal", max_vcal );

  result = fApi->getPulseheightVsDAC( dacName, 0, 255, flags, fParNtrig );

  // plot:

  vector<TH1D*> hmax;

  for( size_t roc = 0; roc < nRocs; ++roc ) {

    h1 = new TH1D( Form( "PH_vs_%s_at_Vcal_%2d_c%02d_r%02d_C%02d",
			 dacName.c_str(), max_vcal,
			 col, row, int(roc) ),
		   Form( "PH vs %s at Vcal %2d c%02d r%02d C%02d",
			 dacName.c_str(), max_vcal,
			 col, row, int(roc) ),
		   256, -0.5, 255.5 );
    h1->SetMinimum(0);
    h1->SetMaximum(256);
    setTitles( h1, Form( "%s [DAC]", dacName.c_str() ),
	       Form( "Vcal %2d <PH> [ADC]", max_vcal ) );
    hmax.push_back(h1);
    fHistList.push_back(h1);

  } // rocs

  // data:

  for( size_t i = 0; i < result.size(); ++i ) {

    int idac = result[i].first;

    vector<pixel> vpix = result[i].second;

    for( size_t ipx = 0; ipx < vpix.size(); ++ipx ) {

      uint8_t roc = vpix[ipx].roc_id;

      if( roc < nRocs &&
	  vpix[ipx].column == col &&
	  vpix[ipx].row == row ) {
	h1 = hmax.at(roc);
	h1->Fill( idac, vpix[ipx].getValue()); // already averaged
      } // valid

    } // pix

  } // dac values

  for( size_t roc = 0; roc < nRocs; ++roc ) {
    hmax[roc]->Draw();
    PixTest::update();
  }
  fDisplayedHist = find( fHistList.begin(), fHistList.end(), hmax[nRocs-1] );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  /* Find VOffsetRO value that puts the midpoint between
     minimal and maximal Vcal to 133 */

  int voffset_opt[16];
  for( size_t i = 0; i < 16; ++i )
    voffset_opt[i] = -1;

  for( size_t roc = 0; roc < nRocs; ++roc ) {

    for( int i = 0; i < 256; ++i ) {
      int ph0 = static_cast<int>(hmin[roc]->GetBinContent(i+1));
      if( ph0 <= 0 ) continue;
      int ph4 = static_cast<int>(hmax[roc]->GetBinContent(i+1));
      if( ph4 <= 0 ) continue;
      if( roc == 0 ) {
	LOG(logINFO) << "VOffsetRO " << setw(3) << i
		     << ": min " << setw(3) << ph0
		     << ", max " << setw(3) << ph4
		     << ", mid " << setw(3) << (ph0+ph4)/2;
      }
      if( ( ph0 + ph4 ) / 2 < 133 ) {
	voffset_opt[roc] = i;
	break;
      }
    }
  } // rocs

  // Abort if the mid VoffsetOp could not be found

  for( size_t roc = 0; roc < nRocs; ++roc ) {

    if( voffset_opt[roc] == -1 ) {
      LOG(logINFO)
	<< "[SetPh] Warning: Cannot find VoffsetOp midpoint!";
      return;
    }

    LOG(logINFO)
      << "[SetPh] Found VOffsetRO value: "
      << voffset_opt[roc];

    fApi->setDAC( "VOffsetRO", voffset_opt[roc], roc );

  } // rocs

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Scan VIref_ADC to stretch the pulse height at max Vcal

  LOG(logINFO)
    << "[SetPh] Finding mid VIref_ADC value ...";

  // Find optimal VIref_ADC using a single pixel

  fApi->setDAC( "Vcal", max_vcal ); // all ROCs

  dacName = "VIref_ADC";

  result = fApi->getPulseheightVsDAC( dacName, 0, 255, flags, fParNtrig );

  // plot:

  vector<TH1D*> href;

  for( size_t roc = 0; roc < nRocs; ++roc ) {

    h1 = new TH1D( Form( "PH_vs_%s_at_Vcal_%2d_c%02d_r%02d_C%02d",
			 dacName.c_str(), max_vcal,
			 col, row, int(roc) ),
		   Form( "PH vs %s at Vcal %2d c%02d r%02d C%02d",
			 dacName.c_str(), max_vcal,
			 col, row, int(roc) ),
		   256, -0.5, 255.5 );
    h1->SetMinimum(0);
    h1->SetMaximum(256);
    setTitles( h1, Form( "%s [DAC]", dacName.c_str() ),
	       Form( "Vcal %2d <PH> [ADC]", max_vcal ) );
    href.push_back(h1);
    fHistList.push_back(h1);

  } // rocs

  // data:

  for( size_t i = 0; i < result.size(); ++i ) {

    int idac = result[i].first;

    vector<pixel> vpix = result[i].second;

    for( size_t ipx = 0; ipx < vpix.size(); ++ipx ) {

      uint8_t roc = vpix[ipx].roc_id;

      if( roc < nRocs &&
	  vpix[ipx].column == col &&
	  vpix[ipx].row == row ) {
	h1 = href.at(roc);
	h1->Fill( idac, vpix[ipx].getValue()); // already averaged
      } // valid

    } // pix

  } // dac values

  for( size_t roc = 0; roc < nRocs; ++roc ) {
    href[roc]->Draw();
    PixTest::update();
  }
  fDisplayedHist = find( fHistList.begin(), fHistList.end(), href[nRocs-1] );

  // Adjust VIref_ADC to have PH < 210 for this pixel

  int viref_adc_opt[16];
  for( size_t i = 0; i < 16; ++i )
    viref_adc_opt[i] = -1;

  for( size_t roc = 0; roc < nRocs; ++roc ) {
    for( int i = 0; i < 256; ++i ) {
      int ph = static_cast<int>(href[roc]->GetBinContent(i+1));
      if( ph <= 0 ) continue;
      if( ph < 210 ) {
	viref_adc_opt[roc] = i;
	break; // VIref_ADC has descending curve
      }
    }

    // Abort if optimal VIref_ADC value could not be found

    if( viref_adc_opt[roc] == -1 ) {
      LOG(logINFO)
	<< "[SetPh] Warning: Cannot adjust pulse height range!";
      return;
    }
    LOG(logINFO)
      << "[SetPh] Found VIref_ADC value: "
      << viref_adc_opt[roc];
    fApi->setDAC( dacName, viref_adc_opt[roc], roc ); // for one pixel

  } // rocs

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // check all pixels:

  fApi->_dut->testAllPixels(true);

  // book maps per ROC:

  vector<TH2D*> maps9;
  TH2D *h2(0);
  vector<TH1D*> hsts9;
  //TH1D *h1(0);

  for( size_t roc = 0; roc < nRocs; ++roc ) {

    h2 = new TH2D( Form( "maxPhMap_C%d", int(roc) ),
		   Form( "max PH map ROC %d", int(roc) ),
		   52, -0.5, 51.5, 80, -0.5, 79.5 );
    h2->SetMinimum(0);
    h2->SetMaximum(256);
    setTitles( h2, "col", "row" );
    h2->GetZaxis()->SetTitle( "<PH> [ADC]" );
    h2->SetStats(0);
    maps9.push_back(h2);
    fHistList.push_back(h2);

    h1 = new TH1D( Form( "maxPhDistribution_C%d", int(roc) ),
		   Form( "max Pulse height distribution ROC %d", int(roc) ),
		   256, -0.5, 255.5 );
    setTitles( h1, "<PH> [ADC]", "pixels" );
    h1->SetStats(1);
    hsts9.push_back(h1);
    fHistList.push_back(h1);
  }

  // measure:

  fApi->setDAC( "Vcal", max_vcal );

  bool again = 0;
  do {

    vector<pixel> vpix = fApi->getPulseheightMap( flags, fParNtrig ); // all pixels, all ROCs

    LOG(logINFO) << "vpix.size() " << vpix.size();

    // data:

    for( size_t ipx = 0; ipx < vpix.size(); ++ipx ) {
      h2 = maps9.at( vpix[ipx].roc_id );
      if( h2 ) h2->SetBinContent( vpix[ipx].column + 1, vpix[ipx].row + 1,
				  vpix[ipx].getValue());
      h1 = hsts9.at( vpix[ipx].roc_id );
      if( h1 ) h1->Fill( vpix[ipx].getValue());
    }

    for( size_t roc = 0; roc < nRocs; ++roc ) {
      h2 = maps9[roc];
      h2->Draw("colz");
      PixTest::update();
    }
    fDisplayedHist = find( fHistList.begin(), fHistList.end(), h2 );

    // check against clipping (ADC overflow):

    again = 0;

    for( size_t roc = 0; roc < nRocs; ++roc ) {

      LOG(logINFO) << "max Ph map for ROC " << roc;

      Int_t locmaxx,locmaxy,locmaxz;
      h2 = maps9[roc];
      h2->GetMaximumBin( locmaxx, locmaxy,locmaxz );
      double phmax = h2->GetBinContent( locmaxx, locmaxy );
      LOG(logINFO)
	<< "Ph max " << setw(3) << phmax
	<< " at " << setw(2) << locmaxx-1 // bin counting starts at 1
	<< "," << setw(2) << locmaxy-1; // pixel counting starts at 0
      if( phmax > 254.5 ) {
	voffset_opt[roc] += 5; // reduce offset
	viref_adc_opt[roc] += 3; // reduce gain
	fApi->setDAC( "VOffsetRO", voffset_opt[roc], roc );
	fApi->setDAC( "VIref_ADC", viref_adc_opt[roc], roc );
	LOG(logINFO)
	  << "Found overflow "
	  << " VoffsetOp " << setw(3) << voffset_opt[roc]
	  << " VIref_ADC " << setw(3) << viref_adc_opt[roc];
	if( voffset_opt[roc] < 251 && viref_adc_opt[roc] < 253 ) {
	  again = 1;
	  hsts9[roc]->Reset();
	}
      }
      else {
	fApi->_dut->testAllPixels( false, roc ); // remove ROC from list
      }
    } // rocs
  } // do
  while( again );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // check for underflow:

  vector<TH2D*> maps0;
  vector<TH1D*> hsts0;

  for( size_t roc = 0; roc < nRocs; ++roc ) {

    h2 = new TH2D( Form( "minPhMap_C%d", int(roc) ),
		   Form( "min PH map ROC %d", int(roc) ),
		   52, -0.5, 51.5, 80, -0.5, 79.5 );
    h2->SetMinimum(0);
    h2->SetMaximum(256);
    setTitles( h2, "col", "row" );
    h2->GetZaxis()->SetTitle( "<PH> [ADC]" );
    h2->SetStats(0);
    maps0.push_back(h2);
    fHistList.push_back(h2);

    h1 = new TH1D( Form( "minPhDistribution_C%d", int(roc) ),
		   Form( "min Pulse height distribution ROC %d", int(roc) ),
		   256, -0.5, 255.5 );
    setTitles( h1, "<PH> [ADC]", "pixels" );
    h1->SetStats(1);
    hsts0.push_back(h1);
    fHistList.push_back(h1);
  }

  fApi->_dut->testAllPixels(true);

  for( size_t roc = 0; roc < nRocs; ++roc )
    fApi->setDAC( "Vcal", min_vcal[roc], roc );

  LOG(logINFO) << "check against underflow...";

  do {

    vector<pixel> vpix = fApi->getPulseheightMap( flags, fParNtrig );

    LOG(logINFO) << "vpix.size() " << vpix.size();

    // protection against non-responding pixels
    // (distinguish PH zero from no entry)

    for( size_t roc = 0; roc < nRocs; ++roc ) {
      h2 = maps0[roc];
      for( int ix = 0; ix < h2->GetNbinsX(); ++ix )
	for( int iy = 0; iy < h2->GetNbinsY(); ++iy )
	  h2->SetBinContent( ix + 1, iy + 1, 256 ); // set to max
    }

    // data:

    for( size_t ipx = 0; ipx < vpix.size(); ++ipx ) {
      if( vpix[ipx].roc_id == 0 &&
	  vpix[ipx].column == 0 &&
	  vpix[ipx].row == 0 &&
	  vpix[ipx].getValue() == 0 ) {
	LOG(logINFO) << "vpix[" << ipx << "] all zero, skipped";
	continue;
      }
      h2 = maps0.at( vpix[ipx].roc_id );
      if( h2 ) h2->SetBinContent( vpix[ipx].column + 1, vpix[ipx].row + 1,
				  vpix[ipx].getValue());
      h1 = hsts0.at( vpix[ipx].roc_id );
      if( h1 ) h1->Fill( vpix[ipx].getValue());
    }

    for( size_t roc = 0; roc < nRocs; ++roc ) {
      h2 = maps0[roc];
      h2->Draw("colz");
      PixTest::update();
    }
    fDisplayedHist = find( fHistList.begin(), fHistList.end(), h2 );

    // check against underflow:

    again = 0;

    for( size_t roc = 0; roc < nRocs; ++roc ) {

      LOG(logINFO) << "min Ph map for ROC " << roc;

      Int_t locminx,locminy,locminz;
      h2 = maps0[roc];
      h2->GetMinimumBin( locminx, locminy,locminz );
      double phmin = h2->GetBinContent( locminx, locminy );
      LOG(logINFO)
	<< "Ph min " << setw(3) << phmin
	<< " at " << setw(2) << locminx-1 // bin counting starts at 1
	<< "," << setw(2) << locminy-1; // pixel counting starts at 0
      if( phmin < 0.5 ) {
	voffset_opt[roc] -= 5; // increase offset
	viref_adc_opt[roc] += 3; // reduce gain
	fApi->setDAC( "VOffsetRO", voffset_opt[roc], roc );
	fApi->setDAC( "VIref_ADC", viref_adc_opt[roc], roc );
	LOG(logINFO)
	  << "Found underflow "
	  << " VoffsetOp " << setw(3) << voffset_opt[roc]
	  << " VIref_ADC " << setw(3) << viref_adc_opt[roc];
	if( voffset_opt[roc] > 4 && viref_adc_opt[roc] < 253 ) {
	  again = 1;
	  hsts0[roc]->Reset();
	}
      }
      else {
	fApi->_dut->testAllPixels( false, roc ); // remove ROC from list
      }
    } // rocs
  } // do
  while( again );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // We have two extreme points:
  //   the pixel with the highest pulse height
  //   the pixel with the lowest pulse height
  // Two quantities:
  //   mean and range
  // Two parameters:
  //   offset and gain
  // Find optimum:
  //   mean = 133
  //   range = 200

  fApi->_dut->testAllPixels( false ); // mask and disable all

  for( size_t roc = 0; roc < nRocs; ++roc ) {

    LOG(logINFO) << "Ph range fine tuning for ROC " << roc;

    Int_t locminx,locminy,locminz;
    h2 = maps0[roc];
    h2->GetMinimumBin( locminx, locminy,locminz );
    double min_ph = h2->GetBinContent( locminx, locminy );
    uint32_t min_col = locminx - 1; // bin counting starts at 1
    uint32_t min_row = locminy - 1; // pixel counting starts at 0
    LOG(logINFO)
      << "Ph min " << setw(3) << min_ph
      << " at " << setw(2) << min_col
      << "," << setw(2) << min_row;

    Int_t locmaxx,locmaxy,locmaxz;
    h2 = maps9[roc];
    h2->GetMaximumBin( locmaxx, locmaxy,locmaxz );
    double max_ph = h2->GetBinContent( locmaxx, locmaxy );
    uint32_t max_col = locmaxx - 1; // bin counting starts at 1
    uint32_t max_row = locmaxy - 1; // pixel counting starts at 0
    LOG(logINFO)
      << "Ph max " << setw(3) << max_ph
      << " at " << setw(2) << max_col
      << "," << setw(2) << max_row;

    int range = static_cast<int>(max_ph - min_ph);
    int mid = static_cast<int>(max_ph + min_ph)/2;
    int iter = 0;

    // iterate:

    while( ( abs(range-200) > 5 || abs(mid-133) > 5 ) &&
	   voffset_opt[roc] < 255 &&
	   voffset_opt[roc] >   0 &&
	   viref_adc_opt[roc] < 255 &&
	   viref_adc_opt[roc] >   0 ) {

    LOG(logINFO)
      << setw(2) << iter
      << ". gain_dac " << setw(3) << viref_adc_opt[roc]
      << ", offset_dac " << setw(3) << voffset_opt[roc]
      << ": min_ph " << setw(3) << min_ph
      << ", max_ph " << setw(3) << max_ph
      << ": range "  << setw(3) << range
      << ", mid " << setw(3) << mid
      << endl;

      iter++;

      int dmid = 133-mid;
      if( abs(dmid) > 5 ) {
	voffset_opt[roc] -= dmid/2; // slope -1/2;
	if( voffset_opt[roc] < 0 )
	  voffset_opt[roc] = 0;
	else if( voffset_opt[roc] > 255 )
	  voffset_opt[roc] = 255;
	fApi->setDAC( "VOffsetRO", voffset_opt[roc] );
      }

      if(      range > 205 )
	fApi->setDAC( "VIref_ADC", ++viref_adc_opt[roc], roc ); // negative slope
      else if( range < 195 )
	fApi->setDAC( "VIref_ADC", --viref_adc_opt[roc], roc ); // negative slope

      fApi->setDAC( "Vcal", min_vcal[roc], roc );
      fApi->_dut->testPixel( min_col, min_row, true, roc );
      vector<pixel> vpix0 = fApi->getPulseheightMap( flags, fParNtrig );
      
      for( size_t ipx = 0; ipx < vpix0.size(); ++ipx )
	if( vpix0[ipx].roc_id == roc &&
	    vpix0[ipx].column == min_col &&
	    vpix0[ipx].row == min_row )
	  min_ph = vpix0[ipx].getValue();
      fApi->_dut->testPixel( min_col, min_row, false, roc );

      fApi->setDAC( "Vcal", max_vcal, roc );
      fApi->_dut->testPixel( max_col, max_row, true, roc );
      vector<pixel> vpix9 = fApi->getPulseheightMap( flags, fParNtrig );
      for( size_t ipx = 0; ipx < vpix9.size(); ++ipx )
	if( vpix9[ipx].roc_id == roc &&
	    vpix9[ipx].column == max_col &&
	    vpix9[ipx].row == max_row )
	  max_ph = vpix9[ipx].getValue();
      fApi->_dut->testPixel( max_col, max_row, false, roc );

      range = static_cast<int>(max_ph - min_ph);
      mid = static_cast<int>(max_ph + min_ph)/2;

    } // while

    LOG(logINFO)
      << setw(2) << iter
      << ". gain_dac " << setw(3) << viref_adc_opt[roc]
      << ", offset_dac " << setw(3) << voffset_opt[roc]
      << ", min_ph " << setw(3) << min_ph
      << ", max_ph " << setw(3) << max_ph
      << ", range "  << setw(3) << range
      << ", mid " << setw(3) << mid
      << endl;

    LOG(logINFO) << "SetPh Final for ROC " << setw(2) << roc;
    LOG(logINFO) << " 17  VOffsetRO "
		 << setw(3) << (int) fApi->_dut->getDAC( roc, "VOffsetRO" );
    LOG(logINFO) << " 20  VIref_ADC "
		 << setw(3) << (int) fApi->_dut->getDAC( roc, "VIref_ADC" );

  } // rocs

  fApi->setDAC( "Vcal", cal ); // restore
  fApi->setDAC( "CtrlReg", ctl ); // restore
  LOG(logINFO) << "back to CtrlReg " << int(ctl);

  LOG(logINFO) << "PixTestSetPh::doTest() done for " << nRocs << " ROCs";
}
