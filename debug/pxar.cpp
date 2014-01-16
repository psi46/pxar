#include <unistd.h>
#include "pxar.h"
#include <iomanip>
#include <iostream>
#include <string>
#include <cstring>

int main(int argc, char* argv[]) {

  std::cout << argc << " arguments provided." << std::endl;

  bool module = false;

  // Prepare some vectors for all the configurations we use:
  std::vector<std::pair<std::string,uint8_t> > sig_delays;
  std::vector<std::pair<std::string,double> > power_settings;
  std::vector<std::pair<uint16_t,uint8_t> > pg_setup;

  // DTB delays

  // Board 84
  /*  sig_delays.push_back(std::make_pair("clk",4));
  sig_delays.push_back(std::make_pair("ctr",4));
  sig_delays.push_back(std::make_pair("sda",19));
  sig_delays.push_back(std::make_pair("tin",9));*/
  

  sig_delays.push_back(std::make_pair("clk",2));
  sig_delays.push_back(std::make_pair("ctr",2));
  sig_delays.push_back(std::make_pair("sda",17));
  sig_delays.push_back(std::make_pair("tin",7));
  sig_delays.push_back(std::make_pair("deser160phase",4));
  // Nasty things to catch:
  sig_delays.push_back(std::make_pair("waggawagga",219));
  sig_delays.push_back(std::make_pair("tin",7));

  // Power settings:
  power_settings.push_back(std::make_pair("va",1.9));
  power_settings.push_back(std::make_pair("vd",2.6));
  power_settings.push_back(std::make_pair("ia",1.190));
  power_settings.push_back(std::make_pair("id",1.10));
  // Try to do some nasty stuff:
  power_settings.push_back(std::make_pair("vxyz",-1.9));
  power_settings.push_back(std::make_pair("vhaha",200.9));


  // Prepare some empty TBM vector:
  std::vector<std::vector<std::pair<std::string,uint8_t> > > tbmDACs;

  std::string roctype = "psi46digv2";

  // Create some fake DUT/DAC parameters since we can't read configs yet:
  std::vector<std::vector<std::pair<std::string,uint8_t> > > rocDACs;
  std::vector<std::pair<std::string,uint8_t> > dacs;

  if(roctype.compare("psi46digv2") == 0) {
    // DAC settings to operate a V2 chip are slightly different, this ine has seen 50MRAD already:
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
  }
  else {
    dacs.push_back(std::make_pair("Vdig",5));
    // Let's try to trich the API and set Vdig again:
    dacs.push_back(std::make_pair("Vdig",7));
    dacs.push_back(std::make_pair("Vana",84));
    dacs.push_back(std::make_pair("Vsf",30));
    dacs.push_back(std::make_pair("Vcomp",12));
    dacs.push_back(std::make_pair("VwllPr",60));
    dacs.push_back(std::make_pair("VwllSh",60));
    dacs.push_back(std::make_pair("VhldDel",230));
    dacs.push_back(std::make_pair("Vtrim",29));
    dacs.push_back(std::make_pair("VthrComp",86));
    dacs.push_back(std::make_pair("VIBias_Bus",1));
    dacs.push_back(std::make_pair("Vbias_sf",6));
    dacs.push_back(std::make_pair("VoffsetOp",40));
    dacs.push_back(std::make_pair("VOffsetRO",129));
    dacs.push_back(std::make_pair("VIon",120));
    dacs.push_back(std::make_pair("Vcomp_ADC",100));
    dacs.push_back(std::make_pair("VIref_ADC",91));
    dacs.push_back(std::make_pair("VIbias_roc",150));
    dacs.push_back(std::make_pair("VIColOr",50));
    dacs.push_back(std::make_pair("Vcal",220));
    dacs.push_back(std::make_pair("CalDel",122));
    dacs.push_back(std::make_pair("CtrlReg",4));
    dacs.push_back(std::make_pair("WBC",100));
    dacs.push_back(std::make_pair("WBCDEFG",100));
  }

  // Get some pixelConfigs up and running:
  std::vector<std::vector<pxar::pixelConfig> > rocPixels;
  std::vector<pxar::pixelConfig> pixels;

  for(int col = 0; col < 52; col++) {
    for(int row = 0; row < 80; row++) {
      pxar::pixelConfig newpix;
      newpix.column = col;
      newpix.row = row;
      newpix.trim = 15;
      newpix.mask = true;
      newpix.enable = false;

      pixels.push_back(newpix);
    }
  }


  // Prepare for running a module setup
  if(argc > 2 && strcmp(argv[2],"-mod") == 0) {
    std::cout << "Module setup." << std::endl;
    module = true;

    // Pattern Generator:
    pg_setup.push_back(std::make_pair(0x1000,15)); // PG_REST
    pg_setup.push_back(std::make_pair(0x0400,106)); // PG_CAL
    pg_setup.push_back(std::make_pair(0x2200,0));  // PG_TRG PG_SYNC

    // TBM configuration:
    std::vector<std::pair<std::string,uint8_t> > regs;
    regs.push_back(std::make_pair("clear",0xF0));       // Init TBM, Reset ROC
    regs.push_back(std::make_pair("counters",0x01));    // Disable PKAM Counter
    regs.push_back(std::make_pair("mode",0xC0));        // Set Mode = Calibration
    regs.push_back(std::make_pair("pkam_set",0x10));    // Set PKAM Counter
    regs.push_back(std::make_pair("delays",0x00));      // Set Delays
    regs.push_back(std::make_pair("temperature",0x00)); // Turn off Temperature Measurement

    tbmDACs.push_back(regs);

    // 16 ROC configs:
    for(int i = 0; i < 16; i++) {
      rocDACs.push_back(dacs);
      rocPixels.push_back(pixels);
    }

  }
  // Prepare for running a single ROC setup
  else {
    std::cout << "Single ROC setup." << std::endl;

    // Pattern Generator:
    pg_setup.push_back(std::make_pair(0x0800,25));    // PG_RESR
    pg_setup.push_back(std::make_pair(0x0400,101+5)); // PG_CAL
    pg_setup.push_back(std::make_pair(0x0200,16));    // PG_TRG
    pg_setup.push_back(std::make_pair(0x0100,0));     // PG_TOK

    // One ROC config:
    rocDACs.push_back(dacs);
    rocPixels.push_back(pixels);
  }

  // Create new API instance:
  try {
    _api = new pxar::api("*",argv[1] ? argv[1] : "DEBUG");
  
    //_api->flashTB("/tmp/dtb_v0111.flash");
    //_api->flashTB("/tmp/Pixel/dtb_v01122.flash");
    //_api->flashTB("/tmp/dtb_v1.14.flash");

    // Initialize the testboard:
    _api->initTestboard(sig_delays, power_settings, pg_setup);

    // Read DUT info, should result in error message, not initialized:
    _api->_dut->info();

    // Initialize the DUT (power it up and stuff):
    _api->initDUT("tbm08",tbmDACs,"psi46dig",rocDACs,rocPixels);

    // Read DUT info, should print above filled information:
    _api->_dut->info();

    // Read current:
    std::cout << "Analog current: " << _api->getTBia()*1000 << "mA" << std::endl;
    std::cout << "Digital current: " << _api->getTBid()*1000 << "mA" << std::endl;

    _api->HVon();
    sleep(1);

    // Set some DAC after the "real" DUT initialization (all active ROCs):
    _api->setDAC("Vcal",101,0);
    // And check if the DUT has the updated value:
    std::cout << "New Vcal value: " << (int)_api->_dut->getDAC(0,"vCaL") << std::endl;


    /*
    // Test power-cycling and re-programming:
    _api->Poff();
    //    sleep(1);
    _api->Pon();
    */

    // ##########################################################
    // call a 'demo' (i.e. fake) DAC scan routine
    /*
    std::vector< std::pair<uint8_t, std::vector<pxar::pixel> > > data = _api->getDebugVsDAC("vana", 20, 28, 50, 16);

    // check out the data we received:
    std::cout << " number of stored (DAC,pixels) pairs in data: " << data.size() << std::endl;
    // loop over dac values:
    for (std::vector< std::pair<uint8_t, std::vector<pxar::pixel> > >::iterator dacit = data.begin();dacit != data.end(); ++dacit){
      std::cout << "   dac value: " << (int) dacit->first << " has " << dacit->second.size() << " fired pixels " << std::endl;
      // loop over fired pixels and show value
      for (std::vector<pxar::pixel>::iterator pixit = dacit->second.begin(); pixit != dacit->second.end();++pixit){
	std::cout << "       pixel " << (int)  pixit->column << ", " << (int)  pixit->row << " has value "<< (int)  pixit->value <<  std::endl;
      }
    }
    */
    // ##########################################################

    
    // ##########################################################
    // Call the first real test (pixel efficiency map):
    
    // Enable all pixels first:
    _api->_dut->testAllPixels(true);

    // Head:
    for(int i = 25; i < 55; i++) _api->_dut->maskPixel(5,i,true);

    int j = 24;
    int k = 56;
    for(int i = 6; i < 11; i++) {
      _api->_dut->maskPixel(i,j--,true);
      _api->_dut->maskPixel(i,j--,true);
      _api->_dut->maskPixel(i,k++,true);
      _api->_dut->maskPixel(i,k++,true);
    }

    j = 15;
    k = 65;
    for(int i = 11; i < 14; i++) {
      _api->_dut->maskPixel(i,j--,true);
      _api->_dut->maskPixel(i,j--,true);
      j++;
      _api->_dut->maskPixel(i,k++,true);
      _api->_dut->maskPixel(i,k++,true);
      k--;
    }

    for(int i = 14; i < 17; i++) {
      _api->_dut->maskPixel(i,12,true);
      _api->_dut->maskPixel(i,13,true);
      _api->_dut->maskPixel(i,68,true);
      _api->_dut->maskPixel(i,69,true);
    }

    j = 68;
    k = 13;
    for(int i = 17; i < 20; i++) {
      _api->_dut->maskPixel(i,j--,true);
      _api->_dut->maskPixel(i,j--,true);
      j++;
      _api->_dut->maskPixel(i,k++,true);
      _api->_dut->maskPixel(i,k++,true);
      k--;
    }

    j = 65;
    k = 16;
    for(int i = 20; i < 25; i++) {
      _api->_dut->maskPixel(i,j--,true);
      _api->_dut->maskPixel(i,j--,true);
      _api->_dut->maskPixel(i,k++,true);
      _api->_dut->maskPixel(i,k++,true);
    }

    for(int i = 25; i < 55; i++) _api->_dut->maskPixel(25,i,true);

    // Eyes:
    _api->_dut->maskPixel(7,37,true);
    _api->_dut->maskPixel(7,41,true);

    // Mouth:
    for(int i = 30; i < 50; i++) _api->_dut->maskPixel(19,i,true);
    _api->_dut->maskPixel(18,28,true);
    _api->_dut->maskPixel(18,29,true);
    _api->_dut->maskPixel(18,30,true);
    _api->_dut->maskPixel(18,50,true);
    _api->_dut->maskPixel(18,51,true);
    _api->_dut->maskPixel(18,52,true);
    _api->_dut->maskPixel(17,27,true);
    _api->_dut->maskPixel(17,28,true);
    _api->_dut->maskPixel(17,52,true);
    _api->_dut->maskPixel(17,53,true);

    //_api->_dut->maskColumn(8,true);

    // Call the test:
    int nTrig = 10;
    std::vector< pxar::pixel > mapdata = _api->getEfficiencyMap(0,nTrig);
    std::cout << "Data size returned: " << mapdata.size() << std::endl;

    
    std::cout << "ASCII Sensor Efficiency Map:" << std::endl;
    unsigned int row = 0;
    int oldroc = 0;
    for (std::vector< pxar::pixel >::iterator mapit = mapdata.begin(); mapit != mapdata.end(); ++mapit) {
      
      //std::cout << "Px " << (int)mapit->column << ", " << (int)mapit->row << " has efficiency " << (int)mapit->value << "/" << nTrig << " = " << (mapit->value/nTrig) << std::endl;
      if(oldroc != (int)mapit->roc_id) {
	char ch;
	std::cout << "Press the \"Any\" key. Go home if you don't find it.";
	std::cin >> ch;
	oldroc = (int)mapit->roc_id;
	std::cout << "Looking at ROC " << oldroc << " now" << std::endl;
      }

      if((int)mapit->value == nTrig) std::cout << "X";
      else if((int)mapit->value == 0) std::cout << "-";
      else if((int)mapit->value > nTrig) std::cout << "#";
      else std::cout << (int)mapit->value;

      row++;
      if(row >= 80) { 
	row = 0;
	std::cout << std::endl;
      }
    }
    
    // ##########################################################


    // ##########################################################
    // Call the second real test (a DAC scan for some pixels):
    
    // disable pixel(s)
    _api->_dut->testAllPixels(false);
    //    _api->_dut->testPixel(34,12,true);
    //    _api->_dut->testPixel(33,12,true);
    //    _api->_dut->testPixel(34,11,true);
    _api->_dut->testPixel(14,12,true);

    _api->_dut->info();
    

    // Call the test:
    unsigned nTrig2 = 10;
    std::vector< std::pair<uint8_t, std::vector<pxar::pixel> > > 
      effscandata = _api->getEfficiencyVsDAC("caldel", 0, 150, 0, nTrig2);
    
    // Check out the data we received:
    std::cout << "Number of stored (DAC, pixels) pairs in data: " << effscandata.size() << std::endl;
    
    // Loop over dac values:
    for(std::vector< std::pair<uint8_t, std::vector<pxar::pixel> > >::iterator dacit = effscandata.begin();
	dacit != effscandata.end(); ++dacit) {
      std::cout << "  dac value: " << (int)dacit->first << " has " << dacit->second.size() << " fired pixels " << std::endl;
      // Loop over fired pixels and show value
      for (std::vector<pxar::pixel>::iterator pixit = dacit->second.begin(); pixit != dacit->second.end();++pixit){
	std::cout << "      pixel " << (int)  pixit->column << ", " << (int)  pixit->row << " has value "<< (int)  pixit->value << std::endl;
      }
    }
    
    // ##########################################################
    
    // ##########################################################
    // Call the third real test (a DACDAC scan for some pixels):
    /*
    // disable pixel(s)
    _api->_dut->testAllPixels(false);
    _api->_dut->testPixel(33,12,true);

    // Call the test:
    nTrig = 10;
    int limit = 120;
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > effscan2ddata = _api->getEfficiencyVsDACDAC("caldel", 0, limit, "vthrcomp", 0, limit, 0, nTrig);
    //std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > effscan2ddata = _api->getEfficiencyVsDACDAC("vbias_sf", 0, limit, "vcomp", 0, limit, 0, nTrig);
    
    // Check out the data we received:
    std::cout << "Number of stored (DAC, pixels) pairs in data: " << effscan2ddata.size() << std::endl;
    
    int dac = 0;
    std::cout << "VthrComp vs. CalDel:" << std::endl;
    for(std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit = effscan2ddata.begin(); dacit != effscan2ddata.end(); ++dacit) {
      
      if(dacit->second.second.empty()) {
	std::cout << "-";
	continue;
      }
      int value = dacit->second.second.at(0).value;
      if(value == nTrig) std::cout << "X";
      else if(value == 0) std::cout << " ";
      else if(value > nTrig) std::cout << "#";
      else std::cout << value;

      dac++;
      if(dac >= limit) { 
	dac = 0;
	std::cout << std::endl;
      }
    }
    */
    // ##########################################################

    // ##########################################################
    // Let's spy a bit on the DTB scope ports:

    _api->SignalProbe("D1","sda");
    _api->SignalProbe("A2","sda");

    // ##########################################################

    // ##########################################################
    // Do some Raw data acquisition:

    // All on!
    _api->_dut->testAllPixels(false);
    _api->_dut->maskAllPixels(false);

    for(int i = 0; i < 1; i++) {
      _api->_dut->testPixel(i,5,true);
      //_api->_dut->testPixel(i,6,true);
      //_api->_dut->testPixel(i,7,true);
      //_api->_dut->testPixel(i,8,true);
      /*      _api->_dut->testPixel(i,9,true);
      _api->_dut->testPixel(i,10,true);
      _api->_dut->testPixel(i,11,true);
      _api->_dut->testPixel(i,12,true);*/
    }

    _api->daqStart(pg_setup);
    _api->daqTrigger(800);
    _api->daqStop();
    std::vector<uint16_t> daqdat = _api->daqGetBuffer();
    
    std::cout << "Raw DAQ data blob:" << std::endl;
    for(std::vector<uint16_t>::iterator it = daqdat.begin();
	it != daqdat.end();
	++it) {
      if(((*it) & 0xF000) > 0x4000) std::cout << std::endl;
      std::cout << std::hex << std::setw(4) << std::setfill('0') << (*it) << " ";
    }
    std::cout << std::dec << std::endl;
    
    // ##########################################################

    _api->HVoff();

    // And end that whole thing correcly:
    std::cout << "Done." << std::endl;
    delete _api;
  }
  catch (...) {
    std::cout << "pxar cauhgt an exception from the board. Exiting." << std::endl;
    delete _api;
    return -1;
  }
  
  return 0;
}
