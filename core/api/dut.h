/**
 * pxar DUT class header
 * to be included by any executable linking to libpxar
 */

#ifndef PXAR_DUT_H
#define PXAR_DUT_H

/** Declare all classes that need to be included in shared libraries on Windows
 *  as class DLLEXPORT className
 */
#include "pxardllexport.h"

/** Cannot use stdint.h when running rootcint on WIN32 */
#if ((defined WIN32) && (defined __CINT__))
typedef int int32_t;
typedef short int int16_t;
typedef unsigned int uint32_t;
typedef unsigned short int uint16_t;
typedef unsigned char uint8_t;
#else
#include <stdint.h>
#endif


#include <string>
#include <vector>
#include <map>
#include "datatypes.h"
#include "exceptions.h"

namespace pxar {

  class DLLEXPORT dut {

    /** Allow the API class to access private members of the DUT - noone else
     *  should be able to access them!
     */
    friend class pxarCore;

  public:

    /** Default DUT constructor
     */
  dut() : _initialized(false), _programmed(false), roc(), tbm(), sig_delays(),
      va(0), vd(0), ia(0), id(0), pg_setup(), pg_sum(0), trigger_source(TRG_SEL_PG_DIR) {}

    // GET functions to read information

    /** Info function printing a listing of the current DUT objects and their states
     */
    void info();

    /** Function returning the number of enabled pixels on a specific ROC:
     */
    size_t getNEnabledPixels(uint8_t rocid);

    /** Function returning the number of enabled pixels on all ROCs:
     */
    size_t getNEnabledPixels();

    /** Function returning the number of masked pixels on a specific ROC:
     */
    size_t getNMaskedPixels(uint8_t rocid);

    /** Function returning the number of masked pixels on all ROCs:
     */
    size_t getNMaskedPixels();

    /** Function returning the number of enabled TBMs:
     */
    size_t getNEnabledTbms();

    /** Function returning the number of TBMs:
     */
    size_t getNTbms();

    /** Function returning the TBM type programmed:
     */
    std::string getTbmType();

    /** Function returning the number of enabled ROCs:
     */
    size_t getNEnabledRocs();

    /** Function returning the number of ROCs:
     */
    size_t getNRocs();

    /** Function returning the ROC type programmed:
     */
    std::string getRocType();

    /** Function returning the enabled pixels configs for a specific ROC ID:
     */
    std::vector< pixelConfig > getEnabledPixels(size_t rocid);

    /** Function returning the enabled pixels configs for a ROC with given I2C address:
     */
    std::vector< pixelConfig > getEnabledPixelsI2C(size_t roci2c);

    /** Function returning the enabled pixels configs for all ROCs:
     */
    std::vector< pixelConfig > getEnabledPixels();

    /** Function returning all masked pixels configs for a specific ROC:
     */
    std::vector< pixelConfig > getMaskedPixels(size_t rocid);

    /** Function returning all masked pixels configs for all ROCs:
     */
    std::vector< pixelConfig > getMaskedPixels();

    /** Function returning the enabled ROC configs
     */
    std::vector< rocConfig > getEnabledRocs();

    /** Function returning the Ids of all enabled ROCs in a uint8_t vector:
     */
    std::vector< uint8_t > getEnabledRocIDs();

    /** Function returning the I2C addresses of all enabled ROCs in a uint8_t vector:
     */
    std::vector< uint8_t > getEnabledRocI2Caddr();

    /** Function returning the I2C addresses of all ROCs in a uint8_t vector:
     */
    std::vector< uint8_t > getRocI2Caddr();

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
    std::vector<std::pair<std::string,uint8_t> > getDACs(size_t rocId);

    /** Function to read current values from all DAC on TBM tbmId
     */
    std::vector< std::pair<std::string,uint8_t> > getTbmDACs(size_t tbmId);

    /** Function returning the token chain lengths:
     */
    std::vector<uint8_t> getTbmChainLengths(size_t tbmId);

    /** Helper function to print current values from all DAC on ROC rocId
     *  to stdout
     */
    void printDACs(size_t rocId);

    /** SET functions to allow enabling and disabling from the outside **/

    /** Function to enable the given ROC:
     */
    void setROCEnable(size_t rocId, bool enable);

    /** Function to enable the given TBM:
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

    /** Function to update all trim bits of a given ROC.
     */
    bool updateTrimBits(std::vector<pixelConfig> trimming, uint8_t rocid);

    /** Function to update trim bits for one particular pixel on a given ROC.
     */
    bool updateTrimBits(uint8_t column, uint8_t row, uint8_t trim, uint8_t rocid);

    /** Function to update trim bits for one particular pixel on a given ROC.
     */
    bool updateTrimBits(pixelConfig trim, uint8_t rocid);

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
     *  for a specific ROC selected by its I2C address:
     */
    std::vector< bool > getEnabledColumns(size_t roci2c);

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

    /** DUT member to store the delay sum of the full Pattern Generator
     *  command list
     */
    uint32_t pg_sum;

    /** DUT member to store the selected trigger source to be activated
     */
    uint16_t trigger_source;

  }; //class DUT

} //namespace pxar

#endif /* PXAR_DUT_H */
