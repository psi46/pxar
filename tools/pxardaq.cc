#ifndef WIN32
#include <unistd.h>
#else
#include <Windows.h>
#endif

#include "pxar.h"
#include "timer.h"
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdio>
#include <stdlib.h>
#include <signal.h>

bool daq_loop = true;

void sighandler(int sig) {
  std::cout << "Signal " << sig << " caught..." << std::endl;
  std::cout << "Finishing and shutting down." << std::endl;
  daq_loop = false;
}

void wait(int sec) {
#ifdef WIN32
   Sleep(sec*1000);
#else
   sleep(sec);
#endif
}

int getspill() {
   // Open spill number file:
   std::ifstream myfile;
   int spillnumber = 0;
   myfile.open ("/home/pixel_dev/.currentSpill");
   myfile >> spillnumber;
  
   // Close spill file:
   myfile.close();
   return spillnumber;       
}

int main(int argc, char* argv[]) {

  std::cout << argc << " arguments provided." << std::endl;
  std::string verbosity, filename;
  uint32_t triggers = 0;
  bool testpulses = false;
  bool spills = false;
  bool oos = false;

  uint8_t hubid = 31;

  // Quick and hacky cli arguments reading:
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i],"-h")) {
      std::cout << "Help:" << std::endl;
      std::cout << "-f filename    file to store DAQ data in" << std::endl;
      std::cout << "-n triggers    number of triggers to be sent" << std::endl;
      std::cout << "-v verbosity   verbosity level, default INFO" << std::endl;
      std::cout << "-sp            lock on accelerator spills" << std::endl;
      std::cout << "-tp            activate test pulses" << std::endl;
      std::cout << "-oos           test OutOfSync problem w/ 100 triggers & 1 token" << std::endl;
      return 0;
    }
    else if (!strcmp(argv[i],"-f")) {
      filename = std::string(argv[++i]);
      std::cout << "Writing to file " << filename << std::endl;
      continue;
    }
    else if (!strcmp(argv[i],"-n")) {
      triggers = atoi(argv[++i]);
      std::cout << "Sending " << triggers << " triggers" << std::endl;
      continue;
    }
    else if (!strcmp(argv[i],"-v")) {
      verbosity = std::string(argv[++i]);
      continue;
    }               
    else if (!strcmp(argv[i],"-tp")) {
      testpulses = true;
      continue;
    }
    else if (!strcmp(argv[i],"-sp")) {
      spills = true;
      continue;
    }
    else if (!strcmp(argv[i],"-oos")) {
      oos = true;
      continue;
    }
    else {
      std::cout << "Unrecognized command line option " << argv[i] << std::endl;
    }
  }

  // Prepare some vectors for all the configurations we use:
  std::vector<std::pair<std::string,uint8_t> > sig_delays;
  std::vector<std::pair<std::string,double> > power_settings;
  std::vector<std::pair<std::string,uint8_t> > pg_setup;
  std::vector<std::pair<std::string,uint8_t> > pg_setup2;

  // DTB delays
  sig_delays.push_back(std::make_pair("clk",2));
  sig_delays.push_back(std::make_pair("ctr",2));
  sig_delays.push_back(std::make_pair("sda",17));
  sig_delays.push_back(std::make_pair("tin",7));
  sig_delays.push_back(std::make_pair("deser160phase",4));

  // Power settings:
  power_settings.push_back(std::make_pair("va",1.9));
  power_settings.push_back(std::make_pair("vd",2.6));
  power_settings.push_back(std::make_pair("ia",1.190));
  power_settings.push_back(std::make_pair("id",1.10));

  // Pattern Generator:
  int pattern_delay = 0;
  if(oos) {
    std::cout << "Pattern generator preparation for OOS tests." << std::endl;
    // No ROC reset
    // 99x no CAL, only TRG and TOK for Xray background
    for(int i = 0; i < 99; i++) {
      pg_setup.push_back(std::make_pair("trigger",16));    // PG_TRG
      // Delay adjusted fro trigger rate of 90kHz
      pg_setup.push_back(std::make_pair("token",426));     // PG_TOK    
    }
    // One additional cycle w/ CAL TRG TOK
    pg_setup.push_back(std::make_pair("calibrate",101+5)); // PG_CAL
    pg_setup.push_back(std::make_pair("trigger",16));    // PG_TRG
    pg_setup.push_back(std::make_pair("token",0));     // PG_TOK

    // Second PG for the first reset and one trigger:
    pg_setup2.push_back(std::make_pair("resetroc",25));    // PG_RESR
    pg_setup2.push_back(std::make_pair("calibrate",101+5)); // PG_CAL
    pg_setup2.push_back(std::make_pair("trigger",16));    // PG_TRG
    pg_setup2.push_back(std::make_pair("token",0));     // PG_TOK
    
    // And done.
    pattern_delay = 1000000;
  }
  else if(testpulses) {
    std::cout << "Pattern generator preparation for testpulse patterns." << std::endl;
     pg_setup.push_back(std::make_pair("resetroc",25));    // PG_RESR
     pg_setup.push_back(std::make_pair("calibrate",101+5)); // PG_CAL
     pg_setup.push_back(std::make_pair("trigger",16));    // PG_TRG
     pg_setup.push_back(std::make_pair("token",0));     // PG_TOK
     pattern_delay = 1000;
  }
  else {
     pg_setup.push_back(std::make_pair("trigger",46));    // PG_TRG
     pg_setup.push_back(std::make_pair("token",0));     // PG_TOK
     pattern_delay = 100;
  }

  // Prepare some empty TBM vector:
  std::vector<std::vector<std::pair<std::string,uint8_t> > > tbmDACs;

  // Set the type of the ROC correctly:
  std::string roctype = "psi46digv21";

  // Create some hardcoded DUT/DAC parameters since we can't read configs yet:
  std::vector<std::vector<std::pair<std::string,uint8_t> > > rocDACs;
  std::vector<std::pair<std::string,uint8_t> > dacs;

  dacs.push_back(std::make_pair("Vdig",8));
  dacs.push_back(std::make_pair("Vana",78));
  dacs.push_back(std::make_pair("Vsf",80));
  dacs.push_back(std::make_pair("Vcomp",12));
  dacs.push_back(std::make_pair("VwllPr",150));
  dacs.push_back(std::make_pair("VwllSh",150));
  dacs.push_back(std::make_pair("VhldDel",117));
  dacs.push_back(std::make_pair("Vtrim",152));
  dacs.push_back(std::make_pair("VthrComp",89));
  dacs.push_back(std::make_pair("VIBias_Bus",30));
  dacs.push_back(std::make_pair("Vbias_sf",6));
  dacs.push_back(std::make_pair("VoffsetOp",60));
  dacs.push_back(std::make_pair("VOffsetRO",225));
  dacs.push_back(std::make_pair("VIon",45));
  dacs.push_back(std::make_pair("Vcomp_ADC",10));
  dacs.push_back(std::make_pair("VIref_ADC",70));
  dacs.push_back(std::make_pair("VIbias_roc",150));
  dacs.push_back(std::make_pair("VIColOr",99));
  dacs.push_back(std::make_pair("Vcal",199));
  dacs.push_back(std::make_pair("CalDel",140));
  dacs.push_back(std::make_pair("CtrlReg",0));
  dacs.push_back(std::make_pair("WBC",200));
  dacs.push_back(std::make_pair("rbreg",12));

  // Get some pixelConfigs up and running:
  std::vector<std::vector<pxar::pixelConfig> > rocPixels;
  std::vector<pxar::pixelConfig> pixels;

  for(int col = 0; col < 52; col++) {
    for(int row = 0; row < 80; row++) {
      pixels.push_back(pxar::pixelConfig(col,row,15));
    }
  }

  // One ROC config:
  rocDACs.push_back(dacs);
  rocPixels.push_back(pixels);

  // Create new API instance:
  try {
    _api = new pxar::pxarCore("*",verbosity != "" ? verbosity : "INFO");
  
    // Initialize the testboard:
    if(!_api->initTestboard(sig_delays, power_settings, pg_setup)) {
      delete _api;
      return -1;
    }

    // Initialize the DUT (power it up and stuff):
    if (!_api->initDUT(hubid,"tbm08",tbmDACs,roctype,rocDACs,rocPixels)){
      std::cout << " initDUT failed -> invalid configuration?! " << std::endl;
      delete _api;
      return -2;
    }
    
    // Read current:
    std::cout << "Analog current: " << _api->getTBia()*1000 << "mA" << std::endl;
    std::cout << "Digital current: " << _api->getTBid()*1000 << "mA" << std::endl;

    _api->HVon();

    // ##########################################################
    // Do some Raw data acquisition:
    
    // All on!
    _api->_dut->testAllPixels(false);
    _api->_dut->maskAllPixels(false);

    // Set some pixels up for getting calibrate signals:
    if(testpulses || oos) {
       std::cout << "Setting up pixels for calibrate pulses..." << std::endl;
      for(int i = 0; i < 3; i++) {
         _api->_dut->testPixel(i,5,true);
         _api->_dut->testPixel(i,6,true);
      }
    }

    // Read DUT info, should print above filled information:
    _api->_dut->info();

    // Setup signal handlers to allow interruption:
    signal(SIGABRT, &sighandler);
    signal(SIGTERM, &sighandler);
    signal(SIGINT, &sighandler);

    int oldspillnumber = 0;
    pxar::timer * spillruntime = new pxar::timer();

    // Wait for next spill until we start the DAQ:
    if(spills) {
      oldspillnumber = getspill();
      std::cout << "Waiting for spill " << getspill() << " to finish..." << std::endl;
      while(daq_loop && getspill() == oldspillnumber) {
	std::cout << "." << std::flush;
	wait(1);
      }
      std::cout << std::endl << "Starting DAQ at spill " << getspill() << std::endl;
    }

    // Sent one PG cycle with a reset:
    if(oos) {
      _api->daqStart();
      _api->daqTrigger(1);
      _api->daqStop();
      std::vector<uint16_t> garbage = _api->daqGetBuffer();
    }

    //Start the main DAQ loop:
    while(daq_loop) {

      if(spills) {
	oldspillnumber = getspill();
	spillruntime = new pxar::timer();

	std::cout << "Waiting for beam section of the spill..." << std::endl;
	while(spillruntime->get() < 47000 && getspill() == oldspillnumber && daq_loop) {
          wait(1);
          std::cout << "Spill " << getspill() << " runs since " << (spillruntime->get()/1000) << "sec...\r" << std::flush;
	}
	std::cout << std::endl << "Data acquisition for spill " << getspill() << " started." << std::endl;
      }

      // Start the DAQ:
      _api->daqStart();

      // Send the triggers:
      if(triggers != 0) {
	std::cout << "Start sending " << triggers << " triggers..." << std::endl;
	_api->daqTrigger(triggers);
	daq_loop = false;
      }
      // Enter the trigger loop:
      else {
	_api->daqTriggerLoop(pattern_delay);
	while(_api->daqStatus() && daq_loop) {
          if(spills) {
	    std::cout << "." << std::flush;
	    if(getspill() != oldspillnumber) { 
	      std::cout << std::endl << "New spill: " << getspill() << std::endl;
	      oldspillnumber = getspill();
	      spillruntime = new pxar::timer();
	      break; 
	    }
          }
          wait(1);
	}
      }
    
      // Stop the DAQ:
      _api->daqStop();

      // And read out the full buffer:
      std::cout << "Start reading data from DTB RAM." << std::endl;
      std::vector<uint16_t> daqdat = _api->daqGetBuffer();
      std::cout << "Read " << daqdat.size() << " words of data: ";
      if(daqdat.size() > 550000) std::cout << (daqdat.size()/524288) << "MB." << std::endl;
      else std::cout << (daqdat.size()/512) << "kB." << std::endl;

      // If we are running on spills just take that number as filename:
      if(spills) {
	std::stringstream sstr;
	sstr << (getspill()-1);
	filename = "tbdata/spill_" + sstr.str() + ".dat";
      }

      // Write all the data to the file:
      if(filename == "") { filename = "defaultdata.dat"; }
      std::ofstream fout(filename.c_str(), std::ios::out | std::ios::binary);
      fout.write(reinterpret_cast<const char*>(&daqdat[0]), sizeof(daqdat[0])*daqdat.size());
      fout.close();
      std::cout << "Wrote data to file " << filename << std::endl;

    } // End of DAQ loop

    delete spillruntime;
    _api->HVoff();

    // And end that whole thing correcly:
    std::cout << "Done. Thanks for taking data." << std::endl;
    delete _api;
  }
  catch (...) {
    std::cout << "pxar takedata mode caught an exception. Exiting." << std::endl;
    delete _api;
    return -1;
  }
  
  return 0;
}
