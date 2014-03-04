#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>
#include "PixTestThreshMap.hh"
#include "log.h"

#include <TH2.h>


ClassImp(PixTestThreshMap)

// ----------------------------------------------------------------------
PixTestThreshMap::PixTestThreshMap(PixSetup *a, std::string name) : PixTest(a, name) {
  PixTest::init();
  init(); 
  LOG(logDEBUG) << "PixTestThreshMap ctor(PixSetup &a, string, TGTab *)";
}


//----------------------------------------------------------
PixTestThreshMap::PixTestThreshMap() : PixTest() {
  LOG(logDEBUG) << "PixTestThreshMap ctor()";
}

// ----------------------------------------------------------------------
bool PixTestThreshMap::setParameter(string parName, string sval) {
  bool found(false);
  string stripParName; 
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 

      LOG(logDEBUG) << "  ==> parName: " << parName;
      LOG(logDEBUG) << "  ==> sval:    " << sval;
      if (!parName.compare("Ntrig")) {
	fParNtrig = static_cast<uint16_t>(atoi(sval.c_str())); 
	setToolTips();
      }
      if (!parName.compare("VcalLo")) {
	fParLoDAC = static_cast<uint16_t>(atoi(sval.c_str())); 
	setToolTips();
      }
      if (!parName.compare("VcalHi")) {
	fParHiDAC = static_cast<uint16_t>(atoi(sval.c_str())); 
	setToolTips();
      }
      if (!parName.compare("BumpBond")) {
	bBumpBond = static_cast<bool>(atoi(sval.c_str())); 
	setToolTips();
      }
      break;
    }
  }
  return found; 
}


// ----------------------------------------------------------------------
void PixTestThreshMap::init() {
  LOG(logINFO) << "PixTestThreshMap::init()";

  setToolTips();
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd();

}

// ----------------------------------------------------------------------
void PixTestThreshMap::setToolTips() {
  fTestTip    = string("send Ntrig \"calibrates\" and count how many hits were measured\n")
    + string("the result is a hitmap, not an efficiency map")
    ;
  fSummaryTip = string("all ROCs are displayed side-by-side. Note the orientation:\n")
    + string("the canvas bottom corresponds to the narrow module side with the cable")
    ;
}


// ----------------------------------------------------------------------
void PixTestThreshMap::bookHist(string name) {
  fDirectory->cd(); 
  LOG(logDEBUG) << "nothing done with " << name; 
}


//----------------------------------------------------------
PixTestThreshMap::~PixTestThreshMap() {
  LOG(logDEBUG) << "PixTestThreshMap dtor";
}


// ----------------------------------------------------------------------
void PixTestThreshMap::doTest() {
  PixTest::update(); 
  fDirectory->cd();
  LOG(logINFO) << "PixTestThreshMap::doTest() ntrig = " << int(fParNtrig);
  PixTest::update(); 

 fDirectory->cd();
  vector<TH2D*> maps;
  vector<TH1D*> hResults;
  TH2D *h2(0);
  TH1D *h3(0);
  string name("Thresh");

  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs();

  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    id2idx.insert(make_pair(rocIds[iroc], iroc));
    h2 = bookTH2D(Form("%s_C%d", name.c_str(), iroc), Form("%s_C%d", name.c_str(), rocIds[iroc]), 52, 0., 52., 80, 0., 80.);
    h2->SetMinimum(0.);
    h2->SetDirectory(fDirectory);
    setTitles(h2, "col", "row");
    maps.push_back(h2);
  }

  
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);


    for(int r = 0; r < 80 ; r++)
      for (int c = 0 ; c <52 ;c++)
      {
        fApi->_dut->testPixel(c, r, true);
        fApi->_dut->maskPixel(c, r, false);
      }

  LOG(logINFO) << "getmap ";

  std::vector<pixel> results = fApi->getThresholdMap("Vcal", fParLoDAC,fParHiDAC,FLAG_RISING_EDGE, fParNtrig);
   
  LOG(logINFO) << "results: " << results.size();

  for(int idx = 0; idx < results.size() ; idx++) {
    
    maps[id2idx[results[idx].roc_id]]->SetBinContent(results[idx].column,results[idx].row,results[idx].value);

  }


  if(bBumpBond) {

    for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
      id2idx.insert(make_pair(rocIds[iroc], iroc));
      hResults.push_back( bookTH1D(Form("%s2_C%d", name.c_str(), iroc), Form("%s_C%d BumpBond",name.c_str(), rocIds[iroc]), 100,-50,50));
      LOG(logDEBUGAPI) << "hResults " << hResults[iroc];
      //setTitles(hResults[iroc],"/{delta}Threshhold","counts");
      //hResults[iroc]->SetMinimum(0.);
      //hResults[iroc]->SetDirectory(fDirectory);
      fApi->setDAC("CtrlReg",4,rocIds[iroc]);	
    } 

    std::vector<pixel> results2 = fApi->getThresholdMap("Vcal", fParLoDAC,fParHiDAC,FLAG_RISING_EDGE, fParNtrig);

    LOG(logDEBUGAPI) << results2.size();

    for(unsigned int i = 0 ; i < rocIds.size() ; i++)
    {
       fApi->setDAC("CtrlReg",0,rocIds[i]);
    }

    LOG(logDEBUG) << "Filling Bumpbond results.";
/*
    int num = 0 ;
    for(int i = 0 ; i < ROCNUMROWS ; i++ ) {
	for(int j = 0 ; j < ROCNUMCOLS ; j++ ) {
	    for( unsigned int idx = 0 ; idx < rocIds.size(); ++idx) {

*/
	   for(int num = 0 ; num < results.size() ; num++) {	
		if(results[num].column != results2[num].column )
		{
		 LOG(logDEBUG) << "Data not aligned.";
		}
	
		int thr1 = results[num].value;
		int thr2 = results2[num].value;
		if(thr1 > 0 && thr2 > 0)
		   hResults[rocIds[results[num].roc_id]]->Fill(thr1-thr2);
	    }

//	}
  //  }
     for( unsigned int idx = 0 ; idx < rocIds.size(); ++idx) {
  	fHistList.push_back(hResults[idx]);
     }

  } else LOG(logINFO) << "No Bumpbond";

  for (unsigned int i = 0; i < maps.size(); ++i) {
    fHistOptions.insert(make_pair(maps[i], "colz"));
  }

  copy(maps.begin(), maps.end(), back_inserter(fHistList));
  
  
  TH2D *h = (TH2D*)(*fHistList.begin());

  h->Draw(getHistOption(h).c_str());
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);

  PixTest::update(); 
  LOG(logINFO) << "PixTestThreshMap::doTest() done";
}


