// -- author: Wolfram Erdmann
// get analog current vs vana from testboard and roc readback

#include <algorithm>  // std::find
#include "PixTestShowIana.hh"
#include "log.h"
#include "constants.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestShowIana)

//------------------------------------------------------------------------------
PixTestShowIana::PixTestShowIana( PixSetup *a, std::string name )
: PixTest(a, name)
{
  PixTest::init();
  init();
}

//------------------------------------------------------------------------------
PixTestShowIana::PixTestShowIana() : PixTest()
{
}

//------------------------------------------------------------------------------
bool PixTestShowIana::setParameter( string parName, string sval )
{
   if  (parName == sval) return true; // silence the compiler warnings
  return true;
}

//------------------------------------------------------------------------------
void PixTestShowIana::init()
{
  LOG(logINFO) << "PixTestShowIana::init()";
  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory )
    fDirectory = gFile->mkdir( fName.c_str() );
  fDirectory->cd();
}

// ----------------------------------------------------------------------
void PixTestShowIana::setToolTips()
{
  fTestTip = string( "show analog current vs Vana\n measured by testboard and roc readback");
  fSummaryTip = string("summary plot to be implemented");
}

//------------------------------------------------------------------------------
void PixTestShowIana::bookHist(string name)
{
  LOG(logDEBUG) << "nothing done with " << name;
}

//------------------------------------------------------------------------------
PixTestShowIana::~PixTestShowIana()
{
  LOG(logDEBUG) << "PixTestShowIana dtor";
  std::list<TH1*>::iterator il;
  fDirectory->cd();
  for( il = fHistList.begin(); il != fHistList.end(); ++il ) {
    LOG(logINFO) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory);
    (*il)->Write();
  }
  
}


//------------------------------------------------------------------------------
uint8_t PixTestShowIana::readRocADC(uint8_t adc)
{
    fApi->setDAC("readback", adc);

    //read 32 events
    fApi->daqTrigger(32);
    vector<pxar::Event> events = fApi->daqGetEventBuffer();
    
    if ( events.size()<32 ){
        cout << "only got " << events.size() << endl;
        return 0;
    }

    // and extract the readback data from the header bits
    uint8_t value=0;
    int n=-1; // ignore everything before the start marker
    
    //for( auto e : events){ // -std=c++0x/c++11 would be nice    
    for(vector<pxar::Event>::iterator ie=events.begin(); ie!=events.end(); ie++){
        pxar::Event & e= *ie; 

        if ( ( n>=0 ) && (n<16) ){
            value = (value << 1 ) + (e.header & 1);
            n++;
        }
        else if ( ((e.header & 2) >>1) == 1 ){
            n=0;
        }
    }
    
    //int roc_readback  = ( value & 0xF000 ) >> 12;
    //int cmd_readback  = ( value & 0x0F00 ) >>  8;
    uint8_t data = value & 0x00FF;
    
    return data;
}


//------------------------------------------------------------------------------
void PixTestShowIana::doTest()
{
  LOG(logINFO) << "PixTestShowIana::doTest() " ;

  fDirectory->cd();
  fHistList.clear();
  PixTest::update();

   TH1D* h1 = new TH1D( 
    Form( "iatb_vs_vana_roc_%02d",0),
    Form( "iatb vs vana roc %2d",0),
            256, 0,256) ;
   h1->SetMinimum(0);
   TH1D* h2 = new TH1D( 
    Form( "iaroc_vs vana_roc_%02d",0),
    Form( "iaroc vs vana roc %2d",0),
            256, 0,256) ;
    h2->SetMinimum(0);
   

  uint8_t savedvana = fApi->_dut->getDAC( 0, "Vana" );
  fApi->_dut->maskAllPixels(true);
  
  //size_t nRocs = fPixSetup->getConfigParameters()->getNrocs();
  vector<uint8_t> dac2save;
  
  fApi->daqStart();

  for(int dacvalue=0; dacvalue<=256; dacvalue++){
    fApi->setDAC("Vana", dacvalue);
    double ia1 = fApi->getTBia()*1e3;
    double ia=0;
    int n=0;
    for(;n<10; n++){
        ia = fApi->getTBia()*1e3;
        if ( TMath::Abs(ia-ia1) < 1 ) break;
        ia1=ia;
    }


    int ia2 = readRocADC( 12 );
    h1->SetBinContent( dacvalue, ia);
    h2->SetBinContent( dacvalue, ia2);
  }
  
  fApi->daqStop();

  // plot:

  fHistList.push_back(h1);
  fHistList.push_back(h2);

  h1->Draw();
  h2->Draw();
  PixTest::update();
  fDisplayedHist = find( fHistList.begin(), fHistList.end(), h1 );

  // restore the previous state
  fApi->setDAC( "Vana", savedvana );
}

