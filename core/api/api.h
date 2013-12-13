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

  struct pixel {
    uint8_t roc_id;
    uint8_t column;
    uint8_t row;
    int32_t value;
  };

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
    

    /** TEST functions **/

    /** Set a DAC value on the DUT
     *
     *  This function will both update the bookkeeping value in the DUT
     *  struct and program the actual device 
     */
    bool setDAC(std::string dacName, uint8_t dacValue);

    /** Method to scan a DAC and measure the pulse height
     *
     *  Returns a std vector of pixels, with the value of the pixel struct being
     *  the averaged pulse height over nTriggers triggers
     */
    std::vector<pixel> getPulseheightVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
					   uint32_t flags, uint32_t nTriggers=16);

    /** Method to scan a DAC and measure the pixel threshold
     *
     *  Returns a std vector of pixels, with the value of the pixel struct being
     *  the number of hits in that pixel. Efficiency == 1 for nhits == nTriggers
     */
    std::vector<pixel> getEfficiencyVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
					  uint32_t flags, uint32_t nTriggers=16);

    /** Method to scan a DAC and measure the pulse height
     *
     *  Returns a std vector of pixels, with the value of the pixel struct being
     *  the threshold value of that pixel
     */
    std::vector<pixel> getThresholdVsDAC(std::string dacName, uint8_t dacMin, uint8_t dacMax, 
					 uint32_t flags, uint32_t nTriggers=16);

    /** Method to scan a 2D DAC-Range (DAC1 vs. DAC2)  and measure the pulse height
     *
     *  Returns a std vector of pixels, with the value of the pixel struct being
     *  the averaged pulse height over nTriggers triggers
     */
    std::vector<pixel> getPulseheightVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
					      std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
					      uint32_t flags, uint32_t nTriggers=16);
    std::vector<pixel> getEfficiencyVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
					     std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
					     uint32_t flags, uint32_t nTriggers=16);
    std::vector<pixel> getThresholdVsDACDAC(std::string dac1name, uint8_t dac1min, uint8_t dac1max, 
					    std::string dac2name, uint8_t dac2min, uint8_t dac2max, 
					    uint32_t flags, uint32_t nTriggers=16);

    std::vector<pixel> getPulseheightMap(uint32_t flags, uint32_t nTriggers=16);
    std::vector<pixel> getEfficiencyMap(uint32_t flags, uint32_t nTriggers=16);
    std::vector<pixel> getThresholdMap(uint32_t flags, uint32_t nTriggers=16);

    int32_t getReadbackValue(std::string parameterName);

    /** DAQ functions **/

    bool daqStart();
    
    std::vector<pixel> daqGetEvent();

    bool daqStop();
    

    /** DUT implementation **/

  private:

  };

}

#endif /* PXAR_API_H */
