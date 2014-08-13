/**
 * pxar HV power supply implementation for Keithley 2410
 */

#include "hvsupply.h"
#include "exceptions.h"
#include "helper.h"
#include "log.h"
#include <cstdlib>
#include <cmath>
#include <iostream>

using namespace pxar;
using namespace std;

HVSupply::HVSupply(const string &portname)
{

  printf("Make sure the Keithley2410 is using the following RS232 Settings:\n");
  printf("Baud Rate: 57600\n");
  printf("Bits: 8\n");
  printf("Parity: None\n");
  printf("Terminator: <CR>+<LF>\n");
  printf("Control Flow: None\n");

  voltsCurrent = 0;
  hvIsOn = false;
  supplyTripped = false;

  serial.setPortName(portname);
  serial.setBaudRate(57600);
  serial.setRemoveEcho(false);

  bool portIsOpen = serial.openPort();
  if (!portIsOpen) {
    LOG(logCRITICAL) << "Error connecting via RS232 port!";
    throw UsbConnectionError("Error connecting via RS232 port!");
  }
  LOG(logDEBUG) << "Opened COM port to Keithley 2410";

  serial.writeData("*RST");                    // Reset the unit to factory settings
  serial.writeData(":ROUT:TERM REAR");         // Switch to rear outlet
  serial.writeData(":SOUR:FUNC VOLT");         // Set supply to voltage
  serial.writeData(":SENS:FUNC 'CURR:DC'");    // Set measurement to DC current
  serial.writeData(":FUNC:CONC ON");           // Turn concurrent mode on, i.e. allow readout of voltage and current simultaneously
  serial.writeData(":SENS:CURR:NPLC 10");      // Set integration period to maximum (=10). 
                                               //     Unit is power line cycles, i.e. 10/60=0.167s in the US and 10/50=0.2s in Europe
  serial.writeData(":FORM:ELEM VOLT,CURR");    // Select readout format, e.g. get a number pair with voltage and current
  

}

HVSupply::~HVSupply()
{
  LOG(logDEBUG) << "Turning Power Supply OFF";
  serial.writeData("OUTPUT 0");
  string answer;
  serial.writeReadBack(":OUTP:STAT?", answer);
  LOG(logDEBUG) <<"State of Keithley after shut down: " <<  answer;

  // Switch back to local mode:
  serial.writeData(":SYST:LOC");
  serial.closePort();
}

bool HVSupply::hvOn()
{
  hvIsOn = true;
  LOG(logDEBUG) << "Turning KEITHLEY 2410 on";
  serial.writeData(":OUTP:STAT ON");
  
  string answer;
  serial.writeReadBack(":OUTP:STAT?",answer);
  LOG(logDEBUG) <<"State of Keithley: " <<  answer;
  int a; 
  sscanf(answer.c_str(), "%d", &a);
  return (a == 1) ? true : false;
}

bool HVSupply::hvOff()
{
  hvIsOn = false;
  LOG(logDEBUG) << "Turning Keithley 2410 off";
  serial.writeData(":OUTP:STAT OFF");
  
  string answer;
  serial.writeReadBack(":OUTP:STAT?", answer);
  LOG(logDEBUG) <<"State of Keithley: " <<  answer;
  int a; 
  sscanf(answer.c_str(), "%d", &a);
  return (a == 0) ? true : false;
}

bool HVSupply::setVolts(double volts)
{
  voltsCurrent = volts;
  string data("SOUR:VOLT:IMM:AMPL ");
  data += to_string(volts);
  return serial.writeData(data);
}

bool HVSupply::isTripped()
{
  string answer;
  serial.writeReadBack(":SENS:CURR:PROT:TRIP?", answer);
  int a; 
  sscanf(answer.c_str(), "%d", &a); 
  supplyTripped = (a == 1) ? true : false;
  return supplyTripped;
}
    
void HVSupply::getVoltsAmps(double &volts, double &amps)
{
  string answer;
  serial.writeReadBack(":READ?", answer);
  sscanf(answer.c_str(), "%le,%le", &volts, &amps);
}

double HVSupply::getVolts()
{
  double volts(0), amps(0);
  getVoltsAmps(volts, amps);
  return volts;
}

double HVSupply::getAmps()
{
  double volts(0), amps(0);
  getVoltsAmps(volts, amps);
  return amps;
}

bool HVSupply::setMicroampsLimit(double microamps){
  string command = ":CURR:PROT:LEV " + to_string(microamps*1E-6); 
  return serial.writeData(command) > 0;
}

double HVSupply::getMicroampsLimit(){
  string answer;
  serial.writeReadBack(":CURR:PROT:LEV?",answer);
  double limitAmps;
  sscanf(answer.c_str(), "%le", &limitAmps);
  return (limitAmps * 1E6);
}

void HVSupply::sweepStart(double voltStart, double voltStop, double voltStep, double delay){
  this->voltStart = voltStart;
  this->voltStop = voltStop;
  this->voltStep = voltStep;
  this->delay = delay;
  this->sweepReads = ceil(fabs((voltStart - voltStop) / voltStep));
  this->currentSweepRead = 0;
  
  string command;
  
  command = ":SOUR:VOLT:START " + to_string(voltStart);
  serial.writeData(command);                           //Set Start Voltage
  command = ":SOUR:VOLT:STOP " + to_string(voltStop);
  serial.writeData(command);                           //Set Stop Voltage
  command = ":SOUR:VOLT:STEP " + to_string(voltStep);
  serial.writeData(command);                           //Set Voltage Step
  serial.writeData(":SOUR:VOLT:MODE SWE");             //Enable speep mode
  serial.writeData(":SOUR:SWE:RANG AUTO");             //Set range to auto
  serial.writeData(":SOUR:SWE:SPAC LIN");              //Use linear sweep
  command = ":TRIG:COUNT " + to_string(sweepReads);
  serial.writeData(command);                           //Number of measurements
  command = ":SOUR:DEL " + to_string(delay);
  serial.writeData(command);                           //Delay between source and measure
  serial.writeData(":OUTP:STAT ON");
  serial.writeData(":READ?");
  serial.setReadSuffix(",");
}

bool HVSupply::sweepRunning(){
  return currentSweepRead < sweepReads;
}

void HVSupply::sweepRead(double &voltSet, double &voltRead, double &amps){
  if(currentSweepRead >= sweepReads) return;
  string voltStr;
  string ampsStr;
  if (currentSweepRead < sweepReads - 1){
    serial.readData(voltStr);
    serial.readData(ampsStr);
  } else{ //Final Reading
    serial.readData(voltStr);
    serial.setReadSuffix("\r\n");
    serial.readData(ampsStr);
    serial.writeData(":OUTP:STAT OFF");
  }
  
  voltSet = voltStart + voltStep*currentSweepRead;
  sscanf(voltStr.c_str(), "%le", &voltRead);
  sscanf(ampsStr.c_str(), "%le", &amps);
  
  currentSweepRead++;
}

