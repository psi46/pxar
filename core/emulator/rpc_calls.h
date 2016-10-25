#pragma once
#include <vector>
#include <map>

#include "log.h"
#include "constants.h"

class CRpcError {
 public:
  enum errorId {
    UNDEF
  } error;
  int functionId;
 CRpcError() : error(CRpcError::UNDEF), functionId(-1) {}
 CRpcError(errorId e) : error(e) {}
  void SetFunction(unsigned int cmdId) { functionId = cmdId; }
  const char *GetMsg();
  void What() {};
};

class CTestboard {

  uint16_t vd, va, id, ia;
  size_t nrocs_loops;
  std::vector<uint8_t> roci2c;
  uint8_t tbmtype;
  uint16_t trigger;

  uint32_t eventcounter;
  
  std::vector<std::vector<uint16_t> > daq_buffer; // Data buffers
  std::vector<bool> daq_status; // Channel status
  std::vector<size_t> daq_event; // Event counters

  std::vector<uint16_t> pg_setup; // pattern generator
  // hub map of core maps of registers
  std::map<uint8_t,std::map<uint8_t, std::map<uint8_t, uint8_t> > > tbm_registers;
  uint8_t active_tbm;

 public:
 CTestboard() : vd(0), va(0), id(0), ia(0),
    nrocs_loops(0), roci2c(), tbmtype(TBM_NONE),trigger(TRG_SEL_PG_DIR),
    eventcounter(0),
    daq_buffer(), daq_status(), daq_event(), tbm_registers(), active_tbm(0)
  {
    // Initialize all available DAQ channels:
    for(size_t i = 0; i < DTB_DAQ_CHANNELS; i++) {
      daq_buffer.push_back(std::vector<uint16_t>());
      daq_status.push_back(false);
      daq_event.push_back(0);
    }
  }
  ~CTestboard() { }

  int32_t GetHostRpcCallCount() { return 999; }
  std::vector<std::string> GetHostRpcCallNames() { return std::vector<std::string>(); }
  bool GetRpcCallName(int32_t, std::string &name) {
    name = "GetRpcCallHash$I";
    return false;
  };
  uint32_t GetRpcCallHash() { return 0x0; };
  bool RpcLink() { return true; }


  // === DTB connection ====================================================
  
  inline bool Open(std::string &, bool init=true) {
    if (init) Init();
    return true;
  }

  void Close() {}

  bool SelectInterface(std::string) {
    bool ifaceFound = false;
    return ifaceFound;
  }

  void ClearInterface() {}


  uint32_t GetInterfaceListSize() { return 1; }

  std::vector<std::pair<std::string,std::string> > GetDeviceList() {
    std::vector<std::pair<std::string,std::string> > deviceList;
    deviceList.push_back(std::make_pair("dtb_emulator","0x0"));
    return deviceList;
  }

  void SetTimeout(unsigned int) {}
  bool IsConnected() { return true; }
  const char * ConnectionError() { return "none."; }

  void Flush() { }
  void Clear() { }


  // === DTB identification ================================================

  void GetInfo(std::string &info);
  uint16_t GetBoardId();
  void GetHWVersion(std::string &version);
  uint16_t GetFWVersion();
  uint16_t GetSWVersion();
  uint16_t GetUser1Version();

  // === DTB service ======================================================

  // --- upgrade
  uint16_t UpgradeGetVersion();
  uint8_t  UpgradeStart(uint16_t version);
  uint8_t  UpgradeData(std::string &record);
  uint8_t  UpgradeError();
  void     UpgradeErrorMsg(std::string &msg);
  void     UpgradeExec(uint16_t recordCount);


  // === DTB functions ====================================================

  void Init();
  void Welcome();
  void SetLed(uint8_t x);

  uint16_t GetADC(uint8_t addr);


  // --- Clock, Timing ----------------------------------------------------
  void cDelay(uint16_t clocks);
  void uDelay(uint16_t us);


  // --- Signal Delay -----------------------------------------------------
  void Sig_SetMode(uint8_t signal, uint8_t mode);
  void Sig_SetPRBS(uint8_t signal, uint8_t speed);
  void Sig_SetDelay(uint8_t signal, uint16_t delay, int8_t duty = 0);
  void Sig_SetLevel(uint8_t signal, uint8_t level);
  void Sig_SetOffset(uint8_t offset);
  void Sig_SetLVDS();
  void Sig_SetLCDS();
  void Sig_SetRdaToutDelay(uint8_t delay);

  // --- Clock Settings ---------------------------------------------------
  bool IsClockPresent();
  void SetClock(uint8_t MHz);
  void SetClockSource(uint8_t source);
  void SetClockStretch(uint8_t src, uint16_t delay, uint16_t width);


  // --- digital signal probe ---------------------------------------------
  void SignalProbeD1(uint8_t signal);
  void SignalProbeD2(uint8_t signal);

  void SignalProbeDeserD1(uint8_t deser, uint8_t signal);
  void SignalProbeDeserD2(uint8_t deser, uint8_t signal);

  // --- analog signal probe ----------------------------------------------
  void SignalProbeA1(uint8_t signal);
  void SignalProbeA2(uint8_t signal);
  void SignalProbeADC(uint8_t signal, uint8_t gain = 0);


  // --- ROC/Module power VD/VA -------------------------------------------
  void Pon();	// switch ROC power on
  void Poff();	// switch ROC power off

  void _SetVD(uint16_t mV);
  void _SetVA(uint16_t mV);
  void _SetID(uint16_t uA100);
  void _SetIA(uint16_t uA100);

  uint16_t _GetVD();
  uint16_t _GetVA();
  uint16_t _GetID();
  uint16_t _GetIA();

  uint16_t _GetVD_Reg();
  uint16_t _GetVDAC_Reg();
  uint16_t _GetVD_Cap();

  void HVon();
  void HVoff();
  void ResetOn();
  void ResetOff();
  uint8_t GetStatus();
  void SetRocAddress(uint8_t addr);

  bool GetPixelAddressInverted();
  void SetPixelAddressInverted(bool status);


  // --- pulse pattern generator ------------------------------------------
  void Pg_SetCmd(uint16_t addr, uint16_t cmd);
  void Pg_SetCmdAll(std::vector<uint16_t> &cmd);
  void Pg_SetSum(uint16_t delays);
  void Pg_Stop();
  void Pg_Single();
  void Pg_Trigger();
  void Pg_Triggers(uint32_t triggers, uint16_t period);
  void Pg_Loop(uint16_t period);

  // --- trigger ----------------------------------------------------------
  void Trigger_Select(uint16_t mask);
  void Trigger_Delay(uint8_t delay);
  void Trigger_Timeout(uint16_t timeout);
  void Trigger_SetGenPeriodic(uint32_t periode);
  void Trigger_SetGenRandom(uint32_t rate);
  void Trigger_Send( uint8_t send);

  // --- data aquisition --------------------------------------------------
  uint32_t Daq_Open(uint32_t buffersize, uint8_t channel); // max # of samples
  void Daq_Close(uint8_t channel);
  void Daq_Start(uint8_t channel);
  void Daq_Stop(uint8_t channel);
  void Daq_MemReset(uint8_t channel);
  uint32_t Daq_GetSize(uint8_t channel);
  uint8_t Daq_FillLevel(uint8_t channel);
  uint8_t Daq_FillLevel();
  uint8_t Daq_Read(std::vector<uint16_t> &data, uint32_t blocksize = 65536, uint8_t channel = 0);
  uint8_t Daq_Read(std::vector<uint16_t> &data, uint32_t blocksize, uint32_t &availsize, uint8_t channel = 0);
	

  void Daq_Select_ADC(uint16_t blocksize, uint8_t source, uint8_t start, uint8_t stop = 0);
  void Daq_Select_Deser160(uint8_t shift);
  void Daq_Select_Deser400();
  void Daq_Deser400_Reset(uint8_t reset);
  void Daq_Deser400_OldFormat(bool old);
  void Deser400_GateRun(uint8_t width, uint8_t period);
  void Daq_DeselectAll();
	
  void Daq_Select_Datagenerator(uint16_t startvalue);

  // --- DESER400 configuration -------------------------------------------
  void Deser400_Enable(uint8_t deser);
  void Deser400_Disable(uint8_t deser);
  void Deser400_DisableAll();

  void Deser400_SetPhase(uint8_t deser, uint8_t phase);
  void Deser400_SetPhaseAuto(uint8_t deser);
  void Deser400_SetPhaseAutoAll();

  uint8_t Deser400_GetXor(uint8_t deser);
  uint8_t Deser400_GetPhase(uint8_t deser);


  // --- ROC/module Communication -----------------------------------------
  // -- set the i2c address for the following commands
  void roc_I2cAddr(uint8_t id);
  // -- sends "ClrCal" command to ROC
  void roc_ClrCal();
  // -- sets a single (DAC) register
  void roc_SetDAC(uint8_t reg, uint8_t value);

  // -- set pixel bits (count <= 60)
  //    M - - - 8 4 2 1
  void roc_Pix(uint8_t col, uint8_t row, uint8_t value);

  // -- trimm a single pixel (count < =60)
  void roc_Pix_Trim(uint8_t col, uint8_t row, uint8_t value);

  // -- mask a single pixel (count <= 60)
  void roc_Pix_Mask(uint8_t col, uint8_t row);

  // -- set calibrate at specific column and row
  void roc_Pix_Cal(uint8_t col, uint8_t row, bool sensor_cal = false);

  // -- enable/disable a double column
  void roc_Col_Enable(uint8_t col, bool on);

  // -- enable/disable all double columns
  void roc_AllCol_Enable(bool on);

  // -- mask all pixels of a column and the coresponding double column
  void roc_Col_Mask(uint8_t col);

  // -- mask all pixels and columns of the chip
  void roc_Chip_Mask();

  // == TBM functions =====================================================
  bool TBM_Present(); 
  void tbm_Enable(bool on);
  void tbm_Addr(uint8_t hub, uint8_t port);
  void mod_Addr(uint8_t hub);
  void mod_Addr(uint8_t hub0, uint8_t hub1);
  void tbm_Set(uint8_t reg, uint8_t value);
  void tbm_SelectRDA(uint8_t channel);
  bool tbm_Get(uint8_t reg, uint8_t &value);
  bool tbm_GetRaw(uint8_t reg, uint32_t &value);

  int16_t TrimChip(std::vector<int16_t> &trim);

  bool notokenpass(uint8_t tbmtype, uint8_t channel);
  
  // == Trigger Loop functions for Host-side DAQ ROC/Module testing ==============
  // Exported RPC-Calls for the Trimbit storage setup:
  bool SetI2CAddresses(std::vector<uint8_t> &roc_i2c);
  bool SetTrimValues(uint8_t roc_i2c, std::vector<uint8_t> &trimvalues);
	
  void SetLoopTriggerDelay(uint16_t delay);
  void SetLoopTrimDelay(uint16_t delay);
  void LoopInterruptReset();

  // Exported RPC-Calls for Maps
  bool LoopMultiRocAllPixelsCalibrate(std::vector<uint8_t> &roc_i2c, uint16_t nTriggers, uint16_t flags);
  bool LoopMultiRocOnePixelCalibrate(std::vector<uint8_t> &roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags);
  bool LoopSingleRocAllPixelsCalibrate(uint8_t roc_i2c, uint16_t nTriggers, uint16_t flags);
  bool LoopSingleRocOnePixelCalibrate(uint8_t roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags);

	  
  // Exported RPC-Calls for 1D DacScans
  bool LoopMultiRocAllPixelsDacScan(std::vector<uint8_t> &roc_i2c, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high);
  bool LoopMultiRocAllPixelsDacScan(std::vector<uint8_t> &roc_i2c, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1step, uint8_t dac1low, uint8_t dac1high);

  bool LoopMultiRocOnePixelDacScan(std::vector<uint8_t> &roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high);
  bool LoopMultiRocOnePixelDacScan(std::vector<uint8_t> &roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1step, uint8_t dac1low, uint8_t dac1high);

  bool LoopSingleRocAllPixelsDacScan(uint8_t roc_i2c, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high);
  bool LoopSingleRocAllPixelsDacScan(uint8_t roc_i2c, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1step, uint8_t dac1low, uint8_t dac1high);

  bool LoopSingleRocOnePixelDacScan(uint8_t roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high);
  bool LoopSingleRocOnePixelDacScan(uint8_t roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1step, uint8_t dac1low, uint8_t dac1high);


  // Exported RPC-Calls for 2D DacDacScans
  bool LoopMultiRocAllPixelsDacDacScan(std::vector<uint8_t> &roc_i2c, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2low, uint8_t dac2high);
  bool LoopMultiRocAllPixelsDacDacScan(std::vector<uint8_t> &roc_i2c, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1step, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2step, uint8_t dac2low, uint8_t dac2high);

  bool LoopMultiRocOnePixelDacDacScan(std::vector<uint8_t> &roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2low, uint8_t dac2high);
  bool LoopMultiRocOnePixelDacDacScan(std::vector<uint8_t> &roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1step, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2step, uint8_t dac2low, uint8_t dac2high);

  bool LoopSingleRocAllPixelsDacDacScan(uint8_t roc_i2c, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2low, uint8_t dac2high);
  bool LoopSingleRocAllPixelsDacDacScan(uint8_t roc_i2c, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1step, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2step, uint8_t dac2low, uint8_t dac2high);

  bool LoopSingleRocOnePixelDacDacScan(uint8_t roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2low, uint8_t dac2high);
  bool LoopSingleRocOnePixelDacDacScan(uint8_t roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1step, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2step, uint8_t dac2low, uint8_t dac2high);


  // Debug-RPC-Calls returnung a Checker Board Pattern
  void LoopCheckerBoard(uint8_t roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2low, uint8_t dac2high);

};
