// -- author: Wolfram Erdmann
// threshold map

#include <sstream>   // parsing
#include <algorithm>  // std::find

#include "PixTestBBMap.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestBBMap)

//------------------------------------------------------------------------------
PixTestBBMap::PixTestBBMap( PixSetup *a, std::string name )
: PixTest(a, name), fParNtrig(-1)
{
  PixTest::init();
  init();
  LOG(logDEBUG) << "PixTestBBMap ctor(PixSetup &a, string, TGTab *)";
}

//------------------------------------------------------------------------------
PixTestBBMap::PixTestBBMap() : PixTest()
{
  LOG(logDEBUG) << "PixTestBBMap ctor()";
}



//------------------------------------------------------------------------------
bool PixTestBBMap::setParameter( string parName, string sval )
{
  for( uint32_t i = 0; i < fParameters.size(); ++i ) {

    if( fParameters[i].first == parName ) {

      stringstream s(sval);

      if( !parName.compare( "Ntrig" ) ) s >> fParNtrig;

      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
void PixTestBBMap::init()
{
  LOG(logDEBUG) << "PixTestBBMap::init()";
  
  fDirectory = gFile->GetDirectory( fName.c_str() );
  if( !fDirectory ) {
    fDirectory = gFile->mkdir( fName.c_str() );
  }
  fDirectory->cd();
}

// ----------------------------------------------------------------------
void PixTestBBMap::setToolTips()
{
  fTestTip = string( "measure threshold map");
  fSummaryTip = string("summary plot to be implemented");
}

//------------------------------------------------------------------------------
void PixTestBBMap::bookHist(string name)
{
  LOG(logDEBUG) << "nothing done with " << name;
}

//------------------------------------------------------------------------------
PixTestBBMap::~PixTestBBMap()
{
  LOG(logDEBUG) << "PixTestBBMap dtor";
  std::list<TH1*>::iterator il;
  fDirectory->cd();
  for( il = fHistList.begin(); il != fHistList.end(); ++il ) {
    LOG(logINFO) << "Write out " << (*il)->GetName();
    (*il)->SetDirectory(fDirectory);
    (*il)->Write();
  }
}

//------------------------------------------------------------------------------
void PixTestBBMap::doTest()
{
  LOG(logINFO) << "PixTestBBMap::doTest() ntrig = " << fParNtrig;

  fDirectory->cd();
  fHistList.clear();
  PixTest::update();


  fApi->_dut->testAllPixels(true); // all pix, all rocs
  fApi->setDAC("ctrlreg", 4);
  fApi->_dut->maskAllPixels(false);
  
  
  int flag= 0; //FLAG_XTALK;
  
  map< int, vector<pixel> > results;
  map< int, vector<pixel> >::iterator map_it;
  
  fApi->setDAC("vcal", 255);
  results[ 255 ] =  fApi->getEfficiencyMap(flag, fParNtrig);
  size_t  nmax = results[255].size();
  
  // binary search for the lowest vcal
  int vlo=0;
  int vhi=255;
  while(vlo < vhi-1){
        int vcal = (vlo+vhi)/2;
        fApi->setDAC("vcal", vcal);
        results[ vcal ] =  fApi->getEfficiencyMap(flag, fParNtrig);
        cout << vcal << " " << results[vcal].size() << endl;
        
        if ( results[vcal].size()>0 ){
            vhi=vcal;
        }else{
            vlo=vcal;
        }
        cout << "vlo " << vhi << " (" << results[vlo].size() << ")  " ;
        cout << "vhi " << vlo << " (" << results[vhi].size() << ")"  << endl;
  }    
  int vmin = vlo;
  
  // binary search for the highest vcal
  vlo=0;
  vhi=255;
  while(vlo < vhi-1){
        int vcal = (vlo+vhi)/2;
        fApi->setDAC("vcal", vcal);
        if (results.find( vcal ) == results.end()){
            results[ vcal ] =  fApi->getEfficiencyMap(flag, fParNtrig);
        }
        cout << vcal << " " << results[vcal].size() << endl;
        
        if ( results[vcal].size()==nmax ){
            vhi=vcal;
        }else{
            vlo=vcal;
        }
        cout << "vlo " << vhi << " (" << results[vlo].size() << ")  " ;
        cout << "vhi " << vlo << " (" << results[vhi].size() << ")"  << endl;
  }    
  int vmax = vhi;
    
        
  cout << "scan " << vmin << " ... " << vmax << endl;
  
  TH1* h=new TH2D("BB","BB",52,0,52,80,0,80);
  for(int vcal=vmin; vcal<vmax; vcal++){
     fApi->setDAC("vcal", vcal);
    if (results.find( vcal ) == results.end()){
        results[ vcal ] =  fApi->getEfficiencyMap(flag, fParNtrig);
    }
    cout << "vcal=" << vcal << "  " << results[vcal].size() << endl;

    for(vector<pixel>::iterator pix_it=results[vcal].begin();
                      pix_it != results[vcal].end(); pix_it++){
        pixel & pix = *pix_it;
        if (( pix.value>5 ) && (h->GetBinContent( pix.column, pix.row ) ==0)){
            h->SetBinContent( pix.column, pix.row, vcal);
        }
    }
  } 

   //map_it = results.begin();
   //for ( ; map_it !=results.end(); map_it++){
   //    cout << (*map_it).first << endl;
  // }
        
  
  //vector<TH1*> maps = thrMaps("vcal", "BB", fParNtrig); 
  //vector<TH1> maps;
  //for (unsigned int i = 0; i < maps.size(); ++i) {
  //  fHistOptions.insert(make_pair(maps[i], "colz"));
  //}

  //copy(maps.begin(), maps.end(), back_inserter(fHistList));

  fHistList.push_back( h );
  //TH2D *h = (TH2D*)(*fHistList.begin());

  //h->Draw(getHistOption(h).c_str());
  h->Draw("colz");
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
  PixTest::update(); 
 

  //LOG(logINFO) << "PixTestBBMap::doTest() done for " << maps.size() << " ROCs";

}
