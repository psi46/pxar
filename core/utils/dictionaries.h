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
#include "api.h"

namespace pxar {

  /** class to store a DAC config
   *  contains register id and valid range of the DAC
   */
  class dacConfig {
  public:
    dacConfig() {};
  dacConfig(uint8_t id, uint8_t size) : _id(id), _size(size) {};
    uint8_t _id;
    uint8_t _size;
  };

  /** Map for DAC name lookup
   *  All DAC names are lower case, DAC register selection is case-insensitive.
   *  Singleton class, only one object of this floating around.
   */

  class RegisterDictionary {
  public:
    static RegisterDictionary * getInstance() {
      static RegisterDictionary instance; // Guaranteed to be destroyed.
      // Instantiated on first use.
      return &instance;
    }

    // Return the register id for the name in question:
    inline uint8_t getRegister(std::string name) {
      try { return _dacs[name]._id; }
      catch(...) { return 0x0; }
    }

    // Return the register size for the register in question:
    inline uint8_t getSize(std::string name) {
      try { return _dacs[name]._size; }
      catch(...) { return 0x0; }
    }

    // Return the register size for the register in question:
    inline uint8_t getSize(uint8_t id) {

      for(std::map<std::string, dacConfig>::iterator iter = _dacs.begin(); iter != _dacs.end(); ++iter) {
	if((*iter).second._id == id) return (*iter).second._size;
      }
      
      return 0x0;
    }

  private:
    RegisterDictionary() {
      _dacs["vdig"]       = dacConfig(ROC_DAC_Vdig,4);
      _dacs["vana"]       = dacConfig(ROC_DAC_Vana,8);
      _dacs["vsf"]        = dacConfig(ROC_DAC_Vsf,8);
      _dacs["vcomp"]      = dacConfig(ROC_DAC_Vcomp,8);
      _dacs["vwllpr"]     = dacConfig(ROC_DAC_VwllPr,8);
      _dacs["vhlddel"]    = dacConfig(ROC_DAC_VhldDel,8);
      _dacs["vtrim"]      = dacConfig(ROC_DAC_Vtrim,8);
      _dacs["vthrcomp"]   = dacConfig(ROC_DAC_VthrComp,8);
      _dacs["vibias_bus"] = dacConfig(ROC_DAC_VIBias_Bus,8);
      _dacs["vbias_sf"]   = dacConfig(ROC_DAC_Vbias_sf,5);
      // FIXME to be continued...

      // DACs removed from psi46digV2:
      _dacs["vleak_comp"] = dacConfig(ROC_DAC_Vleak_comp,8);
      _dacs["vrgpr"]      = dacConfig(ROC_DAC_VrgPr,8);

    };

    std::map<std::string, dacConfig> _dacs;
    // Dont forget to declare these two. You want to make sure they
    // are unaccessable otherwise you may accidently get copies of
    // your singleton appearing.
    RegisterDictionary(RegisterDictionary const&);              // Don't Implement
    void operator=(RegisterDictionary const&); // Don't implement
  };

} //namespace pxar

#endif /* PXAR_DICTIONARIES_H */
