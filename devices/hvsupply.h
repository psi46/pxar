/**
 * pxar HV power supply interface header
 * to be included by any executable linking to a HV power supply library
 */

#ifndef PXAR_HVSUPPLY_H
#define PXAR_HVSUPPLY_H

/** Declare all classes that need to be included in shared libraries on Windows 
 *  as class DLLEXPORT className
 */
#include "pxardllexport.h"
#include "rs232.h"

#include <string>
#include <sstream>
#include <vector>

/** Cannot use stdint.h when running rootcint on WIN32 */
#if ((defined WIN32) && (defined __CINT__))
typedef int int32_t;
typedef unsigned int uint32_t;
typedef unsigned short int uint16_t;
typedef unsigned char uint8_t;
#else
#include <stdint.h>
#endif

namespace pxar {

  /** pxar interface class for HV Power Supply devices
   *
   *  Correct implementation for your device has to be chosen at compile time.
   */
  class DLLEXPORT HVSupply {
    RS232Conn serial;
    
  /** Private variables related to keeping track of IV sweeps*/
    int sweepReads;
    int currentSweepRead;
    bool sweepIsRunning;
    double voltStart;
    double voltStop;
    double voltStep;
    double delay;

  /** State variables */
    double voltsCurrent;
    bool hvIsOn;
    bool supplyTripped;
  
  public:

    /** Default constructor for the hvsupply library
     *
     *  Connects to the device, initializes communication
     */
    HVSupply(const std::string &portname, double timeout);

    /** Default destructor for the hvsupply library
     *
     *  Will turn off the HV and terminate connection to the HV Power Supply device.
     */
    ~HVSupply();

    /** Turn on the HV output
     */
    bool hvOn();
    
    /** Turn off the HV output
     */
    bool hvOff();
    
    /** Sets the desired voltage
     */
    bool setVolts(double volts);
    
    /** Reads back the configured voltage. Value is given in v (Volts)
     */
    double getVolts();

    /** Reads back the current drawn. Value is given in A (Amperes)
     */
    double getAmps();

    /** Reads back the voltage and the current drawn. Value is given in A (Amperes)
     */
    void getVoltsAmps(double &volts, double &amps);

    /** Enables compliance mode and sets the current limit (to be given in uA,
     *  micro Ampere)
     */
    bool setMicroampsLimit(double microamps);

    /** Reads back the set current limit in compliance mode. Value is given
     *  in uA (micro Ampere)
     */
    double getMicroampsLimit();

    /** Did the HV supply trip?
     */
    bool isTripped();
    
    /** Initiate an IV sweep
     */
    void sweepStart(double voltStart, double voltStop, double voltStep, double delay);
    /** Check if IV Sweep has more readings
     */
    bool sweepRunning();
    /** Consume a reading from the IV curve in progress. Returns true if sweep was aborted
     */
    bool sweepRead(double &voltSet, double &voltRead, double &amps);
  
  }; // class hvsupply


  /** to_string
   *  Converts stringstream compatable objects to their string representation
   */
  template <typename T>
  std::string to_string(T i){
    std::ostringstream oss;
    oss << i;
    return oss.str();
  }

} //namespace pxar

#endif /* PXAR_HVSUPPLY_H */
