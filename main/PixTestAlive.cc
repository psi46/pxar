#include <iostream>
#include "PixTestAlive.hh"

#include <TH2.h>

using namespace std;

ClassImp(PixTestAlive)

// ----------------------------------------------------------------------
PixTestAlive::PixTestAlive(PixSetup &a, std::string name) : PixTest(a, name), fParNtrig(-1), fParVcal(-1) {
  PixTest::init(a, name);
  init(); 
  cout << "PixTestAlive ctor(PixSetup &a, string, TGTab *)" << endl;
}

// ----------------------------------------------------------------------
PixTestAlive::PixTestAlive(TBInterface *tb, std::string name, PixTestParameters *tp) : PixTest(tb, name, tp), fParNtrig(-1), fParVcal(-1) {
  PixTest::init(tb, name, tp);
  init(); 
  cout << "PixTestAlive ctor(TBInterface *, string, TGTab *, TGTextView *)" << endl;
}

//----------------------------------------------------------
PixTestAlive::PixTestAlive() : PixTest() {
  cout << "PixTestAlive ctor()" << endl;
}

// ----------------------------------------------------------------------
bool PixTestAlive::setParameter(string parName, string sval) {
  bool found(false);
  for (map<string,string>::iterator imap = fParameters.begin(); imap != fParameters.end(); ++imap) {
    cout << "---> " << imap->first << endl;
    if (0 == imap->first.compare(parName)) {
      found = true; 

      fParameters[parName] = sval;
      cout << "  ==> parName: " << parName << endl;
      cout << "  ==> sval:    " << sval << endl;
      if (!parName.compare("Ntrig")) fParNtrig = atoi(sval.c_str()); 
      if (!parName.compare("Vcal")) fParVcal = atoi(sval.c_str()); 
      break;
    }
  }
  return found; 
}


// ----------------------------------------------------------------------
void PixTestAlive::init() {
  cout << "PixTestAlive::init()" << endl;

  fDirectory = gDirectory->mkdir(fName.c_str()); 
  fDirectory->cd(); 

  TH2D *h2(0);
  fHistList.clear();
  for (int i = 0; i < 16; ++i){
    h2 = new TH2D(Form("PixelAlive_C%d", i), Form("PixelAlive_C%d", i), 52, 0., 52., 80, 0., 80.); 
    h2->SetMinimum(0.); 
    setTitles(h2, "col", "row"); 
    fHistList.push_back(h2); 
  }

}


//----------------------------------------------------------
PixTestAlive::~PixTestAlive() {
  cout << "PixTestAlive dtor" << endl;
  std::list<TH1*>::iterator il; 
  fDirectory->cd(); 
  for (il = fHistList.begin(); il != fHistList.end(); ++il) {
    cout << "Write out " << (*il)->GetName() << endl;
    (*il)->SetDirectory(fDirectory); 
    (*il)->Write(); 
  }
}


// ----------------------------------------------------------------------
void PixTestAlive::doTest() {
  cout << "PixTestAlive::doTest() ntrig = " << fParNtrig << endl;
  clearHist();
  // -- FIXME: Should/could separate better test from display?
  // -- FIXME: loop over range from DUT
  for (int ichip = 0; ichip < 16; ++ichip) {
    vector<int> results = fTB->GetEfficiencyMap(fParNtrig, 0);
    TH2D *h = (TH2D*)fDirectory->Get(Form("PixelAlive_C%d", ichip));
    if (h) {
      for (int i = 0; i < results.size(); ++i) {
	h->SetBinContent(i/NROW+1, i%NROW+1, results[i]); 
      }
    } else {
      cout << "XX did not find " << Form("PixelAlive_C%d", ichip) << endl;
    }
    //    cout << "done with doTest" << endl;
    h->Draw("colz");
    fDisplayedHist = find(fHistList.begin(), fHistList.end(), h);
    cout << "fDisplayedHist = " << (*fDisplayedHist)->GetName() 
	 << " begin? " << (fDisplayedHist == fHistList.begin())
	 << " end? " << (fDisplayedHist == fHistList.end())
	 << endl;
    PixTest::update(); 
  }
}
