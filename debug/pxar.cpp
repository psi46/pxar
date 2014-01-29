#ifndef WIN32
#include <unistd.h>
#endif

#include "pxar.h"
#include <iomanip>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>

void DecodeTbmHeader(unsigned int raw)
{
  int evNr = raw >> 8;
  int stkCnt = raw & 6;
  printf("  EV(%3i) STF(%c) PKR(%c) STKCNT(%2i)",
	 evNr,
	 (raw&0x0080)?'1':'0',
	 (raw&0x0040)?'1':'0',
	 stkCnt
	 );
}

void DecodeTbmTrailer(unsigned int raw)
{
  int dataId = (raw >> 6) & 0x3;
  int data   = raw & 0x3f;
  printf("  NTP(%c) RST(%c) RSR(%c) SYE(%c) SYT(%c) CTC(%c) CAL(%c) SF(%c) D%i(%2i)",
	 (raw&0x8000)?'1':'0',
	 (raw&0x4000)?'1':'0',
	 (raw&0x2000)?'1':'0',
	 (raw&0x1000)?'1':'0',
	 (raw&0x0800)?'1':'0',
	 (raw&0x0400)?'1':'0',
	 (raw&0x0200)?'1':'0',
	 (raw&0x0100)?'1':'0',
	 dataId,
	 data
	 );
}

void DecodePixel(unsigned int raw)
{
  unsigned int ph = (raw & 0x0f) + ((raw >> 1) & 0xf0);
  raw >>= 9;
  int c =    (raw >> 12) & 7;
  c = c*6 + ((raw >>  9) & 7);
  int r =    (raw >>  6) & 7;
  r = r*6 + ((raw >>  3) & 7);
  r = r*6 + ( raw        & 7);
  int y = 80 - r/2;
  int x = 2*c + (r&1);
  printf("   Pixel [%05o] %2i/%2i: %3u", raw, x, y, ph);
}

void Decode(std::vector<uint16_t> data) {

  unsigned int hdr, trl;
  unsigned int raw;
  for (int i=0; i<data.size(); i++)
    {
      int d = data[i] & 0xf;
      int q = (data[i]>>4) & 0xf;
      switch (q)
	{
	case  0: printf("  0(%1X)", d); break;

	case  1: printf("\n R1(%1X)", d); raw = d; break;
	case  2: printf(" R2(%1X)", d);   raw = (raw<<4) + d; break;
	case  3: printf(" R3(%1X)", d);   raw = (raw<<4) + d; break;
	case  4: printf(" R4(%1X)", d);   raw = (raw<<4) + d; break;
	case  5: printf(" R5(%1X)", d);   raw = (raw<<4) + d; break;
	case  6: printf(" R6(%1X)", d);   raw = (raw<<4) + d;
	  DecodePixel(raw);
	  break;

	case  7: printf("\nROC-HEADER(%1X): ", d); break;

	case  8: printf("\n\nTBM H1(%1X) ", d); hdr = d; break;
	case  9: printf("H2(%1X) ", d);       hdr = (hdr<<4) + d; break;
	case 10: printf("H3(%1X) ", d);       hdr = (hdr<<4) + d; break;
	case 11: printf("H4(%1X) ", d);       hdr = (hdr<<4) + d;
	  DecodeTbmHeader(hdr);
	  break;

	case 12: printf("\nTBM T1(%1X) ", d); trl = d; break;
	case 13: printf("T2(%1X) ", d);       trl = (trl<<4) + d; break;
	case 14: printf("T3(%1X) ", d);       trl = (trl<<4) + d; break;
	case 15: printf("T4(%1X) ", d);       trl = (trl<<4) + d;
	  DecodeTbmTrailer(trl);
	  break;
	default : break;
	}
    }
  printf("\n");

}

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
    if (!_api->initDUT("tbm08",tbmDACs,roctype,rocDACs,rocPixels)){
      std::cout << " initDUT failed -> invalid configuration?! " << std::endl;
      return -2;
    }
    // Read DUT info, should print above filled information:
    _api->_dut->info();

    // Read current:
    std::cout << "Analog current: " << _api->getTBia()*1000 << "mA" << std::endl;
    std::cout << "Digital current: " << _api->getTBid()*1000 << "mA" << std::endl;

    _api->HVon();

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
    
    // Call the test:
    int nTrig = 10;
    std::vector< pxar::pixel > mapdata = _api->getEfficiencyMap(0,nTrig);
    std::cout << "Data size returned: " << mapdata.size() << std::endl;

    
    std::cout << "ASCII Sensor Efficiency Map:" << std::endl;
    unsigned int row = 0;
    int oldroc = 0;
    for (std::vector< pxar::pixel >::iterator mapit = mapdata.begin(); mapit != mapdata.end(); ++mapit) {
      
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
      effscandata = _api->getEfficiencyVsDAC("caldel", 70, 150, 0, nTrig2);
    
    // Check out the data we received:
    std::cout << "Number of stored (DAC, pixels) pairs in data: " << effscandata.size() << std::endl;
    
    // Loop over dac values:
    for(std::vector< std::pair<uint8_t, std::vector<pxar::pixel> > >::iterator dacit = effscandata.begin();
	dacit != effscandata.end(); ++dacit) {
      std::cout << "  dac value: " << (int)dacit->first << " has " << dacit->second.size() << " fired pixels " << std::endl;
      // Loop over fired pixels and show value
      for (std::vector<pxar::pixel>::iterator pixit = dacit->second.begin(); pixit != dacit->second.end();++pixit){
	std::cout << "    ROC " << (int)pixit->roc_id << "  pixel " << (int)  pixit->column << ", " << (int)  pixit->row << " has value "<< (int)  pixit->value << std::endl;
      }
    }
    
    // ##########################################################
    
    // ##########################################################
    // Call the third real test (a DACDAC scan for some pixels):
    
    // disable pixel(s)
    _api->_dut->testAllPixels(false);
    _api->_dut->testPixel(33,12,true);

    // Call the test:
    nTrig = 10;
    int lower = 40;
    int limit = 120;
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > effscan2ddata = _api->getEfficiencyVsDACDAC("caldel", lower, limit, "vthrcomp", lower, limit, 0, nTrig);
    //std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pxar::pixel> > > > effscan2ddata = _api->getEfficiencyVsDACDAC("vbias_sf", 0, limit, "vcomp", 0, limit, 0, nTrig);
    
    // Check out the data we received:
    std::cout << "Number of stored (DAC, pixels) pairs in data: " << effscan2ddata.size() << std::endl;
    
    int dac = lower;
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
	dac = lower;
	std::cout << std::endl;
      }
    }
    
    // ##########################################################


    // ##########################################################
    // Let's spy a bit on the DTB scope ports:

    _api->SignalProbe("D1","sda");
    _api->SignalProbe("A2","sda");

    // ##########################################################


    // ##########################################################
    // Try a threshold map:
    /*
    _api->_dut->testAllPixels(true);

    // Call the test:
    int nTrig5 = 10;
    std::vector< pxar::pixel > thrmap = _api->getThresholdMap("vcal",0,nTrig5);
    std::cout << "Data size returned: " << thrmap.size() << std::endl;

    
    std::cout << "ASCII Sensor Threshold Map:" << std::endl;
    unsigned int row2 = 0;
    for (std::vector< pxar::pixel >::iterator mapit = thrmap.begin(); mapit != thrmap.end(); ++mapit) {
      
      if((int)mapit->value == nTrig5) std::cout << "X";
      else if((int)mapit->value == 0) std::cout << "-";
      else if((int)mapit->value > nTrig5) std::cout << "#";
      else std::cout << (int)mapit->value;
      row2++;
      if(row2 >= 80) { 
	row2 = 0;
	std::cout << std::endl;
      }
    }
    */
    // ##########################################################


    // ##########################################################
    // Try threshold for a single pixel:
    
    _api->_dut->testAllPixels(false);
    _api->_dut->testPixel(5,5,true);

    // Call the test:
    int nTrig6 = 10;
    std::vector< pxar::pixel > thrmap1 = _api->getThresholdMap("vthrcomp",0x0,nTrig6);
    std::cout << "Data size returned: " << thrmap1.size() << std::endl;

    
    for (std::vector< pxar::pixel >::iterator mapit = thrmap1.begin(); mapit != thrmap1.end(); ++mapit) {
      std::cout << "Threshold for pixel " 
		<< (int)mapit->column << "," << (int)mapit->row << " is " 
		<< (int)mapit->value << std::endl;
    }

    // ##########################################################


    // ##########################################################
    // Do some Raw data acquisition:
    
    // All on!
    _api->_dut->testAllPixels(false);
    _api->_dut->maskAllPixels(false);

    for(int i = 0; i < 3; i++) {
      _api->_dut->testPixel(i,5,true);
      //_api->_dut->testPixel(i,6,true);
      //_api->_dut->testPixel(i,7,true);
      //_api->_dut->testPixel(i,8,true);
      //_api->_dut->testPixel(i,9,true);
      //_api->_dut->testPixel(i,10,true);
      //_api->_dut->testPixel(i,11,true);
      //_api->_dut->testPixel(i,12,true);
    }

    _api->daqStart(pg_setup);
    _api->daqTrigger(5);
    _api->daqStop();
    std::vector<uint16_t> daqdat = _api->daqGetBuffer();

    // Run the helper function:
    if(module) Decode(daqdat);

    std::cout << "Raw DAQ data blob:" << std::endl;
    for(std::vector<uint16_t>::iterator it = daqdat.begin();
	it != daqdat.end();
	++it) {
      if(((*it) & 0xF000) > 0x4000) std::cout << std::endl;
      if((*it) == 0x0080) std::cout << std::endl;
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
