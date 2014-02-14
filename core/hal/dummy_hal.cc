#include "hal.h"
#include "log.h"
#include "timer.h"
#include "helper.h"
#include "constants.h"
#include <fstream>

using namespace pxar;

hal::hal(std::string name) :
  _initialized(false),
  _compatible(false),
  nTBMs(0),
  deser160phase(4)
{
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

void hal::initTestboard(std::map<uint8_t,uint8_t> sig_delays, std::vector<std::pair<uint16_t,uint8_t> > pg_setup, double va, double vd, double ia, double id) {
  // We are ready for operations now, mark the HAL as initialized:
  _initialized = true;
}

void hal::SetupPatternGenerator(std::vector<std::pair<uint16_t,uint8_t> > pg_setup) {
}

bool hal::flashTestboard(std::ifstream& flashFile) {
  LOG(logCRITICAL) << "ERROR UPGRADE: Could not upgrade this DTB version!";
  return false;
}

void hal::initTBM(uint8_t tbmId, std::map< uint8_t,uint8_t > regVector) {
}

void hal::initROC(uint8_t rocId, uint8_t roctype, std::map< uint8_t,uint8_t > dacVector) {
}

void hal::PrintInfo() {
  LOG(logINFO) << "DTB startup information" << std::endl 
	       << "--- DTB info------------------------------------------" << std::endl
	       << " DUMMY DUMMY DUMMY " << std::endl
	       << "------------------------------------------------------";
}

void hal::mDelay(uint32_t ms) {
  // Wait for the given time in milliseconds:
#ifdef WIN32
  Sleep(ms);
#else
  usleep(ms*1000);
#endif
}

bool hal::CheckCompatibility(){
  // We are though all checks, testboard is successfully connected:
  return true;
}

bool hal::FindDTB(std::string &usbId) {
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


void hal::setTBia(double IA) {
}

void hal::setTBva(double VA){
}

void hal::setTBid(double ID) {
}

void hal::setTBvd(double VD) {
}


bool hal::rocSetDACs(uint8_t rocId, std::map< uint8_t, uint8_t > dacPairs) {
  // Everything went all right:
  return true;
}

bool hal::rocSetDAC(uint8_t rocId, uint8_t dacId, uint8_t dacValue) {
  return true;
}

bool hal::tbmSetRegs(uint8_t tbmId, std::map< uint8_t, uint8_t > regPairs) {
  // Everything went all right:
  return true;
}

bool hal::tbmSetReg(uint8_t /*tbmId*/, uint8_t regId, uint8_t regValue) {
  return true;
}

void hal::RocSetMask(uint8_t rocid, bool mask, std::vector<pixelConfig> pixels) {
}

void hal::PixelSetMask(uint8_t rocid, uint8_t column, uint8_t row, bool mask, uint8_t trim) {
}

void hal::ColumnSetEnable(uint8_t rocid, uint8_t column, bool enable) {
}

void hal::PixelSetCalibrate(uint8_t rocid, uint8_t column, uint8_t row, int32_t flags) {
}

void hal::RocClearCalibrate(uint8_t rocid) {
}


// ---------------- TEST FUNCTIONS ----------------------

std::vector<Event*> hal::MultiRocAllPixelsCalibrate(std::vector<uint8_t> rocids, std::vector<int32_t> parameter) {

  // uint32_t flags = static_cast<uint32_t>(parameter.at(0));
  // uint16_t nTriggers = parameter.at(1);

  std::vector<Event*> data;
  return data;
}

std::vector<Event*> hal::MultiRocOnePixelCalibrate(std::vector<uint8_t> rocids, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  // uint32_t flags = static_cast<uint32_t>(parameter.at(0));
  // uint16_t nTriggers = parameter.at(1);

  std::vector<Event*> data;
  return data;
}

std::vector<Event*> hal::SingleRocAllPixelsCalibrate(uint8_t rocid, std::vector<int32_t> parameter) {

  // uint32_t flags = static_cast<uint32_t>(parameter.at(0));
  // uint16_t nTriggers = parameter.at(1);

  std::vector<Event*> data;
  return data;

}

std::vector<Event*> hal::SingleRocOnePixelCalibrate(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  // int32_t flags = parameter.at(0);
  // uint16_t nTriggers = parameter.at(1);

  std::vector<Event*> data;
  return data;

}


std::vector<Event*> hal::MultiRocAllPixelsDacScan(std::vector<uint8_t> rocids, std::vector<int32_t> parameter) {

  // uint8_t dacreg = parameter.at(0);
  // uint8_t dacmin = parameter.at(1);
  // uint8_t dacmax = parameter.at(2);
  // int32_t flags = parameter.at(3);
  // uint16_t nTriggers = parameter.at(4);


  std::vector<Event*> data;
  return data;

}

std::vector<Event*> hal::MultiRocOnePixelDacScan(std::vector<uint8_t> rocids, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  // uint8_t dacreg = parameter.at(0);
  // uint8_t dacmin = parameter.at(1);
  // uint8_t dacmax = parameter.at(2);
  // int32_t flags = parameter.at(3);
  // uint16_t nTriggers = parameter.at(4);

  std::vector<Event*> data;
  return data;

}

std::vector<Event*> hal::SingleRocAllPixelsDacScan(uint8_t rocid, std::vector<int32_t> parameter) {

  // uint8_t dacreg = parameter.at(0);
  // uint8_t dacmin = parameter.at(1);
  // uint8_t dacmax = parameter.at(2);
  // int32_t flags = parameter.at(3);
  // uint16_t nTriggers = parameter.at(4);


  std::vector<Event*> data;
  return data;

}

std::vector<Event*> hal::SingleRocOnePixelDacScan(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  // uint8_t dacreg = parameter.at(0);
  // uint8_t dacmin = parameter.at(1);
  // uint8_t dacmax = parameter.at(2);
  // int32_t flags = parameter.at(3);
  // uint16_t nTriggers = parameter.at(4);


  std::vector<Event*> data;
  return data;

}

std::vector<Event*> hal::MultiRocAllPixelsDacDacScan(std::vector<uint8_t> rocids, std::vector<int32_t> parameter) {

  // uint8_t dac1reg = parameter.at(0);
  // uint8_t dac1min = parameter.at(1);
  // uint8_t dac1max = parameter.at(2);
  // uint8_t dac2reg = parameter.at(3);
  // uint8_t dac2min = parameter.at(4);
  // uint8_t dac2max = parameter.at(5);
  // int32_t flags = parameter.at(6);
  // uint16_t nTriggers = parameter.at(7);


  std::vector<Event*> data;
  return data;

}

std::vector<Event*> hal::MultiRocOnePixelDacDacScan(std::vector<uint8_t> rocids, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  // uint8_t dac1reg = parameter.at(0);
  // uint8_t dac1min = parameter.at(1);
  // uint8_t dac1max = parameter.at(2);
  // uint8_t dac2reg = parameter.at(3);
  // uint8_t dac2min = parameter.at(4);
  // uint8_t dac2max = parameter.at(5);
  // int32_t flags = parameter.at(6);
  // uint16_t nTriggers = parameter.at(7);


  std::vector<Event*> data;
  return data;

}

std::vector<Event*> hal::SingleRocAllPixelsDacDacScan(uint8_t rocid, std::vector<int32_t> parameter) {

  // uint8_t dac1reg = parameter.at(0);
  // uint8_t dac1min = parameter.at(1);
  // uint8_t dac1max = parameter.at(2);
  // uint8_t dac2reg = parameter.at(3);
  // uint8_t dac2min = parameter.at(4);
  // uint8_t dac2max = parameter.at(5);
  // int32_t flags = parameter.at(6);
  // uint16_t nTriggers = parameter.at(7);

  std::vector<Event*> data;
  return data;

}

std::vector<Event*> hal::SingleRocOnePixelDacDacScan(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {

  // uint8_t dac1reg = parameter.at(0);
  // uint8_t dac1min = parameter.at(1);
  // uint8_t dac1max = parameter.at(2);
  // uint8_t dac2reg = parameter.at(3);
  // uint8_t dac2min = parameter.at(4);
  // uint8_t dac2max = parameter.at(5);
  // int32_t flags = parameter.at(6);
  // uint16_t nTriggers = parameter.at(7);

  std::vector<Event*> data;
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

void hal::SignalProbeD1(uint8_t signal) {
}

void hal::SignalProbeD2(uint8_t signal) {
}

void hal::SignalProbeA1(uint8_t signal) {
}

void hal::SignalProbeA2(uint8_t signal) {
}

bool hal::daqStart(uint8_t deser160phase, uint8_t nTBMs, uint32_t buffersize) {
  return true;
}

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

void hal::daqTrigger(uint32_t nTrig) {

}

void hal::daqTriggerLoop(uint16_t period) {
}

uint32_t hal::daqBufferStatus() {
  return 0;
}

bool hal::daqStop() {
  return true;
}

bool hal::daqClear() {
  return true;
}
