#ifndef PXAR_HAL_H
#define PXAR_HAL_H

#include "rpc_impl.h"
#include "api.h"

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

    /** Function to check the status of the HAL
     */
    bool status();


    // DEVICE INITIALIZATION

    /** Initialize the testboard with the signal delay settings:
     */
    void initTestboard(std::map<uint8_t,uint8_t> sig_delays, std::vector<std::pair<uint16_t, uint8_t> > pg_setup, double va, double vd, double ia, double id);

    /** Flashes the given firmware file to the testboard FPGA
     *  Powers down the DUT first.
     */
    bool flashTestboard(std::ifstream& flashFile);

    /** Initialize attached TBMs with their settings and configuration
     */
    void initTBM(uint8_t tbmId, std::map< uint8_t,uint8_t > regVector);

    /** Initialize attached ROCs with their settings and configuration
     *  This is the startup-routine for single ROCs. It first powers up the
     *  testboard output if necessary, sets the ROC's I2C address and then
     *  programs all DAC registers for the given ROC.
     */
    void initROC(uint8_t rocId, std::map< uint8_t,uint8_t > dacVector);

    /** turn off HV
     */
    void HVoff();

    /** turn on HV
     */
    void HVon();

    /** turn on power
     */
    void Pon();

    /** turn off power
     */
    void Poff();

    /** Set a DAC on a specific ROC rocId
     */
    bool rocSetDAC(uint8_t rocId, uint8_t dacId, uint8_t dacValue);

    /** Set all DACs on a specific ROC rocId
     *  DACs are provided as map of uint8_t,uint8_t pairs  with DAC Id and DAC value.
     */
    bool rocSetDACs(uint8_t rocId, std::map< uint8_t, uint8_t > dacPairs);

    /** Set a register on a specific TBM tbmId
     */
    bool tbmSetReg(uint8_t tbmId, uint8_t regId, uint8_t regValue);

    /** Set all registers on a specific TBM tbmId
     *  registers are provided as map of uint8_t,uint8_t pairs with Reg Id and value.
     */
    bool tbmSetRegs(uint8_t tbmId, std::map< uint8_t, uint8_t > regPairs);


    // TESTBOARD GET COMMANDS
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


    // Testboard probe channel commands:
    /** Selects "signal" as output for the DTB probe channel D1 (digital) 
     */
    void SignalProbeD1(uint8_t signal);

    /** Selects "signal" as output for the DTB probe channel D2 (digital) 
     */
    void SignalProbeD2(uint8_t signal);

    /** Selects "signal" as output for the DTB probe channel A1 (analog) 
     */
    void SignalProbeA1(uint8_t signal);

    /** Selects "signal" as output for the DTB probe channel A2 (analog) 
     */
    void SignalProbeA2(uint8_t signal);


    // TEST COMMANDS
    std::vector< std::vector<pixel> >* DummyPixelTestSkeleton(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter);
    std::vector< std::vector<pixel> >* DummyRocTestSkeleton(uint8_t rocid, std::vector<int32_t> parameter);
    std::vector< std::vector<pixel> >* DummyModuleTestSkeleton(std::vector<int32_t> parameter);

    //FIXME DEBUG
    int32_t PH(int32_t col, int32_t row, int32_t trim, int16_t nTriggers);

    /** Function to return ROC maps of calibration pulses
     *  Public flags contain possibility to route the calibrate pulse via the sensor (FLAG_USE_CALS)
     *  Private flags allow selection of output value (pulse height or efficiency)
     */
    std::vector< std::vector<pixel> >* RocCalibrateMap(uint8_t rocid, std::vector<int32_t> parameter);

    /** Function to return "Pixel maps" of calibration pulses, i.e. pinging a single pixel.
     *  Public flags contain possibility to route the calibrate pulse via the sensor (FLAG_USE_CALS)
     *  Private flags allow selection of output value (pulse height or efficiency)
     */
    std::vector< std::vector<pixel> >* PixelCalibrateMap(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter);

    /** Function to scan a given DAC for a pixel
     *  Public flags contain possibility to route the calibrate pulse via the sensor (FLAG_USE_CALS)
     *  Private flags allow selection of output value (pulse height or efficiency)
     */
    std::vector< std::vector<pixel> >* PixelCalibrateDacScan(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter);

    /** Function to scan two given DAC ranges for a pixel
     *  Public flags contain possibility to route the calibrate pulse via the sensor (FLAG_USE_CALS)
     *  Private flags allow selection of output value (pulse height or efficiency)
     */
    std::vector< std::vector<pixel> >* PixelCalibrateDacDacScan(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter);

    /** Mask all pixels on a specific ROC rocId
     */
    void RocSetMask(uint8_t rocid, bool mask, std::vector<pixelConfig> pixels = std::vector<pixelConfig>());

    /** Mask the specified pixel on ROC rocId
     */
    void PixelSetMask(uint8_t rocid, uint8_t column, uint8_t row, bool mask, uint8_t trim = 15);

  private:

    /** Private instance of the testboard RPC interface, routes all
     *  hardware access:
     */
    CTestboard * _testboard;

    /** Initialization status of the HAL instance, marks the "ready for
     *  operations" status
     */
    bool _initialized;

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

    // TESTBOARD SET COMMANDS
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


    /** Helper template function to compact and pack linear vectors into
     *  pixel struct vectors
     */
    template <typename T> std::vector<pixel> delinearize(uint8_t rocId, std::vector<T> tvec);

  };

}
#endif
