/**
 * pxar dictionary header
 * This file contains all dictionaries for parameter name lookup:
 *
 * DAC registers, their range and names
 * TBM parameters
 * DTB parameters
 */

#ifndef PXAR_DICTIONARIES_H
#define PXAR_DICTIONARIES_H

#include <string>
#include <map>
#include <stdint.h>
#include "constants.h"

namespace pxar {

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

  /** Map for DAC name lookup
   *  All DAC names are lower case, DAC register selection is case-insensitive.
   */
  struct DAC_DICT {
    static std::map<std::string,dacConfig> create_map() {
      std::map<std::string,dacConfig> m;
      m["vdig"]       = dacConfig(ROC_DAC_Vdig,0,255);
      m["vana"]       = dacConfig(ROC_DAC_Vana,0,255);
      m["vsf"]        = dacConfig(ROC_DAC_Vsf,0,255);
      m["vcomp"]      = dacConfig(ROC_DAC_Vcomp,0,255);
      m["vleak_comp"] = dacConfig(ROC_DAC_Vleak_comp,0,255);
      m["vrgpr"]      = dacConfig(ROC_DAC_VrgPr,0,255);
      m["vwllpr"]     = dacConfig(ROC_DAC_VwllPr,0,255);
      m["vhlddel"]    = dacConfig(ROC_DAC_VhldDel,0,255);
      m["vtrim"]      = dacConfig(ROC_DAC_Vtrim,0,255);
      m["vthrcomp"]   = dacConfig(ROC_DAC_VthrComp,0,255);
      m["vibias_bus"] = dacConfig(ROC_DAC_VIBias_Bus,0,255);
      m["vbias_sf"]   = dacConfig(ROC_DAC_Vbias_sf,0,255);
      // FIXME to be continued...
      return m;
    }
    static const std::map<std::string,dacConfig> DAC;
  };


} //namespace pxar

#endif /* PXAR_DICTIONARIES_H */
