#include <unistd.h>
#include "pxar.h"
#include <iostream>
#include <string>

int main()
{

  // Create new API instance:
  try {
    _api = new pxar::api("*","DEBUGHAL");
  
    // Try some test or so:

    // Create some dummy testboard config, not doing anything right now:
    std::vector<std::pair<std::string,uint8_t> > sig_delays;
    sig_delays.push_back(std::make_pair("clk",2));
    sig_delays.push_back(std::make_pair("ctr",20));
    sig_delays.push_back(std::make_pair("sda",19));
    sig_delays.push_back(std::make_pair("tin",7));

    std::vector<std::pair<std::string,double> > power_settings;
    std::vector<std::pair<std::string,uint8_t> > pg_setup;
    // Initialize the testboard:
    _api->initTestboard(sig_delays, power_settings, pg_setup);

    //_api->flashTB("/tmp/dtb_v0111.flash");
    //_api->flashTB("/tmp/Pixel/dtb_v01122.flash");
    
    // Create some fake DUT/DAC parameters since we can't read configs yet:
    std::vector<std::vector<std::pair<std::string,uint8_t> > > rocDACs;

    std::vector<std::pair<std::string,uint8_t> > dacs;
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
	newpix.trim = row;
	newpix.mask = false;
	newpix.enable = true;

	pixels.push_back(newpix);
      }
    }
    rocPixels.push_back(pixels);

    // Prepare some empty TBM vector:
    std::vector<std::vector<std::pair<std::string,uint8_t> > > tbmDACs;

    // Read DUT info, should result in error message, not initialized:
    _api->_dut->info();

    // Initialize the DUT (power it up and stuff):
    _api->initDUT("",tbmDACs,"psi46digV3",rocDACs,rocPixels);

    // Read DUT info, should print above filled information:
    _api->_dut->info();

    // Read current:
    std::cout << "Analog current: " << _api->getTBia()*1000 << "mA" << std::endl;
    std::cout << "Digital current: " << _api->getTBid()*1000 << "mA" << std::endl;

    sleep(1);

    // Do some debug readout: Pulseheight of px 3,3 with 5 triggers:
    _api->debug_ph(3,3,15,5);

    sleep(1);

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
    /*
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
      else std::cout << (int)mapit->value;

      row++;
      if(row >= 80) { 
	row = 0;
	std::cout << std::endl;
      }
    }
    */
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
