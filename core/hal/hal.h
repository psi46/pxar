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

    /** Configure FIXME
     */
    void Configure();

    /** DEVICE INITIALIZATION **/

    /** Initialize attached TBMs with their settings and configuration
     */
    void initTBM();

    /** Initialize attached ROCs with their settings and configuration
     */
    void initROC();


    /** TESTBOARD GET COMMANDS **/
    /** Read the testboard analog current
     */
    int32_t getTBia();

    /** Read the testboard analog voltage
     */
    int32_t getTBva();

    /** Read the testboard digital current
     */
    int32_t getTBid();

    /** Read the testboard digital voltage
     */
    int32_t getTBvd();

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

    /** Set a DAC on a specific ROC
     *  DAC is referenced by its id, the range is checken in teh function
     *  If the dacValue is out of range the function's return is false.
     */
    bool rocSetDAC(uint8_t dacId, uint8_t dacValue);
  };

}
#endif
