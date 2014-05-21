/**
 * pxar HV power supply implementation for Keithley 273
 */

#include "hvsupply.h"

using namespace pxar;

// Constructor: Connects to the device, initializes communication
hvsupply::hvsupply() {}

// Destructor: Will turn off the HV and terminate connection to the HV Power Supply device.
hvsupply::~hvsupply() {}

// Turn on the HV output
bool hvsupply::hvOn() {
  return false;
}
    
// Turn off the HV output
bool hvsupply::hvOff() {
  return false;
}
    
// Sets the desired voltage
bool hvsupply::setVoltage(uint32_t /*volts*/) {
  return false;
}
    
// Reads back the configured voltage
uint32_t hvsupply::getVoltage() {
  return 0;
}

// Enables Compliance mode and sets the current limit (to be given in uA, micro Ampere)
bool hvsupply::setCurrentLimit(uint32_t /*microampere*/) {
  return false;
}

// Reads back the set current limit in compliance mode. Value is given in uA (micro Ampere)
uint32_t hvsupply::getCurrentLimit() {
  return 0;
}
