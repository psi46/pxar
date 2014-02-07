#include "hal.h"
#include "log.h"
#include "timer.h"
#include "constants.h"
#include <fstream>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

using namespace pxar;

hal::hal(std::string /*name*/) :
  _initialized(false),
  _compatible(true),   // we are always compatible!
  _fallback_mode(false)
{
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
 /* initialize random seed: */
  srand (time(NULL));
}

hal::~hal() {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  _initialized = false;
}

bool hal::status() {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  return _initialized;
}

uint32_t hal::GetHashForString(const char * s)
{
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  // Using some primes
  uint32_t h = 31;
  while (*s) { h = (h * 54059) ^ (s[0] * 76963); s++; }
  return h%86969;
}

uint32_t hal::GetHashForStringVector(const std::vector<std::string> & v)
{
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  uint32_t ret = 0;
  for (size_t i=0; i < v.size(); i++) {ret += ((i+1)*(GetHashForString(v[i].c_str())));}
  return ret;
}

void hal::initTestboard(std::map<uint8_t,uint8_t> /*sig_delays*/, std::vector<std::pair<uint16_t,uint8_t> > /*pg_setup*/, double /*va*/, double /*vd*/, double /*ia*/, double /*id*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  // We are ready for operations now, mark the HAL as initialized:
  _initialized = true;
}

void hal::SetupPatternGenerator(std::vector<std::pair<uint16_t,uint8_t> > /*pg_setup*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}

bool hal::flashTestboard(std::ifstream& /*flashFile*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  LOG(logINFO) << "Starting DTB firmware upgrade...";
  LOG(logINFO) << "Download running... ";
  LOG(logINFO) << "DTB download complete.";
  return true;
}

void hal::initTBM(uint8_t /*tbmId*/, std::map< uint8_t,uint8_t > /*regVector*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}

void hal::initROC(uint8_t /*rocId*/, uint8_t /*roctype*/, std::map< uint8_t,uint8_t > /*dacVector*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}

void hal::PrintInfo() {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  std::string info;
  LOG(logINFO) << "DTB startup information" << std::endl 
	       << "--- DTB info------------------------------------------" << std::endl
	       << info
	       << "------------------------------------------------------";
}

void hal::mDelay(uint32_t ms) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  // Wait for the given time in milliseconds:
#ifdef WIN32
  Sleep(ms);
#else
  usleep(ms*1000);
#endif
}

bool hal::CheckCompatibility(){
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  // We are though all checks, testboard is successfully connected:
  return true;
}

bool hal::FindDTB(std::string &/*usbId*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  LOG(logINFO) 
    << "       _      _               _   ____ _____ ____   " << endl
    << "__   _(_)_ __| |_ _   _  __ _| | |  _ \\_   _| __ ) 	" << endl
    << "\\ \\ / / | '__| __| | | |/ _` | | | | | || | |  _ \\ 	" << endl
    << " \\ V /| | |  | |_| |_| | (_| | | | |_| || | | |_) |	" << endl
    << "  \\_/ |_|_|   \\__|\\__,_|\\__,_|_| |____/ |_| |____/ 	" << endl
    << "                                                   	" << endl
    << "                                 _           _ 	" << endl
    << "  ___ ___  _ __  _ __   ___  ___| |_ ___  __| |	" << endl
    << " / __/ _ \\| '_ \\| '_ \\ / _ \\/ __| __/ _ \\/ _` |	" << endl
    << "| (_| (_) | | | | | | |  __/ (__| ||  __/ (_| |	" << endl
    << " \\___\\___/|_| |_|_| |_|\\___|\\___|\\__\\___|\\__,_|     " << endl
    << "                                               " << endl;

  return true;
}

double hal::getTBia() {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  // Return the VA analog current in A:
  return 0.042;
}

double hal::getTBva(){
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  // Return the VA analog voltage in V:
  return 0.042;
}

double hal::getTBid() {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  // Return the VD digital current in A:
  return 0.042;
}

double hal::getTBvd() {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  // Return the VD digital voltage in V:
  return 0.042;
}


void hal::setTBia(double /*IA*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}

void hal::setTBva(double /*VA*/){
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}

void hal::setTBid(double /*ID*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}

void hal::setTBvd(double /*VD*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}


bool hal::rocSetDACs(uint8_t /*rocId*/, std::map< uint8_t, uint8_t > /*dacPairs*/) {  
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  // Everything went all right:
  return true;
}

bool hal::rocSetDAC(uint8_t /*rocId*/, uint8_t /*dacId*/, uint8_t /*dacValue*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  return true;
}

bool hal::tbmSetRegs(uint8_t /*tbmId*/, std::map< uint8_t, uint8_t > /*regPairs*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  // Everything went all right:
  return true;
}

bool hal::tbmSetReg(uint8_t /*tbmId*/, uint8_t /*regId*/, uint8_t /*regValue*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  return true;
}

void hal::RocSetMask(uint8_t /*rocid*/, bool /*mask*/, std::vector<pixelConfig> /*pixels*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}

void hal::PixelSetMask(uint8_t /*rocid*/, uint8_t /*column*/, uint8_t /*row*/, bool /*mask*/, uint8_t /*trim*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}

void hal::ColumnSetEnable(uint8_t /*rocid*/, uint8_t /*column*/, bool /*enable*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}

void hal::PixelSetCalibrate(uint8_t /*rocid*/, uint8_t /*column*/, uint8_t /*row*/, int32_t /*flags*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}

void hal::RocClearCalibrate(uint8_t /*rocid*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}


// ---------------- TEST FUNCTIONS ----------------------

std::vector< std::vector<pixel> >* hal::RocCalibrateMap(uint8_t rocid, std::vector<int32_t> /*parameter*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();
  std::vector<pixel> data;
  for (int i=0;i<42;i++) {
    pixel newpixel;
    newpixel.column = rand()%51;
    newpixel.row = rand()%71;
    newpixel.roc_id = rocid;
    newpixel.value = 42;
    data.push_back(newpixel);
  }
  result->push_back(data);
  return result;
}

std::vector< std::vector<pixel> >* hal::RocThresholdMap(uint8_t rocid, std::vector<int32_t> /*parameter*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();
  std::vector<pixel> data;
  for (int i=0;i<42;i++) {
    pixel newpixel;
    newpixel.column = rand()%51;
    newpixel.row = rand()%71;
    newpixel.roc_id = rocid;
    newpixel.value = 42;
    data.push_back(newpixel);
  }
  result->push_back(data);
  return result;
}

std::vector< std::vector<pixel> >* hal::PixelCalibrateMap(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> /*parameter*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();
  std::vector<pixel> data;
  pixel newpixel;
  newpixel.column = column;
  newpixel.row = row;
  newpixel.roc_id = rocid;
  newpixel.value = 42;
  data.push_back(newpixel);
  result->push_back(data);
  return result;
}

std::vector< std::vector<pixel> >* hal::PixelThresholdMap(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> /*parameter*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();
  std::vector<pixel> data;
  pixel newpixel;
  newpixel.column = column;
  newpixel.row = row;
  newpixel.roc_id = rocid;
  newpixel.value = 42;
  data.push_back(newpixel);
  result->push_back(data);
  return result;
}

std::vector< std::vector<pixel> >* hal::PixelCalibrateDacScan(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  int32_t dacmin = parameter.at(1);
  int32_t dacmax = parameter.at(2);

  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();
  for (int i=dacmin;i<dacmax;i++){
    std::vector<pixel> data;
    pixel newpixel;
    newpixel.column = column;
    newpixel.row = row;
    newpixel.roc_id = rocid;
    newpixel.value = 42;
    data.push_back(newpixel);
    result->push_back(data);
  }
  return result;
}

std::vector< std::vector<pixel> >* hal::PixelCalibrateDacDacScan(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  int32_t dac1min = parameter.at(1);
  int32_t dac1max = parameter.at(2);
  int32_t dac2min = parameter.at(4);
  int32_t dac2max = parameter.at(5);

  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();

  for (int i=dac1min;i<dac1max;i++){
    for (int k=dac1min;k<dac1max;k++){
      std::vector<pixel> data;
      pixel newpixel;
      newpixel.column = column;
      newpixel.row = row;
      newpixel.roc_id = rocid;
      newpixel.value = 42;
      data.push_back(newpixel);
      result->push_back(data);
    }
  }
  return result;
}

// Testboard power switches:

void hal::HVon() {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}

void hal::HVoff() {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}
 
void hal::Pon() {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}

void hal::Poff() {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}



// Testboard probe channel selectors:

void hal::SignalProbeD1(uint8_t /*signal*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}

void hal::SignalProbeD2(uint8_t /*signal*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}

void hal::SignalProbeA1(uint8_t /*signal*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}

void hal::SignalProbeA2(uint8_t /*signal*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}

bool hal::daqStart(uint8_t /*deser160phase*/, uint8_t /*nTBMs*/, uint32_t /*buffersize*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  return true;
}

void hal::daqTrigger(uint32_t /*nTrig*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}

void hal::daqTriggerLoop(uint16_t /*period*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
}

uint32_t hal::daqBufferStatus() {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  uint32_t buffered_data = 0;
  return buffered_data;
}

bool hal::daqStop(uint8_t /*nTBMs*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  return true;
}

std::vector<uint16_t> hal::daqRead(uint8_t /*nTBMs*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  return std::vector<uint16_t>() ;
}

std::vector<uint16_t> * hal::daqReadChannel(uint8_t /*channel*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  return new std::vector<uint16_t>;
}

bool hal::daqReset(uint8_t /*nTBMs*/) {
  LOG(logDEBUGHAL) << "Dummy HAL method called.";
  return true;
}
