#include <iostream>
#include <stdlib.h>     /* atof, atoi */

#include "PixTest.hh"
#include "log.h"

using namespace std;
using namespace pxar;

ClassImp(PixTest)

// ----------------------------------------------------------------------
PixTest::PixTest(PixSetup *a, string name) {
  //  LOG(logINFO) << "PixTest ctor(PixSetup, string)";
  init(a, name); 
}

// ----------------------------------------------------------------------
PixTest::PixTest() {
  //  LOG(logINFO) << "PixTest ctor()";
  
}


// ----------------------------------------------------------------------
void PixTest::init(PixSetup *a, string name) {
  //  LOG(logINFO)  << "PixTest::init()";
  fPixSetup       = a;
  fApi            = a->getApi(); 
  fTestParameters = a->getPixTestParameters(); 

  fName = name;
  fParameters = a->getPixTestParameters()->getTestParameters(name); 

  for (map<string,string>::iterator imap = fParameters.begin(); imap != fParameters.end(); ++imap) {
    setParameter(imap->first, imap->second); 
  }
}


// ----------------------------------------------------------------------
void PixTest::bookHist(string name) {

}


// ----------------------------------------------------------------------
vector<TH2D*> PixTest::thresholdMaps(string type, string name, int ntrig) {

}


// ----------------------------------------------------------------------
vector<TH2D*> PixTest::efficiencyMaps(string name, int ntrig) {

  vector<pixel> results; 
  if (fApi) results = fApi->getEfficiencyMap(0, ntrig);

  fDirectory->cd(); 
  vector<TH2D*> maps;
  TH2D *h2(0); 
  for (int i = 0; i < fPixSetup->getConfigParameters()->getNrocs(); ++i){
    h2 = new TH2D(Form("%s_C%d", name.c_str(), i), Form("%s_C%d", name.c_str(), i), 52, 0., 52., 80, 0., 80.); 
    h2->SetMinimum(0.); 
    h2->SetDirectory(fDirectory); 
    setTitles(h2, "col", "row"); 
    maps.push_back(h2); 
  }
  
  for (int i = 0; i < results.size(); ++i) {
    cout << "results[i].roc_id = " << int(results[i].roc_id) 
	 << " col/row = " << int(results[i].column) << "/" << int(results[i].row)
	 << endl;
    h2 = maps[results[i].roc_id];
    if (h2) h2->SetBinContent(results[i].column +1, results[i].row + 1, static_cast<float>(results[i].value)/ntrig); 
  }

  return maps; 
}



// ----------------------------------------------------------------------
bool PixTest::setParameter(string parName, string value) {
  //  cout << " PixTest::setParameter wrong function" << endl;
  return false;
}


// ----------------------------------------------------------------------
bool PixTest::getParameter(std::string parName, int &ival) {
  bool found(false);
  for (map<string,string>::iterator imap = fParameters.begin(); imap != fParameters.end(); ++imap) {
    if (0 == imap->first.compare(parName)) {
      found = true; 
      break;
    }
  }
  if (found) {
    ival = atoi(fParameters[parName].c_str()); 
  }
  return found; 
}


  // ----------------------------------------------------------------------
bool PixTest::getParameter(std::string parName, float &fval) {
  bool found(false);
  for (map<string,string>::iterator imap = fParameters.begin(); imap != fParameters.end(); ++imap) {
    if (0 == imap->first.compare(parName)) {
      found = true; 
      break;
    }
  }
  if (found) {
    fval = atof(fParameters[parName].c_str()); 
  }
  return found; 
}


// ----------------------------------------------------------------------
void PixTest::dumpParameters() {
  LOG(logINFO) << "Parameters for test" << getName();
  for (map<string,string>::iterator imap = fParameters.begin(); imap != fParameters.end(); ++imap) {
    LOG(logINFO) << imap->first << ": " << imap->second;
  }
}


// ----------------------------------------------------------------------
PixTest::~PixTest() {
  //  LOG(logINFO) << "PixTestBase dtor()";
}

// ----------------------------------------------------------------------
void PixTest::testDone() {
  //  LOG(logINFO) << "PixTest::testDone()";
  Emit("testDone()"); 
}

// ----------------------------------------------------------------------
void PixTest::update() {
  //  cout << "PixTest::update()" << endl;
  Emit("update()"); 
}


// ----------------------------------------------------------------------
void PixTest::doTest() {
  //  LOG(logINFO) << "PixTest::doTest()";
}


// ----------------------------------------------------------------------
void PixTest::doAnalysis() {
  //  LOG(logINFO) << "PixTest::doAnalysis()";
}


// ----------------------------------------------------------------------
TH1* PixTest::nextHist() {
  std::list<TH1*>::iterator itmp = fDisplayedHist;  
  ++itmp;
  if (itmp == fHistList.end()) {
    // -- wrap around and point to first histogram in list
    fDisplayedHist = fHistList.begin(); 
    return (*fDisplayedHist); 
  } else {
    ++fDisplayedHist; 
    return (*fDisplayedHist); 
  }
}

// ----------------------------------------------------------------------
TH1* PixTest::previousHist() {
  if (fDisplayedHist == fHistList.begin()) {
    // -- wrap around and point to last histogram in list
    fDisplayedHist = fHistList.end(); 
    --fDisplayedHist;
    return (*fDisplayedHist); 
  } else {
    --fDisplayedHist; 
    return (*fDisplayedHist); 
  }

}

// ----------------------------------------------------------------------
void PixTest::setTitles(TH1 *h, const char *sx, const char *sy, float size, 
			float xoff, float yoff, float lsize, int font) {
  if (h == 0) {
    LOG(logINFO) << " Histogram not defined";
  } else {
    h->SetXTitle(sx);                  h->SetYTitle(sy); 
    h->SetTitleOffset(xoff, "x");      h->SetTitleOffset(yoff, "y");
    h->SetTitleSize(size, "x");        h->SetTitleSize(size, "y");
    h->SetLabelSize(lsize, "x");       h->SetLabelSize(lsize, "y");
    h->SetLabelFont(font, "x");        h->SetLabelFont(font, "y");
    h->GetXaxis()->SetTitleFont(font); h->GetYaxis()->SetTitleFont(font);
    h->SetNdivisions(508, "X");
  }
}

// ----------------------------------------------------------------------
void PixTest::clearHist() {
  for (list<TH1*>::iterator il = fHistList.begin(); il != fHistList.end(); ++il) {
    (*il)->Reset();
  }
}
