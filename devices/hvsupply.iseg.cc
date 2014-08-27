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
#include <cstring>
#include <iostream>

using namespace pxar;
using namespace std;

double outToDouble(const string &wordStr) {
  char* word = (char*)calloc(wordStr.size()+1, sizeof(char));
  strcpy(word, wordStr.c_str());
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
  free(word);
  return (sign*value*pow(10,orderSign*order));
}

void handleAnswers(const string &answer, bool &supplyTripped) {
  
  if(answer == "S1=ON") { 
    LOG(logDEBUG) << "Device is ON."; }
  else if(answer == "S1=OFF") {
    LOG(logCRITICAL) << "HV Power Supply set to OFF!";
    throw InvalidConfig("HV Power Supply set to OFF!");
  }
  else if(answer == "S1=TRP") { 
    LOG(logCRITICAL) << "Power supply tripped!";
    supplyTripped = true;
  }
  else if(answer == "S1=MAN") { 
    LOG(logCRITICAL) << "HV Power Supply set to MANUAL mode!";
    throw InvalidConfig("HV Power Supply set to MANUAL mode!");
  }
  else if(answer == "S1=L2H") {
    LOG(logDEBUG) << "Voltage is rising."; }
  else if(answer == "S1=H2L") {
    LOG(logDEBUG) << "Voltage is falling."; }
  else {
    LOG(logDEBUG) << "Unknown device return value.";
    supplyTripped = false;
  }
}

// Constructor: Connects to the device, initializes communication
HVSupply::HVSupply(const string &portName, double timeout) {

  // "S1": Read status
  // "D1": New set-voltage
  // "G1": Apply new set-voltage

  voltsCurrent = 0;
  hvIsOn = false;
  supplyTripped = false;

  serial.setPortName(portName);
  serial.setBaudRate(9600);
  serial.setFlowControl(false);
  serial.setParity(false);       //Uses no parity bit
  serial.setRemoveEcho(true);
  bool portIsOpen = serial.openPort();

  if(!portIsOpen) {
    LOG(logCRITICAL) << "Error connecting via RS232 port!";
    throw UsbConnectionError("Error connecting via RS232 port!");
  }
  LOG(logDEBUG) << "Opened COM port to Iseg device.";

  string answer;
  serial.writeReadBack("S1", answer);
  if(answer == "S1=ON") {
    // Try once more:
    serial.writeReadBack("S1", answer);
  }
  handleAnswers(answer, supplyTripped);
  if(answer == "S1=ON") {
    LOG(logCRITICAL) << "Devide did not return proper status code!";
    throw UsbConnectionError("Devide did not return proper status code!");
  }
  LOG(logDEBUG) << "Communication initialized.";

  hvOff();
  mDelay(2000);
}

// Destructor: Will turn off the HV and terminate connection to the HV Power Supply device.
HVSupply::~HVSupply() {
  hvOff();
  mDelay(2000);
  serial.closePort(); // Close the COM port
}

// Turn on the HV output
bool HVSupply::hvOn() {
  LOG(logDEBUG) << "Turning HV ON (" << voltsCurrent << " V)";

  string command = "D1=" + to_string(voltsCurrent);
  string answer;
  serial.writeReadBack(command, answer);
  serial.writeReadBack("G1", answer);
  handleAnswers(answer, supplyTripped);

  // State machine: HV is on
  hvIsOn = true;
  return false;
}

// Turn off the HV output
bool HVSupply::hvOff() {
  LOG(logDEBUG) << "Turning HV OFF";

  string answer;
  serial.writeReadBack("D1=0", answer);
  serial.writeReadBack("G1", answer);
  handleAnswers(answer, supplyTripped);

  // State machine: HV is off
  hvIsOn = false;
  return true;
}
    
// Sets the desired voltage
bool HVSupply::setVolts(double volts) {
  // Internally store voltage:
  voltsCurrent = volts;
  // Only write to device if state machine indicates HV on:
  if(hvIsOn) {
    LOG(logDEBUG) << "Setting HV to " << voltsCurrent << " V.";
    string command = "D1=" + to_string(voltsCurrent);
    string answer;
    serial.writeReadBack(command, answer);
    serial.writeReadBack("G1", answer);
    handleAnswers(answer, supplyTripped);
  }
  else {
    LOG(logDEBUG) << "Set HV to " << voltsCurrent << " V, not activated.";
  }
  // FIXME not checking for return codes yet!
  return true;
}
    
// Reads back the configured voltage
double HVSupply::getVolts() {
  string answer;
  serial.writeReadBack("U1", answer);
  return outToDouble(answer);
}

// Reads back the current drawn
double HVSupply::getAmps() {
  string answer;
  serial.writeReadBack("I1", answer);
  return outToDouble(answer);
}

void HVSupply::getVoltsAmps(double &volts, double &amps){
  volts = getVolts();
  amps = getAmps();
}


// Enables Compliance mode and sets the current limit (to be given in uA, micro Ampere)
bool HVSupply::setMicroampsLimit(double microamps) {
  if(microamps > 99) {
    LOG(logERROR) << "Current limit " << microamps << " uA too high."
		  << " Device only delivers 50 uA.";
    return false;
  }

  LOG(logDEBUG) << "Setting current limit to " << microamps << " uA.";

  // Factor 100 is required since this value is the sensitive region,
  // not milliamps!
  string command = "LS1=" + to_string(microamps*1000); //Why 1000, not 100?
  string answer;
  serial.writeReadBack(command, answer);
  serial.writeReadBack("G1", answer);
  handleAnswers(answer, supplyTripped);
  return true;
}

// Reads back the set current limit in compliance mode. Value is given in uA (micro Ampere)
double HVSupply::getMicroampsLimit() {
  string answer;
  serial.writeReadBack("LS1", answer);
  // Return value is in Ampere, give in uA:
  return outToDouble(answer)*1000000;
}

// ----------------------------------------------------------------------
bool HVSupply::isTripped() {

  if(supplyTripped) return true;
  return false;
}

void HVSupply::sweepStart(double voltStart, double voltStop, double voltStep, double delay){
  this->voltStart = voltStart;
  this->voltStop = voltStop;
  this->voltStep = voltStep;
  this->delay = delay;
  this->sweepReads = ceil(fabs((voltStart - voltStop) / voltStep));
  this->currentSweepRead = 0;
  hvOn();
}

bool HVSupply::sweepRunning(){
  return currentSweepRead < sweepReads;
}

void HVSupply::sweepRead(double &voltSet, double &voltRead, double &amps){
  if(currentSweepRead >= sweepReads) return;
  voltSet = voltStart + voltStep*currentSweepRead;
  setVolts(voltSet);
  usleep(delay * 1E6);
  getVoltsAmps(voltRead, amps);
  currentSweepRead++;
  if(currentSweepRead == sweepReads) hvOff(); //TODO: May need to ramp down voltage
}

