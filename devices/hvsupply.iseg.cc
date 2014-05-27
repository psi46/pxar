/**
 * pxar HV power supply implementation for Iseg Power Supply
 */

#include "hvsupply.h"
#include "rs232.h"
#include "exceptions.h"
#include "helper.h"
#include <cstdlib>
#include <cmath>

using namespace pxar;

double outToDouble(char * word) {
  int sign=1;
  int orderSign=1;
  int order = 0;

  if(word[0] == '-') {
    sign=-1;
    if(word[6] == '-') orderSign = -1;
  }
  else if(word[5] == '-') orderSign = -1;
    
  char *istr;
  istr = strtok (&word[0]," +-");
  double value = atof(istr);
  while (istr != NULL) {
    order = atoi(istr);
    istr = strtok (NULL," +-");
  }

  return (sign*value*pow(10,orderSign*order));
}

// Set Voltage
float voltage_set;
bool hv_status;

// Constructor: Connects to the device, initializes communication
hvsupply::hvsupply() {

   hv_status = false;
  const int comPortNumber = 16; /* /dev/ttyUSB0 */
  if(!openComPort(comPortNumber,9600)) throw UsbConnectionError("Error connecting via RS232 port!");

  // "S1": Read status
  char answer[256] = { 0 };
  writeCommandAndReadAnswer("S1", answer);
  writeCommandAndReadAnswer("S1", answer);
  
  // "D1": New set-voltage
  // "G1": Apply new set-voltage
  hvOff();
  mDelay(2000);
}

// Destructor: Will turn off the HV and terminate connection to the HV Power Supply device.
hvsupply::~hvsupply() {
  hvOff();
  mDelay(2000);
  closeComPort(); // Close the COM port
}

// Turn on the HV output
bool hvsupply::hvOn() {
  char command[256] = {0};
  char answer[256] = {0};
  sprintf(&command[0],"D1=%.f",voltage_set);
  writeCommandAndReadAnswer(command, answer);
  writeCommandAndReadAnswer("G1", answer);
 
  // State machine: HV is on
  hv_status = true;
  return false;
}
    
// Turn off the HV output
bool hvsupply::hvOff() {
  char answer[256] = {0};
  writeCommandAndReadAnswer("D1=0", answer);
  writeCommandAndReadAnswer("G1", answer);
  // FIXME not checking for return codes yet!

  // State machine: HV is off
  hv_status = false;
  return true;
}
    
// Sets the desired voltage
bool hvsupply::setVoltage(double volts) {
  // Internally store voltage:
  voltage_set = static_cast<float>(volts);
  // Only write to device if state machine indicates HV on:
  if(hv_status) {  
    char command[256] = {0};
    char answer[256] = {0};
    sprintf(&command[0],"D1=%.f",voltage_set);
    writeCommandAndReadAnswer(command, answer);
    writeCommandAndReadAnswer("G1", answer);
  }
  // FIXME not checking for return codes yet!
  return true;
}
    
// Reads back the configured voltage
double hvsupply::getVoltage() {
  char answer[256] = {0};
  writeCommandAndReadAnswer("U1", answer);
  return outToDouble(answer);
}

// Reads back the current drawn
double hvsupply::getCurrent() {
  char answer[256] = {0};
  writeCommandAndReadAnswer("I1", answer);
  return outToDouble(answer);
}

// Enables Compliance mode and sets the current limit (to be given in uA, micro Ampere)
bool hvsupply::setCurrentLimit(uint32_t /*microampere*/) {
  return false;
}

// Reads back the set current limit in compliance mode. Value is given in uA (micro Ampere)
double hvsupply::getCurrentLimit() {
  char answer[256] = {0};
  writeCommandAndReadAnswer("N1", answer);
  return outToDouble(answer);
}
