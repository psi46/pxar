/**
 * pxar API class header
 * to be included by any executable linking to libpxar
 */

#ifndef PXAR_API_H
#define PXAR_API_H

#include <string>
#include <vector>
#include <map>
#include <stdint.h>

#include "config.h"

namespace pxar {

  /** Class for storing pixel readout data
   */
  class pixel {
  public:
  pixel() : roc_id(0), column(0), row(0), value(0) {};
    /** Function to fill the pixel with linear encoded data from RPC transfer
     */
    inline void fill(int32_t address, int32_t data) {
      // Fill the data
      value = data;
      
      // Split the address and distribute it over ROC, column and row:
      // pixel column: max(51 -> 110011), requires 6bits
      // pixel row: max(79 -> 1001111), requires 7bits
      // roc id: max(15 -> 1111), requires 4bits

      // 32 bits:
      // -------- ----IIII --CCCCCC -RRRRRRR
      roc_id = (address>>16)&15;
      column = (address>>8)&63;
      row = (address)&127;
    };
    uint8_t roc_id;
    uint8_t column;
    uint8_t row;
    int32_t value;
  };

  /** Class to store the configuration for single pixels (i.e. their mask state, trim bit settings
   *  and whether they belong to the currently run test ("enable"). By default, pixels are masked.
   */
  class pixelConfig {
  public:
  pixelConfig() : 
    column(0), row(0), 
      trim(15), mask(true), enable(false) {};
    uint8_t column;
    uint8_t row;
    uint8_t trim;
    bool mask;
    bool enable;
  };

  /** Class for ROC states
   *  Contains a DAC vector for their settings, a type flag and an enable switch
   *  and a vector for pixelConfig
   */
  class rocConfig {
  public:
  rocConfig() : pixels(), dacs(), type(0), enable(true) {};
    std::vector< pixelConfig > pixels;
    std::map< uint8_t,uint8_t > dacs;
    uint8_t type;
    bool enable;
  };

  /** Class for TBM states
   *  Contains a DAC vector for their settings, a type flag and an enable switch
   */
  class tbmConfig {
  public:
  tbmConfig() : dacs(), type(0), enable(true) {};
    std::map< uint8_t,uint8_t > dacs;
    uint8_t type;
    bool enable;
  };

  /** Forward declaration, implementation follows below...
   */
  class dut;

  /** Forward declaration, not including the header file!
   */
  class hal;
  /** Define typedefs to allow easy passing of member function
   *   addresses from the HAL class, used e.g. in loop expansion routines.
   *   Follows advice of http://www.parashift.com/c++-faq/typedef-for-ptr-to-memfn.html
   */
  typedef  std::vector< std::vector<pixel> >* (hal::*HalMemFnPixel)(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter);
  typedef  std::vector< std::vector<pixel> >* (hal::*HalMemFnRoc)(uint8_t rocid, std::vector<int32_t> parameter);
  typedef  std::vector< std::vector<pixel> >* (hal::*HalMemFnModule)(std::vector<int32_t> parameter);



  /** pxar API class definition
   *  this is the central API through which all calls from tests and user space
   *  functions have to be routed in order to interact with the hardware.
   */
  class api {

  public:

    /** Default constructor for the libpxar API
     *  Fetches a new HAL instance and opens the testboard connection
     */
    api(std::string usbId = "*", std::string logLevel = "WARNING");

    /** Default destructor for libpxar API
     */
    ~api();

    /** Returns the version string for the pxar API
     */
    std::string getVersion() {return PACKAGE_STRING;};

    /** Initializer method for the testboard
     *  Initializes the tesboard with signal delay settings, and voltage/current
     *  limit settings (power_settings) and the initial pattern generator setup
     *  (pg_setup), all provided via vectors of pairs with descriptive name.
     *  Name lookup via central dictionaries.
     */
    bool initTestboard(std::vector<std::pair<std::string,uint8_t> > sig_delays,
                       std::vector<std::pair<std::string,double> > power_settings,
                       std::vector<std::pair<uint16_t, uint8_t> > pg_setup);
  
    /** Initializer method for the DUT (attached devices)
     *  This function requires the types and DAC settings for all TBMs and ROCs
     *  contained in the setup. All values will be checked for validity (DAC
     *  ranges, position and number of pixels, etc.)
     *
     *  All parameters are supplied via vectors, the size of the vector represents
     *  the number of devices.
     */
    bool initDUT(std::string tbmtype, 
		 std::vector<std::vector<std::pair<std::string,uint8_t> > > tbmDACs,
		 std::string roctype,
		 std::vector<std::vector<std::pair<std::string,uint8_t> > > rocDACs,
		 std::vector<std::vector<pixelConfig> > rocPixels);

    /** Programming method for the DUT (attached devices)
     *  This function requires the DUT structure to be filled (initialized).
     *
     *  All parameters are taken from the DUT struct, the enabled devices are programmed.
     *  This function needs to be called after power cycling the testboard output (using Poff,
     *  Pon).
     *
     *  A DUT flag is set which prevents test functions to be executed if not programmed.
     */
    bool programDUT(); 
  

    /** DTB functions **/

    /** Function to flash a new firmware onto the DTB via
     *  the regular USB connection.
     */
    bool flashTB(std::string filename);

    /** Function to read out analog DUT supply current on the testboard
     */
    double getTBia();

    /** Function to read out analog DUT supply voltage on the testboard
     */
    double getTBva();

    /** Function to read out digital DUT supply current on the testboard
     */
    double getTBid();

    /** Function to read out digital DUT supply voltage on the testboard
     */
    double getTBvd();

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

     /** Selects "signal" as output for the DTB probe channel "probe"
      *  (digital or analog)
      *
      *  The signal identifier is checked against a dictionary to be valid.
      *  In case of an invalid signal identifier the output is turned off.
      */
    bool SignalProbe(std::string probe, std::string name);


    /** Function to read values from the integrated digital scope on the DTB
     */
    //getScopeData();

    /** Function to select the channel of the integrated digital scope on the DTB
     */
    //setScopeChannel();



    /** TEST functions **/

    /** Set a DAC value on the DUT (ROCs)
     *  The "rocid" parameter can be used to select a specific ROC to program. If rocid is set to
     *  a value < 0 all enabled ROCs will be programmed with the given DAC value.
     *
     *  This function will both update the bookkeeping value in the DUT
     *  struct and program the actual device 
     */
    bool setDAC(std::string dacName, uint8_t dacValue, int8_t rocid = -1);

    /** Set a register value on the DUT (TBMs)
     *  The "tbmid" parameter can be used to select a specific TBM to program. If tbmid is set to
     *  a value < 0 all enabled TBMs will be programmed with the given register value.
     *
     *  This function will both update the bookkeeping value in the DUT
     *  struct and program the actual device 
     */
    bool setTbmReg(std::string regName, uint8_t regValue, int8_t tbmid = -1);

    /** Method to scan a DAC and measure the pulse height
     *
     *  Returns a std vector of pairs containing set dac value and a pixel vector, with the value of the pixel struct being
     *  the averaged pulse height over nTriggers triggers
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > > getPulseheightVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
									       uint16_t flags = 0, uint32_t nTriggers=16);

    /** DEBUG ROUTINE DELME FIXME WHATEVER
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > > getDebugVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
									 uint16_t flags = 0, uint32_t nTriggers=16);

    /** Method to scan a DAC and measure the efficiency
     *
     *  Returns a std vector of pairs containing set dac value and pixels, with the value of the pixel struct being
     *  the number of hits in that pixel. Efficiency == 1 for nhits == nTriggers
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > > getEfficiencyVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
					  uint16_t flags = 0, uint32_t nTriggers=16);

    /** Method to scan a DAC and measure the pixel threshold
     *
     *  Returns a std vector of pairs containing set dac value and pixels, with the value of the pixel struct being
     *  the threshold value of that pixel
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > > getThresholdVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
					 uint16_t flags = 0, uint32_t nTriggers=16);

    /** Method to scan a 2D DAC-Range (DAC1 vs. DAC2)  and measure the pulse height
     *
     *  Returns a std vector containing pairs of DAC1 values and pais of DAC2 values with a pixel vector
     *  with the value of the pixel struct being the averaged pulse height over nTriggers triggers
     */
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > getPulseheightVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
					      std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
					      uint16_t flags = 0, uint32_t nTriggers=16);

    /** Method to scan a 2D DAC-Range (DAC1 vs. DAC2)  and measure the efficiency
     *
     *  Returns a std vector containing pairs of DAC1 values and pais of DAC2 values with a pixel vector
     *  with the value of the pixel struct being the number of hits in that pixel.
     *   Efficiency == 1 for nhits == nTriggers
     */
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > getEfficiencyVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
					     std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
					     uint16_t flags = 0, uint32_t nTriggers=16);

    /** Method to scan a 2D DAC-Range (DAC1 vs. DAC2)  and measure the threshold
     *
     *  Returns a std vector containing pairs of DAC1 values and pais of DAC2 values with a pixel vector
     *  with the value of the pixel struct being the averaged pixel threshold.
     */
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > getThresholdVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
					    std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
					    uint16_t flags = 0, uint32_t nTriggers=16);

    /** Method to get a chip map of the pulse height
     *
     *  Returns a std vector of pixels, with the value of the pixel struct being
     *  the averaged pulse height over nTriggers triggers
     */
    std::vector<pixel> getPulseheightMap(uint16_t flags = 0, uint32_t nTriggers=16);

    /** Method to get a chip map of the efficiency
     *
     *  Returns a std vector of pixels, with the value of the pixel struct being
     *  the number of hits in that pixel. Efficiency == 1 for nhits == nTriggers
     */
    std::vector<pixel> getEfficiencyMap(uint16_t flags = 0, uint32_t nTriggers=16);

    /** Method to get a chip map of the pixel threshold
     *
     *  Returns a std vector of pixels, with the value of the pixel struct being
     *  the threshold value of that pixel
     */
    std::vector<pixel> getThresholdMap(uint16_t flags = 0, uint32_t nTriggers=16);

    int32_t getReadbackValue(std::string parameterName);

    /** DEBUG METHOD -- FIXME/DELME
     */
    int32_t debug_ph(int32_t col, int32_t row, int32_t trim, int16_t nTriggers);

    /** DAQ functions **/

    /** Function to set up a new data acquisition
     *  This function takes a new Pattern Generator setup as argument, if left empty the one which is currently
     *  programmed via initTestboard is used. The given pattern generator only lives for the time of the 
     *  data acquisition and is replaced by the previous one after stopping the DAQ.
     */
    bool daqStart(std::vector<std::pair<uint16_t, uint8_t> > pg_setup);
    
    /** Function to read out the earliest event in buffer from the currently
     *  data acquisition. If no event is buffered, the function will wait for
     *  the next event to arrive and then return it.
     */
    std::vector<pixel> daqGetEvent();

    /** Function to fire the previously defined pattern commands nTrig times, the
     *  function parameter defaults to 1.
     */
    void daqTrigger(uint32_t nTrig = 1);

    /** Function to stop the running data acquisition
     *  This triggers also a reprogramming of the old (test-) Pattern Generator setup, so no additional
     *  steps are needed before one can do regular tests again. The patterns are taken from the DUT struct
     *  in which they are stored from initTestboard.
     */
    bool daqStop();

    /** Function to return the full event buffer from the testboard RAM after the
     *  data acquisition has been stopped. No decoding performed, raw data.
     */
    std::vector<uint16_t> daqGetBuffer();
    


    /** DUT object for book keeping of settings
     */
    dut * _dut;

    /** Status function for the API, returns true if everything is setup correctly
     *  for operation
     */
    bool status();
    
  private:

    /** Private HAL object for the API to access hardware routines
     */
    hal * _hal;

    /** Routine to loop over all active ROCs/pixels and call the
     *  appropriate pixel, ROC or module HAL methods for execution
     */
    std::vector< std::vector<pixel> >* expandLoop(HalMemFnPixel pixelfn, HalMemFnRoc rocfn, HalMemFnModule modulefn, std::vector<int32_t> param, bool forceSerial = false);

    /** repacks Dac scan data into pairs of Dac values with fired pixel vectors
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > >* repackDacScanData (std::vector< std::vector<pixel> >* data, uint8_t dacMin, uint8_t dacMax);

    /** repacks DacDac scan data into pairs of Dac values with fired pixel vectors
     */
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > >* repackDacDacScanData (std::vector< std::vector<pixel> >* data, uint8_t dac1min, uint8_t dac1max, uint8_t dac2min, uint8_t dac2max);

    /** compacts data over ROC loops (ROC0<data>,ROC1<data>, ...) into (data(roc0,roc1)) where the data blocks can be subdivided into e.g. DAC ranges
     */
    std::vector< std::vector<pixel> >* compactRocLoopData (std::vector< std::vector<pixel> >* data, uint8_t nRocs);

    /** Helper function for conversion from string to register value
     *  Type tells it whether it is a DTB, TBM or ROC register to look for
     * Range check for DAC and testboard register values. This function 
     *  return the value itself if valid
     *  or upper/lower range boundary in case of over/underflow:
     */
    bool verifyRegister(std::string name, uint8_t &id, uint8_t &value, uint8_t type);

    /** Helper function for conversion from device type string to code
     */
    uint8_t stringToDeviceCode(std::string name);

    /** Routine to loop over all ROCs/pixels and figure out the most efficient way to
     *  (un)mask and trim them for the upcoming test according to the information stored
     *  in the DUT struct.
     */
    void MaskAndTrim();

    // FIXME missing documentation
    void SetCalibrateBits(bool enable);

    /** Helper function to check validity of the pattern generator settings coming from the user space
     */
    bool verifyPatternGenerator(std::vector<std::pair<uint16_t,uint8_t> > &pg_setup);


  }; // class api


  class dut {
    
    /** Allow the API class to access private members of the DUT - noone else should be able to access them! 
     */
    friend class api;
    
  public:

    /** GET functions to read information **/

    /** Info function printing a listing of the current DUT objects and their states
     */
    void info();

    /** Function returning the number of enabled pixels on a specific ROC:
     */
    int32_t getNEnabledPixels(uint8_t rocid);

    /** Function returning the number of masked pixels on a specific ROC:
     */
    int32_t getNMaskedPixels(uint8_t rocid);

    /** Function returning the number of enabled TBMs:
     */
    int32_t getNEnabledTbms();

    /** Function returning the number of enabled ROCs:
     */
    int32_t getNEnabledRocs();

    /** Function returning the enabled pixels configs for a specific ROC:
     */
    std::vector< pixelConfig > getEnabledPixels(size_t rocid);

    /** Function returning for every column if it includes an enabled pixel for a specific ROC:
     */
    std::vector< bool > getEnabledColumns(size_t rocid);

    /** Function returning the enabled ROC configs
     */
    std::vector< rocConfig > getEnabledRocs();

    /** Function returning the enabled TBM configs
     */
    std::vector< tbmConfig > getEnabledTbms();

    /** Function returning the status of a given pixel:
     */
    bool getPixelEnabled(uint8_t column, uint8_t row);

    /** Function to check if all pixels on all ROCs are enabled:
     */
    bool getAllPixelEnable();

    /** Function to check if all ROCs of a module are enabled:
     */
    bool getModuleEnable();

    /** Function returning the configuration of a given pixel:
     */
    pixelConfig getPixelConfig(size_t rocid, uint8_t column, uint8_t row);

    /** Function to read the current value from a DAC on ROC rocId
     */
    uint8_t getDAC(size_t rocId, std::string dacName);

    /** Function to read current values from all DAC on ROC rocId
     */
    std::vector<std::pair<uint8_t,uint8_t> > getDACs(size_t rocId);

    /** Helper function to print current values from all DAC on ROC rocId
     *  to stdout
     */
    void printDACs(size_t rocId);

    /** SET functions to allow enabling and disabling from the outside **/

    /** Function to enable the given ROC:
     */
    void setROCEnable(size_t rocId, bool enable);

    /** Function to enable the given ROC:
     */
    void setTBMEnable(size_t tbmId, bool enable);

    /** Function to enable the given pixel on all ROCs:
     */
    void testPixel(uint8_t column, uint8_t row, bool enable, int8_t rocid = -1);

    /** Function to mask the given pixel on a specific ROC
     */
    void maskPixel(uint8_t column, uint8_t row, bool mask, int8_t rocid = -1);

    /** Function to mask the all pixels in one column on a specific ROC
     */
    void maskColumn(uint8_t column, bool mask, int8_t rocid = -1);

    /** Function to enable all pixels on all ROCs:
     */
    void testAllPixels(bool enable, int8_t rocid = -1);

    /** Function to enable all pixels on all ROCs:
     */
    void maskAllPixels(bool mask, int8_t rocid = -1);
   
    /** Function to check the status of the DUT
     */
    bool status();


  private:

    /** Initialization status of the DUT instance, marks the "ready for
     *  operations" status
     */
    bool _initialized;

    /** Initialization status of the DUT devices, true when successfully 
     *  programmed and ready for operations
     */
    bool _programmed;

    std::vector< rocConfig > roc;
    std::vector< tbmConfig > tbm;

    std::map<uint8_t,uint8_t> sig_delays;
    double va, vd, ia, id;
    std::vector<std::pair<uint16_t,uint8_t> > pg_setup;

  }; //class DUT

} //namespace pxar

#endif /* PXAR_API_H */
