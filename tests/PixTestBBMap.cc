// -- author: Wolfram Erdmann
// Bump Bonding tests, really just a threshold map using cals

#include <sstream>   // parsing
#include <algorithm>  // std::find

#include "PixTestBBMap.hh"
#include "log.h"
#include "constants.h"   // roctypes

using namespace std;
using namespace pxar;

ClassImp(PixTestBBMap)

//------------------------------------------------------------------------------
PixTestBBMap::PixTestBBMap( PixSetup *a, std::string name )
: PixTest(a, name), fParNtrig(-1), fParVcalS(200),fPartest(0)
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

      if( !parName.compare( "Ntrig" ) ) { s >> fParNtrig; return true;}
      if( !parName.compare( "VcalS" ) ) { s >> fParVcalS; return true;}
      if( !parName.compare( "test" ) )  { s >> fPartest;  return true;}
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
  fTestTip = string( "Bump Bonding Test = threshold map for CalS");
  fSummaryTip = string("module summary");
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


// ----------------------------------------------------------------------
vector<TH1*> PixTestBBMap::thrMaps(string dac, string name, uint8_t daclo, uint8_t dachi, unsigned int ntrigrequest, uint16_t flag) {
  vector<TH1*>   resultMaps; 
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 

  if (rocIds.size()==0){
      return resultMaps;
  }
  
  fApi->_dut->testAllPixels(true); // all pix, all rocs
  fApi->_dut->maskAllPixels(false);
  
  map< int, vector<pixel> > results;
  uint8_t vthr = daclo;
  unsigned int dvt = 10;
  size_t npfnd = 0;
  // rough scan from high threshold until someone responds, all pixels enabled
  std::vector< std::pair<uint8_t, std::vector<pixel> > > a;
  while(( npfnd == 0) && (vthr < dachi) ){
    a = fApi->getEfficiencyVsDAC("VthrComp", vthr, vthr+dvt-1, flag, 1);
    for(unsigned int i=0; i<dvt; i++){
        npfnd = a.at(i).second.size();
        if ( npfnd>0 )break;
        vthr++;
    }
  }
  
  LOG(logDEBUG) << "fMaps: vthr=" << (int)vthr << "  npx= " << npfnd;
  
  
  // from now only enable one pixel at a time  ( flag=FLAG_FORCE_MASKED )
  size_t npix = rocIds.size()*ROC_NUMROWS*ROC_NUMCOLS;
  unsigned int maxNtrig =  DTB_SOURCE_BUFFER_SIZE / (npix*6);
  uint16_t ntrig = ntrigrequest < maxNtrig ? ntrigrequest : maxNtrig;

 
  vector< int > x1( npix, 0 ), n1( npix, 0);
  vector< float > vt(npix, -1);


  dvt = 1;
  npfnd=0;
  while( (npfnd<npix) && (vthr<dachi) ){
      
    fApi->setDAC("VthrComp", vthr);
    vector<pixel> result = fApi->getEfficiencyMap(flag | FLAG_FORCE_MASKED, ntrig);
    
    for(vector<pixel>::iterator pix_it=result.begin();
      pix_it != result.end(); pix_it++){
      pixel & pix = *pix_it;
      if ( pix.value >0) {
        int pidx = getIdxFromId(pix.roc_id)*4160+pix.column*80+pix.row;
        if (vt[pidx]<0) {
          if (pix.value <ntrig/2) {
            n1[pidx]=pix.value;
            x1[pidx]=vthr;
          } else if (pix.value>=ntrig/2){
            if (n1[pidx]==0){
                vt[pidx]=vthr;
            }else{
              int n2 = pix.value;
              int x2 = vthr;
              vt[pidx] = float(n2*x1[pidx]-n1[pidx]*x2+0.5*ntrig*(x2-x1[pidx]))/(n2-n1[pidx]);
           }
            npfnd+=1;
          } // above threshold
        } // not done yet
      } // has hits
    }//pixel
    

    LOG(logDEBUG) << "vthr=" << (int) vthr << "  found " << npfnd << " / " << npix;

    //if ((npfnd> npix/2) && (result.size()==0)) break; // probably above the noise level
    vthr += dvt;
    
    
  }
  
  
  // create and fill histograms 
  TH2D*  h1 = 0;
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
      int roc_id = getIdFromIdx( iroc );
      h1 = bookTH2D(Form("thr_%s_%s_C%d", name.c_str(), dac.c_str(), roc_id), 
		    Form("thr_%s_%s_C%d", name.c_str(), dac.c_str(), iroc), 
		    52, 0., 52., 80, 0., 80.);
      resultMaps.push_back(h1); 
      fHistOptions.insert(make_pair(h1, "colz"));
      
      for(int icol=0; icol<52; icol++){
        for(int irow=0; irow<80; irow++){
          int pidx = iroc*4160 + icol*80 + irow; 
          if (vt[pidx]>0)  {
            h1->SetBinContent( icol+1, irow+1, vt[pidx]);
          }else{
            h1->SetBinContent( icol+1, irow+1, 256.);
          }
        }
      }  
  }

  // copy ( ? )
  copy(resultMaps.begin(), resultMaps.end(), back_inserter(fHistList));
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h1);
  if (h1) h1->Draw(getHistOption(h1).c_str());
  PixTest::update();   
  
  return resultMaps;
}



  void PixTestBBMap::saveDAC( std::string dac){
    vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
    vector< uint8_t> values( 0 );
    for(size_t i=0; i<rocIds.size(); i++){
        values.push_back( fApi->_dut->getDAC( rocIds[i], dac ) );
    }
    fSavedDACs[ dac ] = values;
  }
  
  void PixTestBBMap::restoreDACs(){
    vector< uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();   
    for( std::map< std::string, vector< uint8_t> >::iterator 
        it = fSavedDACs.begin(); it != fSavedDACs.end(); it++){
        for(size_t i=0; i<rocIds.size(); i++){
            fApi->setDAC(it->first, it->second[i], rocIds[i]);
        }
    }
  }

//------------------------------------------------------------------------------
void PixTestBBMap::doTest()
{
  LOG(logINFO) << "PixTestBBMap::doTest() Ntrig = " << fParNtrig
               << " VcalS = " << fParVcalS << endl;

   // save dacs
   fSavedDACs.clear();
   saveDAC("vthrcomp");
   saveDAC("ctrlreg");
   saveDAC("vcal");
   
  fDirectory->cd();
  fHistList.clear();
  PixTest::update();


  int flag= FLAG_CALS;
  fApi->setDAC("ctrlreg", 4);     
  fApi->setDAC("vcal", fParVcalS);    

  if (fPartest>0){ 
    flag = 0;
    fApi->setDAC("ctrlreg", 0);      // low range
    fApi->setDAC("vcal", fPartest);
  } // tests 
   
  bool subtractXtalk = false;
  
  if (subtractXtalk){ cout << "just to avoid compiler warnings" << endl;}
  
  vector<TH1*>  thrmaps = thrMaps("VthrComp", "VcalSMap", 0, 150, fParNtrig, flag); 

    

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int idx = 0; idx < rocIds.size(); ++idx){
      
      unsigned int rocId = getIdFromIdx( idx );
      TH2D* rocmap = (TH2D*) thrmaps[idx];
      
      /*      
      if (subtractXtalk) {
        fHistList.push_back( rocmap0 );
        rocmap = (TH2D*) (mit->second)->Clone(Form("BB %2d diff",rocId));
        rocmap->Add( rocmap0, -1. );
        fHistOptions.insert(make_pair( rocmap, "colz" ));
        fHistList.push_back( rocmap );
      }
      */
      // fill distribution from map
      TH1D* hdist = bookTH1D( //new TH1D( 
        Form("CalS-threshold-distribution-%0d",rocId), 
        Form("CalS-threshold distribution %2d",rocId), 
        257, 0., 257.);
      
      for(int col=0; col<ROC_NUMCOLS; col++){
          for(int row=0; row<ROC_NUMROWS; row++){
              hdist->Fill( rocmap->GetBinContent( col+1, row+1 ) );
          }
      }
      fHistList.push_back( hdist );
  }
  
  TH2D * module =  (TH2D*) moduleMap("BB module map");
  fHistList.push_back( module);
  
  if (fHistList.size()>0){
    TH2D *h = (TH2D*)(*fHistList.begin());
    h->Draw(getHistOption(h).c_str());
    fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
    PixTest::update(); 
  }else{
    LOG(logDEBUG) << "no histogram produced, this is probably a bug";
  }
  
  restoreDACs();

}
