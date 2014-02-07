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
 /* initialize random seed: */
  srand (time(NULL));
}

hal::~hal() {
  _initialized = false;
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

void hal::initTestboard(std::map<uint8_t,uint8_t> /*sig_delays*/, std::vector<std::pair<uint16_t,uint8_t> > /*pg_setup*/, double /*va*/, double /*vd*/, double /*ia*/, double /*id*/) {
  // We are ready for operations now, mark the HAL as initialized:
  _initialized = true;
}

void hal::SetupPatternGenerator(std::vector<std::pair<uint16_t,uint8_t> > /*pg_setup*/) {
}

bool hal::flashTestboard(std::ifstream& /*flashFile*/) {
  LOG(logINFO) << "Starting DTB firmware upgrade...";
  LOG(logINFO) << "Download running... ";
  LOG(logINFO) << "DTB download complete.";
  return true;
}

void hal::initTBM(uint8_t /*tbmId*/, std::map< uint8_t,uint8_t > /*regVector*/) {
}

void hal::initROC(uint8_t /*rocId*/, uint8_t /*roctype*/, std::map< uint8_t,uint8_t > /*dacVector*/) {
}

void hal::PrintInfo() {
  std::string info;
  LOG(logINFO) << "DTB startup information" << std::endl 
	       << "--- DTB info------------------------------------------" << std::endl
	       << info
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

bool hal::FindDTB(std::string &/*usbId*/) {
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
  // Return the VA analog current in A:
  return 0.042;
}

double hal::getTBva(){
  // Return the VA analog voltage in V:
  return 0.042;
}

double hal::getTBid() {
  // Return the VD digital current in A:
  return 0.042;
}

double hal::getTBvd() {
  // Return the VD digital voltage in V:
  return 0.042;
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

bool hal::tbmSetRegs(uint8_t /*tbmId*/, std::map< uint8_t, uint8_t > /*regPairs*/) {
  // Everything went all right:
  return true;
}

bool hal::tbmSetReg(uint8_t /*tbmId*/, uint8_t /*regId*/, uint8_t /*regValue*/) {
  return true;
}

void hal::RocSetMask(uint8_t /*rocid*/, bool /*mask*/, std::vector<pixelConfig> /*pixels*/) {
}

void hal::PixelSetMask(uint8_t /*rocid*/, uint8_t /*column*/, uint8_t /*row*/, bool /*mask*/, uint8_t /*trim*/) {
}

void hal::ColumnSetEnable(uint8_t /*rocid*/, uint8_t /*column*/, bool /*enable*/) {
}

void hal::PixelSetCalibrate(uint8_t /*rocid*/, uint8_t /*column*/, uint8_t /*row*/, int32_t /*flags*/) {
}

void hal::RocClearCalibrate(uint8_t /*rocid*/) {
}


// ---------------- TEST FUNCTIONS ----------------------

std::vector< std::vector<pixel> >* hal::RocCalibrateMap(uint8_t rocid, std::vector<int32_t> /*parameter*/) {

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
  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();
  std::vector<pixel> data;
  for (int i=0;i<42;i++) {
    pixel newpixel;
    newpixel.column = column;
    newpixel.row = row;
    newpixel.roc_id = rocid;
    newpixel.value = 42;
    data.push_back(newpixel);
  }
  result->push_back(data);
  return result;
}

std::vector< std::vector<pixel> >* hal::PixelThresholdMap(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> /*parameter*/) {
  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();
  std::vector<pixel> data;
  for (int i=0;i<42;i++) {
    pixel newpixel;
    newpixel.column = column;
    newpixel.row = row;
    newpixel.roc_id = rocid;
    newpixel.value = 42;
    data.push_back(newpixel);
  }
  result->push_back(data);
  return result;
}

std::vector< std::vector<pixel> >* hal::PixelCalibrateDacScan(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> /*parameter*/) {
  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();
  std::vector<pixel> data;
  for (int i=0;i<42;i++) {
    pixel newpixel;
    newpixel.column = column;
    newpixel.row = row;
    newpixel.roc_id = rocid;
    newpixel.value = 42;
    data.push_back(newpixel);
  }
  result->push_back(data);
  return result;
}

std::vector< std::vector<pixel> >* hal::PixelCalibrateDacDacScan(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> /*parameter*/) {
  std::vector< std::vector<pixel> >* result = new std::vector< std::vector<pixel> >();
  std::vector<pixel> data;
  for (int i=0;i<42;i++) {
    pixel newpixel;
    newpixel.column = column;
    newpixel.row = row;
    newpixel.roc_id = rocid;
    newpixel.value = 42;
    data.push_back(newpixel);
  }
  result->push_back(data);
  return result;
}

// Testboard power switches:

void hal::HVon() {
}

void hal::HVoff() {
}
 
void hal::Pon() {
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

bool hal::daqStart(uint8_t /*deser160phase*/, uint8_t /*nTBMs*/, uint32_t /*buffersize*/) {
  return true;
}

void hal::daqTrigger(uint32_t /*nTrig*/) {
}

void hal::daqTriggerLoop(uint16_t /*period*/) {
}

uint32_t hal::daqBufferStatus() {

  uint32_t buffered_data = 0;
  return buffered_data;
}

bool hal::daqStop(uint8_t /*nTBMs*/) {
  return true;
}

std::vector<uint16_t> hal::daqRead(uint8_t /*nTBMs*/) {
  
  return std::vector<uint16_t>() ;
}

std::vector<uint16_t> * hal::daqReadChannel(uint8_t /*channel*/) {
  return new std::vector<uint16_t>;
}

bool hal::daqReset(uint8_t /*nTBMs*/) {
  return true;
}
