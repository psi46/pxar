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
using namespace std;

HVSupply::HVSupply(string portname)
{
  if (!openComPort(portname,57600)) {
    LOG(logCRITICAL) << "Error connecting via RS232 port!";
    throw UsbConnectionError("Error connecting via RS232 port!");
  }

  LOG(logDEBUG) << "Opened COM port to Keithley 2410";

  writeData("*RST");                    // Reset the unit to factory settings
  writeData(":SYST:BEEP:STAT OFF");     // Turn off beeper
  writeData(":ROUT:TERM REAR");         // Switch to rear outlet
  writeData(":SOUR:VOLT:MODE FIX");     // Select fixed voltage mode
  writeData(":FUNC:CONC ON");           // Turn concurrent mode on, i.e. allow readout of voltage and current simultaneously
  writeData(":SENS:AVER:TCON REP");     // Choose repeating filter, i.e. make sure the average is made of the selected nuber of readings
  writeData(":SENS:AVER:COUNT 1");      // Use 1 readings to average
  writeData(":SENS:AVER:STAT ON");      // Enable the averaging
  writeData(":CURR:PROT:LEV 100E-6");   // Set compliance limit to 100 uA
  writeData(":SENS:CURR:RANG 20E-6");   // Select measuring range of 20 uA
  writeData(":SENS:CURR:NPLC 10");      // Set integration period to maximum (=10). 
                                        //     Unit is power line cycles, i.e. 10/60=0.167s in the US and 10/50=0.2s in Europe
  writeData("SOUR:VOLT:IMM:AMPL -100"); // Set a voltage of -100 V immediately (why?)
  writeData(":FORM:ELEM VOLT,CURR");    // Select readout format, e.g. get a number pair with voltage and current

}

HVSupply::~HVSupply()
{
  LOG(logDEBUG) << "Turning Power Supply OFF";
  writeData("OUTPUT 0");
  string answer;
  writeReadBack(":OUTP:STAT?", answer);
  LOG(logDEBUG) <<"State of Keithley after shut down: " <<  answer;

  // Switch back to local mode:
  writeData(":SYST:LOC");
  closeComPort();
}

bool HVSupply::hvOn()
{
  LOG(logDEBUG) << "Turning KEITHLEY 2410 on";
  writeData("OUTPUT 1");
  writeData(":INIT");
  
  string answer;
  writeReadBack(":OUTP:STAT?",answer);
  LOG(logDEBUG) <<"State of Keithley: " <<  answer;
  int a; 
  sscanf(answer.c_str(), "%d", &a);
  return (a == 1) ? true : false;
}

bool HVSupply::hvOff()
{
  LOG(logDEBUG) << "Turning Keithley 2410 off";
  writeData("OUTPUT 0");
  
  string answer;
  writeReadBack(":OUTP:STAT?", answer);
  LOG(logDEBUG) <<"State of Keithley: " <<  answer;
  int a; 
  sscanf(answer.c_str(), "%d", &a);
  return (a == 0) ? true : false;
}

bool HVSupply::setVoltage(double volts)
{
  string data("SOUR:VOLT:IMM:AMPL ");
  data += to_string(volts);
  return writeData(data);
}

bool HVSupply::tripped()
{
  string answer;
  writeReadBack(":SENS:CURR:PROT:TRIP?", answer);
  int a; 
  sscanf(answer.c_str(), "%d", &a); 
  return (a == 1) ? true : false;
}
    
void HVSupply::getVoltageCurrent(float &voltage, float &current)
{
  string answer;
  writeReadBack(":READ?", answer);
  sscanf(answer.c_str(), "%e,%e", &voltage, &current);
}

double HVSupply::getVoltage()
{
  float voltage(0), current(0);

  string answer;
  writeReadBack(":READ?", answer);
  sscanf(answer.c_str(), "%e,%e", &voltage, &current);
  return voltage;
}

double HVSupply::getCurrent()
{
  float current(0), voltage(0);

  string answer;
  writeReadBack(":READ?", answer);
  sscanf(answer.c_str(), "%e,%e", &voltage, &current);
  return current;
}

bool HVSupply::setCurrentLimit(int microampere)
{
  return false;
}

double HVSupply::getCurrentLimit()
{
  return 2000;
}

void HVSupply::sweepStart(double vStart, double vEnd, double vStep, double delay){
  
}

bool HVSupply::sweepRunning(){
  return false;
}

void HVSupply::sweepRead(double &voltSet, double &voltRead, double &current){

}

