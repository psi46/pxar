/**
 * pxar HV power supply implementation for Iseg Power Supply
 */

#include "hvsupply.h"
#include "rs232.h"
#include "exceptions.h"
#include "helper.h"
#include "log.h"
#include <cstdlib>
#include <cmath>
#include <iostream>

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

void handleAnswers(char* answer) {
  
  if(strcmp(answer,"S1=ON") == 0) { 
    LOG(logDEBUG) << "Device is ON."; }
  else if(strcmp(answer,"S1=OFF") == 0) { 
    LOG(logDEBUG) << "Device is OFF."; }
  else if(strcmp(answer,"S1=TRP") == 0) { 
    LOG(logCRITICAL) << "Power supply tripped!"; }
  else if(strcmp(answer,"S1=L2H") == 0) {
    LOG(logDEBUG) << "Voltage is rising."; }
  else if(strcmp(answer,"S1=H2L") == 0) {
    LOG(logDEBUG) << "Voltage is falling."; }
  else {
    LOG(logDEBUG) << "Unknown device reyurn value."; }
}

// Set Voltage
float voltage_set;
bool hv_status;

// Constructor: Connects to the device, initializes communication
hvsupply::hvsupply() {

  // "S1": Read status
  // "D1": New set-voltage
  // "G1": Apply new set-voltage

  hv_status = false;
  const int comPortNumber = 16; /* /dev/ttyUSB0 */
  if(!openComPort(comPortNumber,9600)) {
    LOG(logCRITICAL) << "Error connecting via RS232 port!";
    throw UsbConnectionError("Error connecting via RS232 port!");
  }
  LOG(logDEBUG) << "Opened COM port to Iseg device.";

  char answer[256] = { 0 };
  writeCommandAndReadAnswer("S1", answer);
  if(strcmp(answer,"S1=ON") != 0) {
    // Try once more:
    writeCommandAndReadAnswer("S1", answer);
  }
  handleAnswers(answer);
  if(strcmp(answer,"S1=ON") != 0) {
    LOG(logCRITICAL) << "Devide did not return proper status code!";
    throw UsbConnectionError("Devide did not return proper status code!");
  }
  LOG(logDEBUG) << "Communication initialized.";

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
  LOG(logDEBUG) << "Turning HV ON (" << voltage_set << " V)";

  char command[256] = {0};
  char answer[256] = {0};
  sprintf(&command[0],"D1=%.f",voltage_set);
  writeCommandAndReadAnswer(command, answer);
  writeCommandAndReadAnswer("G1", answer);
  handleAnswers(answer);

  // State machine: HV is on
  hv_status = true;
  return false;
}
    
// Turn off the HV output
bool hvsupply::hvOff() {
  LOG(logDEBUG) << "Turning HV OFF";

  char answer[256] = {0};
  writeCommandAndReadAnswer("D1=0", answer);
  writeCommandAndReadAnswer("G1", answer);
  handleAnswers(answer);

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
    LOG(logDEBUG) << "Setting HV to " << voltage_set << " V.";
    char command[256] = {0};
    char answer[256] = {0};
    sprintf(&command[0],"D1=%.f",voltage_set);
    writeCommandAndReadAnswer(command, answer);
    writeCommandAndReadAnswer("G1", answer);
    handleAnswers(answer);
  }
  else {
    LOG(logDEBUG) << "Set HV to " << voltage_set << " V, not activated.";
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
bool hvsupply::setCurrentLimit(uint32_t microampere) {
  if(microampere > 99) {
    LOG(logERROR) << "Current limit " << microampere << " uA too high."
		  << " Device only delivers 50 uA.";
    return false;
  }

  LOG(logDEBUG) << "Setting current limit to " << microampere << " uA.";

  char command[256] = {0};
  char answer[256] = {0};
  // Factor 100 is required since this value is the sensitive region,
  // not milliamps!
  sprintf(&command[0],"LS1=%i",microampere*1000);
  writeCommandAndReadAnswer(command, answer);
  writeCommandAndReadAnswer("G1", answer);
  handleAnswers(answer);
  return true;
}

// Reads back the set current limit in compliance mode. Value is given in uA (micro Ampere)
double hvsupply::getCurrentLimit() {
  char answer[256] = {0};
  writeCommandAndReadAnswer("LS1", answer);
  // Return value is in Ampere, give in uA:
  return outToDouble(answer)*1000000;
}
