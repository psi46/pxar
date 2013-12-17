#ifndef PXAR_HAL_H
#define PXAR_HAL_H

#include "rpc_impl.h"

namespace pxar {

  class hal
  {

  public:
    /** Default constructor for cerating a new HAL instance. Takes
     *  a testboard USB ID name as parameter and tries to connect to
     *  the board. Exception is thrown if connection fails.
     */
    hal(std::string name = "*");

    /** Default destructor for HAL objects. Testboard USB connection is
     *  closed and RPC object destroyed.
     */
    ~hal();


    /** DEVICE INITIALIZATION **/

    /** Initialize the testboard with the signal delay settings:
     */
    void initTestboard();

    /** Initialize attached TBMs with their settings and configuration
     */
    void initTBM();

    /** Initialize attached ROCs with their settings and configuration
     */
    void initROC();


    /** TESTBOARD GET COMMANDS **/
    /** Read the testboard analog current
     */
    double getTBia();

    /** Read the testboard analog voltage
     */
    double getTBva();

    /** Read the testboard digital current
     */
    double getTBid();

    /** Read the testboard digital voltage
     */
    double getTBvd();

  private:

    /** Private instance of the testboard RPC interface, routes all
     *  hardware access:
     */
    CTestboard * _testboard;

    /** Print the info block with software and firmware versions,
     *  MAC and USB ids etc. read from the connected testboard
     */
    void PrintInfo();

    /** Check for matching pxar / testboard software and firmware versions
     * and scan the RPC commands one by one if ion doubt.
     */
    void CheckCompatibility();

    /** Find attached USB devices that match the DTB naming scheme.
     *
     *  If usbId = "*" check for all attached devices and list them,
     *  if a specific USB ID is given check that device and return.
     */
    bool FindDTB(std::string &usbId);

    /** Delay helper function
     *  Uses usleep() to wait the given time in milliseconds
     */
    void mDelay(uint32_t ms);

    /** Set a DAC on a specific ROC rocId
     *  DAC is referenced by its id, the range is checken in teh function
     *  If the dacValue is out of range the function's return is false.
     */
    bool rocSetDAC(uint8_t rocId, uint8_t dacId, uint8_t dacValue);

    /** Set all DACs on a specific ROC rocId
     *  DACs are provided as vector of std::pair with DAC Id and DAC value.
     */
    bool rocSetDACs(uint8_t rocId, std::vector< std::pair<uint8_t, uint8_t> > dacPairs);


    /** TESTBOARD SET COMMANDS **/
    /** Set the testboard analog current limit
     */
    void setTBia(double IA);

    /** Set the testboard analog voltage
     */
    void setTBva(double VA);

    /** Set the testboard digital current limit
     */
    void setTBid(double ID);

    /** Set the testboard digital voltage
     */
    void setTBvd(double VD);

  };

}
#endif
