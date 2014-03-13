// -- author: Wolfram Erdmann
// Bump Bonding tests

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
: PixTest(a, name), fParNtrig(-1), fParMethod("ph"), fParVcalS(255),fPartest(0)
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
      if( !parName.compare( "Method" ) ) s >> fParMethod;
      if( !parName.compare( "VcalS" ) ) s >> fParVcalS;
      if( !parName.compare( "test" ) ) s >> fPartest;

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
  fTestTip = string( "Bump Bonding Test");
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
int PixTestBBMap::search(map< int, vector<pixel> > & results, size_t nmax, int flag)
{
  // TODO : do this on a per ROC basis
  int vlo=0;
  int vhi=255;
  while(vlo < vhi-1){
    int vcal = (vlo+vhi)/2;
    fApi->setDAC("vcal", vcal);
    results[ vcal ] =  fApi->getEfficiencyMap(flag, fParNtrig);
    LOG(logDEBUG) << vcal << " " << results[vcal].size();
    if (results.find( vcal ) == results.end()){
      results[ vcal ] =  fApi->getEfficiencyMap(flag, fParNtrig);
    }
        
    if ( results[vcal].size() >= nmax ){
      vhi=vcal;
    }else{
      vlo=vcal;
    }

    LOG(logDEBUG) << "vlo " << vhi << " (" << results[vlo].size() << ")  " 
		  << "vhi " << vlo << " (" << results[vhi].size() << ")";
  }  
   
  if (nmax==0) {
    return vlo; // lower bound
  }else{
    return vhi; // upper bound
  }
}


//------------------------------------------------------------------------------
map< uint8_t, TH1* > PixTestBBMap::doThresholdMap(
    string label,
    int vmin, int vmax, int step,
    int flag)
{
  // get vcal threshold maps for the rocs
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  
  // cache results
  map< int, vector<pixel> > results;
  map< int, vector<pixel> >::iterator map_it;
  
  // number of pixels that respond at all
  fApi->setDAC("ctrlreg", 4);      // high range
  fApi->setDAC("vcal", 255);
  results[ 256 ] =  fApi->getEfficiencyMap(flag, fParNtrig);
  size_t  nmax = results[256].size();
  
  // measure the threshold (low range)
  fApi->setDAC("ctrlreg", 0);      // low range
  // find the range of values we need to scan
  if(vmax ==0 ){
    vmin = search( results,    0, flag);
    vmax = search( results, nmax, flag);
  }
  
  // create the histograms holding the result
  map< uint8_t, TH1* > h;
  for( size_t i=0; i<rocIds.size(); i++){
    uint8_t rocId = rocIds[i];
    h[rocId] = bookTH2D(Form(("BB-%02d-"+label).c_str(),rocId),
                      Form(("BB %02d "+label).c_str(),rocId),
                      52,0,52,80,0,80);
    fHistOptions.insert(make_pair(h[rocId], "colz"));
  }


  // scan vcal and fill the histograms with the threshold value
  if (vmin>=vmax){
      LOG(logINFO) << "vmin= "<< vmin << "   vmax = " << vmax << "!  Please debug!";
  }
  int dv= (vmax-vmin)>(2*step) ? step : 1;

  
  for(int vcal=vmin; vcal<vmax; vcal+=dv){
      
    // get the efficiency map for this vcal unless it is already available
    if (results.find( vcal ) == results.end()){
      fApi->setDAC("vcal", vcal);
      results[ vcal ] =  fApi->getEfficiencyMap(flag, fParNtrig);
    }
  
    // find pixels that crossed the threshold
    for(vector<pixel>::iterator pix_it=results[vcal].begin(); 
    pix_it != results[vcal].end(); pix_it++){
      pixel & pix = *pix_it;
      try{
        if (( pix.value>=fParNtrig/2 ) && (h[pix.roc_id]->GetBinContent( pix.column+1, pix.row+1 ) ==0))
        {
            h.at(pix.roc_id)->SetBinContent( pix.column+1, pix.row+1, vcal);
        }
      }catch (const std::out_of_range & e) {
          LOG(logDEBUG) << "this should not have happened";
      }
    }
  } 

  return h;
}



//------------------------------------------------------------------------------
void PixTestBBMap::phScan( 
  vector< uint8_t >   & rocIds, 
  map< uint8_t, int> & vmin, 
  map< uint8_t, int> & phmin, 
  map< uint8_t, int> & phmax,
  map< uint8_t, float > & phslope)
{
  for( size_t iroc=0; iroc<rocIds.size(); iroc++ ){
    uint8_t rocId = rocIds[iroc];
    fApi->_dut->testPixel(20, 20, true, rocId); // fixme, allow other choices
    fApi->_dut->maskPixel(20, 20, false);
  }
  
  vector< pair< uint8_t, vector<pixel> > > 
          result = fApi->getPulseheightVsDAC( "Vcal", 0, 255, 0, fParNtrig );
                    
  vmin.clear();
  phmin.clear();
  phmax.clear();
  phslope.clear();
  map< uint8_t, int> vlo, phlo, vhi, phhi; // temp. for slope finding
  map<uint8_t, int> v2, v1; 
  
  for( unsigned int i = 0; i < result.size(); ++i ) {
    int dac = result[i].first;
    vector<pixel> vpix = result[i].second;
    
    for( unsigned int ipx = 0; ipx < vpix.size(); ++ipx ) {
        // threshold (not used )
        uint8_t roc = vpix[ipx].roc_id;
        setmin(vmin, roc, dac);
        // max range
        int ph = vpix[ipx].value;
        setmin(phmin, roc, ph);
        setmax(phmax, roc, ph);
        // slope
        if ( (ph>8) && (vlo[roc]==0) ){
            vlo[roc]=dac;
            phlo[roc]=ph;
        }
        if ( (ph < 256-8) && (ph>phhi[roc]) ){
            vhi[roc] = dac;
            phhi[roc] = ph;
        }
            
    } // pix

  } // vcal dac values

  for(uint8_t iroc=0; iroc<rocIds.size(); iroc++){
      uint8_t rocId = rocIds[iroc];
      if (vhi[iroc]>vlo[iroc]){
        phslope[rocId] = ( phhi[iroc]-phlo[iroc] )/float(vhi[iroc]-vlo[iroc]);
      }else{
        phslope[rocId]=0;
      }
  }

} 


//------------------------------------------------------------------------------
// adjust the two PH dacs of a psi46digv21 ROC for good range for small pulse-heights
void PixTestBBMap::adjustPHDacsDigv21()
{    
  map<uint8_t, int> vmin, phmin, phmax, phOffset, phScale;
  map<uint8_t, float> phslope;
  
  for(uint8_t iroc=0; iroc<rocIds.size(); iroc++){
      uint8_t rocId = rocIds[iroc];
      phOffset[ rocId ] = 200;
      phScale[ rocId ] =100;
  }  
  
  // target regions, FIXME make adjustable
  int phmin1=16, phmin2=32;
  int phmax1=224, phmax2=240;
  float smin=0.8, smax=1.0;

  int nit=0;
  while( ++ nit < 100){
      
    for( size_t iroc=0; iroc<rocIds.size(); iroc++ ){
       uint8_t rocId = rocIds[iroc];
       fApi->setDAC("CtrlReg",   0, rocId);
       fApi->setDAC("PHoffset", phOffset[rocId], rocId);
       fApi->setDAC("PHscale",   phScale[rocId], rocId);
    }
      
    phScan( rocIds, vmin, phmin, phmax, phslope);
     
    bool done = true;
    for(uint8_t iroc=0; iroc<rocIds.size(); iroc++){
      uint8_t rocId = rocIds[iroc];
        
      // skip this roc when we're happy  
      if (   ( phmin[rocId]>phmin1 ) && (phmin[rocId] < phmin2 )
          && ( phmax[rocId]>phmax1 ) && (phmax[rocId] < phmax2 ) ) continue;
          
      // otherwise adjust slope ...
      
      if ( (phslope[rocId]>0) && ( phslope[rocId]< smin ) ){
         // increase slope (decrease phScale)
         int ds  = min(16, int( 1+ 0.05*phScale[rocId] / phslope[rocId]) ); 
         if (phScale[rocId]>8+ds){
             phScale[rocId]-=ds;
             done=false;
         }
     }
     
     else if ( (phslope[rocId]> smax ) ){
         // decrease slope (increase phScale)
         int ds  = min(16, int( 1+ 0.05*phScale[rocId] / phslope[rocId]) ); 
         if (phScale[rocId] > 255-ds){
             phScale[rocId]+=ds;
             done=false;
         }
     }

     // ... and offset
     
     if (  ( phmin[rocId]< phmin1 ) & (phmax[rocId]< phmax2) ){
         // increase offset
         if (phOffset[rocId]<255-8){ 
             phOffset[rocId]+=8; 
             done=false;
         };
     }
     
     else if( (phmin[rocId]>phmin2) & (phmax[rocId]>phmax1) ){
         //decrease offset
         if (phOffset[rocId] > 8){ 
             phOffset[rocId]-=8; 
             done=false;
         }
     } 

    }// iroc
    if (done) break;
  }  // while 

}

map< uint8_t, TH1*> PixTestBBMap::fillRocHistograms( 
    vector<pixel> & result, 
    string title )
{
  map<uint8_t, TH1*> h;
  h.clear();
  
  string hid(title);
  hid.replace(hid.begin(), hid.end(), ' ', '-');
  // make histograms
  for( size_t i=0; i<rocIds.size(); i++){
    uint8_t rocId = rocIds[i];
    h[rocId] = bookTH2D( Form( hid.c_str()  ,rocId ),
                         Form( title.c_str(),rocId ),
                      52,0,52,80,0,80 );
    fHistOptions.insert(make_pair(h[rocId], "colz"));
  }
  
  // fill
  for( unsigned int ipx = 0; ipx < result.size(); ++ipx ) {
      pixel & pix = result[ipx];
      try{
            h.at(pix.roc_id)->SetBinContent( pix.column+1, pix.row+1, pix.value);
      }catch (const std::out_of_range & e) {
          LOG(logDEBUG) << "this should not have happened";
      }  
  }
  
  return h;
}


//------------------------------------------------------------------------------
map< uint8_t, TH1* > PixTestBBMap::doPulseheightMap(
    string label,
    bool adjustPH,
    int flag)
{
  // get pulse-height maps for the rocs
  // returns a map  rocid -> histogram
  map<uint8_t, TH1*> h;  
  
  rocIds = fApi->_dut->getEnabledRocIDs();
  size_t nroc = rocIds.size();
  if (nroc==0){
      LOG(logCRITICAL) << "nothing to test";
      return h;
  }
  
  // uint8_t roctype = fApi->_dut->?????
  uint8_t roctype = ROC_PSI46DIGV21;

  
  if (adjustPH) {
    // arm test pixels and sweep vcal, low range
    fApi->_dut->testAllPixels(false);
    fApi->_dut->maskAllPixels(true);
  
    if ( roctype == ROC_PSI46DIGV21 ) {
      adjustPHDacsDigv21(); // assuming all rocs are of the same type of course
    }
  }
  
  // ph dacs adjusted, now get pulse-height maps
  fApi->_dut->testAllPixels(true);
  fApi->_dut->maskAllPixels(false);
  
  fApi->setDAC("CtrlReg", 0); // acts on all ROCs  
  if (fPartest == 0){
    fApi->setDAC("Vcal", fParVcalS);
  }else{
    fApi->setDAC("Vcal", fPartest);
    flag = 0;
  }
  
  vector <pixel> result = fApi->getPulseheightMap(flag, fParNtrig); 
  map< uint8_t, TH1* > hraw = fillRocHistograms( result, "BB %02d "+label);

  if(false){
      // raw
      return hraw;
  }
  
  
  // simple pulse-height calibration correction
  int step=16;
  map<int, map< uint8_t, TH1* > > ph;
  for(int vcal=0; vcal<255; vcal+=step){
    fApi->setDAC("Vcal", vcal);
    vector <pixel> result = fApi->getPulseheightMap(flag, fParNtrig);
    string title( Form("phmap vcal %3d ",vcal) );
    ph[vcal] = fillRocHistograms( result, title+" %2d "+label);
  }
  
  vector<pixel> empty;
  map<uint8_t, TH1*> phcal = fillRocHistograms( empty , "calibrated %2d "+label );
  
  for(int vcal=0; vcal<255-step; vcal+=step){
    for(uint8_t iroc=0; iroc<rocIds.size(); iroc++){
      uint8_t rocid= rocIds[iroc];
      for(int col=0; col<ROC_NUMCOLS; col++){
        for(int row=0; row<ROC_NUMROWS; row++){
          double ph0 = ph[vcal     ][rocid]->GetBinContent( col+1, row+1);
          double ph1 = ph[vcal+step][rocid]->GetBinContent( col+1, row+1);
          double phraw = hraw[rocid]->GetBinContent( col+1, row+1);
          double phv  = phcal[rocid]->GetBinContent( col+1, row+1);
          if ((phraw > ph0) && (phraw <= ph1)){
              phv =  vcal+step*(phraw-ph0)/(ph1-ph0);
              phcal[rocid]->SetBinContent( col+1, row+1, phv);
          }else if( ( phv == 0) && (ph0  > phraw) ){
            phcal[rocid]->SetBinContent( col+1, row+1, ph0);
            /*
            cout <<  "col " << col << "  row "  << row << "  raw=" << phraw
            << "    vcal=" << vcal << ":" << ph0
            << "    vcal=" << vcal+step << ":" << ph1 
            <<endl;
             */
          }
       }
    }  
   }
  }
  return phcal;
}

//------------------------------------------------------------------------------
void PixTestBBMap::doTest()
{
  LOG(logINFO) << "PixTestBBMap::doTest() ntrig = " << fParNtrig;

  fDirectory->cd();
  fHistList.clear();
  PixTest::update();

  fApi->setDAC("ctrlreg", 0);      // low range
  fApi->_dut->testAllPixels(true); // all pix, all rocs
  fApi->_dut->maskAllPixels(false);

  
  int flag= FLAG_CALS;
  if (fPartest>1){ flag = 0;} // tests  
  bool subtractXtalk = false;
  
  map< uint8_t, TH1* > maps;  
  map< uint8_t, TH1* > maps0;

  if (fParMethod == "thr"){
      
    // threshold Method
    maps  = doThresholdMap( "cal",  0, 0 , 4, flag );
    if (subtractXtalk) maps0 = doThresholdMap( "xtalk",0, 0 , 4, flag | FLAG_XTALK );
    
  }else if ( fParMethod == "ph" ){
      
    // pulse-height method 
    maps  = doPulseheightMap("cal", true, flag);
    if (subtractXtalk) maps0 = doPulseheightMap("xtalk", false, flag | FLAG_XTALK );

  }


 
  for(map< uint8_t, TH1*>::iterator mit=maps.begin(); mit !=maps.end(); mit++){
      
      fHistList.push_back( mit->second );
      TH2D* rocmap= (TH2D*) mit->second;
      
      if (subtractXtalk) {
        fHistList.push_back( maps0[ mit->first] );
        rocmap = (TH2D*) (mit->second)->Clone(Form("BB %2d diff",mit->first));
        rocmap->Add( maps0[mit->first], -1. );
        fHistOptions.insert(make_pair( rocmap, "colz" ));
        fHistList.push_back( rocmap );
      }
      
      // fill distribution from map
      TH1D* hdist = new TH1D( 
        Form("BB-distrbution-%0d",mit->first), 
        Form("BB distrbution %2d",mit->first), 
        256, 0., 256.);
      for(int col=0; col<ROC_NUMCOLS; col++){
          for(int row=0; row<ROC_NUMROWS; row++){
              hdist->Fill( rocmap->GetBinContent( col+1, row+1 ) );
          }
      }
      fHistList.push_back( hdist );
  }
  
  if (fHistList.size()>0){
    TH2D *h = (TH2D*)(*fHistList.begin());
    h->Draw(getHistOption(h).c_str());
    fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
    PixTest::update(); 
  }else{
    LOG(logDEBUG) << "no histogram produced, this is probably a bug";
  }
}
