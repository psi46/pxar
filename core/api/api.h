/**
 * pxar API class header
 * to be included by any executable linking to libpxar
 */

#ifndef PXAR_API_H
#define PXAR_API_H

#include <string>
#include <vector>
#include <stdint.h>

namespace pxar {

  /** Struct for storing pixel readout data
   */
  struct pixel {
    uint8_t roc_id;
    uint8_t column;
    uint8_t row;
    int32_t value;
  };

  /** Struct to store the configuration for single pixels (i.e. their mask state, trim bit settings
   *  and the arming state (enable)
   */
  struct pixelConfig {
    uint8_t column;
    uint8_t row;
    uint8_t trim;
    bool mask;
    bool enable;
  };

  /** class to store a DAC config
   *  contains register id and valid range of the DAC
   */
  class dacConfig {
  public:
    dacConfig() {};
    dacConfig(uint8_t id, uint8_t min, uint8_t max) : _id(id), _min(min), _max(max) {};
    uint8_t _id;
    uint8_t _min;
    uint8_t _max;
  };

  /** Struct for ROC states
   *  Contains a DAC vector for their settings, a type flag and an enable switch
   *  and a vector for pixelConfig
   */
  struct rocConfig {
    std::vector< pixelConfig > pixels;
    std::vector< std::pair<uint8_t,uint8_t> > dacs;
    uint8_t type;
    bool enable;
  };

  /** Struct for TBM states
   *  Contains a DAC vector for their settings, a type flag and an enable switch
   */
  struct tbmConfig {
    std::vector< std::pair<uint8_t,uint8_t> > dacs;
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

    /** Initializer method for the testboard
     *  Initializes the tesboard with delay and voltage/current limit settings
     */
    bool initTestboard();
  
    /** Initializer method for the DUT
     *  Fills the DUT (the attached device(s))
     */
    bool initDUT(std::vector<std::pair<uint8_t,uint8_t> > dacVector);



    /** DTB fcuntions **/

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

    /** Function to read values from the integrated digital scope on the DTB
     */
    //getScopeData();

    /** Function to select the channel of the integrated digital scope on the DTB
     */
    //setScopeChannel();



    /** TEST functions **/

    /** Set a DAC value on the DUT
     *
     *  This function will both update the bookkeeping value in the DUT
     *  struct and program the actual device 
     */
    bool setDAC(std::string dacName, uint8_t dacValue);

    /** Method to scan a DAC and measure the pulse height
     *
     *  Returns a std vector of pairs containing set dac value and a pixel vector, with the value of the pixel struct being
     *  the averaged pulse height over nTriggers triggers
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > > getPulseheightVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
									       uint32_t flags = 0, uint32_t nTriggers=16);

    /** DEBUG ROUTINE DELME FIXME WHATEVER
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > > getDebugVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
									 uint32_t flags = 0, uint32_t nTriggers=16);

    /** Method to scan a DAC and measure the efficiency
     *
     *  Returns a std vector of pairs containing set dac value and pixels, with the value of the pixel struct being
     *  the number of hits in that pixel. Efficiency == 1 for nhits == nTriggers
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > > getEfficiencyVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
					  uint32_t flags = 0, uint32_t nTriggers=16);

    /** Method to scan a DAC and measure the pixel threshold
     *
     *  Returns a std vector of pairs containing set dac value and pixels, with the value of the pixel struct being
     *  the threshold value of that pixel
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > > getThresholdVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
					 uint32_t flags = 0, uint32_t nTriggers=16);

    /** Method to scan a 2D DAC-Range (DAC1 vs. DAC2)  and measure the pulse height
     *
     *  Returns a std vector containing pairs of DAC1 values and pais of DAC2 values with a pixel vector
     *  with the value of the pixel struct being the averaged pulse height over nTriggers triggers
     */
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > getPulseheightVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
					      std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
					      uint32_t flags = 0, uint32_t nTriggers=16);

    /** Method to scan a 2D DAC-Range (DAC1 vs. DAC2)  and measure the efficiency
     *
     *  Returns a std vector containing pairs of DAC1 values and pais of DAC2 values with a pixel vector
     *  with the value of the pixel struct being the number of hits in that pixel.
     *   Efficiency == 1 for nhits == nTriggers
     */
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > getEfficiencyVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
					     std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
					     uint32_t flags = 0, uint32_t nTriggers=16);

    /** Method to scan a 2D DAC-Range (DAC1 vs. DAC2)  and measure the threshold
     *
     *  Returns a std vector containing pairs of DAC1 values and pais of DAC2 values with a pixel vector
     *  with the value of the pixel struct being the averaged pixel threshold.
     */
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > getThresholdVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
					    std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
					    uint32_t flags = 0, uint32_t nTriggers=16);

    /** Method to get a chip map of the pulse height
     *
     *  Returns a std vector of pixels, with the value of the pixel struct being
     *  the averaged pulse height over nTriggers triggers
     */
    std::vector<pixel> getPulseheightMap(uint32_t flags = 0, uint32_t nTriggers=16);

    /** Method to get a chip map of the efficiency
     *
     *  Returns a std vector of pixels, with the value of the pixel struct being
     *  the number of hits in that pixel. Efficiency == 1 for nhits == nTriggers
     */
    std::vector<pixel> getEfficiencyMap(uint32_t flags = 0, uint32_t nTriggers=16);

    /** Method to get a chip map of the pixel threshold
     *
     *  Returns a std vector of pixels, with the value of the pixel struct being
     *  the threshold value of that pixel
     */
    std::vector<pixel> getThresholdMap(uint32_t flags = 0, uint32_t nTriggers=16);

    int32_t getReadbackValue(std::string parameterName);

    /** DEBUG METHOD -- FIXME/DELME
     */
    int32_t debug_ph(int32_t col, int32_t row, int32_t trim, int16_t nTriggers);

    /** DAQ functions **/

    /** Function to set up a new data acquisition
     */
    bool daqStart();
    
    /** Function to read out the earliest event in buffer from the currently
     *  data acquisition. If no event is buffered, the function will wait for
     *  the next event to arrive and then return it.
     */
    std::vector<pixel> daqGetEvent();

    /** Function to stop the running data acquisition
     */
    bool daqStop();

    /** Function to return the full event buffer from the testboard RAM after the
     *  data acquisition has been stopped.
     */
    // <?> daqGetBuffer();
    


    /** DUT object for book keeping of settings
     */
    dut * _dut;

    /** Status function for the API, returns ture if everything is setup correctly
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
    std::vector< std::pair<uint8_t, std::vector<pixel> > > repackDacScanData (std::vector< std::vector<pixel> >* data, uint8_t dacMin, uint8_t dacMax);

    /** compacts data over ROC loops (ROC0<data>,ROC1<data>, ...) into (data(roc0,roc1)) where the data blocks can be subdivided into e.g. DAC ranges
     */
    std::vector< std::vector<pixel> >* compactRocLoopData (std::vector< std::vector<pixel> >* data, uint8_t nRocs);


  }; // class api


  class dut {
    
    /** Allow the API class to access private members of the DUT - noone else should be able to access them! 
     */
    friend class api;
    
  public:

    /** GET functions to read information **/

    /** Function returning the number of enabled pixels on each ROC:
     */
    int32_t getNEnabledPixels();

    /** Function returning the number of enabled ROCs:
     */
    int32_t getNEnabledRocs();

    /** Function returning the enabled pixels configs for a specific ROC:
     */
    std::vector< pixelConfig > getEnabledPixels(size_t rocid);

    /** Function returning the enabled roc configs
     */
    std::vector< rocConfig > getEnabledRocs();

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
    void setPixelEnable(uint8_t column, uint8_t row, bool enable);

    /** Function to enable all pixels on all ROCs:
     */
    void setAllPixelEnable(bool enable);

   
    /** Function to check the status of the DUT
     */
    bool status();


  private:

    /** Initialization status of the DUT instance, marks the "ready for
     *  operations" status
     */
    bool _initialized;

    std::vector< rocConfig > roc;
    std::vector< tbmConfig > tbm;

  }; //class DUT

} //namespace pxar

#endif /* PXAR_API_H */
