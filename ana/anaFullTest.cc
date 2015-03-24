#include "anaFullTest.hh"

#include <fstream>
#include <sstream>
#include <cstdlib>

#include <TROOT.h>
#include <TSystem.h>
#if defined(WIN32)
#else
#include <TUnixSystem.h>
#endif

using namespace std;

// ----------------------------------------------------------------------
anaFullTest::anaFullTest(): fNrocs(16), fTrimVcal(40) {
  cout << "anaFullTest ctor" << endl;
  c0 = (TCanvas*)gROOT->FindObject("c0"); 
  if (!c0) c0 = new TCanvas("c0","--c0--",0,0,656,700);

  fDiffMetric = 0;

  fDacs.clear(); 
  fDacs.push_back("vana"); 
  fDacs.push_back("caldel"); 
  fDacs.push_back("vthrcomp"); 
  fDacs.push_back("vtrim"); 
  fDacs.push_back("phscale"); 
  fDacs.push_back("phoffset"); 
}

// ----------------------------------------------------------------------
anaFullTest::~anaFullTest() {
  cout << "anaFullTest dtor" << endl;

}

// ----------------------------------------------------------------------
void anaFullTest::bookModuleSummary(string modulename) {

  moduleSummary *ms = new moduleSummary; 
  ms->moduleName = modulename; 

  for (unsigned int idac = 0; idac < fDacs.size(); ++idac) {
    for (int iroc = 0; iroc < fNrocs; ++iroc) {
      TH1D *h = new TH1D(Form("%s_%s_C%d", modulename.c_str(), fDacs[idac].c_str(), iroc), 
			 Form("%s_%s_C%d", modulename.c_str(), fDacs[idac].c_str(), iroc), 
			 256, 0., 256.); 
      if (fDacs[idac] == "vana") ms->vana.push_back(h); 
      if (fDacs[idac] == "caldel") ms->caldel.push_back(h); 
      if (fDacs[idac] == "vthrcomp") ms->vthrcomp.push_back(h); 
      if (fDacs[idac] == "vtrim") ms->vtrim.push_back(h); 
      if (fDacs[idac] == "phscale") ms->phscale.push_back(h); 
      if (fDacs[idac] == "phoffset") ms->phoffset.push_back(h); 
    }
    TH1D *h = new TH1D(Form("%s_%s_rms", modulename.c_str(), fDacs[idac].c_str()), 
		       Form("%s_%s_rms", modulename.c_str(), fDacs[idac].c_str()), 
		       50, 0., 5.); 

    if (fDacs[idac] == "vana") ms->rmsVana = h; 
    if (fDacs[idac] == "caldel") ms->rmsCaldel = h; 
    if (fDacs[idac] == "vthrcomp") ms->rmsVthrcomp = h; 
    if (fDacs[idac] == "vtrim") ms->rmsVtrim = h; 
    if (fDacs[idac] == "phscale") ms->rmsPhscale = h; 
    if (fDacs[idac] == "phoffset") ms->rmsPhoffset = h; 
  }

  string what(""); 
  for (int iroc = 0; iroc < fNrocs; ++iroc) {
    what = "ndeadpixels"; 
    TH1D *h = new TH1D(Form("%s_%s_C%d", modulename.c_str(), what.c_str(), iroc), 
		       Form("%s_%s_C%d", modulename.c_str(), what.c_str(), iroc), 
		       100, 0., 100.);
    ms->ndeadpixels.push_back(h); 

    what = "ndeadbumps"; 
    h = new TH1D(Form("%s_%s_C%d", modulename.c_str(), what.c_str(), iroc), 
		 Form("%s_%s_C%d", modulename.c_str(), what.c_str(), iroc), 
		 100, 0., 100.);
    ms->ndeadbumps.push_back(h); 

    what = "deadsep"; 
    h = new TH1D(Form("%s_%s_C%d", modulename.c_str(), what.c_str(), iroc), 
		 Form("%s_%s_C%d", modulename.c_str(), what.c_str(), iroc), 
		 150, 0., 150.);
    ms->deadsep.push_back(h); 
    
  }  

  ms->trimthrpos = new TH1D(Form("%s_trimthrpos", modulename.c_str()), 
			    Form("%s_trimthrpos", modulename.c_str()), 
			    80, 39., 41.); 

  ms->trimthrpos->SetXTitle("VCAL [DAC]");

  ms->trimthrrms = new TH1D(Form("%s_trimthrrms", modulename.c_str()), 
			    Form("%s_trimthrrms", modulename.c_str()), 
			    50, 0., 2.); 
  ms->trimthrrms->SetXTitle("VCAL [DAC]");

  ms->p1pos = new TH1D(Form("%s_p1pos", modulename.c_str()), 
		    Form("%s_p1pos", modulename.c_str()), 
		    50, 0.5, 1.0); 

  ms->p1rms = new TH1D(Form("%s_p1rms", modulename.c_str()), 
		    Form("%s_p1rms", modulename.c_str()), 
		    50, 0., 0.2); 
  
  fModSummaries.insert(make_pair(modulename, ms)); 

}


// ----------------------------------------------------------------------
void anaFullTest::validateFullTests() {
  // -- PSI module
  addFullTests("D14-0001", "-003");
  // -- ETH modules 
  addFullTests("D14-0006", "-000");
  addFullTests("D14-0008", "-000");
  addFullTests("D14-0009", "-000");

  TH1D *hVana = new TH1D("hVana", Form("Vana %s", fDiffMetric == 0?"difference":"RMS"), 50, 0., 5.);
  TH1D *hCaldel = new TH1D("hCaldel", Form("CalDel %s", fDiffMetric == 0?"difference":"RMS"), 50, 0., 5.);
  TH1D *hVthrcomp = new TH1D("hVthrcomp", Form("VthrComp %s", fDiffMetric == 0?"difference":"RMS"), 50, 0., 5.);
  TH1D *hVtrim = new TH1D("hVtrim", Form("Vtrim %s", fDiffMetric == 0?"difference":"RMS"), 50, 0., 10.);
  TH1D *hPhs = new TH1D("hPhs", Form("phscale %s", fDiffMetric == 0?"difference":"RMS"), 50, 0., 5.);
  TH1D *hPho = new TH1D("hPho", Form("phoffset %s", fDiffMetric == 0?"difference":"RMS"), 50, 0., 5.);

  map<string, moduleSummary*>::iterator ib = fModSummaries.begin();

  TH1D *hTrimThrPos = (TH1D*)ib->second->trimthrpos->Clone("hTrimThrPos");
  hTrimThrPos->SetTitle("Trim Threshold");
  hTrimThrPos->Reset();

  TH1D *hTrimThrRms = (TH1D*)ib->second->trimthrrms->Clone("hTrimThrRms");
  hTrimThrRms->SetTitle("Trim Threshold RMS");
  hTrimThrRms->Reset();

  TH1D *hp1Pos = (TH1D*)ib->second->p1pos->Clone("hp1Pos");
  hp1Pos->SetTitle("p1");
  hp1Pos->Reset();

  TH1D *hp1Rms = (TH1D*)ib->second->p1rms->Clone("hp1Rms");
  hp1Rms->SetTitle("p1 RMS");
  hp1Rms->Reset();

  for (map<string, moduleSummary*>::iterator it = fModSummaries.begin(); it != fModSummaries.end(); ++it) {

    cout << "summarizing " << it->second->moduleName << endl;
    for (int iroc = 0; iroc < fNrocs; ++iroc) {
      hVana->Fill(diff(it->second->vana[iroc])); 
      hCaldel->Fill(diff(it->second->caldel[iroc])); 
      hVthrcomp->Fill(diff(it->second->vthrcomp[iroc])); 
      hVtrim->Fill(diff(it->second->vtrim[iroc])); 
      hPhs->Fill(diff(it->second->phscale[iroc])); 
      hPho->Fill(diff(it->second->phoffset[iroc])); 
    }
    
    for (int ibin = 1; ibin <= it->second->trimthrpos->GetNbinsX(); ++ibin) {
      hTrimThrPos->SetBinContent(ibin, hTrimThrPos->GetBinContent(ibin)+it->second->trimthrpos->GetBinContent(ibin)); 
    }

    for (int ibin = 1; ibin <= it->second->trimthrrms->GetNbinsX(); ++ibin) {
      hTrimThrRms->SetBinContent(ibin, hTrimThrRms->GetBinContent(ibin) + it->second->trimthrrms->GetBinContent(ibin)); 
    }

    for (int ibin = 1; ibin <= it->second->p1pos->GetNbinsX(); ++ibin) {
      hp1Pos->SetBinContent(ibin, hp1Pos->GetBinContent(ibin)+it->second->p1pos->GetBinContent(ibin)); 
    }

    for (int ibin = 1; ibin <= it->second->p1rms->GetNbinsX(); ++ibin) {
      hp1Rms->SetBinContent(ibin, hp1Rms->GetBinContent(ibin) + it->second->p1rms->GetBinContent(ibin)); 
    }
    
  }
			  
			  
  c0->Clear();
  c0->cd(1);

  hVana->Draw();
  c0->SaveAs("ftval-vana.pdf"); 
  
  hCaldel->Draw();
  c0->SaveAs("ftval-caldel.pdf"); 

  hVthrcomp->Draw();
  c0->SaveAs("ftval-vthrcomp.pdf"); 

  hVtrim->Draw();
  c0->SaveAs("ftval-vtrim.pdf"); 

  hPhs->Draw();
  c0->SaveAs("ftval-phscale.pdf"); 

  hPho->Draw();
  c0->SaveAs("ftval-phoffset.pdf"); 

  hTrimThrPos->Draw();
  c0->SaveAs("ftval-trimthrpos.pdf"); 

  hTrimThrRms->Draw();
  c0->SaveAs("ftval-trimthrrms.pdf"); 

  hp1Pos->Draw();
  c0->SaveAs("ftval-p1pos.pdf"); 

  hp1Rms->Draw();
  c0->SaveAs("ftval-p1rms.pdf"); 


}


// ----------------------------------------------------------------------
void anaFullTest::addFullTests(string mname, string mpattern) {
  vector<string> dirs = glob(mname+mpattern); 
  cout << dirs.size() << endl;
  for (unsigned int idirs = 0; idirs < dirs.size(); ++idirs) {
    cout << dirs[idirs] << endl;
  }

  bookModuleSummary(mname); 

  for (unsigned int idirs = 0; idirs < dirs.size(); ++idirs) {
    readDacFile(dirs[idirs], "vana", fModSummaries[mname]->vana);
    readDacFile(dirs[idirs], "caldel", fModSummaries[mname]->caldel);
    readDacFile(dirs[idirs], "vthrcomp", fModSummaries[mname]->vthrcomp);
    readDacFile(dirs[idirs], "vtrim", fModSummaries[mname]->vtrim);
    readDacFile(dirs[idirs], "phscale", fModSummaries[mname]->phscale);
    readDacFile(dirs[idirs], "phoffset", fModSummaries[mname]->phoffset);

   
//     readLogFile(dirs[idirs], "number of dead pixels (per ROC):", fModSummaries[mname]->ndeadpixels);
//     readLogFile(dirs[idirs], "number of dead bumps (per ROC):", fModSummaries[mname]->ndeadbumps);
//     readLogFile(dirs[idirs], "separation cut       (per ROC):", fModSummaries[mname]->deadsep);    
    readLogFile(dirs[idirs], "vcal mean:", fModSummaries[mname]->trimthrpos);
    readLogFile(dirs[idirs], "vcal RMS:", fModSummaries[mname]->trimthrrms);
    readLogFile(dirs[idirs], "p1 mean:", fModSummaries[mname]->p1pos);
    readLogFile(dirs[idirs], "p1 RMS:", fModSummaries[mname]->p1rms);
      
    /*
      vcal mean:
      vcal RMS: 
      bits mean:
      bits RMS: 

      VthrComp mean:
      VthrComp RMS:
      Vcal mean:
      Vcal RMS:
     */
  }

  for (int iroc = 0; iroc < fNrocs; ++iroc) {
    fModSummaries[mname]->rmsVana->Fill(diff(fModSummaries[mname]->vana[iroc])); 
    fModSummaries[mname]->rmsCaldel->Fill(diff(fModSummaries[mname]->caldel[iroc])); 
    fModSummaries[mname]->rmsVthrcomp->Fill(diff(fModSummaries[mname]->vthrcomp[iroc])); 
    fModSummaries[mname]->rmsVtrim->Fill(diff(fModSummaries[mname]->vtrim[iroc])); 
    fModSummaries[mname]->rmsPhscale->Fill(diff(fModSummaries[mname]->phscale[iroc])); 
    fModSummaries[mname]->rmsPhoffset->Fill(diff(fModSummaries[mname]->phoffset[iroc])); 
  }

  c0->Clear(); 
  c0->Divide(3,3);

  c0->cd(1); 
  fModSummaries[mname]->rmsVana->Draw();
  c0->cd(2); 
  fModSummaries[mname]->rmsCaldel->Draw();
  
  c0->cd(3); 
  fModSummaries[mname]->rmsVthrcomp->Draw();
  
  c0->cd(4); 
  fModSummaries[mname]->rmsVtrim->Draw();

  c0->cd(5); 
  fModSummaries[mname]->rmsPhscale->Draw();

  c0->cd(6); 
  fModSummaries[mname]->rmsPhoffset->Draw();

  c0->cd(7); 
  fModSummaries[mname]->trimthrpos->Draw();

  c0->cd(8); 
  fModSummaries[mname]->trimthrrms->Draw();

  c0->cd(9); 
  fModSummaries[mname]->p1pos->Draw();
  
  c0->SaveAs(Form("%s.pdf", mname.c_str())); 

}

// ----------------------------------------------------------------------
void anaFullTest::readLogFile(std::string dir, std::string tag, std::vector<TH1D*> hists) {

  ifstream IN; 

  char buffer[1000];
  string sline; 
  string::size_type s1;
  vector<double> x;
  IN.open(Form("%s/pxar.log", dir.c_str())); 
  while (IN.getline(buffer, 1000, '\n')) {
    sline = buffer; 
    s1 = sline.find(tag.c_str()); 
    if (string::npos == s1) continue;
    sline = sline.substr(s1+tag.length()+1);
    break;
  }

  x = splitIntoRocs(sline);
  for (unsigned int i = 0; i < x.size(); ++i) {
    //     cout << "Filling into " << hist->GetName() << " x = " << x[i] << endl;
    hists[i]->Fill(x[i]); 
  }

  IN.close(); 
}

// ----------------------------------------------------------------------
void anaFullTest::readLogFile(std::string dir, std::string tag, TH1D* hist) {

  ifstream IN; 

  char buffer[1000];
  string sline; 
  string::size_type s1;
  vector<double> x;
  IN.open(Form("%s/pxar.log", dir.c_str())); 
  while (IN.getline(buffer, 1000, '\n')) {
    sline = buffer; 
    s1 = sline.find(tag.c_str()); 
    if (string::npos == s1) continue;
    sline = sline.substr(s1+tag.length()+1);
    break;
  }
  x = splitIntoRocs(sline);
  for (unsigned int i = 0; i < x.size(); ++i) {
    //     cout << "Filling into " << hist->GetName() << " x = " << x[i] << endl;
    hist->Fill(x[i]); 
  }

  IN.close(); 
}



// ----------------------------------------------------------------------
void anaFullTest::readDacFile(string dir, string dac, vector<TH1D*> vals) {
  ifstream IN; 

  char buffer[1000];
  string sline; 
  string::size_type s1;
  int val(0); 
  for (int i = 0; i < fNrocs; ++i) {
    IN.open(Form("%s/dacParameters%d_C%d.dat", dir.c_str(), fTrimVcal, i)); 
    while (IN.getline(buffer, 1000, '\n')) {
      if (buffer[0] == '#') {continue;}
      if (buffer[0] == '/') {continue;}
      if (buffer[0] == '\n') {continue;}
      sline = buffer; 
      s1 = sline.find(dac.c_str()); 
      if (string::npos != s1) {
	sline = sline.substr(s1+dac.length()+1); 
	val = atoi(sline.c_str()); 
	vals[i]->Fill(val); 
      }
    }
    IN.close(); 
  }

}



// ----------------------------------------------------------------------
vector<string> anaFullTest::glob(string basename) {
  vector<string> lof; 
#if defined(WIN32)
#else
  TString fname;
  const char *file;
  TSystem *lunix = gSystem; //new TUnixSystem();
  void *pDir = lunix->OpenDirectory(".");
  while ((file = lunix->GetDirEntry(pDir))) {
    fname = file;
    if (fname.Contains(basename.c_str())) {
      lof.push_back(string(fname));
    }
  }  
#endif
  return lof; 
}


// ----------------------------------------------------------------------
double anaFullTest::diff(TH1D *h) {

  if (0 == fDiffMetric) {
    int minbin = h->FindFirstBinAbove(0.1); 
    int maxbin = h->FindLastBinAbove(0.1); 
    return (maxbin-minbin);
  } else if (1 == fDiffMetric) {
    return h->GetRMS(); 
  }

  return -1.; 
}


// ----------------------------------------------------------------------
std::vector<double> anaFullTest::splitIntoRocs(std::string line) {
  istringstream istring(line);
  vector<double> result; 
  double x(0.); 
  cout << "splitting: " << line << endl;
  for (int iroc = 0; iroc < fNrocs; ++iroc) {
    istring >> x;
    result.push_back(x); 
  }

  for (unsigned int i = 0; i < result.size(); ++i) {
    cout << result[i] << " "; 
  }
  cout << endl;

  return result;
}
