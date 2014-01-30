#ifndef WIN32
#include <unistd.h>
#endif

#include "pxar.h"
#include <iomanip>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>
#include <stdlib.h>

int main(int argc, char* argv[]) {

  std::cout << argc << " arguments provided." << std::endl;
  std::string verbosity, filename;
  uint32_t triggers = 0;

  // Quick and hacky cli arguments reading:
  for (int i = 0; i < argc; i++) {
    if (!strcmp(argv[i],"-h")) {
      std::cout << "Help:" << std::endl;
      std::cout << "-f filename    file to store DAQ data in" << std::endl;
      std::cout << "-n triggers    number of triggers to be sent" << std::endl;
      std::cout << "-v verbosity   verbosity level, default INFO" << std::endl;
      return 0;
    }
    if (!strcmp(argv[i],"-f")) {
      filename = std::string(argv[++i]);
    }
    if (!strcmp(argv[i],"-n")) {
      triggers = atoi(argv[++i]);
    }
    if (!strcmp(argv[i],"-v")) {
      verbosity = std::string(argv[++i]);
    }               
  }

  // Prepare some vectors for all the configurations we use:
  std::vector<std::pair<std::string,uint8_t> > sig_delays;
  std::vector<std::pair<std::string,double> > power_settings;
  std::vector<std::pair<uint16_t,uint8_t> > pg_setup;

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
  pg_setup.push_back(std::make_pair(0x0800,25));    // PG_RESR
  pg_setup.push_back(std::make_pair(0x0400,101+5)); // PG_CAL
  pg_setup.push_back(std::make_pair(0x0200,16));    // PG_TRG
  pg_setup.push_back(std::make_pair(0x0100,0));     // PG_TOK

  // Prepare some empty TBM vector:
  std::vector<std::vector<std::pair<std::string,uint8_t> > > tbmDACs;

  // Set the type of the ROC correctly:
  std::string roctype = "psi46digv2";

  // Create some hardcoded DUT/DAC parameters since we can't read configs yet:
  std::vector<std::vector<std::pair<std::string,uint8_t> > > rocDACs;
  std::vector<std::pair<std::string,uint8_t> > dacs;

  dacs.push_back(std::make_pair("Vdig",8));
  dacs.push_back(std::make_pair("Vana",120));
  dacs.push_back(std::make_pair("Vsf",40));
  dacs.push_back(std::make_pair("Vcomp",12));
  dacs.push_back(std::make_pair("VwllPr",30));
  dacs.push_back(std::make_pair("VwllSh",30));
  dacs.push_back(std::make_pair("VhldDel",117));
  dacs.push_back(std::make_pair("Vtrim",1));
  dacs.push_back(std::make_pair("VthrComp",40));
  dacs.push_back(std::make_pair("VIBias_Bus",30));
  dacs.push_back(std::make_pair("Vbias_sf",6));
  dacs.push_back(std::make_pair("VoffsetOp",60));
  dacs.push_back(std::make_pair("VOffsetRO",150));
  dacs.push_back(std::make_pair("VIon",45));
  dacs.push_back(std::make_pair("Vcomp_ADC",50));
  dacs.push_back(std::make_pair("VIref_ADC",70));
  dacs.push_back(std::make_pair("VIbias_roc",150));
  dacs.push_back(std::make_pair("VIColOr",99));
  dacs.push_back(std::make_pair("Vcal",220));
  dacs.push_back(std::make_pair("CalDel",122));
  dacs.push_back(std::make_pair("CtrlReg",4));
  dacs.push_back(std::make_pair("WBC",100));

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
    _api = new pxar::api("*",verbosity != "" ? verbosity : "INFO");
  
    // Initialize the testboard:
    if(!_api->initTestboard(sig_delays, power_settings, pg_setup)) {
      delete _api;
      return -1;
    }

    // Initialize the DUT (power it up and stuff):
    if (!_api->initDUT("tbm08",tbmDACs,roctype,rocDACs,rocPixels)){
      std::cout << " initDUT failed -> invalid configuration?! " << std::endl;
      delete _api;
      return -2;
    }
    
    // Read DUT info, should print above filled information:
    _api->_dut->info();

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
    for(int i = 0; i < 3; i++) {
      _api->_dut->testPixel(i,5,true);
      _api->_dut->testPixel(i,6,true);
    }

    // Start the DAQ:
    _api->daqStart(pg_setup);
    uint32_t daq_check_triggers = 0;
    
    // Send the triggers:
    _api->daqTrigger(triggers);

    //FIXME not working...
    //_api->daqTriggerLoop(10);

    // Stop the DAQ:
    _api->daqStop();

    // And read out the full buffer:
    std::vector<uint16_t> daqdat = _api->daqGetBuffer();

    std::cout << "Size of data read from board: " << daqdat.size() << std::endl;
    // Count triggers:
    for(std::vector<uint16_t>::iterator it = daqdat.begin();
	it != daqdat.end();
	++it) {
      if(((*it) & 0xF000) > 0x4000 || (*it) == 0x0080) {
	// std::cout << std::endl;
	daq_check_triggers++;
      }
    }
    std::cout << "Found " << daq_check_triggers << " triggers in data, expected " << triggers << std::endl;

    // Write all the data to the file:
    FILE * pFile;
    pFile = fopen(filename != "" ? filename.c_str() : "defaultdata.dat","w");
    fwrite (&daqdat[0], sizeof(uint16_t), daqdat.size(), pFile); 
    fclose(pFile);

    // ##########################################################

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
