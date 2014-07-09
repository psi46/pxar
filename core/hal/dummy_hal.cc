#include "hal.h"
#include "log.h"
#include "timer.h"
#include "helper.h"
#include "constants.h"
#include <fstream>
#include <stdlib.h>

using namespace pxar;

hal::hal(std::string /*name*/) :
  _initialized(false),
  _compatible(false),
  tbmtype(0),
  deser160phase(4)
{
  // Print the useful SW/FW versioning info:
  PrintInfo();

  // Initialize rand():
  srand (time(NULL));

  // Check if all RPC calls are matched:
  if(CheckCompatibility()) {
    // Set compatibility flag
    _compatible = true;
  }
}

hal::~hal() {
}

bool hal::status() {
  return _initialized;
}

uint32_t hal::GetHashForString(const char * s)
{
  // Using some primes
  uint32_t h = 31;
  while (*s) { h = (h * 54059) ^ (s[0] * 76963); s++; }
  return h%86969;
}

uint32_t hal::GetHashForStringVector(const std::vector<std::string> & v)
{
  uint32_t ret = 0;
  for (size_t i=0; i < v.size(); i++) {ret += ((i+1)*(GetHashForString(v[i].c_str())));}
  return ret;
}

void hal::initTestboard(std::map<uint8_t,uint8_t> /*sig_delays*/, std::vector<std::pair<uint16_t,uint8_t> > /*pg_setup*/, uint16_t /*delaysum*/, double /*va*/, double /*vd*/, double /*ia*/, double /*id*/) {
  
  // We are ready for operations now, mark the HAL as initialized:
  _initialized = true;
}

void hal::SetupPatternGenerator(std::vector<std::pair<uint16_t,uint8_t> > /*pg_setup*/, uint16_t /*delaysum*/) {
}

void hal::setTestboardDelays(std::map<unsigned char, unsigned char, std::less<unsigned char>, std::allocator<std::pair<unsigned char const, unsigned char> > >) {
}

void hal::setTestboardPower(double, double, double, double) {
}

bool hal::flashTestboard(std::ifstream& /*flashFile*/) {
  LOG(logCRITICAL) << "ERROR UPGRADE: Could not upgrade this DTB version! It's a dummy after all!";
  return false;
}

void hal::initTBMCore(uint8_t /*tbmId*/, std::map< uint8_t,uint8_t > /*regVector*/) {
}

void hal::initROC(uint8_t /*rocId*/, uint8_t /*roctype*/, std::map< uint8_t,uint8_t > /*dacVector*/) {
}

void hal::PrintInfo() {
  LOG(logINFO) << "DTB startup information" << std::endl 
	       << "--- DTB info------------------------------------------" << std::endl
	       << " DUMMY DUMMY DUMMY " << std::endl
	       << "------------------------------------------------------";
}

bool hal::CheckCompatibility(){
  // We are though all checks, testboard is successfully connected:
  return true;
}

bool hal::FindDTB(std::string &/*usbId*/) {
  return true;
}

double hal::getTBia() {
  // Return the VA analog current in A:
  return (5.23);
}

double hal::getTBva(){
  // Return the VA analog voltage in V:
  return (5.23);
}

double hal::getTBid() {
  // Return the VD digital current in A:
  return (5.23);
}

double hal::getTBvd() {
  // Return the VD digital voltage in V:
  return (5.23);
}


void hal::setTBia(double /*IA*/) {
}

void hal::setTBva(double /*VA*/){
}

void hal::setTBid(double /*ID*/) {
}

void hal::setTBvd(double /*VD*/) {
}


bool hal::rocSetDACs(uint8_t /*rocId*/, std::map< uint8_t, uint8_t > /*dacPairs*/) {
  // Everything went all right:
  return true;
}

bool hal::rocSetDAC(uint8_t /*rocId*/, uint8_t /*dacId*/, uint8_t /*dacValue*/) {
  return true;
}

bool hal::tbmSetRegs(std::map< uint8_t, uint8_t > /*regPairs*/) {
  // Everything went all right:
  return true;
}

bool hal::tbmSetReg(uint8_t /*regId*/, uint8_t /*regValue*/) {
  return true;
}

void hal::RocSetMask(uint8_t /*rocid*/, bool /*mask*/, std::vector<pixelConfig> /*pixels*/) {
}

void hal::AllColumnsSetEnable(uint8_t /*rocid*/, bool /*enable*/) {
}

void hal::PixelSetCalibrate(uint8_t /*rocid*/, uint8_t /*column*/, uint8_t /*row*/, uint16_t /*flags*/) {
}

void hal::RocClearCalibrate(uint8_t /*rocid*/) {
}

void hal::SetupTrimValues(unsigned char, std::vector<pxar::pixelConfig, std::allocator<pxar::pixelConfig> >) {
}

void hal::SetupI2CValues(std::vector<unsigned char, std::allocator<unsigned char> >) {
}

// ---------------- TEST FUNCTIONS ----------------------

std::vector<Event*> hal::MultiRocAllPixelsCalibrate(std::vector<uint8_t> rocids, std::vector<int32_t> parameter) {

  uint32_t flags = static_cast<uint32_t>(parameter.at(0));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(1));

  LOG(logDEBUGHAL) << "Expecting " << nTriggers*ROC_NUMROWS*ROC_NUMCOLS << " events.";
  std::vector<Event*> data;
  size_t total_pixel = 0;

  for(size_t i = 0; i < ROC_NUMCOLS; i++) {
    for(size_t j = 0; j < ROC_NUMROWS; j++) {
      for(size_t k = 0; k < nTriggers; k++) {
	Event* evt = new Event();
	// Generate a slightly random pulse height between 90 and 100:
	uint16_t pulseheight = rand() % 2 + 90;
	for(std::vector<uint8_t>::iterator roc = rocids.begin(); roc != rocids.end(); ++roc) {
	  // Introduce some address encoding issues:
	  if((flags&FLAG_CHECK_ORDER) != 0 && i == 0 && j == 1) { evt->pixels.push_back(pixel(*roc,i,j+1,pulseheight)); } // PX 0,1 answers as PX 0,2
	  else if((flags&FLAG_CHECK_ORDER) != 0 && i == 0 && j == 2) { } // PX 0,2 is dead
	  else { evt->pixels.push_back(pixel(*roc,i,j,pulseheight)); }

	  // If the full chip is unmasked, add some noise hits:
	  if((flags&FLAG_FORCE_UNMASKED) != 0 && (rand()%2) != 0) {
	    evt->pixels.push_back(pixel(*roc,rand()%52,rand()%80,pulseheight));
	  }

	  // Count pixels:
	  total_pixel += evt->pixels.size();
	}
	data.push_back(evt);
      }
    }
  }

  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events (" << total_pixel << " pixels).";
  return data;
}

std::vector<Event*> hal::MultiRocOnePixelCalibrate(std::vector<uint8_t> rocids, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  // uint32_t flags = static_cast<uint32_t>(parameter.at(0));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(1));

  LOG(logDEBUGHAL) << "Expecting " << nTriggers << " events.";
  std::vector<Event*> data;

  for(size_t k = 0; k < nTriggers; k++) {
    Event* evt = new Event();
    for(std::vector<uint8_t>::iterator roc = rocids.begin(); roc != rocids.end(); ++roc) {
      evt->pixels.push_back(pixel(*roc,column,row,90));
    }
    data.push_back(evt);
  }

  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events.";

  return data;
}

std::vector<Event*> hal::SingleRocAllPixelsCalibrate(uint8_t rocid, std::vector<int32_t> parameter) {

  uint32_t flags = static_cast<uint32_t>(parameter.at(0));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(1));

  LOG(logDEBUGHAL) << "Flags: " << listFlags(flags);
  LOG(logDEBUGHAL) << "Expecting " << nTriggers*ROC_NUMROWS*ROC_NUMCOLS << " events.";
  std::vector<Event*> data;
  size_t total_pixel = 0;

  for(size_t i = 0; i < ROC_NUMCOLS; i++) {
    for(size_t j = 0; j < ROC_NUMROWS; j++) {
      for(size_t k = 0; k < nTriggers; k++) {
	Event* evt = new Event();
	// Generate a slightly random pulse height between 90 and 100:
	uint16_t pulseheight = rand() % 2 + 90;
	// Introduce some address encoding issues:
	if((flags&FLAG_CHECK_ORDER) != 0 && i == 0 && j == 1) { evt->pixels.push_back(pixel(rocid,i,j+1,pulseheight));} // PX 0,1 answers as PX 0,2
	else if((flags&FLAG_CHECK_ORDER) != 0 && i == 0 && j == 2) { } // PX 0,2 is dead
	if((flags&FLAG_CHECK_ORDER) != 0 && i == 0 && j == 6) { evt->pixels.push_back(pixel(rocid,i,j+1,pulseheight));} // PX 0,6 answers as PX 0,7
	else { evt->pixels.push_back(pixel(rocid,i,j,pulseheight)); }

	  // If the full chip is unmasked, add some noise hits:
	  if((flags&FLAG_FORCE_UNMASKED) != 0 && (rand()%2) != 0) {
	    evt->pixels.push_back(pixel(rocid,rand()%52,rand()%80,pulseheight));
	  }

	  // Count pixels:
	  total_pixel += evt->pixels.size();
	data.push_back(evt);
      }
    }
  }

  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events (" << total_pixel << " pixels).";

  return data;
}

std::vector<Event*> hal::SingleRocOnePixelCalibrate(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  // int32_t flags = parameter.at(0);
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(1));

  LOG(logDEBUGHAL) << "Expecting " << nTriggers << " events.";
  std::vector<Event*> data;

  for(size_t k = 0; k < nTriggers; k++) {
    Event* evt = new Event();
    evt->pixels.push_back(pixel(rocid,column,row,90));
    data.push_back(evt);
  }

  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events.";

  return data;
}


std::vector<Event*> hal::MultiRocAllPixelsDacScan(std::vector<uint8_t> rocids, std::vector<int32_t> parameter) {

  uint8_t dacmin = static_cast<uint8_t>(parameter.at(1));
  uint8_t dacmax = static_cast<uint8_t>(parameter.at(2));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(4));

  LOG(logDEBUGHAL) << "Expecting " << static_cast<size_t>(dacmax-dacmin+1)*nTriggers*ROC_NUMROWS*ROC_NUMCOLS << " events.";
  std::vector<Event*> data;

  uint8_t dachalf = static_cast<uint8_t>(dacmax-dacmin)/2;

  for(size_t i = 0; i < ROC_NUMCOLS; i++) {
    for(size_t j = 0; j < ROC_NUMROWS; j++) {
      for(size_t dac = 0; dac < static_cast<uint8_t>(dacmax-dacmin+1); dac++) {
	for(size_t k = 0; k < nTriggers; k++) {
	  Event* evt = new Event();
	  for(std::vector<uint8_t>::iterator roc = rocids.begin(); roc != rocids.end(); ++roc) {
	    // Mimic some edge adt 50% of the DAC range:
	    if(dac > dachalf) evt->pixels.push_back(pixel(*roc,i,j,90));
	  }
	  data.push_back(evt);
	}
      }
    }
  }

  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events.";

  return data;
}

std::vector<Event*> hal::MultiRocOnePixelDacScan(std::vector<uint8_t> rocids, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  uint8_t dacmin = static_cast<uint8_t>(parameter.at(1));
  uint8_t dacmax = static_cast<uint8_t>(parameter.at(2));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(4));

  LOG(logDEBUGHAL) << "Expecting " << static_cast<size_t>(dacmax-dacmin+1)*nTriggers << " events.";
  std::vector<Event*> data;

  uint8_t dachalf = static_cast<uint8_t>(dacmax-dacmin)/2;

  for(size_t dac = 0; dac < static_cast<uint8_t>(dacmax-dacmin+1); dac++) {
    for(size_t k = 0; k < nTriggers; k++) {
      Event* evt = new Event();
      for(std::vector<uint8_t>::iterator roc = rocids.begin(); roc != rocids.end(); ++roc) {
	// Mimic some edge adt 50% of the DAC range:
	if(dac > dachalf) evt->pixels.push_back(pixel(*roc,column,row,90));
      }
      data.push_back(evt);
    }
  }

  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events.";

  return data;
}

std::vector<Event*> hal::SingleRocAllPixelsDacScan(uint8_t rocid, std::vector<int32_t> parameter) {

  uint8_t dacmin = static_cast<uint8_t>(parameter.at(1));
  uint8_t dacmax = static_cast<uint8_t>(parameter.at(2));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(4));

  LOG(logDEBUGHAL) << "Expecting " << static_cast<size_t>(dacmax-dacmin+1)*nTriggers*ROC_NUMROWS*ROC_NUMCOLS << " events.";
  std::vector<Event*> data;

  uint8_t dachalf = static_cast<uint8_t>(dacmax-dacmin)/2;

  for(size_t i = 0; i < ROC_NUMCOLS; i++) {
    for(size_t j = 0; j < ROC_NUMROWS; j++) {
      for(size_t dac = 0; dac < static_cast<uint8_t>(dacmax-dacmin+1); dac++) {
	for(size_t k = 0; k < nTriggers; k++) {
	  Event* evt = new Event();
	  // Mimic some edge adt 50% of the DAC range:
	  if(dac > dachalf) evt->pixels.push_back(pixel(rocid,i,j,90));
	  data.push_back(evt);
	}
      }
    }
  }

  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events.";

  return data;
}

std::vector<Event*> hal::SingleRocOnePixelDacScan(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  uint8_t dacmin = static_cast<uint8_t>(parameter.at(1));
  uint8_t dacmax = static_cast<uint8_t>(parameter.at(2));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(4));

  LOG(logDEBUGHAL) << "Expecting " << static_cast<size_t>(dacmax-dacmin+1)*nTriggers << " events.";
  std::vector<Event*> data;

  uint8_t dachalf = static_cast<uint8_t>(dacmax-dacmin)/2;

  for(size_t dac = 0; dac < static_cast<uint8_t>(dacmax-dacmin+1); dac++) {
    for(size_t k = 0; k < nTriggers; k++) {
      Event* evt = new Event();
      // Mimic some edge adt 50% of the DAC range:
      if(dac > dachalf) evt->pixels.push_back(pixel(rocid,column,row,90));
      data.push_back(evt);
    }
  }

  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events.";

  return data;
}

std::vector<Event*> hal::MultiRocAllPixelsDacDacScan(std::vector<uint8_t> rocids, std::vector<int32_t> parameter) {

  uint8_t dac1min = static_cast<uint8_t>(parameter.at(1));
  uint8_t dac1max = static_cast<uint8_t>(parameter.at(2));
  uint8_t dac2min = static_cast<uint8_t>(parameter.at(4));
  uint8_t dac2max = static_cast<uint8_t>(parameter.at(5));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(7));

  LOG(logDEBUGHAL) << "Expecting " << static_cast<size_t>(dac2max-dac2min+1)*static_cast<size_t>(dac1max-dac1min+1)*nTriggers*ROC_NUMROWS*ROC_NUMCOLS << " events.";
  std::vector<Event*> data;

  // Create a working band:
  uint8_t dacquat = static_cast<uint8_t>(dac1max-dac1min)/4;

  for(size_t i = 0; i < ROC_NUMCOLS; i++) {
    for(size_t j = 0; j < ROC_NUMROWS; j++) {
      for(size_t dac1 = 0; dac1 < static_cast<uint8_t>(dac1max-dac1min+1); dac1++) {
	for(size_t dac2 = 0; dac2 < static_cast<uint8_t>(dac2max-dac2min+1); dac2++) {
	  for(size_t k = 0; k < nTriggers; k++) {
	    Event* evt = new Event();
	    for(std::vector<uint8_t>::iterator roc = rocids.begin(); roc != rocids.end(); ++roc) {
	      // Mimic some working band of the two DACs:
	      if(dac2 < static_cast<uint8_t>(dac1*1.2+dacquat) && dac2 > static_cast<uint8_t>(dac1*0.8-dacquat)) {
		evt->pixels.push_back(pixel(*roc,i,j,90));
	      }
	    }
	    data.push_back(evt);
	  }
	}
      }
    }
  }

  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events.";

  return data;
}

std::vector<Event*> hal::MultiRocOnePixelDacDacScan(std::vector<uint8_t> rocids, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  uint8_t dac1min = static_cast<uint8_t>(parameter.at(1));
  uint8_t dac1max = static_cast<uint8_t>(parameter.at(2));
  uint8_t dac2min = static_cast<uint8_t>(parameter.at(4));
  uint8_t dac2max = static_cast<uint8_t>(parameter.at(5));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(7));

  LOG(logDEBUGHAL) << "Expecting " << static_cast<size_t>(dac2max-dac2min+1)*static_cast<size_t>(dac1max-dac1min+1)*nTriggers << " events.";
  std::vector<Event*> data;

  // Create a working band:
  uint8_t dacquat = static_cast<uint8_t>(dac1max-dac1min)/4;

  for(size_t dac1 = 0; dac1 < static_cast<uint8_t>(dac1max-dac1min+1); dac1++) {
    for(size_t dac2 = 0; dac2 < static_cast<uint8_t>(dac2max-dac2min+1); dac2++) {
      for(size_t k = 0; k < nTriggers; k++) {
	Event* evt = new Event();
	for(std::vector<uint8_t>::iterator roc = rocids.begin(); roc != rocids.end(); ++roc) {
	  // Mimic some working band of the two DACs:
	  if(dac2 < static_cast<uint8_t>(dac1*1.2+dacquat) && dac2 > static_cast<uint8_t>(dac1*0.8-dacquat)) {
	    evt->pixels.push_back(pixel(*roc,column,row,90));
	  }
	}
	data.push_back(evt);
      }
    }
  }

  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events.";

  return data;
}

std::vector<Event*> hal::SingleRocAllPixelsDacDacScan(uint8_t rocid, std::vector<int32_t> parameter) {

  uint8_t dac1min = static_cast<uint8_t>(parameter.at(1));
  uint8_t dac1max = static_cast<uint8_t>(parameter.at(2));
  uint8_t dac2min = static_cast<uint8_t>(parameter.at(4));
  uint8_t dac2max = static_cast<uint8_t>(parameter.at(5));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(7));

  LOG(logDEBUGHAL) << "Expecting " << static_cast<size_t>(dac2max-dac2min+1)*static_cast<size_t>(dac1max-dac1min+1)*nTriggers*ROC_NUMROWS*ROC_NUMCOLS << " events.";
  std::vector<Event*> data;

  // Create a working band:
  uint8_t dacquat = static_cast<uint8_t>(dac1max-dac1min)/4;

  for(size_t i = 0; i < ROC_NUMCOLS; i++) {
    for(size_t j = 0; j < ROC_NUMROWS; j++) {
      for(size_t dac1 = 0; dac1 < static_cast<uint8_t>(dac1max-dac1min+1); dac1++) {
	for(size_t dac2 = 0; dac2 < static_cast<uint8_t>(dac2max-dac2min+1); dac2++) {
	  for(size_t k = 0; k < nTriggers; k++) {
	    Event* evt = new Event();
	    // Mimic some working band of the two DACs:
	    if(dac2 < static_cast<uint8_t>(dac1*1.2+dacquat) && dac2 > static_cast<uint8_t>(dac1*0.8-dacquat)) {
	      evt->pixels.push_back(pixel(rocid,i,j,90));
	    }
	    data.push_back(evt);
	  }
	}
      }
    }
  }

  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events.";

  return data;
}

std::vector<Event*> hal::SingleRocOnePixelDacDacScan(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  uint8_t dac1min = static_cast<uint8_t>(parameter.at(1));
  uint8_t dac1max = static_cast<uint8_t>(parameter.at(2));
  uint8_t dac2min = static_cast<uint8_t>(parameter.at(4));
  uint8_t dac2max = static_cast<uint8_t>(parameter.at(5));
  uint16_t nTriggers = static_cast<uint16_t>(parameter.at(7));

  LOG(logDEBUGHAL) << "Expecting " << static_cast<size_t>(dac2max-dac2min+1)*static_cast<size_t>(dac1max-dac1min+1)*nTriggers << " events.";
  std::vector<Event*> data;

  // Create a working band:
  uint8_t dacquat = static_cast<uint8_t>(dac1max-dac1min)/4;

  for(size_t dac1 = 0; dac1 < static_cast<uint8_t>(dac1max-dac1min+1); dac1++) {
    for(size_t dac2 = 0; dac2 < static_cast<uint8_t>(dac2max-dac2min+1); dac2++) {
      for(size_t k = 0; k < nTriggers; k++) {
	Event* evt = new Event();
	// Mimic some working band of the two DACs:
	if(dac2 < static_cast<uint8_t>(dac1*1.2+dacquat) && dac2 > static_cast<uint8_t>(dac1*0.8-dacquat)) {
	  evt->pixels.push_back(pixel(rocid,column,row,90));
	}
	data.push_back(evt);
      }
    }
  }

  LOG(logDEBUGHAL) << "Readout size: " << data.size() << " Events.";

  return data;
}

// Testboard power switches:

void hal::HVon() {
  // Wait a little and let the HV relais do its job:
  mDelay(400);
}

void hal::HVoff() {
}
 
void hal::Pon() {
  // Wait a little and let the power switch do its job:
  mDelay(300);
}

void hal::Poff() {
}



// Testboard probe channel selectors:

void hal::SignalProbeD1(uint8_t /*signal*/) {
}

void hal::SignalProbeD2(uint8_t /*signal*/) {
}

void hal::SignalProbeA1(uint8_t /*signal*/) {
}

void hal::SignalProbeA2(uint8_t /*signal*/) {
}

void hal::SetClockSource(uint8_t /*src*/) {
}

void hal::SetClockStretch(uint8_t /*src*/, uint16_t /*delay*/, uint16_t /*width*/) {
}

void hal::daqStart(uint8_t /*deser160phase*/, uint8_t /*nTBMs*/, uint32_t /*buffersize*/) {}

Event* hal::daqEvent() {

  Event* current_Event = new Event();

  return current_Event;
}

std::vector<Event*> hal::daqAllEvents() {

  std::vector<Event*> evt;
  return evt;
}

rawEvent* hal::daqRawEvent() {

  rawEvent* current_Event = new rawEvent();
  return current_Event;
}

std::vector<rawEvent*> hal::daqAllRawEvents() {

  std::vector<rawEvent*> raw;
  return raw;
}

std::vector<uint16_t> hal::daqBuffer() {
  
  std::vector<uint16_t> raw;
  return raw;
}

void hal::daqTrigger(uint32_t /*nTrig*/, uint16_t /*period*/) {}

void hal::daqTriggerLoop(uint16_t /*period*/) {}

void hal::daqTriggerLoopHalt() {}

uint32_t hal::daqBufferStatus() { return 0; }

void hal::daqStop() {}

void hal::daqClear() {}
