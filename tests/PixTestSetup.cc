// -- author: Martino Dall'Osso
// to set clk and deser160phase for single roc test

#include <stdlib.h>   // atof, atoi
#include <algorithm>  // std::find

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

bool PixTestSetup::setParameter(string parName, string sval)
{
  bool found(false);
  ParOutOfRange = false;
  std::transform(parName.begin(), parName.end(), parName.begin(), ::tolower);
  for (unsigned int i = 0; i < fParameters.size(); ++i) 
    {
      if (fParameters[i].first == parName) 
	{
	  found = true; 

	  if (!parName.compare("clkmax")) 
	    {
	      fClkMax = atoi(sval.c_str()); 
	      if(fClkMax < 0 || fClkMax > 25)	
		{ 
		  LOG(logWARNING) << "PixTestSetup::setParameter() ClkMax out of range (0-25)";
		  found=false; ParOutOfRange = true;
		}
	    }
	  if (!parName.compare("desermax")) 
	    {
	      fDeserMax = atoi(sval.c_str()); 
	      if(fDeserMax < 0 || fDeserMax > 7)	
		{ 
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
  LOG(logINFO) << "PixTestSetup::init()";

  fDirectory = gFile->GetDirectory(fName.c_str());
  if (!fDirectory)
    fDirectory = gFile->mkdir(fName.c_str());
  fDirectory->cd();
}

void PixTestSetup::setToolTips()
{
  fTestTip = string(Form("scan testboard parameter settings and check for valid readout\n")
		    + string("TO BE IMPLEMENTED!!"))  //FIXME
    ;
  fSummaryTip = string("summary plot to be implemented")  //FIXME
    ;

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
  int Ntrig = 2;
	
  LOG(logINFO) << "PixTestSetup::doTest() ntrig = " << Ntrig;

  bookHist("bla"); //FIXME

  // Setup a new pattern with only res and token:
  std::vector<std::pair<std::string, uint8_t> > pg_setup;
  pg_setup.push_back(make_pair("resetroc", 25));    // PG_RESR b001000
  pg_setup.push_back(make_pair("token", 0));     // PG_TOK  b000001
  uint16_t period = 28;
  fApi->setPatternGenerator(pg_setup);
  LOG(logINFO) << "PixTestSetup:: pg set to RES|TOK";


  TH2D *histo = new TH2D(Form("DeserphaseClkScan"), Form("DeserphaseClkScan"), fDeserMax, 0., (fDeserMax+1), fClkMax, 0., (fClkMax+1));
  histo->GetXaxis()->SetTitle("deser160phase");
  histo->GetYaxis()->SetTitle("clk");
  fHistList.push_back(histo);

  std::vector<rawEvent> daqRawEv;
  int ideser, iclk;
  int good_clk = -1, good_deser = -1;

  for (ideser = 0; ideser <= fDeserMax; ideser++) printf("        %i", ideser);
  cout << endl;

  // Start the DAQ:
  fApi->daqStart();

  // Loop over the Signal Delay range we want to scan:
  for (iclk = 0; iclk <= fClkMax; iclk++) {
    printf("%2i:", iclk);
    for (ideser = 0; ideser <= fDeserMax; ideser++) {

      // Set up the delays in the DTB:
      fApi->setTestboardDelays(getMagicDelays(iclk,ideser));

      // Send the triggers and read out the events:
      fApi->daqTrigger(Ntrig,period);
      daqRawEv = fApi->daqGetRawEventBuffer();

      // Trying to find the ROC header 0x7f8 in the raw DESER160 data. Using the last
      // of the sent triggers only:
      if(!daqRawEv.empty()) {
	// Get the last vector entry:
	rawEvent evt = daqRawEv.back();
	// Get the first word from ROC header:
	int h = int(evt.data.at(0) & 0xffc);
	if (h == 0x7f8) {
	  printf(" <%03X>", h);
	  histo->Fill(ideser, iclk);
	  good_clk = iclk; good_deser = ideser;					
	}
	else printf("  %03X ", h);

	// Additionally print the event length:
	if (evt.data.size() < 10) printf("[%u]", static_cast<unsigned int>(evt.data.size()));
	else printf("[*]");
      }
      else cout << "  ......";
    }
    cout << endl;
  }

  // Stop the DAQ:
  fApi->daqStop();

  histo->Draw("colz");
  fDisplayedHist = find(fHistList.begin(), fHistList.end(), histo);
  PixTest::update(); //needed?
    
  int finalclk, finaldeser;

  if (good_clk != -1 && good_deser != -1) {
    //algorithm to choose the best values - TO BE IMPLEMENTED FIXME
    //now initialized to the second to last (not good for long cable):
    finalclk = good_clk-1;
    finaldeser = good_deser;
    //	LOG(logINFO) << "PixTestSetup:: good delays are:" << endl << "clk = "<<  finalclk << " -  deser160 = " << finaldeser << endl;
    //FIXME setTbParameters(finalclk, finaldeser);
  }

  else {
    //back to default values
    finalclk = finaldeser = 4;
    LOG(logINFO) << "PixTestSetup::doTest() none good delays found. Back to default values (clk 4 - deser 4)"<< endl;
    //setTbParameters(finalclk, finaldeser);
  }

  // FIXME Set final clk and deser160:
  sig_delays = fPixSetup->getConfigParameters()->getTbSigDelays();
  fApi->setTestboardDelays(sig_delays);

  // Reset the pattern generator to the configured default:
  fApi->setPatternGenerator(fPixSetup->getConfigParameters()->getTbPgSettings());

  
  fHistList.clear();
	
  saveTbParameters();
  LOG(logINFO) << "PixTestSetup::doTest() done" ;
  LOG(logINFO) << "clk = " << finalclk << ", deser160 = " << finaldeser;
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
  LOG(logINFO) << "Write Tb parameters to file"; 
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
