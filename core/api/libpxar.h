
namespace pxar {

  struct pixel {
    uint8_t roc_id;
    uint8_t column;
    uint8_t row;
    int32_t value;
  }

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
     *  This function will both update the bookkeeping value in the DUT
     *  struct and program the actual device 
     */
    bool setDAC(std::string dacName, uint8_t dacValue);

    /** Method to scan a DAC and measure the pulse height
     *  Returns a std vector of pixels, with the value of the pixel struct being
     *  the averaged pulse height over ntrig triggers
     */
    std::vector<pixel> getPulseheightVsDAC(string dac, int dacmin, int dacmax, int flag, int ntrig=16);

    /** Method to scan a DAC and measure the pixel threshold
     *  Returns a std vector of pixels, with the value of the pixel struct being
     *  number of 
     */
    std::vector<pixel> getEfficiencyVsDAC(string dac, int dacmin, int dacmax, int flag, int ntrig=16);

    /** Method to scan a DAC and measure the pulse height
     *  Returns a std vector of pixels, with the value of the pixel struct being
     *  
     */
    std::vector<pixel> getThresholdVsDAC(string dac, int dacmin, int dacmax, int flag, int ntrig=16);

    std::vector<pixel> getPulseheightVsDACDAC(string dac1, int dac1min, int dac1max, string dac2, int dac2min, int dac2max, int flag, int ntrig=10);
    std::vector<pixel> getEfficiencyVsDACDAC(string dac1, int dac1min, int dac1max, string dac2, int dac2min, int dac2max, int flag, int ntrig=10);
    std::vector<pixel> getThresholdVsDACDAC(string dac1, int dac1min, int dac1max, string dac2, int dac2min, int dac2max, int flag, int ntrig=10);

    std::vector<pixel> getPulseheightMap(int flag, int ntrig=10);
    std::vector<pixel> getEfficiencyMap(int flag, int ntrig=10);
    std::vector<pixel> getThresholdMap(int flag, int ntrig=10);

    getReadbackValue();

    /** DAQ functions **/

    daqStart();
    
    daqGetEvent();

    daqStop();
    

    /** DUT implementation **/

  private:

  };

}

