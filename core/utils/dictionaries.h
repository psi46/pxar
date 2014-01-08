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

      //------- DTB registers -----------------------------
      // FIXME what are the upper values for signal delays?
      _dacs["clk"]           = dacConfig(SIG_CLK,255);
      _dacs["ctr"]           = dacConfig(SIG_CTR,255);
      _dacs["sda"]           = dacConfig(SIG_SDA,255);
      _dacs["tin"]           = dacConfig(SIG_TIN,255);
      _dacs["deser160phase"] = dacConfig(SIG_DESER160PHASE,255);

      //------- TBM registers -----------------------------


      //------- ROC registers -----------------------------
      // DAC name, register and size reference:
      // http://cms.web.psi.ch/phase1/psi46dig/index.html

      _dacs["vdig"]       = dacConfig(ROC_DAC_Vdig,15);

      _dacs["vana"]       = dacConfig(ROC_DAC_Vana,255);

      _dacs["vsf"]        = dacConfig(ROC_DAC_Vsh,255);
      _dacs["vsh"]        = dacConfig(ROC_DAC_Vsh,255);

      _dacs["vcomp"]      = dacConfig(ROC_DAC_Vcomp,15);
      _dacs["vwllpr"]     = dacConfig(ROC_DAC_VwllPr,255);
      _dacs["vwllsh"]     = dacConfig(ROC_DAC_VwllSh,255);
      _dacs["vhlddel"]    = dacConfig(ROC_DAC_VhldDel,255);
      _dacs["vtrim"]      = dacConfig(ROC_DAC_Vtrim,255);
      _dacs["vthrcomp"]   = dacConfig(ROC_DAC_VthrComp,255);
      _dacs["vibias_bus"] = dacConfig(ROC_DAC_VIBias_Bus,255);
      _dacs["vbias_sf"]   = dacConfig(ROC_DAC_Vbias_sf,15);
      _dacs["voffsetop"]  = dacConfig(ROC_DAC_VoffsetOp,255);
      _dacs["voffsetro"]  = dacConfig(ROC_DAC_VoffsetRO,255);
      _dacs["vion"]       = dacConfig(ROC_DAC_VIon,255);

      _dacs["vcomp_adc"]  = dacConfig(ROC_DAC_VIbias_PH,255);
      _dacs["vibias_ph"]  = dacConfig(ROC_DAC_VIbias_PH,255);

      _dacs["viref_adc"]  = dacConfig(ROC_DAC_VIbias_DAC,255);
      _dacs["vibias_dac"] = dacConfig(ROC_DAC_VIbias_DAC,255);

      _dacs["vicolor"]    = dacConfig(ROC_DAC_VIColOr,255);
      _dacs["vcal"]       = dacConfig(ROC_DAC_Vcal,255);
      _dacs["caldel"]     = dacConfig(ROC_DAC_CalDel,255);

      _dacs["ctrlreg"]    = dacConfig(ROC_DAC_CtrlReg,255);
      _dacs["wbc"]        = dacConfig(ROC_DAC_WBC,255);
      _dacs["readback"]   = dacConfig(ROC_DAC_Readback,15);


      // DACs removed from psi46digV2:
      _dacs["vleak_comp"] = dacConfig(ROC_DAC_Vleak_comp,255);
      _dacs["vleak"] = dacConfig(ROC_DAC_Vleak_comp,255);

      _dacs["vrgpr"]      = dacConfig(ROC_DAC_VrgPr,255);
      _dacs["vrgsh"]      = dacConfig(ROC_DAC_VrgSh,255);

      _dacs["vibiasop"]   = dacConfig(ROC_DAC_VIbiasOp,255);
      _dacs["vbias_op"]   = dacConfig(ROC_DAC_VIbiasOp,255);

      _dacs["vibias_roc"]   = dacConfig(ROC_DAC_VIbias_roc,255);
      _dacs["vnpix"]   = dacConfig(ROC_DAC_Vnpix,255);
      _dacs["vsumcol"]   = dacConfig(ROC_DAC_VsumCol,255);

    };

    std::map<std::string, dacConfig> _dacs;
    // Dont forget to declare these two. You want to make sure they
    // are unaccessable otherwise you may accidently get copies of
    // your singleton appearing.
    RegisterDictionary(RegisterDictionary const&); // Don't Implement
    void operator=(RegisterDictionary const&); // Don't implement
  };


  /** Map for device name lookup
   *  All device names are lower case, check is case-insensitive.
   *  Singleton class, only one object of this floating around.
   */
  class DeviceDictionary {
  public:
    static DeviceDictionary * getInstance() {
      static DeviceDictionary instance; // Guaranteed to be destroyed.
      // Instantiated on first use.
      return &instance;
    }

    // Return the register id for the name in question:
    inline uint8_t getDevCode(std::string name) {
      try { return _devices[name]; }
      catch(...) { return 0x0; }
    }

  private:
    DeviceDictionary() {
      // Device name and types

      // ROC flavors:
      _devices["psi46v2"]       = ROC_PSI46V2;
      _devices["psi46xdb"]      = ROC_PSI46XDB;
      _devices["psi46dig"]      = ROC_PSI46DIG;
      _devices["psi46dig_trig"] = ROC_PSI46DIG_TRIG;
      _devices["psi46digv2_b"]  = ROC_PSI46DIGV2_B;
      _devices["psi46digv2"]    = ROC_PSI46DIGV2;
      _devices["psi46digv3"]    = ROC_PSI46DIGV3;

      // TBM flavors:
      // FIXME this is just an example.
      _devices["tbm07"]         = TBM_07;
      _devices["tbm07a"]        = TBM_07A;
      _devices["tbm08"]         = TBM_08;
    };

    std::map<std::string, uint8_t> _devices;
    DeviceDictionary(DeviceDictionary const&); // Don't Implement
    void operator=(DeviceDictionary const&); // Don't implement
  };

  
  /** Map for DTB digital probe signal name lookup
   *  All signal names are lower case, check is case-insensitive.
   *  Singleton class, only one object of this floating around.
   */
  class ProbeDictionary {
  public:
    static ProbeDictionary * getInstance() {
      static ProbeDictionary instance; // Guaranteed to be destroyed.
      // Instantiated on first use.
      return &instance;
    }

    // Return the register id for the name in question:
    inline uint8_t getSignal(std::string name) {
      try { return _signals[name]; }
      catch(...) { return PROBE_OFF; }
    }

  private:
    ProbeDictionary() {
      // Probe name and values

      // Digital signals:
      _signals["off"]    = PROBE_OFF;
      _signals["clk"]    = PROBE_CLK;
      _signals["sda"]    = PROBE_SDA;
      _signals["pgtok"]  = PROBE_PGTOK;
      _signals["pgtrg"]  = PROBE_PGTRG;
      _signals["pgcal"]  = PROBE_PGCAL;
      _signals["pgresr"] = PROBE_PGRESR;
      _signals["pgrest"] = PROBE_PGREST;
      _signals["pgsync"] = PROBE_PGSYNC;
      _signals["ctr"]    = PROBE_CTR;
      _signals["clkp"]   = PROBE_CLKP;
      _signals["clkg"]   = PROBE_CLKG;
      _signals["crc"]    = PROBE_CRC;

    };

    std::map<std::string, uint8_t> _signals;
    ProbeDictionary(ProbeDictionary const&); // Don't Implement
    void operator=(ProbeDictionary const&); // Don't implement
  };


  /** Map for DTB analog probe signal name lookup
   *  All signal names are lower case, check is case-insensitive.
   *  Singleton class, only one object of this floating around.
   */
  class ProbeADictionary {
  public:
    static ProbeADictionary * getInstance() {
      static ProbeADictionary instance; // Guaranteed to be destroyed.
      // Instantiated on first use.
      return &instance;
    }

    // Return the register id for the name in question:
    inline uint8_t getSignal(std::string name) {
      try { return _signals[name]; }
      catch(...) { return PROBEA_OFF; }
    }

  private:
    ProbeADictionary() {
      // Probe name and values

      // Digital signals:
      _signals["tin"]    = PROBEA_TIN;
      _signals["sdata1"] = PROBEA_SDATA1;
      _signals["sdata2"] = PROBEA_SDATA2;
      _signals["ctr"]    = PROBEA_CTR;
      _signals["clk"]    = PROBEA_CLK;
      _signals["sda"]    = PROBEA_SDA;
      _signals["tout"]   = PROBEA_TOUT;
      _signals["off"]    = PROBEA_OFF;

    };

    std::map<std::string, uint8_t> _signals;
    ProbeADictionary(ProbeADictionary const&); // Don't Implement
    void operator=(ProbeADictionary const&); // Don't implement
  };


} //namespace pxar

#endif /* PXAR_DICTIONARIES_H */
