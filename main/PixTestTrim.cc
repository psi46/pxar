#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>

#include <TH1.h>

#include "PixTestTrim.hh"
#include "log.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestTrim)

// ----------------------------------------------------------------------
PixTestTrim::PixTestTrim(PixSetup *a, std::string name) : PixTest(a, name), 
  fParVcal(-1), fParNtrig(-1), 
  fParVthrCompLo(-1), fParVthrCompHi(-1),
  fParVcalLo(-1), fParVcalHi(-1) {
  PixTest::init(a, name);
  init(); 
  //  LOG(logINFO) << "PixTestTrim ctor(PixSetup &a, string, TGTab *)";
  for (unsigned int i = 0; i < fPIX.size(); ++i) {
    LOG(logDEBUG) << "  setting fPIX" << i <<  " ->" << fPIX[i].first << "/" << fPIX[i].second;
  }
}


//----------------------------------------------------------
PixTestTrim::PixTestTrim() : PixTest() {
  //  LOG(logINFO) << "PixTestTrim ctor()";
}

// ----------------------------------------------------------------------
bool PixTestTrim::setParameter(string parName, string sval) {
  bool found(false);
  for (map<string,string>::iterator imap = fParameters.begin(); imap != fParameters.end(); ++imap) {
    LOG(logDEBUG) << "---> " << imap->first;
    if (0 == imap->first.compare(parName)) {
      found = true; 
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());
      fParameters[parName] = sval;
      if (!parName.compare("Ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParNtrig  ->" << fParNtrig << "<- from sval = " << sval;
      }
      if (!parName.compare("Vcal")) {
	fParVcal = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParVcal  ->" << fParVcal << "<- from sval = " << sval;
      }
      if (!parName.compare("VcalLo")) {
	fParVcalLo = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParVcalLo  ->" << fParVcalLo << "<- from sval = " << sval;
      }
      if (!parName.compare("VcalHi")) {
	fParVcalHi = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParVcalHi  ->" << fParVcalHi << "<- from sval = " << sval;
      }
      if (!parName.compare("VthrCompLo")) {
	fParVthrCompLo = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParVthrCompLo  ->" << fParVthrCompLo << "<- from sval = " << sval;
      }
      if (!parName.compare("VthrCompHi")) {
	fParVthrCompHi = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParVthrCompHi  ->" << fParVthrCompHi << "<- from sval = " << sval;
      }

      break;
    }
  }
  
  return found; 
}


// ----------------------------------------------------------------------
void PixTestTrim::init() {
  setToolTips(); 
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}


// ----------------------------------------------------------------------
void PixTestTrim::setToolTips() {
  fTestTip    = string(Form("trimming results in a uniform in-time threshold\n")
		       + string("TO BE FINISHED!!"))
    ;
  fSummaryTip = string("summary plot to be implemented")
    ;
}

// ----------------------------------------------------------------------
void PixTestTrim::bookHist(string name) {
  fDirectory->cd(); 

  LOG(logDEBUG) << "nothing done with " << name;
  //  fHistList.clear();

}


//----------------------------------------------------------
PixTestTrim::~PixTestTrim() {
  LOG(logDEBUG) << "PixTestTrim dtor";
}


// ----------------------------------------------------------------------
void PixTestTrim::doTest() {
  fDirectory->cd();
  PixTest::update(); 
  LOG(logINFO) << "PixTestTrim::doTest() ntrig = " << fParNtrig;

  fPIX.clear(); 
  if (fApi) fApi->_dut->testAllPixels(true);

  int RFLAG(7); 

  // -- determine minimal VthrComp 
  LOG(logINFO) << "TRIM determine minimal VthrComp"; 
  vector<TH1*> thr0 = scurveMaps("VthrComp", "TrimThr0", fParNtrig, fParVthrCompLo, fParVthrCompHi, RFLAG); 
  TH2D *h(0); 
  for (unsigned int i = 0; i < thr0.size(); ++i) {
    LOG(logDEBUG) << "   " << thr0[i]->GetName();
    if (!strcmp("thr_TrimThr0_VthrComp_C0", thr0[i]->GetName())) {
      h = (TH2D*)thr0[i]; 
      break;
    }
  }


  if (0 == h) {
    LOG(logINFO) << "histogram thr_TrimThr0_VthrComp_C0 not found"; 
    return;
  }

  double minThr(999.); 
  int ix(-1), iy(-1); 
  for (int ic = 0; ic < h->GetNbinsX(); ++ic) {
    for (int ir = 0; ir < h->GetNbinsY(); ++ir) {
      if (h->GetBinContent(ic+1, ir+1) < minThr) {
	minThr = h->GetBinContent(ic+1, ir+1);
	ix = ic; 
	iy = ir; 
      }
    }
  }
  LOG(logINFO) << "  minimal VthrComp threshold" << minThr << " for pixel " << ix << "/" << iy;

  LOG(logINFO) << "TRIM determine highest Vcal thresho"; 
  vector<TH1*> thr1 = scurveMaps("Vcal", "TrimThr1", fParNtrig, fParVcalLo, fParVcalHi, RFLAG); 
  for (unsigned int i = 0; i < thr1.size(); ++i) {
    if (!strcmp("thr_TrimThr1_Vcal_C0", thr1[i]->GetName())) {
      h = (TH2D*)thr1[i]; 
      break;
    }
  }
  double maxThr(-1.); 
  for (int ic = 0; ic < h->GetNbinsX(); ++ic) {
    for (int ir = 0; ir < h->GetNbinsY(); ++ir) {
      if (h->GetBinContent(ic+1, ir+1) > maxThr) {
	maxThr = h->GetBinContent(ic+1, ir+1);
	ix = ic; 
	iy = ir; 
      }
    }
  }
  LOG(logINFO) << "  maximal Vcal threshold " << maxThr << " for pixel " << ix << "/" << iy;

  // -- determine Vtrim for pixel with highest VCAl threshold
  if (fApi) fApi->_dut->testAllPixels(false);
  fApi->_dut->testPixel(ix, iy, true);
  int vtrim = adjustVtrim(); 
  LOG(logINFO) << "  determine Vtrim =  " << vtrim;


  PixTest::update(); 
}

// ----------------------------------------------------------------------
int PixTestTrim::adjustVtrim() {
  int vtrim = -1;
  int thr(255), thrOld(255);
  int ntrig(10); 
  do {
    vtrim++;
    fApi->setDAC("Vtrim", vtrim);
    thrOld = thr;
    thr = pixelThreshold("Vcal", ntrig, 0, 100); 
    LOG(logDEBUG) << vtrim << " thr " << thr;
  }
  while (((thr > fParVcal) || (thrOld > fParVcal) || (thr < 10)) && (vtrim < 200));
  vtrim += 5;
  fApi->setDAC("Vtrim", vtrim);
  LOG(logINFO) << "Vtrim set to " <<  vtrim;
  return vtrim;
}


// ----------------------------------------------------------------------
TH2D* PixTestTrim::trimStep(int correction, TH2D *calMapOld) {

  LOG(logDEBUG) << "nothing done with " << correction << " or " << calMapOld;
//   TH2D* betterCalMap = GetMap("VcalThresholdMap");
//   int trim;
  
//   //save trim map
//   TH2D *trimMap = roc->TrimMap();
  
//   //set new trim bits
//   for (int i = 0; i < ROCNUMCOLS; i++) {
//       for (int k = 0; k < ROCNUMROWS; k++) {
// 	if (aTestRange->IncludesPixel(roc->GetChipId(), i, k)) {
// 	  trim = (int)trimMap->GetBinContent(i+1, k+1);
// 	  if (calMapOld->GetBinContent(i+1, k+1) > vcal) trim-=correction;
// 	  else trim+=correction;
          
// 	  if (trim < 0) trim = 0;
// 	  if (trim > 15) trim = 15;
// 	  GetPixel(i,k)->SetTrim(trim);
// 	}
//       }
//   }
//   AddMap(roc->TrimMap());
  
//   //measure new result
//   TH2D *calMap = thresholdMap->GetMap("VcalThresholdMap", roc, aTestRange, nTrig);
//   AddMap(calMap);
  
//   // test if the result got better
//   for (int i = 0; i < ROCNUMCOLS; i++)
//     {
//       for (int k = 0; k < ROCNUMROWS; k++)
// 	{
// 	  if (aTestRange->IncludesPixel(roc->GetChipId(), i, k))
// 	    {
// 	      trim = GetPixel(i,k)->GetTrim();
              
// 	      if (TMath::Abs(calMap->GetBinContent(i+1, k+1) - vcal) <= TMath::Abs(calMapOld->GetBinContent(i+1, k+1) - vcal))
// 		{
// 		  // it's better now
// 		  betterCalMap->SetBinContent(i+1, k+1, calMap->GetBinContent(i+1, k+1));
// 		}
// 	      else
// 		{
// 		  // it's worse
// 		  betterCalMap->SetBinContent(i+1, k+1, calMapOld->GetBinContent(i+1, k+1));
// 		  GetPixel(i,k)->SetTrim((int)trimMap->GetBinContent(i+1, k+1));
// 		}
// 	    }
// 	}
//     }
  
//   AddMap(roc->TrimMap());
  
//   return betterCalMap;
  return 0;
}
