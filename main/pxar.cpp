#include <unistd.h>
#include "pxar.h"
#include <iostream>

int main()
{

  // Create new API instance:
  try {
    _api = new pxar::api("*","DEBUG");
  
    // Try some test or so:

    // Create some fake DAC parameters since we can't read configs yet:

    std::vector<std::pair<uint8_t,uint8_t> > dacs;
    dacs.push_back(std::make_pair(1,6)); //Vdig
    dacs.push_back(std::make_pair(2,84)); //Vana
    dacs.push_back(std::make_pair(3,30)); //Vsf
    dacs.push_back(std::make_pair(4,12)); //Vcomp
    dacs.push_back(std::make_pair(7,60)); //VwllPr
    dacs.push_back(std::make_pair(9,60)); //VwllSh
    dacs.push_back(std::make_pair(10,230)); //VhldDel
    dacs.push_back(std::make_pair(11,29)); //Vtrim
    dacs.push_back(std::make_pair(12,86)); //VthrComp
    dacs.push_back(std::make_pair(13,1)); //VIBias_Bus
    dacs.push_back(std::make_pair(14,6)); //Vbias_sf
    dacs.push_back(std::make_pair(15,40)); //VoffsetOp
    dacs.push_back(std::make_pair(17,129)); //VOffsetR0
    dacs.push_back(std::make_pair(18,120)); //VIon
    dacs.push_back(std::make_pair(19,100)); //Vcomp_ADC
    dacs.push_back(std::make_pair(20,91)); //VIref_ADC
    dacs.push_back(std::make_pair(21,150)); //VIbias_roc
    dacs.push_back(std::make_pair(22,50)); //VIColOr
    dacs.push_back(std::make_pair(25,200)); //Vcal
    dacs.push_back(std::make_pair(26,122)); //CalDel
    dacs.push_back(std::make_pair(253,0)); //CtrlReg
    dacs.push_back(std::make_pair(254,100)); //WBC
    std::cout << (int)dacs.size() << " DACs loaded.\n";

    // Initialize the testboard:
    _api->initTestboard();

    // Initialize the DUT (power it up and stuff):
    _api->initDUT(dacs);

    // Read current:
    std::cout << "Analog current: " << _api->getTBia()*1000 << "mA" << std::endl;
    std::cout << "Digital current: " << _api->getTBid()*1000 << "mA" << std::endl;

    // Do some debug readout: Pulseheight of px 3,3 with 5 triggers:
    _api->debug_ph(3,3,15,5);

    // Check if we can access the DUT data structure and enable/disable stuff:
    //_api->_dut->getDAC(0,"Vana");
    
    // Print DACs from ROC 0:
    _api->_dut->printDACs(0);

    // disable pixel(s)
    _api->_dut->setAllPixelEnable(false);
    _api->_dut->setPixelEnable(34,12,true);
    _api->_dut->setPixelEnable(33,12,true);
    _api->_dut->setPixelEnable(34,11,true);
    _api->_dut->setPixelEnable(14,12,true);

    // debug some DUT implementation details
    std::cout << " have " << _api->_dut->getNEnabledPixels() << " pixels set to enabled" << std::endl;
    std::cout << " have " << (int) _api->_dut->getPixelConfig(0,8,8).trim << " as trim value on pixel 8,8" << std::endl;

    // call a 'demo' (i.e. fake) DAC scan routine
    std::vector< std::pair<uint8_t, std::vector<pxar::pixel> > > data = _api->getDebugVsDAC("test", 20, 28, 50, 16);

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
