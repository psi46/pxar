#include <unistd.h>
#include "pxar.h"
#include <iostream>
#include <string>

int main()
{

  // Create new API instance:
  try {
    _api = new pxar::api("*","DEBUGRPC");
  
    // Try some test or so:

    // Create some dummy testboard config, not doing anything right now:
    std::vector<std::pair<std::string,uint8_t> > sig_delays;
    sig_delays.push_back(std::make_pair("clk",2));
    sig_delays.push_back(std::make_pair("ctr",20));
    sig_delays.push_back(std::make_pair("sda",19));
    sig_delays.push_back(std::make_pair("tin",7));
    sig_delays.push_back(std::make_pair("deser160phase",4));

    std::vector<std::pair<std::string,double> > power_settings;
    std::vector<std::pair<uint16_t,uint8_t> > pg_setup;


    // Pattern Generator:

    // Module:
    //pg_setup.push_back(std::make_pair(0x1000,15)); // PG_REST
    //pg_setup.push_back(std::make_pair(0x0400,50)); // PG_CAL
    //pg_setup.push_back(std::make_pair(0x2200,0));  // PG_TRG PG_SYNC

    // Single ROC:
    pg_setup.push_back(std::make_pair(0x0800,25));    // PG_RESR
    pg_setup.push_back(std::make_pair(0x0400,101+5)); // PG_CAL
    pg_setup.push_back(std::make_pair(0x0200,16));    // PG_TRG
    pg_setup.push_back(std::make_pair(0x0100,0));     // PG_TOK

    // Power settings:
    power_settings.push_back(std::make_pair("va",1.9));
    power_settings.push_back(std::make_pair("vd",2.6));
    power_settings.push_back(std::make_pair("ia",1.190));
    power_settings.push_back(std::make_pair("id",1.10));
    // Try to do some nasty stuff:
    power_settings.push_back(std::make_pair("vxyz",-1.9));
    power_settings.push_back(std::make_pair("vhaha",200.9));

    // Initialize the testboard:
    _api->initTestboard(sig_delays, power_settings, pg_setup);

    //_api->flashTB("/tmp/dtb_v0111.flash");
    //_api->flashTB("/tmp/Pixel/dtb_v01122.flash");
    
    // Create some fake DUT/DAC parameters since we can't read configs yet:
    std::vector<std::vector<std::pair<std::string,uint8_t> > > rocDACs;

    std::vector<std::pair<std::string,uint8_t> > dacs;
    dacs.push_back(std::make_pair("Vdig",5));
    // Let's try to trich the API and set Vdig again:
    dacs.push_back(std::make_pair("Vdig",6));
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
    dacs.push_back(std::make_pair("Vcal",200));
    dacs.push_back(std::make_pair("CalDel",122));
    dacs.push_back(std::make_pair("CtrlReg",0));
    dacs.push_back(std::make_pair("WBC",100));
    std::cout << (int)dacs.size() << " DACs loaded.\n";

    rocDACs.push_back(dacs);

    // Get some pixelConfigs up and running:
    std::vector<std::vector<pxar::pixelConfig> > rocPixels;
    std::vector<pxar::pixelConfig> pixels;

    for(int col = 0; col < 52; col++) {
      for(int row = 0; row < 80; row++) {
	pxar::pixelConfig newpix;
	newpix.column = col;
	newpix.row = row;
	//Trim: fill random value, should report fail as soon as we have range checks:
	//newpix.trim = row;
	newpix.trim = 15;
	newpix.mask = false;
	newpix.enable = true;

	pixels.push_back(newpix);
      }
    }
    rocPixels.push_back(pixels);

    // Prepare some empty TBM vector:
    std::vector<std::vector<std::pair<std::string,uint8_t> > > tbmDACs;

    std::vector<std::pair<std::string,uint8_t> > regs;
    regs.push_back(std::make_pair("clear",0xF0));       // Init TBM, Reset ROC
    regs.push_back(std::make_pair("counters",0x01));    // Disable PKAM Counter
    regs.push_back(std::make_pair("mode",0xC0));        // Set Mode = Calibration
    regs.push_back(std::make_pair("pkam_set",0x10));    // Set PKAM Counter
    regs.push_back(std::make_pair("delays",0x00));      // Set Delays
    regs.push_back(std::make_pair("temperature",0x00)); // Turn off Temperature Measurement

    tbmDACs.push_back(regs);

    // Read DUT info, should result in error message, not initialized:
    _api->_dut->info();

    // Initialize the DUT (power it up and stuff):
    _api->initDUT("tbm08",tbmDACs,"psi46dig",rocDACs,rocPixels);

    // Read DUT info, should print above filled information:
    _api->_dut->info();

    // Read current:
    std::cout << "Analog current: " << _api->getTBia()*1000 << "mA" << std::endl;
    std::cout << "Digital current: " << _api->getTBid()*1000 << "mA" << std::endl;

    // Set some DAC after the "real" DUT initialization (all active ROCs):
    _api->setDAC("Vcal",101,0);
    // And check if the DUT has the updated value:
    std::cout << "New Vcal value: " << (int)_api->_dut->getDAC(0,"vCaL") << std::endl;

    // Do some debug readout: Pulseheight of px 3,3 with 10 triggers:
    _api->debug_ph(3,3,15,10);
    
    // Test power-cycling and re-programming:
    _api->Poff();
    //    sleep(1);
    _api->Pon();
    _api->debug_ph(3,3,15,10);

    /*
    // Check if we can access the DUT data structure and enable/disable stuff:
    //_api->_dut->getDAC(0,"Vana");
    
    // Print DACs from ROC 0:
    //_api->_dut->printDACs(0);

    // disable pixel(s)
    _api->_dut->setAllPixelEnable(false);
    _api->_dut->setPixelEnable(34,12,true);
    _api->_dut->setPixelEnable(33,12,true);
    _api->_dut->setPixelEnable(34,11,true);
    _api->_dut->setPixelEnable(14,12,true);

    // Mask some pixels, just because we can:
    //_api->maskPixel(0,33,11,true);

    // debug some DUT implementation details
    std::cout << " have " << _api->_dut->getNEnabledPixels() << " pixels set to enabled" << std::endl;
    std::cout << " have " << (int) _api->_dut->getPixelConfig(0,8,8).trim << " as trim value on pixel 8,8" << std::endl;
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
    _api->_dut->setAllPixelEnable(true);

    // Call the test:
    int nTrig = 10;
    std::vector< pxar::pixel > mapdata = _api->getEfficiencyMap(0,nTrig);
    std::cout << "Data size returned: " << mapdata.size() << std::endl;

    std::cout << "ASCII Sensor Efficiency Map:" << std::endl;
    unsigned int row = 0;
    for (std::vector< pxar::pixel >::iterator mapit = mapdata.begin(); mapit != mapdata.end(); ++mapit) {
      
      //std::cout << "Px " << (int)mapit->column << ", " << (int)mapit->row << " has efficiency " << (int)mapit->value << "/" << nTrig << " = " << (mapit->value/nTrig) << std::endl;

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
    _api->_dut->setAllPixelEnable(false);
    _api->_dut->setPixelEnable(34,12,true);
    _api->_dut->setPixelEnable(33,12,true);
    _api->_dut->setPixelEnable(34,11,true);
    _api->_dut->setPixelEnable(14,12,true);

    // Call the test:
    nTrig = 10;
    std::vector< std::pair<uint8_t, std::vector<pxar::pixel> > > 
      effscandata = _api->getEfficiencyVsDAC("vana", 0, 84, 0, nTrig);
    
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
    
    // disable pixel(s)
    _api->_dut->setAllPixelEnable(false);
    _api->_dut->setPixelEnable(33,12,true);
    _api->_dut->setPixelEnable(14,12,true);

    // Call the test:
    nTrig = 10;
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > 
      effscan2ddata = _api->getEfficiencyVsDACDAC("vdig", 0, 7, "vcomp", 0, 12, 0, nTrig);
    
    // Check out the data we received:
    std::cout << "Number of stored (DAC, pixels) pairs in data: " << effscan2ddata.size() << std::endl;
    
    // Loop over dacdac values:
    for(std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit = effscan2ddata.begin();
	dacit != effscan2ddata.end(); ++dacit) {
      std::cout << "  dac1: " << (int)dacit->first << " dac2: " << (int)dacit->second.first
		<< " has " << dacit->second.second.size() << " fired pixels " << std::endl;
      // Loop over fired pixels and show value
      for (std::vector<pxar::pixel>::iterator pixit = dacit->second.second.begin(); pixit != dacit->second.second.end();++pixit){
	std::cout << "      pixel " << (int)  pixit->column << ", " << (int)  pixit->row << " has value "<< (int)  pixit->value << std::endl;
      }
    }
    
    // ##########################################################

    // ##########################################################
    // Let's spy a bit on the DTB scope ports:

    _api->SignalProbe("D1","sda");
    _api->SignalProbe("A2","sda");

    // ##########################################################

    // And end that whole thing correcly:
    std::cout << "Done." << std::endl;
    delete _api;
  }
  catch (...) {
    std::cout << "pxar cauhgt an exception from the board. Exiting." << std::endl;
    return -1;
  }
  
  return 0;
}
