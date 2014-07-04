/**
 * pxar HV power supply implementation for Keithley 2410
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
#define COM_PORT_NUMBER 30 //See listing in rs232.cc

// ----------------------------------------------------------------------
hvsupply::hvsupply() {
  const int comPortNumber = COM_PORT_NUMBER;
  if(!openComPort(comPortNumber,57600)) {
    LOG(logCRITICAL) << "Error connecting via RS232 port!";
    throw UsbConnectionError("Error connecting via RS232 port!");
  }
  LOG(logDEBUG) << "Opened COM port to Keithley 2410";


  writeCommandString("*RST\n");                    // Reset the unit to factory settings
  writeCommandString(":SYST:BEEP:STAT OFF\n");     // Turn off beeper
  writeCommandString(":ROUT:TERM REAR\n");         // Switch to rear outlet
  writeCommandString(":SOUR:VOLT:MODE FIX\n");     // Select fixed voltage mode
  writeCommandString(":FUNC:CONC ON\n");           // Turn concurrent mode on, i.e. allow readout of voltage and current simultaneously
  writeCommandString(":SENS:AVER:TCON REP\n");     // Choose repeating filter, i.e. make sure the average is made of the selected nuber of readings
  writeCommandString(":SENS:AVER:COUNT 1\n");      // Use 1 reading to average. Used to be 2 but then voltages are averaged with previous value... need to understand this FIXME
  writeCommandString(":SENS:AVER:STAT ON\n");      // Enable the averaging
  writeCommandString(":CURR:PROT:LEV 100E-6\n");   // Set compliance limit to 100 uA
  writeCommandString(":SENS:CURR:RANG 20E-6\n");   // Select measuring range of 20 uA
  writeCommandString(":SENS:CURR:NPLC 10\n");      // Set integration period to maximum (=10). Unit is power line cycles, i.e. 10/60=0.167s in the US and 10/50=0.2s in Europe
  writeCommandString(":SOUR:VOLT:IMM:AMPL 0\n");   // Set a voltage of 0 V immediately
  writeCommandString(":FORM:ELEM VOLT,CURR\n");    // Select readout format, e.g. get a number pair with voltage and current

}


// Destructor: Will turn off the HV and terminate connection to the HV Power Supply device.
hvsupply::~hvsupply() {
  LOG(logDEBUG) << "Turning Power Supply OFF";
  writeCommandString("OUTPUT 0");
  char answer[256] = { 0 };
  writeCommandStringAndReadAnswer(":OUTP:STAT?",answer);
  LOG(logDEBUG) <<"State of Keithley after shut down: " <<  answer;

  // Switch back to local mode:
  writeCommandString(":SYST:LOC");
  closeComPort();
}

// ----------------------------------------------------------------------
bool hvsupply::hvOn() {
  LOG(logDEBUG) << "Turning KEITHLEY 2410 on";
  char answer[256] = { 0 };  
  writeCommandString("OUTPUT 1");
  writeCommandString(":INIT");
  writeCommandStringAndReadAnswer(":OUTP:STAT?",answer);
  LOG(logDEBUG) <<"State of Keithley: " <<  answer;
  int a; 
  sscanf(answer, "%d", &a); 
  if (0 == a) {
    return false;
  }
  return true;
}

    
// ----------------------------------------------------------------------
bool hvsupply::hvOff() {
  char answer[1000] = {0};  
  LOG(logDEBUG) << "Turning Keithley 2410 off";
  writeCommandString("OUTPUT 0");
  writeCommandStringAndReadAnswer(":OUTP:STAT?",answer);
  LOG(logDEBUG) <<"State of Keithley: " <<  answer;
  int a; 
  sscanf(answer, "%d", &a); 
  if (1 == a) {
    return false;
  }
  return true;
}


    
// ----------------------------------------------------------------------
bool hvsupply::setVoltage(double volts) {
  char string[100];
  if (volts < 0.) volts = -1.*volts;
  sprintf(string, "SOUR:VOLT:IMM:AMPL -%i", static_cast<int>(volts));
  writeCommandString(string);
  sleep(1); 
  return false;
}


// ----------------------------------------------------------------------
bool hvsupply::tripped() {
  char answer[1000] = {0};  
  writeCommandStringAndReadAnswer(":SENS:CURR:PROT:TRIP?", answer);
  int a; 
  sscanf(answer, "%d", &a); 
  if (1 == a) {
    return true;
  }
  return false;
}

// ----------------------------------------------------------------------
bool hvsupply::interrupted() {
  char answer[1000] = {0};
  writeCommandStringAndReadAnswer(":OUTP:STAT?",answer);
  int a;
  sscanf(answer, "%d", &a);
  if (0 == a) {
    return true;
  }
  return false;
}

// ----------------------------------------------------------------------
std::pair<double, double> hvsupply::getReading() {
  float voltage(0), current(0);
  char answer[1000] = {0};

  writeCommandStringAndReadAnswer(":READ?", answer);
  sscanf(answer, "%e,%e", &voltage, &current);
  return std::make_pair(voltage, current);
}

// ----------------------------------------------------------------------
double hvsupply::getVoltage() {
  float voltage(0), current(0);
  char answer[1000] = {0};

  writeCommandStringAndReadAnswer(":READ?", answer);
  sscanf(answer, "%e,%e", &voltage, &current);
  return voltage;
}

// ----------------------------------------------------------------------
double hvsupply::getCurrent() {

  float current(0), voltage(0);
  char answer[1000] = {0};

  writeCommandStringAndReadAnswer(":READ?", answer);
  sscanf(answer, "%e,%e", &voltage, &current);
  return current;

}

// ----------------------------------------------------------------------
bool hvsupply::setCurrentLimit(uint32_t /*microampere*/) {
  return false;
}

// ----------------------------------------------------------------------
double hvsupply::getCurrentLimit() {
  return 2000;
}
