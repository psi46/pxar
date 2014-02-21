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
#include <iostream>

#define DTB_REG 0xFF
#define TBM_REG 0x0F
#define ROC_REG 0x00

namespace pxar {

  /** class to store a DAC config
   *  contains register id and valid range of the DAC
   */
  class dacConfig {
  public:
    dacConfig() {};
  dacConfig(uint8_t id, uint8_t size) : _type(0), _id(id), _size(size), _preferred(true) {};
  dacConfig(uint8_t id, uint8_t size, uint8_t type) : _type(type), _id(id), _size(size), _preferred(true) {};
  dacConfig(uint8_t id, uint8_t size, uint8_t type, bool preferred) : _type(type), _id(id), _size(size), _preferred(preferred) {};
    uint8_t _type;
    uint8_t _id;
    uint8_t _size;
    bool _preferred;
  };


  /** Map for generic register name lookup
   *  All register names are lower case, register selection is case-insensitive.
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
    inline uint8_t getRegister(std::string name, uint8_t type) {
      if(_registers.find(name)->second._type == type) {
	return _registers.find(name)->second._id;
      }
      else { return type;}
    }

    // Return the register size for the register in question:
    inline uint8_t getSize(std::string name, uint8_t type) {
	if(_registers.find(name)->second._type == type) {
	  return _registers.find(name)->second._size;
	}
	else { return type;}
    }

    // Return the register name for the register in question:
    inline uint8_t getSize(uint8_t id, uint8_t type) {
      for(std::map<std::string, dacConfig>::iterator iter = _registers.begin(); iter != _registers.end(); ++iter) {
	if((*iter).second._type == type && (*iter).second._id == id) {
	  return (*iter).second._size;
	}
      }
      return type;
    }

    // Return the register name for the register in question:
    inline std::string getName(uint8_t id, uint8_t type) {
      for(std::map<std::string, dacConfig>::iterator iter = _registers.begin(); iter != _registers.end(); ++iter) {
	if((*iter).second._type == type && (*iter).second._id == id && (*iter).second._preferred == true) {
	  return (*iter).first;}
      }
      return "";
    }

  private:
    RegisterDictionary() {
      //------- DTB registers -----------------------------
      _registers["clk"]           = dacConfig(SIG_CLK,255,DTB_REG);
      _registers["ctr"]           = dacConfig(SIG_CTR,255,DTB_REG);
      _registers["sda"]           = dacConfig(SIG_SDA,255,DTB_REG);
      _registers["tin"]           = dacConfig(SIG_TIN,255,DTB_REG);
      _registers["deser160phase"] = dacConfig(SIG_DESER160PHASE,7,DTB_REG);


      //------- TBM registers -----------------------------
      _registers["counters"]         = dacConfig(TBM_REG_COUNTER_SWITCHES,255,TBM_REG);
      _registers["mode"]             = dacConfig(TBM_REG_SET_MODE,255,TBM_REG);

      _registers["clear"]            = dacConfig(TBM_REG_CLEAR_INJECT,255,TBM_REG);
      _registers["inject"]           = dacConfig(TBM_REG_CLEAR_INJECT,255,TBM_REG);

      _registers["pkam_set"]         = dacConfig(TBM_REG_SET_PKAM_COUNTER,255,TBM_REG);
      _registers["delays"]           = dacConfig(TBM_REG_SET_DELAYS,255,TBM_REG);
      _registers["temperature"]      = dacConfig(TBM_REG_TEMPERATURE_CONTROL,255,TBM_REG);


      //------- ROC registers -----------------------------
      // DAC name, register and size reference:
      // http://cms.web.psi.ch/phase1/psi46dig/index.html

      _registers["vdig"]       = dacConfig(ROC_DAC_Vdig,15,ROC_REG);
      _registers["vdd"]        = dacConfig(ROC_DAC_Vdig,15,ROC_REG,false);

      _registers["vana"]       = dacConfig(ROC_DAC_Vana,255,ROC_REG);
      _registers["iana"]       = dacConfig(ROC_DAC_Vana,255,ROC_REG,false);

      _registers["vsf"]        = dacConfig(ROC_DAC_Vsh,255,ROC_REG,false);
      _registers["vsh"]        = dacConfig(ROC_DAC_Vsh,255,ROC_REG);

      _registers["vcomp"]      = dacConfig(ROC_DAC_Vcomp,15,ROC_REG);

      _registers["vwllpr"]     = dacConfig(ROC_DAC_VwllPr,255,ROC_REG);
      _registers["fbpre"]      = dacConfig(ROC_DAC_VwllPr,255,ROC_REG,false);

      _registers["vwllsh"]     = dacConfig(ROC_DAC_VwllSh,255,ROC_REG);
      _registers["fbsh"]       = dacConfig(ROC_DAC_VwllSh,255,ROC_REG,false);

      _registers["vhlddel"]    = dacConfig(ROC_DAC_VhldDel,255,ROC_REG);
      _registers["holddel"]    = dacConfig(ROC_DAC_VhldDel,255,ROC_REG,false);

      _registers["vtrim"]      = dacConfig(ROC_DAC_Vtrim,255,ROC_REG);
      _registers["trimscale"]  = dacConfig(ROC_DAC_Vtrim,255,ROC_REG,false);

      _registers["vthrcomp"]   = dacConfig(ROC_DAC_VthrComp,255,ROC_REG);
      _registers["globalthr"]  = dacConfig(ROC_DAC_VthrComp,255,ROC_REG,false);

      _registers["vibias_bus"] = dacConfig(ROC_DAC_VIBias_Bus,255,ROC_REG);

      _registers["voffsetro"]  = dacConfig(ROC_DAC_VoffsetRO,255,ROC_REG,false);
      _registers["phoffset"]   = dacConfig(ROC_DAC_VoffsetRO,255,ROC_REG);

      _registers["vcomp_adc"]  = dacConfig(ROC_DAC_VIbias_PH,255,ROC_REG);
      _registers["vibias_ph"]  = dacConfig(ROC_DAC_VIbias_PH,255,ROC_REG,false);
      _registers["adcpower"]   = dacConfig(ROC_DAC_VIbias_PH,255,ROC_REG,false);

      _registers["viref_adc"]  = dacConfig(ROC_DAC_VIbias_DAC,255,ROC_REG,false);
      _registers["vibias_dac"] = dacConfig(ROC_DAC_VIbias_DAC,255,ROC_REG,false);
      _registers["ibias_dac"]  = dacConfig(ROC_DAC_VIbias_DAC,255,ROC_REG,false);
      _registers["phscale"]    = dacConfig(ROC_DAC_VIbias_DAC,255,ROC_REG);

      _registers["vicolor"]    = dacConfig(ROC_DAC_VIColOr,255,ROC_REG);

      _registers["vcal"]       = dacConfig(ROC_DAC_Vcal,255,ROC_REG);

      _registers["caldel"]     = dacConfig(ROC_DAC_CalDel,255,ROC_REG);

      _registers["ctrlreg"]    = dacConfig(ROC_DAC_CtrlReg,255,ROC_REG);
      _registers["ccr"]        = dacConfig(ROC_DAC_CtrlReg,255,ROC_REG,false);

      _registers["wbc"]        = dacConfig(ROC_DAC_WBC,255,ROC_REG);

      _registers["readback"]   = dacConfig(ROC_DAC_Readback,15,ROC_REG);
      _registers["rbreg"]      = dacConfig(ROC_DAC_Readback,15,ROC_REG,false);

      // DACs removed from psi46digV3 (?):
      _registers["vbias_sf"]   = dacConfig(ROC_DAC_Vbias_sf,15,ROC_REG);
      _registers["voffsetop"]  = dacConfig(ROC_DAC_VoffsetOp,255,ROC_REG);
      _registers["vion"]       = dacConfig(ROC_DAC_VIon,255,ROC_REG);

      // DACs removed from psi46digV2:
      _registers["vleak_comp"] = dacConfig(ROC_DAC_Vleak_comp,255,ROC_REG);
      _registers["vleak"] = dacConfig(ROC_DAC_Vleak_comp,255,ROC_REG);

      _registers["vrgpr"]      = dacConfig(ROC_DAC_VrgPr,255,ROC_REG);
      _registers["vrgsh"]      = dacConfig(ROC_DAC_VrgSh,255,ROC_REG);

      _registers["vibiasop"]   = dacConfig(ROC_DAC_VIbiasOp,255,ROC_REG);
      _registers["vbias_op"]   = dacConfig(ROC_DAC_VIbiasOp,255,ROC_REG,false);

      _registers["vibias_roc"]   = dacConfig(ROC_DAC_VIbias_roc,255,ROC_REG);
      _registers["vnpix"]   = dacConfig(ROC_DAC_Vnpix,255,ROC_REG);
      _registers["vsumcol"]   = dacConfig(ROC_DAC_VsumCol,255,ROC_REG);
    };

    std::map<std::string, dacConfig> _registers;
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
      _devices["psi46digv2.1"]  = ROC_PSI46DIGV21;
      // This name is not correct, but kept for legacy reasons:
      _devices["psi46digv3"]    = ROC_PSI46DIGV21;

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
