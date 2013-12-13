/**
 * pXar API class header
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

  /** Struct to store values for DACs (identified by their register id)
   */
  struct DAC {
    uint8_t id;
    uint8_t value;
  };

  /** Struct for ROC states
   *  Contains a DAC vector for their settings, a type flag and an enable switch
   *  and a vector for pixelConfig
   */
  struct rocConfig {
    std::vector< pixelConfig > pixels;
    std::vector< DAC > dacs;
    uint8_t type;
    bool enable;
  };

  /** Struct for TBM states
   *  Contains a DAC vector for their settings, a type flag and an enable switch
   */
  struct tbmConfig {
    std::vector< DAC > dacs;
    uint8_t type;
    bool enable;
  };

  /* Forward declaration, implementation follows below...
   */
  class dut;

  class api {

  public:

    /** Constructor for the libpxar API
     *
     * 
     */
    api();
    //~api();

    /** Initializer method for the testboard
     *  Opens new testboard instance
     */
    bool initTB();
  
    /** Initializer method for the DUT
     *  Fills the DUT (the attached device(s))
     */
    bool initDUT();


    /** DTB fcuntions **/

    /** Function to flash a new firmware onto the DTB via
     *  the regular USB connection.
     */
    bool flashTB(std::string filename);

    /* GetTB...() */
    //FIXME


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
					   uint32_t flags, uint32_t nTriggers=16);

    /** Method to scan a DAC and measure the efficiency
     *
     *  Returns a std vector of pairs containing set dac value and pixels, with the value of the pixel struct being
     *  the number of hits in that pixel. Efficiency == 1 for nhits == nTriggers
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > > getEfficiencyVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
					  uint32_t flags, uint32_t nTriggers=16);

    /** Method to scan a DAC and measure the pixel threshold
     *
     *  Returns a std vector of pairs containing set dac value and pixels, with the value of the pixel struct being
     *  the threshold value of that pixel
     */
    std::vector< std::pair<uint8_t, std::vector<pixel> > > getThresholdVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
					 uint32_t flags, uint32_t nTriggers=16);

    /** Method to scan a 2D DAC-Range (DAC1 vs. DAC2)  and measure the pulse height
     *
     *  Returns a std vector containing pairs of DAC1 values and pais of DAC2 values with a pixel vector
     *  with the value of the pixel struct being the averaged pulse height over nTriggers triggers
     */
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > getPulseheightVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
					      std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
					      uint32_t flags, uint32_t nTriggers=16);

    /** Method to scan a 2D DAC-Range (DAC1 vs. DAC2)  and measure the efficiency
     *
     *  Returns a std vector containing pairs of DAC1 values and pais of DAC2 values with a pixel vector
     *  with the value of the pixel struct being the number of hits in that pixel.
     *   Efficiency == 1 for nhits == nTriggers
     */
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > getEfficiencyVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
					     std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
					     uint32_t flags, uint32_t nTriggers=16);

    /** Method to scan a 2D DAC-Range (DAC1 vs. DAC2)  and measure the threshold
     *
     *  Returns a std vector containing pairs of DAC1 values and pais of DAC2 values with a pixel vector
     *  with the value of the pixel struct being the averaged pixel threshold.
     */
    std::vector< std::pair<uint8_t, std::pair<uint8_t, std::vector<pixel> > > > getThresholdVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
					    std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
					    uint32_t flags, uint32_t nTriggers=16);

    /** Method to get a chip map of the pulse height
     *
     *  Returns a std vector of pixels, with the value of the pixel struct being
     *  the averaged pulse height over nTriggers triggers
     */
    std::vector<pixel> getPulseheightMap(uint32_t flags, uint32_t nTriggers=16);

    /** Method to get a chip map of the efficiency
     *
     *  Returns a std vector of pixels, with the value of the pixel struct bein
     *  the number of hits in that pixel. Efficiency == 1 for nhits == nTriggers
     */
    std::vector<pixel> getEfficiencyMap(uint32_t flags, uint32_t nTriggers=16);

    /** Method to get a chip map of the pixel threshold
     *
     *  Returns a std vector of pixels, with the value of the pixel struct bein
     *  the threshold value of that pixel
     */
    std::vector<pixel> getThresholdMap(uint32_t flags, uint32_t nTriggers=16);

    int32_t getReadbackValue(std::string parameterName);

    /** DAQ functions **/

    bool daqStart();
    
    std::vector<pixel> daqGetEvent();

    bool daqStop();
    

    /** DUT implementation **/
    
    
    
  private:

  };


  class dut {
    
    /* Allow the API class to access private members of the DUT - noone else should be able to access them! 
     */
    friend class api;
    
  public:

    //get...

  private:
    std::vector< rocConfig > roc;
    std::vector< tbmConfig > tbm;

  };

}

#endif /* PXAR_API_H */
