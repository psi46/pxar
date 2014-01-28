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

/** Define a macro for calls to member functions through pointers 
 *  to member functions (used in the loop expansion routines).
 *  Follows advice of http://www.parashift.com/c++-faq/macro-for-ptr-to-memfn.html
 */
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))


namespace pxar {

  /** Class for storing decoded pixel readout data
   */
  class pixel {
  public:

    /** Default constructor for pixel objects, defaulting all member variables to zero
     */
  pixel() : roc_id(0), column(0), row(0), value(0) {};

    /** Constructor for pixel objects with address and value initialization.
     */
  pixel(int32_t address, int32_t data) : value(data) { decode(address); };

    /** Function to fill the pixel with linear encoded data from RPC transfer.
     *  The address transmitted from the NIOS soft core is encoded in the following
     *  way:
     *
     *  Split the address and distribute it over ROC, column and row:
     *   * pixel column: max(51 -> 110011), requires 6 bits (R)
     *   * pixel row: max(79 -> 1001111), requires 7 bits (C)
     *   * roc id: max(15 -> 1111), requires 4 bits (I)
     *
     *  So everything can be stored in one 32 bits variable:
     *
     *   ........ ....IIII ..CCCCCC .RRRRRRR
     */
    inline void decode(int32_t address) {
      roc_id = (address>>16)&15;
      column = (address>>8)&63;
      row = (address)&127;
    };
    uint8_t roc_id;
    uint8_t column;
    uint8_t row;
    int32_t value;
  };

  /** Class to store the configuration for single pixels (i.e. their mask state,
   *  trim bit settings and whether they belong to the currently run test ("enable").
   *  By default, pixelConfigs have the  mask bit set.
   */
  class pixelConfig {
  public:
  pixelConfig() : 
    column(0), row(0), 
      trim(15), mask(true), enable(false) {};
  pixelConfig(uint8_t _column, uint8_t _row, uint8_t _trim) : 
    column(_column), row(_row), trim(_trim),
      mask(true), enable(false) {};
    uint8_t column;
    uint8_t row;
    uint8_t trim;
    bool mask;
    bool enable;
  };

  /** Class for ROC states
   *
   *  Contains a DAC map for the ROC programming settings, a type flag, enable switch
   *  and a vector of pixelConfigs.
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
   *
   *  Contains a register map for the device register settings, a type flag and an enable switch
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
   *  addresses from the HAL class, used e.g. in loop expansion routines.
   *  Follows advice of http://www.parashift.com/c++-faq/typedef-for-ptr-to-memfn.html
   */
  typedef  std::vector< std::vector<pixel> >* (hal::*HalMemFnPixel)(uint8_t rocid, uint8_t column, uint8_t row, std::vector<int32_t> parameter);
  typedef  std::vector< std::vector<pixel> >* (hal::*HalMemFnRoc)(uint8_t rocid, std::vector<int32_t> parameter);
  typedef  std::vector< std::vector<pixel> >* (hal::*HalMemFnModule)(std::vector<int32_t> parameter);


  /** pxar API class definition
   *
   *  this is the central API through which all calls from tests and user space
   *  functions have to be routed in order to interact with the hardware.
   *
   *  The API level aims to provide a set of high-level function from which
   *  the "user" (or test implementation) can choose. This approach allows
   *  to hide hardware specific functions and calls from the user space code
   *  and automatize e.g. startup procedures.
   *
   *  All input from user space is checked before programming it to the 
   *  devices. Register addresses have an internal lookup mechanism so the
   *  user only hast to provide e.g. the DAC name to be programmed as a string.
   *
   *  Unless otherwise specified (some DAQ functions allow this) all data
   *  returned from API functions is fully decoded and stored in C++ structures
   *  using std::vectors and std::pairs to ease its handling.
   *
   *  Another concept implemented is the Device Under Test (DUT) which is a
   *  class representing the attached hardware to be tested. In order to change
   *  its configuration the user space code interacts with the _dut object and
   *  alters its settings. This is programmed into the devices automatically
   *  before the next test is executed. This approach allows both the efficient
   *  execution of many RPC calls at once and reading back the actual device
   *  configuration at any time during the tests.
   *
   *  Calls to test functions are automatically expanded in a way that they
   *  cover the full device in the most efficient way available. Instead of
   *  scanning 4160 pixels after another the code will select the function
   *  to scan a full ROC in one go automatically.
   */
  class api {

  public:

    /** Default constructor for the libpxar API
     *
     *  Fetches a new HAL instance and opens the connection to the testboard
     *  specified in the "usbId" parameter. An asterisk as "usbId" acts as
     *  wildcard. If only one DTB is connected the algorithm will automatically
     *  connect to this board, if several are connected it will throw a warning.
     */
    api(std::string usbId = "*", std::string logLevel = "WARNING");

    /** Default destructor for libpxar API
     *
     *  Will power down the DTB, disconnect properly from the testboard,
     *  and destroy the HAL object.
     */
    ~api();

    /** Returns the version string for the pxar API.
     *
     *  When using a git checkout the version number will be calculated at
     *  compile time from the latest tagged version plus the number of commits
     *  on top of that. In this case the version number also contains the
     *  commit hash of the latest commit for reference.
     *
     *  In case of a tarball install the version number is hardcoded in the
     *  CMakeLists.txt file.
     */
    std::string getVersion();

    /** Initializer method for the testboard
     *
     *  Initializes the tesboard with signal delay settings, and voltage/current
     *  limit settings (power_settings) and the initial pattern generator setup
     *  (pg_setup), all provided via vectors of pairs with descriptive name.
     *
     *  The name lookup is performed via the central API dictionaries.
     *
     *  All user inputs are checked for sanity. This includes range checks on
     *  the current limits set, a sanity check for the pattern generator command
     *  list (including a check for delay = 0 at the end of the list).
     */
    bool initTestboard(std::vector<std::pair<std::string,uint8_t> > sig_delays,
                       std::vector<std::pair<std::string,double> > power_settings,
                       std::vector<std::pair<uint16_t, uint8_t> > pg_setup);
  
    /** Initializer method for the DUT (attached devices)
     *
     *  This function requires the types and DAC settings for all TBMs and ROCs
     *  contained in the setup. All values will be checked for validity (DAC
     *  ranges, position and number of pixels, etc.)
     *
     *  All parameters are supplied via vectors, the size of the vector
     *  represents the number of devices. DAC names and device types should be
     *  provided as strings. The respective register addresses will be looked up
     *  internally. Strings are checked case-insensitive, old and new DAC names
     *  are both supported.
     */
    bool initDUT(std::string tbmtype, 
		 std::vector<std::vector<std::pair<std::string,uint8_t> > > tbmDACs,
		 std::string roctype,
		 std::vector<std::vector<std::pair<std::string,uint8_t> > > rocDACs,
		 std::vector<std::vector<pixelConfig> > rocPixels);

    /** Programming method for the DUT (attached devices)
     *
     *  This function requires the DUT structure to be filled (initialized).
     *
     *  All parameters are taken from the DUT struct, the enabled devices are 
     *  programmed. This function needs to be called after power cycling the 
     *  testboard output (using Poff, Pon).
     *
     *  A DUT flag is set which prevents test functions to be executed if 
     *  not programmed.
     */
    bool programDUT(); 
  

    // DTB functions

    /** Function to flash a new firmware onto the DTB via the USB connection.
     */
    bool flashTB(std::string filename);

    /** Function to read out analog DUT supply current on the testboard
     *
     *  The current will be returned in units of Ampere
     */
    double getTBia();

    /** Function to read out analog DUT supply voltage on the testboard
     *
     *  The voltage will be returned in units of Volts
     */
    double getTBva();

    /** Function to read out digital DUT supply current on the testboard
     *
     *  The current will be returned in units of Ampere
     */
    double getTBid();

    /** Function to read out digital DUT supply voltage on the testboard
     *
     *  The voltage will be returned in units of Volts
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



    // TEST functions

    /** Set a DAC value on the DUT for one specific ROC
     *
     *  The "rocid" parameter can be used to select a specific ROC to program.
     *
     *  This function will both update the bookkeeping value in the DUT
     *  struct and program the actual device.
     */
    bool setDAC(std::string dacName, uint8_t dacValue, uint8_t rocid);

    /** Set a DAC value on the DUT for all enabled ROC
     *
     *  This function will both update the bookkeeping value in the DUT
     *  struct and program the actual device.
     */
    bool setDAC(std::string dacName, uint8_t dacValue);

    /** Set a register value on a specific TBM of the DUT
     *
     *  The "tbmid" parameter can be used to select a specific TBM to program.
     *  This function will both update the bookkeeping value in the DUT
     *  struct and program the actual device.
     *
     *  This function will set the respective register always in both cores of
     *  the TBM specified.
     */
    bool setTbmReg(std::string regName, uint8_t regValue, uint8_t tbmid);

    /** Set a register value on all TBMs of the DUT
     *
     *  This function will both update the bookkeeping value in the DUT
     *  struct and program the actual device.
     *
     *  This function will set the respective register always in both cores of
     *  all TBMs configured in the DUT.
     */
    bool setTbmReg(std::string regName, uint8_t regValue);

    /** Method to scan a DAC range and measure the pulse height
     *
     *  Returns a vector of pairs containing set dac value and a pixel vector,
     *  with the value of the pixel struct being the averaged pulse height
     *  over "nTriggers" triggers
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > > getPulseheightVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
									       uint16_t flags = 0, uint32_t nTriggers = 16);

    /** Method to scan a DAC range and measure the efficiency
     *
     *  Returns a vector of pairs containing set dac value and pixels,
     *  with the value of the pixel struct being the number of hits in that
     *  pixel. Efficiency == 1 for nhits == nTriggers
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > > getEfficiencyVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
					  uint16_t flags = 0, uint32_t nTriggers=16);

    /** Method to scan a DAC range and measure the pixel threshold
     *
     *  Returns a vector of pairs containing set dac value and pixels,
     *  with the value of the pixel struct being the threshold value of that
     *  pixel.
     *
     *  The threshold is calculated as the 0.5 value of the s-curve of the pixel
     *  using an adapted binary search algorithm for optimal speed.
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > > getThresholdVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
					 uint16_t flags = 0, uint32_t nTriggers=16);

    /** Method to scan a 2D DAC-Range (DAC1 vs. DAC2) and measure the
     *  pulse height
     *
     *  Returns a vector containing pairs of DAC1 values and pais of DAC2
     *  values with a pixel vector. The value of the pixel struct is the
     *  averaged pulse height over "nTriggers" triggers.
     */
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > getPulseheightVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
					      std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
					      uint16_t flags = 0, uint32_t nTriggers=16);

    /** Method to scan a 2D DAC-Range (DAC1 vs. DAC2) and measure the efficiency
     *
     *  Returns a vector containing pairs of DAC1 values and pais of DAC2
     *  values with a pixel vector. The value of the pixel struct is the
     *  number of hits in that pixel. Efficiency == 1 for nhits == nTriggers
     */
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > getEfficiencyVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
					     std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
					     uint16_t flags = 0, uint32_t nTriggers=16);

    /** Method to scan a 2D DAC-Range (DAC1 vs. DAC2) and measure the threshold
     *
     *  Returns a vector containing pairs of DAC1 values and pais of DAC2
     *  values with a pixel vector. The value of the pixel struct is the
     *  averaged pixel threshold.
     *
     *  The threshold is calculated as the 0.5 value of the s-curve of the pixel
     *  using an adapted binary search algorithm for optimal speed.
     */
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > getThresholdVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
					    std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
					    uint16_t flags = 0, uint32_t nTriggers=16);

    /** Method to get a map of the pulse height
     *
     *  Returns a vector of pixels, with the value of the pixel struct being
     *  the averaged pulse height over "nTriggers" triggers
     */
    std::vector<pixel> getPulseheightMap(uint16_t flags = 0, uint32_t nTriggers=16);

    /** Method to get a map of the efficiency
     *
     *  Returns a vector of pixels, with the value of the pixel struct being
     *  the number of hits in that pixel. Efficiency == 1 for nhits == nTriggers
     */
    std::vector<pixel> getEfficiencyMap(uint16_t flags = 0, uint32_t nTriggers=16);

    /** Method to get a map of the pixel threshold
     *
     *  Returns a vector of pixels, with the value of the pixel struct being
     *  the threshold value of that pixel
     *
     *  The threshold is calculated as the 0.5 value of the s-curve of the pixel
     *  using an adapted binary search algorithm for optimal speed.
     */
    std::vector<pixel> getThresholdMap(std::string dacName, uint16_t flags = 0, uint32_t nTriggers=16);

    int32_t getReadbackValue(std::string parameterName);


    // DAQ functions

    /** Function to set up and initialize a new data acquisition session (DAQ)
     *
     *  This function takes a new Pattern Generator setup as argument, if left 
     *  empty the one which is currently programmed via the api::initTestboard 
     *  function is used. The given pattern generator only lives for the time 
     *  of the data acquisition and is replaced by the previous one after 
     *  stopping the DAQ.
     */
    bool daqStart(std::vector<std::pair<uint16_t, uint8_t> > pg_setup);
    
    /** Function to read out the earliest event in buffer from the currently
     *  data acquisition. If no event is buffered, the function will wait for
     *  the next event to arrive and then return it.
     */
    std::vector<pixel> daqGetEvent();

    /** Function to fire the previously defined pattern command list "nTrig"
     *  times, the function parameter defaults to 1.
     */
    void daqTrigger(uint32_t nTrig = 1);

    /** Function to fire the previously defined pattern command list
     *  continuously every "period" clock cycles (default: 1000)
     */
    void daqTriggerLoop(uint16_t period = 1000);

    /** Function to stop the running data acquisition
     *
     *  This triggers also a reprogramming of the old (test-) Pattern Generator
     *  setup, so no additional steps are needed before one can do regular 
     *  tests again. The patterns are taken from the DUT struct in which they 
     *  are stored by the api::initTestboard function.
     */
    bool daqStop();

    /** Function to return the full event buffer from the testboard RAM after
     *  the data acquisition has been stopped. No decoding is performed, this 
     *  function returns the raw data blob from either of the deserializer
     *  modules.
     */
    std::vector<uint16_t> daqGetBuffer();


    /** DUT object for book keeping of settings
     */
    dut * _dut;

    /** Status function for the API
     *
     *  Returns true if everything is setup correctly for operation
     */
    bool status();
    
  private:

    /** Private HAL object for the API to access hardware routines
     */
    hal * _hal;

    /** Routine to loop over all active ROCs/pixels and call the
     *  appropriate pixel, ROC or module HAL methods for execution.
     *
     *  This provides the central functionality of the DUT concept, expandLoop
     *  will check for the most efficient way to carry out a test requested by
     *  the user, i.e. select the full-ROC test instead of the pixel-by-pixel
     *  function, all depending on the configuration of the DUT.
     */
    std::vector< std::vector<pixel> >* expandLoop(HalMemFnPixel pixelfn, HalMemFnRoc rocfn, HalMemFnModule modulefn, std::vector<int32_t> param, bool forceSerial = false);

    /** Repacks map data from (possibly) several ROCs into one long vector
     *  of pixels.
     */
    std::vector<pixel>* repackMapData (std::vector< std::vector<pixel> >* data);

    /** Repacks DAC scan data into pairs of DAC values with fired pixel vectors.
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > >* repackDacScanData (std::vector< std::vector<pixel> >* data, uint8_t dacMin, uint8_t dacMax);

    /** repacks (2D) DAC-DAC scan data into pairs of DAC values with
     *  vectors of the fired pixels.
     */
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > >* repackDacDacScanData (std::vector< std::vector<pixel> >* data, uint8_t dac1min, uint8_t dac1max, uint8_t dac2min, uint8_t dac2max);

    /** Compacts data over ROC loops (ROC0<data>,ROC1<data>, ...) into
     *  (data(roc0,roc1)) where the data blocks can ueasily be subdivided into
     *  e.g. DAC ranges of a scan.
     */
    std::vector< std::vector<pixel> >* compactRocLoopData (std::vector< std::vector<pixel> >* data, uint8_t nRocs);

    /** Helper function for conversion from string to register value
     *
     *  Type tells it whether it is a DTB, TBM or ROC register to look for.
     *  This function also performs a range check for DAC and testboard register
     *  values. This function return the value itself if it is within the valid
     *  range or upper/lower range boundary in case of over/underflow.
     */
    bool verifyRegister(std::string name, uint8_t &id, uint8_t &value, uint8_t type);

    /** Helper function for conversion from device type string to code
     */
    uint8_t stringToDeviceCode(std::string name);

    /** Routine to loop over all ROCs/pixels and figure out the most efficient
     *  way to (un)mask and trim them for the upcoming test according to the 
     *  information stored in the DUT struct.
     *
     *  This function programs all attached devices with the needed trimming and
     *  masking bits for the Pixel Unit Cells (PUC). It sets both the needed PUC
     *  mask&trim bits and the DCOL enable bits. If the bool parameter "trim" is
     *  set to "false" it will just mask all ROCs.
     */
    void MaskAndTrim(bool trim);

    /** Helper function to setup the attached devices for operation using
     *  calibrate pulses.
     *
     *  It sets the Pixel Unit Cell (PUC) Calibrate bit for
     *  every pixels enabled in the test range (those for which the "enable" 
     *  flag has been set using the dut::testPixel() functions) 
     */
    void SetCalibrateBits(bool enable);

    /** Helper function to check validity of the pattern generator settings
     *  coming from the user space.
     *
     *  Right now this only checks for wrong or 
     *  dangerous delay settings either stopping the PG too early or not at
     *  all (missing delay = 0 at the end of the pattern command list)
     */
    bool verifyPatternGenerator(std::vector<std::pair<uint16_t,uint8_t> > &pg_setup);

  }; // class api


  class dut {
    
    /** Allow the API class to access private members of the DUT - noone else
     *  should be able to access them! 
     */
    friend class api;
    
  public:

    /** Default DUT constructor
     */
    dut() : _initialized(false), _programmed(false), roc(), tbm(), sig_delays(),
      va(0), vd(0), ia(0), id(0), pg_setup() {};

    // GET functions to read information

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
    void testPixel(uint8_t column, uint8_t row, bool enable);

    /** Function to enable the given pixel on the specified ROC:
     */
    void testPixel(uint8_t column, uint8_t row, bool enable, uint8_t rocid);

    /** Function to mask the given pixel on all ROCs:
     */
    void maskPixel(uint8_t column, uint8_t row, bool mask);

    /** Function to mask the given pixel on a specific ROC
     */
    void maskPixel(uint8_t column, uint8_t row, bool mask, uint8_t rocid);

    /** Function to mask the all pixels in one column on a specific ROC
     */
    void maskColumn(uint8_t column, bool mask, uint8_t rocid);

    /** Function to mask the all pixels in one column on all ROCs
     */
    void maskColumn(uint8_t column, bool mask);

    /** Function to enable all pixels on all ROCs:
     */
    void testAllPixels(bool enable);

    /** Function to enable all pixels on a specific ROC "rocid":
     */
    void testAllPixels(bool enable, uint8_t rocid);

    /** Function to enable all pixels on a specific ROC "rocid":
     */
    void maskAllPixels(bool mask, uint8_t rocid);

    /** Function to enable all pixels on all ROCs:
     */
    void maskAllPixels(bool mask);
   
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

    /** Function returning for every column if it includes an enabled pixel
     *  for a specific ROC:
     */
    std::vector< bool > getEnabledColumns(size_t rocid);

    /** DUT member to hold all ROC configurations
     */
    std::vector< rocConfig > roc;

    /** DUT member to hold all TBM configurations
     */
    std::vector< tbmConfig > tbm;

    /** DUT member to hold all DTB signal delay configurations
     */
    std::map<uint8_t,uint8_t> sig_delays;

    /** Variables to store the DTB power limit settings
     */
    double va, vd, ia, id;

    /** DUT member to store the current Pattern Generator command list
     */
    std::vector<std::pair<uint16_t,uint8_t> > pg_setup;

  }; //class DUT

} //namespace pxar

#endif /* PXAR_API_H */
