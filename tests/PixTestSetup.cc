// -- author: Martino Dall'Osso
// to set clk and deser160phase for single roc test

#include <stdlib.h>   // atof, atoi
#include <iostream>
#include <algorithm>  // std::find
#include <TBox.h>

#include "PixTestSetup.hh"
#include "log.h"
#include "constants.h"

using namespace std;
using namespace pxar;

ClassImp(PixTestSetup)

//------------------------------------------------------------------------------
PixTestSetup::PixTestSetup(PixSetup *a, std::string name) : PixTest(a, name)
{
  PixTest::init();
  init();
}

//------------------------------------------------------------------------------
PixTestSetup::PixTestSetup() : PixTest() {}

bool PixTestSetup::setParameter(string parName, string sval) {
  bool found(false);
  ParOutOfRange = false;
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 

      if (!parName.compare("clkmax")) {
	fClkMax = atoi(sval.c_str()); 
	if(fClkMax < 0 || fClkMax > 25) { 
	  LOG(logWARNING) << "PixTestSetup::setParameter() ClkMax out of range (0-25)";
	  found=false; ParOutOfRange = true;
	}
      }
      if (!parName.compare("desermax")) {
	fDeserMax = atoi(sval.c_str()); 
	if(fDeserMax < 0 || fDeserMax > 7) { 
	  LOG(logWARNING) << "PixTestSetup::setParameter() DeserMax out of range (0-7)";
	  found=false; ParOutOfRange = true;
	}
      }
      break;
    }
  }
  return found; 
}

void PixTestSetup::init()
{
  LOG(logDEBUG) << "PixTestSetup::init()";

  fDirectory = gFile->GetDirectory(fName.c_str());
  if (!fDirectory)
    fDirectory = gFile->mkdir(fName.c_str());
  fDirectory->cd();
}

void PixTestSetup::setToolTips()
{
  fTestTip = string(Form("scan testboard parameter settings and check for valid readout\n")
		    + string("TO BE IMPLEMENTED!!"));  //FIXME
  fSummaryTip = string("summary plot to be implemented");  //FIXME
}

void PixTestSetup::bookHist(string name)
{
  fDirectory->cd();
  LOG(logDEBUG) << "nothing done with " << name;
}

PixTestSetup::~PixTestSetup()
{
  LOG(logDEBUG) << "PixTestSetup dtor";
  std::list<TH1*>::iterator il;
  fDirectory->cd();
  for (il = fHistList.begin(); il != fHistList.end(); ++il) 
    {
      LOG(logINFO) << "Write out " << (*il)->GetName();
      (*il)->SetDirectory(fDirectory);
      (*il)->Write();
    }
}

//------------------------------------------------------------------------------
void PixTestSetup::doTest()
{
  cacheDacs();
  if (ParOutOfRange) return;
  fDirectory->cd();
  fHistList.clear();
  PixTest::update();
	
  //fixed number of trigger (unstable with only 1 trigger)
  int Ntrig = 10;
	
  LOG(logINFO) << "PixTestSetup::doTest() ntrig = " << Ntrig;

  bookHist("bla"); //FIXME

  // Setup a new pattern with only res and token:
  std::vector<std::pair<std::string, uint8_t> > pg_setup;
  pg_setup.push_back(make_pair("resetroc", 25));    // PG_RESR b001000
  pg_setup.push_back(make_pair("token", 0));     // PG_TOK  b000001
  uint16_t period = 28;
  fApi->setPatternGenerator(pg_setup);
  LOG(logINFO) << "PixTestSetup:: pg set to RES|TOK";


  TH2D *histo = new TH2D(Form("DeserphaseClkScan"), Form("DeserphaseClkScan"), fDeserMax+1, 0., fDeserMax+1, fClkMax+1, 0., fClkMax+1);
  histo->GetXaxis()->SetTitle("deser160phase");
  histo->GetYaxis()->SetTitle("clk");
  fHistList.push_back(histo);

  std::vector<rawEvent> daqRawEv;
  int ideser, iclk;
  int good_clk = -1, good_deser = -1;

  std::stringstream tablehead;
  for (ideser = 0; ideser <= fDeserMax; ideser++) tablehead << std::setw(8) << ideser;
  LOG(logINFO) << tablehead.str();

  // Start the DAQ:
  fApi->daqStart();

  // Loop over the Signal Delay range we want to scan:
  for (iclk = 0; iclk <= fClkMax; iclk++) {
    std::stringstream oneline;
    oneline << std::setw(2) << iclk << ": ";

    for (ideser = 0; ideser <= fDeserMax; ideser++) {

      // Set up the delays in the DTB:
      fApi->setTestboardDelays(getMagicDelays(iclk,ideser));

      // Send the triggers and read out the events:    
	  fApi->daqTrigger(Ntrig,period);	  	       
	  try { daqRawEv = fApi->daqGetRawEventBuffer(); }
	  catch(pxar::DataNoEvent &) {}

      unsigned int head_good = 0;
      unsigned int head_bad = 0;

      // Trying to find the ROC header 0x7f8 in the raw DESER160 data:
	  for (std::vector<rawEvent>::iterator evt = daqRawEv.begin(); evt != daqRawEv.end(); ++evt) {
		  // Get the first word from ROC header:
		  int head = static_cast<int>(evt->data.at(0) & 0xffc);
		  if (head == 0x7f8) { head_good++; }
		  else head_bad++;
	  }

      // Print the stuff:
      if(head_good > 0) {
	oneline << std::hex << "<7f8>" << std::dec;
	histo->Fill(ideser, iclk, head_good);
	good_clk = iclk; good_deser = ideser;					

	// Additionally print number of good headers:
	if (head_good < 10) oneline << "[" << head_good << "]";
	else oneline << "[*]";
      }
      else if(head_bad > 0) {
	oneline << std::hex << " " << std::setw(3) << std::setfill('0') << (daqRawEv.at(0).data.at(0) & 0xffc) << std::setfill(' ') << "    " << std::dec;
      }
      else oneline << " [.]    ";
    }
    LOG(logINFO) << oneline.str();
  }

  // Stop the DAQ:
  fApi->daqStop();


  int finalclk, finaldeser;
  histo->Draw("colz");
  Int_t bin = histo->GetMaximumBin(); //set always to minimum binx/biny  
  Int_t binx, biny, binz;
  histo->GetBinXYZ(bin, binx, biny, binz);

  //to choose a middle value between various 'good clk':
  LOG(logDEBUG) << "PixTestSetup:: choosing the good clk phase.";
  Double_t val1 = histo->GetBinContent(bin);
  Double_t val2 = 0;
  int cnt = 0;
  for (int ibiny = (biny+1); ibiny <= fClkMax; ibiny ++) {
	  val2 = histo->GetBinContent(binx, ibiny);
	  if (val2 < val1) break;
	  else  cnt++;
  }
  biny = biny + ((int)(cnt / 2));

  Double_t x1 = histo->GetXaxis()->GetBinLowEdge(binx);
  Double_t x2 = histo->GetXaxis()->GetBinUpEdge(binx);
  Double_t y1 = histo->GetYaxis()->GetBinLowEdge(biny);
  Double_t y2 = histo->GetYaxis()->GetBinUpEdge(biny);
  TBox b(x1, y1, x2, y2);
  b.SetFillStyle(0);
  b.SetLineWidth(4);
  b.SetLineColor(kBlack);

  // If we found some good settings, find the best:
  if (good_clk != -1 && good_deser != -1) {
    
    // Highlight the maximum
    b.Draw();

    finalclk = biny - 1;
    finaldeser = binx - 1;
    LOG(logINFO) << "Found good delays at " << "CLK = "<< finalclk << ", DESER160 = " << finaldeser;
  }
  else {
    //back to default values
    finalclk = finaldeser = 4;
    LOG(logINFO) << "DTB Delay Setup could not find any good delays.";
	LOG(logINFO) << "Falling back to default values.";
  }

  // Update the histogram, also print the added box around selected settings:
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), histo);
  PixTest::update();

  // Set final clk and deser160:
  sig_delays = getMagicDelays(finalclk,finaldeser);
  for(std::vector<std::pair<std::string,uint8_t> >::iterator sig = sig_delays.begin(); sig != sig_delays.end(); ++sig) {
    fPixSetup->getConfigParameters()->setTbParameter(sig->first, sig->second);
  }

  // Store them in the file.
  saveTbParameters();
  fApi->setTestboardDelays(sig_delays);

  // Reset the pattern generator to the configured default:
  fApi->setPatternGenerator(fPixSetup->getConfigParameters()->getTbPgSettings());
  
  fHistList.clear();

  LOG(logINFO) << "PixTestSetup::doTest() done for.";
}

std::vector<std::pair<std::string,uint8_t> > PixTestSetup::getMagicDelays(uint8_t clk, uint8_t deser160) {
  std::vector<std::pair<std::string,uint8_t> > sigdelays;
  sigdelays.push_back(std::make_pair("clk", clk));
  sigdelays.push_back(std::make_pair("ctr", clk));
  sigdelays.push_back(std::make_pair("sda", clk + 15));
  sigdelays.push_back(std::make_pair("tin", clk + 5));
  sigdelays.push_back(std::make_pair("deser160phase", deser160));
  return sigdelays;
}

// ----------------------------------------------------------------------
void PixTestSetup::saveTbParameters() {
  LOG(logINFO) << "PixTestSetup:: Write Tb parameters to file."; 
  fPixSetup->getConfigParameters()->writeTbParameterFile();
}

// ----------------------------------------------------------------------
void PixTestSetup::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;
  if (!command.compare("savetbparameters")) {
    saveTbParameters(); 
    return;
  }
  LOG(logDEBUG) << "did not find command ->" << command << "<-";
}
