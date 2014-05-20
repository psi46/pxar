/**
 * pxar HV power supply interface header
 * to be included by any executable linking to a HV power supply library
 */

#ifndef PXAR_HVSUPPLY_H
#define PXAR_HVSUPPLY_H

namespace pxar {

  /** pxar interface class for HV Power Supply devices
   *
   *  Correct implementation for your device has to be chosen at compile time.
   */
  class DLLEXPORT hvsupply {

  public:

    /** Default constructor for the hvsupply library
     *
     *  Connects to the device, initializes communication
     */
    hvsupply();

    /** Default destructor for the hvsupply library
     *
     *  Will turn off the HV and terminate connection to the HV Power Supply device.
     */
    ~hvsupply();

    /** Turn on the HV output
     */
    bool hvOn();
    
    /** Turn off the HV output
     */
    bool hvOff();
    
    /** Sets the desired voltage
     */
    bool setVoltage(uint32_t volts);
    
    /** Reads back the configured voltage
     */
    uint32_t getVoltage();

    /** Reads back the configured voltage
     */
    uint32_t getVoltage();

    /** Enables Compliance mode and sets the current limit (to be given in uA,
     *  micro Ampere)
     */
    bool setCurrentLimit(uint32_t microampere);

    /** Reads back the set current limit in compliance mode. Value is given
     *  in uA (micro Ampere)
     */
    uint32_t getCurrentLimit();

  }; // class hvsupply

} //namespace pxar

#endif /* PXAR_HVSUPPLY_H */
