#ifndef WIN32
#include <unistd.h>
#endif

#include "pxar.h"
#include <iomanip>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>

void asciitornado(std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > data, int nTrig, uint8_t column, uint8_t row, uint8_t roc = 0) {

  if(data.empty()) return;

  uint8_t dac1lower = data.front().first;
  uint8_t dac2lower = data.front().second.first;

  uint8_t dac1higher = data.back().first;
  uint8_t dac2higher = data.back().second.first;

  std::cout << std::setw(4) << " ";
  for(int i = 0; i < (dac1higher-dac1lower+3); i++) { std::cout << "_"; }
  std::cout << std::endl;

  for(int dac2 = dac2higher; dac2 >= dac2lower; dac2--) {

    std::cout << std::setw(4);
    if(dac2%10 == 0) std::cout << static_cast<int>(dac2);
    else std::cout << " ";
    std::cout << "|";
    
    for(int dac1 = dac1lower; dac1 <= dac1higher; dac1++) {

      bool found = false;
      for(std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator dacit = data.begin(); dacit != data.end(); ++dacit) {
    
	if((dacit->first == dac1) && (dacit->second.first == dac2) && (!dacit->second.second.empty())) {
	  for (std::vector< pxar::pixel >::iterator pixit = dacit->second.second.begin(); pixit != dacit->second.second.end(); ++pixit) {
	    if(pixit->roc_id == roc && pixit->column == column && pixit->row == row) {
	      found = true;
	      int value = dacit->second.second.at(0).value;
	      if(value == nTrig) std::cout << "X";
	      else if(value > nTrig) std::cout << "#";
	      else std::cout << value;
	    }
	  }
	}
      }
      if(!found) std::cout << " ";
    }
    std::cout <<  "|" << std::endl;
  }

  std::cout << std::setw(5) << "|";
  for(int i = 0; i < (dac1higher-dac1lower+1); i++) { std::cout << "_"; }
  std::cout << "|" << std::endl << "    ";

  // Axis ticks
  for(size_t dac = dac1lower; dac <= dac1higher; dac++) { 
    if(dac%10 == 0) { std::cout << "'"; }
    else std::cout << " ";
  }
  std::cout << std::endl << "    ";

  // Axis labels
  for(size_t dac = dac1lower; dac <= dac1higher; dac++) { 
    if(dac%10 == 0) {
      std::cout << dac;
      if(dac > 10) dac++;
      if(dac > 100) dac++;
    }
    else std::cout << " ";
  }
  std::cout << std::endl << std::endl;
}

void asciihisto(std::vector<std::pair<uint8_t, std::vector<pxar::pixel> > > data, int nTrig, uint8_t column, uint8_t row, uint8_t roc = 0) {

  if(data.empty()) return;

  std::cout << std::endl;
  for(int trig = nTrig; trig >= 0; trig--) {
    if(trig%5 == 0) std::cout << std::setw(3) << trig << " |";
    else std::cout << std::setw(3) << " " << " |";

    for (std::vector<std::pair<uint8_t, std::vector<pxar::pixel> > >::iterator mapit = data.begin(); mapit != data.end(); ++mapit) {

      bool found = false;
      if(!mapit->second.empty()) {
	for(std::vector<pxar::pixel>::iterator it = mapit->second.begin(); it != mapit->second.end(); ++it) {
	  if(it->roc_id == roc && it->column == column && it->row == row) {
	    if(it->value == trig) { std::cout << "o"; found = true; }
	    else if(it->value > trig) { std::cout << "."; found = true; }
	    break;
	  }
	}
      }

      if(!found && trig == 0) { std::cout << "o"; }
      else if(!found) std::cout << " ";
    }
    std::cout << std::endl;
  }
  
  std::cout << "    |";
  for(size_t dac = 0; dac < data.size(); dac++) { std::cout << "_"; }
  std::cout << std::endl << "     ";
  uint8_t daclower = data.front().first;
  uint8_t dachigher= data.back().first;

  // Axis ticks
  for(size_t dac = daclower; dac <= dachigher; dac++) { 
    if(dac%10 == 0) { std::cout << "'"; }
    else std::cout << " ";
  }
  std::cout << std::endl << "    ";

  // Axis labels
  for(size_t dac = daclower; dac <= dachigher; dac++) { 
    if(dac%10 == 0) {
      std::cout << dac;
      if(dac > 10) dac++;
      if(dac > 100) dac++;
    }
    else std::cout << " ";
  }
  std::cout << std::endl << std::endl;
}

void asciimap(std::vector<pxar::pixel> data, int nTrig, uint8_t roc = 0) {

  if(data.empty()) return;

  for(int column = 0; column < 52; column++) {
    for(int row = 0; row < 80; row++) {
      bool found = false;
      for (std::vector< pxar::pixel >::iterator mapit = data.begin(); mapit != data.end(); ++mapit) {
	if(mapit->row == row && mapit->column == column && mapit->roc_id == roc) {
	  if((int)mapit->value == nTrig) std::cout << "X";
	  else if((int)mapit->value == 0) std::cout << "-";
	  else if((int)mapit->value > nTrig) std::cout << "#";
	  else std::cout << (int)mapit->value;
	  found = true;
	  break;
	}
      }
      if(!found) std::cout << " ";
    }
    std::cout << std::endl;
  }
}

int main(int argc, char* argv[]) {

  std::cout << argc << " arguments provided." << std::endl;

  bool module = false;
  uint8_t hubid = 31;

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

  // Power settings:
  power_settings.push_back(std::make_pair("va",1.9));
  power_settings.push_back(std::make_pair("vd",2.6));
  power_settings.push_back(std::make_pair("ia",1.190));
  power_settings.push_back(std::make_pair("id",1.10));

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
  }

  // Get some pixelConfigs up and running:
  std::vector<std::vector<pxar::pixelConfig> > rocPixels;
  std::vector<pxar::pixelConfig> pixels;

  for(int col = 0; col < 52; col++) {
    for(int row = 0; row < 80; row++) {
      pixels.push_back(pxar::pixelConfig(col,row,15));
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

    // Allow CLI parameter flashing:
    if(argc > 2 && strcmp(argv[2],"-f") == 0) {
      if(argc > 3) _api->flashTB(argv[3]);
      delete _api;
      return 0;
    }

    // Initialize the testboard:
    if(!_api->initTestboard(sig_delays, power_settings, pg_setup)) {
      std::cout << "Something is fishy. We probably need to flash the DTB..." << std::endl;
      std::cout << "Would you be so kind and provide the path to the flash file?" << std::endl;
      std::string flashfile;
      std::cin >> flashfile;
      _api->flashTB(flashfile);
      delete _api;
      return 0;
    }
    // Initialize the DUT (power it up and stuff):
    if (!_api->initDUT(hubid,"tbm08",tbmDACs,roctype,rocDACs,rocPixels)){
      std::cout << " initDUT failed -> invalid configuration?! " << std::endl;
      return -2;
    }
    // Read DUT info, should print above filled information:
    _api->_dut->info();

    // Read current:
    std::cout << "Analog current: " << _api->getTBia()*1000 << "mA" << std::endl;
    std::cout << "Digital current: " << _api->getTBid()*1000 << "mA" << std::endl;

    _api->HVon();

    // Testing new DUT functions:
    std::vector<uint8_t> enabledrocs = _api->_dut->getEnabledRocIDs();
    std::cout << "The following " << enabledrocs.size() << " ROCs are enabled:" << std::endl;
    for(std::vector<uint8_t>::iterator it = enabledrocs.begin(); it != enabledrocs.end(); ++it) {
      std::cout << "ROC " << (int)(*it) << ", ";
    }
    std::cout << std::endl;
    std::vector<uint8_t> roci2c = _api->_dut->getEnabledRocI2Caddr();
    std::cout << roci2c.size() << " ROCs are enabled, their I2C addresses aree:" << std::endl;
    for(std::vector<uint8_t>::iterator it = roci2c.begin(); it != roci2c.end(); ++it) {
      std::cout << "I2C " << (int)(*it) << ", ";
    }
    std::cout << std::endl;

    // Fetch all the DACs with names from the DUT:
    std::vector<std::pair<std::string,uint8_t> > dutdacs = _api->_dut->getDACs(0);
    for(std::vector<std::pair<std::string,uint8_t> >::iterator it = dutdacs.begin(); it != dutdacs.end(); ++it) {
      std::cout << "Name: " << it->first << ", value: " << (int)(it->second) << std::endl;
    }

    // ##########################################################
    // Call the first real test (pixel efficiency map):

    // Enable all pixels first:
    _api->_dut->testAllPixels(true);
    _api->_dut->maskAllPixels(false);

    //_api->_dut->maskAllPixels(true);
    _api->_dut->maskPixel(0,0,false);
    _api->_dut->maskPixel(51,79,false);

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

    // Call the test:
    int nTrig = 10;
    std::vector< pxar::pixel > mapdata = _api->getEfficiencyMap(0,nTrig);
    std::cout << "Data size returned: " << mapdata.size() << std::endl;

    
    std::cout << "ASCII Sensor Efficiency Map:" << std::endl;
    
    enabledrocs = _api->_dut->getEnabledRocIDs();
    for(std::vector<uint8_t>::iterator it = enabledrocs.begin(); it != enabledrocs.end(); ++it) {
      asciimap(mapdata,nTrig,(*it));
      std::cout << std::endl << std::endl;
    }

    
    
    // ##########################################################


    // ##########################################################
    // Call the second real test (a DAC scan for some pixels):

    // disable pixel(s)
    _api->_dut->testAllPixels(false);
    //    _api->_dut->testPixel(34,12,true);
    //    _api->_dut->testPixel(33,12,true);
    _api->_dut->testPixel(34,11,true);
    _api->_dut->testPixel(14,12,true);
    _api->_dut->maskPixel(14,12,false);

    _api->_dut->info();
    

    // Call the test:
    unsigned nTrig2 = 20;
    std::vector< std::pair<uint8_t, std::vector<pxar::pixel> > > 
      effscandata = _api->getEfficiencyVsDAC("caldel", 80, 170, 0, nTrig2);
    
    // Check out the data we received:
    std::cout << "Number of stored (DAC, pixels) pairs in data: " << effscandata.size() << std::endl;
    
    enabledrocs = _api->_dut->getEnabledRocIDs();
    for(std::vector<uint8_t>::iterator it = enabledrocs.begin(); it != enabledrocs.end(); ++it) {
      std::cout << "CalDel scan for ROC " << (int)(*it) << std::endl;
      asciihisto(effscandata,nTrig2,14,12,0);
      std::cout << std::endl;
    }

    // ##########################################################
    
    // ##########################################################
    // Call the third real test (a DACDAC scan for some pixels):

    // disable pixel(s)
    _api->_dut->testAllPixels(false);
    _api->_dut->maskAllPixels(true);
    _api->_dut->testPixel(33,12,true);
    _api->_dut->maskPixel(33,12,false);

    // Call the test:
    int nTrig3 = 10;
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > effscan2ddata = _api->getEfficiencyVsDACDAC("caldel", 60, 180, "vthrcomp", 0, 140, 0, nTrig3);
    
    // Check out the data we received:
    std::cout << "Number of stored (DAC, pixels) pairs in data: " << effscan2ddata.size() << std::endl;

    for(std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > >::iterator it = effscan2ddata.begin(); it != effscan2ddata.end(); ++it) {
      //if(!it->second.second.empty()) 
      //	std::cout << "DAC1 " << (int)it->first << " DAC2 " << (int)it->second.first << " " << it->second.second.size() << " pixels." << std::endl;
    }

    asciitornado(effscan2ddata,nTrig3,33,12,0);

    // ##########################################################


    // ##########################################################
    // Let's spy a bit on the DTB scope ports:

    _api->SignalProbe("D1","sda");
    _api->SignalProbe("A2","sda");

    // ##########################################################


    // ##########################################################
    // Try a threshold map:

    _api->_dut->testAllPixels(true);
    _api->_dut->maskAllPixels(false);

    //_api->_dut->testPixel(5,5,true);
    //_api->_dut->maskPixel(5,5,false);

    // Call the test:
    int nTrig5 = 10;
    std::vector< pxar::pixel > thrmap = _api->getThresholdMap("vcal",0,20,FLAG_RISING_EDGE,nTrig5);
    std::cout << "Data size returned: " << thrmap.size() << std::endl;

    // Threshold map:
    enabledrocs = _api->_dut->getEnabledRocIDs();
    for(std::vector<uint8_t>::iterator it = enabledrocs.begin(); it != enabledrocs.end(); ++it) {
      asciimap(thrmap,255,(*it));
      std::cout << std::endl << std::endl;
    }
    // ##########################################################


    // ##########################################################
    // Try threshold for a single pixel:
    /*
    _api->_dut->testAllPixels(false);
    _api->_dut->testPixel(5,5,true);
    _api->_dut->maskPixel(5,5,false);

    // Call the test:
    int nTrig6 = 10;
    std::vector< pxar::pixel > thrmap1 = _api->getThresholdMap("vthrcomp",0x0,nTrig6);
    std::cout << "Data size returned: " << thrmap1.size() << std::endl;

    
    for (std::vector< pxar::pixel >::iterator mapit = thrmap1.begin(); mapit != thrmap1.end(); ++mapit) {
      std::cout << "Threshold for pixel " 
		<< (int)mapit->column << "," << (int)mapit->row << " is " 
		<< (int)mapit->value << std::endl;
    }
    */
    // ##########################################################


    // ##########################################################
    // Do some Raw data acquisition:

    // All on!
    _api->_dut->testAllPixels(false);
    _api->_dut->maskAllPixels(false);

    for(int i = 0; i < 3; i++) {
      _api->_dut->testPixel(i,5,true);
      _api->_dut->testPixel(i,6,true);
      //_api->_dut->testPixel(i,7,true);
      //_api->_dut->testPixel(i,8,true);
      //_api->_dut->testPixel(i,9,true);
      //_api->_dut->testPixel(i,10,true);
      //_api->_dut->testPixel(i,11,true);
      //_api->_dut->testPixel(i,12,true);
    }

    _api->daqStart(pg_setup);
    uint32_t daq_triggers = 5;

    _api->daqTrigger(daq_triggers);

    //_api->daqTriggerLoop(10);
    //sleep(2);
    _api->daqStop();
    std::vector<pxar::Event> daqdat = _api->daqGetEventBuffer();

    std::cout << "Event number read from board: " << daqdat.size() << std::endl;
    std::cout << "DAQ data blob:" << std::endl;
    for(std::vector<pxar::Event>::iterator it = daqdat.begin(); it != daqdat.end(); ++it) {
      std::cout << (*it) << std::endl;
    }

    // ##########################################################

    _api->HVoff();

    // What is the range of caldel?
    std::cout << (int)_api->getDACRange("caldeL") << std::endl;

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
