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
      if( !parName.compare( "SubtractXtalk") ){ s >> fParSubtractXtalk; return true;}
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
// eventually to be replaced by PixTest::thrMaps
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
  // hits seem to appear earlier in this mode
  std::vector< std::pair<uint8_t, std::vector<pixel> > > a;
  while(( npfnd < 10) && (vthr < dachi) ){
    a = fApi->getEfficiencyVsDAC("VthrComp", vthr, vthr+dvt-1, flag, 1);
    for(unsigned int i=0; i<dvt; i++){
        npfnd = a.at(i).second.size();
        if ( npfnd>=10 )break;
        vthr++;
    }
  }
  
  LOG(logDEBUG) << "fMaps: vthr=" << (int)vthr << "  npx= " << npfnd;

  
  size_t npix = rocIds.size()*ROC_NUMROWS*ROC_NUMCOLS;
  unsigned int maxNtrig =  DTB_SOURCE_BUFFER_SIZE / (npix*6);
  uint16_t ntrig = ntrigrequest < maxNtrig ? ntrigrequest : maxNtrig;

  /*
  // assuming we have a reasonable threshold now, get a pixel alive 
  // map to spot dead pixels (to not waste time on them during threshold scans)
  fApi->setDAC("vcal", 255);
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  vector<pixel> pxalive = fApi->getEfficiencyMap(FLAG_FORCE_MASKED, ntrig);
  vector< bool > alive(npix, false);
  size_t npixalive=0;
  for(vector<pixel>::iterator pix_it=pxalive.begin();
      pix_it != pxalive.end(); pix_it++){
      alive[ pidx(*pix_it) ] = (pix_it->value > 0);
      if ( alive[ pidx(*pix_it) ] )npixalive++;
  }  
  LOG(logDEBUG) << "fMaps:  alive " << npixalive;
  if (npixelalive==0){
      LOG(logINFO) << "DUT appears dead ??";
  }

  fApi->setDAC("vcal", fParVcalS);    
*/
 
  // now scan the VthrComp and find thresholds
  vector< int > x1( npix, 0 ), n1( npix, 0);
  vector< float > vt(npix, -1);
  dvt = 1;
  npfnd=0;
  while( (npfnd<fNpixalive) && (vthr<dachi) ){
      
    fApi->setDAC("VthrComp", vthr);
    vector<pixel> result = fApi->getEfficiencyMap(flag | FLAG_FORCE_MASKED, ntrig);
    
    for(vector<pixel>::iterator pix_it=result.begin();
      pix_it != result.end(); pix_it++){
      pixel & pix = *pix_it;
      int i = pidx( pix );
      if ( ( pix.value >0) && fAlive[i] && (vt[i]<0) ) {
          if (pix.value <ntrig/2) {
            n1[i]=pix.value;
            x1[i]=vthr;
          } else if (pix.value>=ntrig/2){
            if (n1[i]==0){
                vt[i]=vthr;
            }else{
              int n2 = pix.value;
              int x2 = vthr;
              vt[i] = float(n2*x1[i]-n1[i]*x2+0.5*ntrig*(x2-x1[i]))/(n2-n1[i]);
           }
            npfnd+=1;
          } // above threshold
      } // has hits
    }//pixel
    

    LOG(logDEBUG) << "vthr=" << (int) vthr << "  found " << npfnd << " / " << fNpixalive;

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
          int i = iroc*4160 + icol*80 + irow; 
          if (vt[i]>0)  {
            h1->SetBinContent( icol+1, irow+1, vt[i]);
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
               << " VcalS = " << fParVcalS
               << " Xtalk = " << fParSubtractXtalk << endl;

   // save dacs
   fSavedDACs.clear();
   saveDAC("vthrcomp");
   saveDAC("ctrlreg");
   saveDAC("vcal");
   
  fDirectory->cd();
  fHistList.clear();
  PixTest::update();

  // make a pixel alive test and ignore dead pixels
  fApi->setDAC("vcal", 255);
  fApi->setDAC("ctrlreg", 4);
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();
  size_t npix = rocIds.size()*ROC_NUMROWS*ROC_NUMCOLS; 
  vector<pixel> pxalive = fApi->getEfficiencyMap(FLAG_FORCE_MASKED, fParNtrig);
  fAlive.reserve(npix);
  fAlive.assign(npix, false);
  fNpixalive=0;
  for(vector<pixel>::iterator pix_it=pxalive.begin();
      pix_it != pxalive.end(); pix_it++){
      fAlive[ pidx(*pix_it) ] = (pix_it->value > 0);
      if ( fAlive[ pidx(*pix_it) ] )fNpixalive++;
  }  
  LOG(logDEBUG) << "fMaps:  alive " << fNpixalive;
  if (fNpixalive==0){
      LOG(logINFO) << "DUT appears dead ??";
  }


  int flag= FLAG_CALS;
  fApi->setDAC("ctrlreg", 4);     // high range
  fApi->setDAC("vcal", fParVcalS);    

  if (fPartest>0){ 
    flag = 0;
    fApi->setDAC("ctrlreg", 0);      // low range
    fApi->setDAC("vcal", fPartest);
  } 
   
  
  vector<TH1*>  thrmapsXtalk;    
  if (fParSubtractXtalk){
      LOG(logDEBUG) << "taking Xtalk maps";
      thrmapsXtalk = thrMaps("VthrComp", "calSMapXtalk", 0, 150, fParNtrig, flag | FLAG_XTALK); 
      LOG(logDEBUG) << "Xtalk maps done";
  }
  
  LOG(logDEBUG) << "taking CalS threshold maps";
  vector<TH1*>  thrmapsCals = thrMaps("VthrComp", "calSMap", 0, 150, fParNtrig, flag);


  //vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int idx = 0; idx < rocIds.size(); ++idx){
      
      unsigned int rocId = getIdFromIdx( idx );
      TH2D* rocmapRaw = (TH2D*) thrmapsCals[idx];
      TH2D* rocmapBB  = (TH2D*) rocmapRaw->Clone(Form("BB-%2d",rocId));
      rocmapBB->SetTitle(Form("bump bonding %2d",rocId));
      TH2D* rocmapXtalk = 0;
      
      if (fParSubtractXtalk) {
        rocmapXtalk = (TH2D*) thrmapsXtalk[idx];  
        fHistList.push_back( rocmapRaw );
        fHistList.push_back( rocmapXtalk );
        rocmapBB->Add( rocmapXtalk, -1. );
      }
      fHistOptions.insert(make_pair( rocmapBB, "colz" ));
      fHistList.push_back( rocmapBB );


      // fill distribution from map
      TH1D* hdist = bookTH1D( 
        Form("CalS-threshold-distribution-%0d",rocId), 
        Form("CalS-threshold distribution %2d",rocId), 
        257, 0., 257.);
      TH1D* hdistXtalk = bookTH1D( 
        Form("CalS-Xtalk-distribution-%0d",rocId), 
        Form("CalS-Xtalk distribution %2d",rocId), 
        257, 0., 257.);
      TH1D* hdistBB = bookTH1D( 
        Form("CalS-Xtalksubtracted-distribution-%0d",rocId), 
        Form("CalS-Xtalksubtracted distribution %2d",rocId), 
        514, -257., 257.);
      
      for(int col=0; col<ROC_NUMCOLS; col++){
          for(int row=0; row<ROC_NUMROWS; row++){
              hdist->Fill( rocmapRaw->GetBinContent( col+1, row+1 ) );
              if (fParSubtractXtalk) {
                hdistXtalk->Fill( rocmapXtalk->GetBinContent( col+1, row+1 ) );
                hdistBB->Fill( rocmapBB->GetBinContent( col+1, row+1 ) );
              }
          }
      }

      fHistList.push_back( hdist);
      if (fParSubtractXtalk) {
          fHistList.push_back(hdistXtalk);
          fHistList.push_back(hdistBB);
      }
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
