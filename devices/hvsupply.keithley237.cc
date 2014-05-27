/**
 * pxar HV power supply implementation for Keithley 237
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
bool hvsupply::setVoltage(double /*volts*/) {
  return false;
}
    
// Reads back the configured voltage
double hvsupply::getVoltage() {
  return 0;
}

// Reads back the current drawn
double hvsupply::getCurrent() {
  return 0;
}

// Enables Compliance mode and sets the current limit (to be given in uA, micro Ampere)
bool hvsupply::setCurrentLimit(uint32_t /*microampere*/) {
  return false;
}

// Reads back the set current limit in compliance mode. Value is given in uA (micro Ampere)
double hvsupply::getCurrentLimit() {
  return 0;
}
