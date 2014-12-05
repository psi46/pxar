#pragma once

#include "rpc.h"
#include <vector>

#ifdef INTERFACE_USB
#include "USBInterface.h"
#endif /* INTERFACE_USB */

#ifdef INTERFACE_ETH
#include "EthernetInterface.h"
#endif /* INTERFACE_ETH */

class CTestboard
{
	RPC_DEFS
	RPC_THREAD

#ifdef INTERFACE_USB
  CUSB *usb;
#endif /* INTERFACE_USB */

#ifdef INTERFACE_ETH
  CEthernet *ethernet;
#endif /* INTERFACE_ETH */

  std::vector<CRpcIo*> interfaceList;

public:
	CRpcIo& GetIo() { return *rpc_io; }

	CTestboard() { 
	  RPC_INIT 

#ifdef INTERFACE_USB
	  usb = NULL;
#endif /* INTERFACE_USB */

#ifdef INTERFACE_ETH
	  ethernet = NULL;
#endif /* INTERFACE_ETH */
	}
	~CTestboard() { RPC_EXIT }

	int32_t GetHostRpcCallCount() { return rpc_cmdListSize; }
	bool GetHostRpcCallName(int32_t id, stringR &callName) { callName = rpc_cmdName[id]; return true; }
	std::vector<std::string> GetHostRpcCallNames() { 
	  std::vector<std::string> rpc_cmdList;
	  for(size_t i = 0; i < rpc_cmdListSize; i++) {
	    rpc_cmdList.push_back(rpc_cmdName[i]);
	  }
	  return rpc_cmdList;
	}

	// === RPC ==============================================================

	// Don't change the following two entries
	RPC_EXPORT uint16_t GetRpcVersion();
	RPC_EXPORT int32_t  GetRpcCallId(string &callName);

	RPC_EXPORT void GetRpcTimestamp(stringR &ts);

	RPC_EXPORT int32_t GetRpcCallCount();
	RPC_EXPORT bool    GetRpcCallName(int32_t id, stringR &callName);
	RPC_EXPORT uint32_t GetRpcCallHash();

	bool RpcLink() {

	  bool error = false;
	  for (unsigned short i = 2; i < rpc_cmdListSize; i++) {
	    try { rpc_GetCallId(i); }
	    catch (CRpcError &e) {
	      e.SetFunction(0);
	      if (!error) { LOG(pxar::logERROR) << "Missing DTB functions:"; }
	      std::string fname(rpc_cmdName[i]);
	      std::string fname_pretty;
	      rpc_TranslateCallName(fname, fname_pretty);
	      LOG(pxar::logERROR) << fname_pretty.c_str();
	      error = true;
	    }
	  }
	  return !error;
	}


	// === DTB connection ====================================================

	inline bool Open(string &name, bool init=true) {
	  rpc_Clear();
	  if (!rpc_io->Open(&(name[0]))) return false;
	  if (init) Init();
	  return true;
	}

	void Close() {
	  rpc_io->Close();
	  rpc_Clear();
	}

	void SelectInterface(CRpcIo * io) {
	  rpc_io = io;
	}

	bool SelectInterface(std::string ifaceName) {

	  bool ifaceFound = false;
	  for(std::vector<CRpcIo*>::iterator iface = interfaceList.begin(); iface != interfaceList.end(); iface++) {
	    try {
	      if(ifaceName == std::string((*iface)->Name())) {
		rpc_io = *iface;
		LOG(pxar::logDEBUGRPC) << "Assigned interface " << std::string((*iface)->Name());
		ifaceFound = true;
	      }
	    }
	    catch (CRpcError &e) {
	      LOG(pxar::logCRITICAL) << "Error querying interface " << std::string((*iface)->Name());
	      return false;
	    }
	  }

	  return ifaceFound;
	}

	void ClearInterface() {
	  rpc_io = &RpcIoNull;
	}

	bool EnumFirst(CRpcIo* io, unsigned int &nDevices) { return io->EnumFirst(nDevices); }
	bool EnumNext(CRpcIo* io, string &name) {
	  char s[64];
	  if (!io->EnumNext(s)) return false;
	  name = s;
	  return true;
	}
	bool Enum(CRpcIo* io, unsigned int pos, string &name) {
	  char s[64];
	  if (!io->Enum(s, pos)) return false;
	  name = s;
	  return true;
	}

	std::vector<CRpcIo*> GetInterfaceList() {
	  interfaceList.clear();

#ifdef INTERFACE_ETH
	  if(ethernet == NULL) {
	    try {
	      ethernet = new CEthernet();
	      interfaceList.push_back(ethernet);
	    }
	    catch(CRpcError e) {
	      LOG(pxar::logERROR) << "Error initiating ethernet. "
				  << "Please ensure proper permissions are granted.";
	    }
	  } else { interfaceList.push_back(ethernet); }
#endif /*INTERFACE_ETH*/

#ifdef INTERFACE_USB
	  if(usb == NULL) {
	    try {
	      usb = new CUSB();
	      interfaceList.push_back(usb);
	    }
	    catch(CRpcError e) {
	      LOG(pxar::logERROR) << "Error initiating usb. "
				  << "Please ensure proper permissions are granted.";
	    }
	  }
	  else { interfaceList.push_back(usb); }
#endif /*INTERFACE_USB*/

	  return interfaceList;
	}

	uint32_t GetInterfaceListSize() {

	  if(interfaceList.empty()) interfaceList = GetInterfaceList();
	  return interfaceList.size();
	}

	std::vector<std::pair<std::string,std::string> > GetDeviceList() {
	  std::vector<std::pair<std::string,std::string> > deviceList;
	  std::string name;
	  unsigned int nDev;
	  unsigned int nr;

	  for(std::vector<CRpcIo*>::iterator iface = interfaceList.begin(); iface != interfaceList.end(); iface++) {
	    try {
	      if (!EnumFirst(*iface,nDev)) continue;
	      for (nr = 0; nr < nDev; nr++) {
		if (!EnumNext(*iface,name)) continue;
		if (name.size() < 4) continue;
		if (name.compare(0, 4, "DTB_") == 0) deviceList.push_back(std::make_pair(std::string((*iface)->Name()),name));
	      }
	    }
	    catch (CRpcError &e) {
	      LOG(pxar::logCRITICAL) << "Error querying interface " << std::string((*iface)->Name());
	      //throw pxar::UsbConnectionError("Error querying interface " + std::string((*iface)->Name()));
	    }
	  }

	  return deviceList;
	}

	void SetTimeout(unsigned int timeout) { rpc_io->SetTimeout(timeout); }

	bool IsConnected() { return rpc_io->Connected(); }
	const char * ConnectionError()
	{ return rpc_io->GetErrorMsg(rpc_io->GetLastError()); }

	void Flush() { rpc_io->Flush(); }
	void Clear() { rpc_io->Clear(); }


	// === DTB identification ================================================

	RPC_EXPORT void GetInfo(stringR &info);
	RPC_EXPORT uint16_t GetBoardId();
	RPC_EXPORT void GetHWVersion(stringR &version);
	RPC_EXPORT uint16_t GetFWVersion();
	RPC_EXPORT uint16_t GetSWVersion();
	RPC_EXPORT uint16_t GetUser1Version();

	// === DTB service ======================================================

	// --- upgrade
	RPC_EXPORT uint16_t UpgradeGetVersion();
	RPC_EXPORT uint8_t  UpgradeStart(uint16_t version);
	RPC_EXPORT uint8_t  UpgradeData(string &record);
	RPC_EXPORT uint8_t  UpgradeError();
	RPC_EXPORT void     UpgradeErrorMsg(stringR &msg);
	RPC_EXPORT void     UpgradeExec(uint16_t recordCount);


	// === DTB functions ====================================================

	RPC_EXPORT void Init();
	RPC_EXPORT void Welcome();
	RPC_EXPORT void SetLed(uint8_t x);

	RPC_EXPORT uint16_t GetADC(uint8_t addr);


	// --- Clock, Timing ----------------------------------------------------
	RPC_EXPORT void cDelay(uint16_t clocks);
	RPC_EXPORT void uDelay(uint16_t us);


	// --- Signal Delay -----------------------------------------------------
	RPC_EXPORT void Sig_SetMode(uint8_t signal, uint8_t mode);
	RPC_EXPORT void Sig_SetPRBS(uint8_t signal, uint8_t speed);
	RPC_EXPORT void Sig_SetDelay(uint8_t signal, uint16_t delay, int8_t duty = 0);
	RPC_EXPORT void Sig_SetLevel(uint8_t signal, uint8_t level);
	RPC_EXPORT void Sig_SetOffset(uint8_t offset);
	RPC_EXPORT void Sig_SetLVDS();
	RPC_EXPORT void Sig_SetLCDS();
	RPC_EXPORT void Sig_SetRdaToutDelay(uint8_t delay);

	// --- Clock Settings ---------------------------------------------------
	RPC_EXPORT bool IsClockPresent();
	RPC_EXPORT void SetClock(uint8_t MHz);
	RPC_EXPORT void SetClockSource(uint8_t source);
	RPC_EXPORT void SetClockStretch(uint8_t src, uint16_t delay, uint16_t width);


	// --- digital signal probe ---------------------------------------------
	RPC_EXPORT void SignalProbeD1(uint8_t signal);
	RPC_EXPORT void SignalProbeD2(uint8_t signal);


	// --- analog signal probe ----------------------------------------------
	RPC_EXPORT void SignalProbeA1(uint8_t signal);
	RPC_EXPORT void SignalProbeA2(uint8_t signal);
	RPC_EXPORT void SignalProbeADC(uint8_t signal, uint8_t gain = 0);


	// --- ROC/Module power VD/VA -------------------------------------------
	RPC_EXPORT void Pon();	// switch ROC power on
	RPC_EXPORT void Poff();	// switch ROC power off

	RPC_EXPORT void _SetVD(uint16_t mV);
	RPC_EXPORT void _SetVA(uint16_t mV);
	RPC_EXPORT void _SetID(uint16_t uA100);
	RPC_EXPORT void _SetIA(uint16_t uA100);

	RPC_EXPORT uint16_t _GetVD();
	RPC_EXPORT uint16_t _GetVA();
	RPC_EXPORT uint16_t _GetID();
	RPC_EXPORT uint16_t _GetIA();

	RPC_EXPORT uint16_t _GetVD_Reg();
	RPC_EXPORT uint16_t _GetVDAC_Reg();
	RPC_EXPORT uint16_t _GetVD_Cap();

	RPC_EXPORT void HVon();
	RPC_EXPORT void HVoff();
	RPC_EXPORT void ResetOn();
	RPC_EXPORT void ResetOff();
	RPC_EXPORT uint8_t GetStatus();
	RPC_EXPORT void SetRocAddress(uint8_t addr);

	RPC_EXPORT bool GetPixelAddressInverted();
	RPC_EXPORT void SetPixelAddressInverted(bool status);


	// --- pulse pattern generator ------------------------------------------
	RPC_EXPORT void Pg_SetCmd(uint16_t addr, uint16_t cmd);
	RPC_EXPORT void Pg_SetCmdAll(vector<uint16_t> &cmd);
	RPC_EXPORT void Pg_SetSum(uint16_t delays);
	RPC_EXPORT void Pg_Stop();
	RPC_EXPORT void Pg_Single();
	RPC_EXPORT void Pg_Trigger();
	RPC_EXPORT void Pg_Triggers(uint32_t triggers, uint16_t period);
	RPC_EXPORT void Pg_Loop(uint16_t period);

	// --- data aquisition --------------------------------------------------
	RPC_EXPORT uint32_t Daq_Open(uint32_t buffersize, uint8_t channel); // max # of samples
	RPC_EXPORT void Daq_Close(uint8_t channel);
	RPC_EXPORT void Daq_Start(uint8_t channel);
	RPC_EXPORT void Daq_Stop(uint8_t channel);
	RPC_EXPORT uint32_t Daq_GetSize(uint8_t channel);
	RPC_EXPORT uint8_t Daq_FillLevel(uint8_t channel);
	RPC_EXPORT uint8_t Daq_FillLevel();
	RPC_EXPORT uint8_t Daq_Read(HWvectorR<uint16_t> &data, uint32_t blocksize = 65536, uint8_t channel = 0);
	RPC_EXPORT uint8_t Daq_Read(HWvectorR<uint16_t> &data, uint32_t blocksize, uint32_t &availsize, uint8_t channel = 0);
	

	RPC_EXPORT void Daq_Select_ADC(uint16_t blocksize, uint8_t source, uint8_t start, uint8_t stop = 0);
	RPC_EXPORT void Daq_Select_Deser160(uint8_t shift);
	RPC_EXPORT void Daq_Select_Deser400();
	RPC_EXPORT void Daq_Deser400_Reset(uint8_t reset);
	RPC_EXPORT void Daq_Deser400_OldFormat(bool old);
	RPC_EXPORT void Daq_DeselectAll();
	
	RPC_EXPORT void Daq_Select_Datagenerator(uint16_t startvalue);


	// --- ROC/module Communication -----------------------------------------
	// -- set the i2c address for the following commands
	RPC_EXPORT void roc_I2cAddr(uint8_t id);
	// -- sends "ClrCal" command to ROC
	RPC_EXPORT void roc_ClrCal();
	// -- sets a single (DAC) register
	RPC_EXPORT void roc_SetDAC(uint8_t reg, uint8_t value);

	// -- set pixel bits (count <= 60)
	//    M - - - 8 4 2 1
	RPC_EXPORT void roc_Pix(uint8_t col, uint8_t row, uint8_t value);

	// -- trimm a single pixel (count < =60)
	RPC_EXPORT void roc_Pix_Trim(uint8_t col, uint8_t row, uint8_t value);

	// -- mask a single pixel (count <= 60)
	RPC_EXPORT void roc_Pix_Mask(uint8_t col, uint8_t row);

	// -- set calibrate at specific column and row
	RPC_EXPORT void roc_Pix_Cal(uint8_t col, uint8_t row, bool sensor_cal = false);

	// -- enable/disable a double column
	RPC_EXPORT void roc_Col_Enable(uint8_t col, bool on);

	// -- enable/disable all double columns
	RPC_EXPORT void roc_AllCol_Enable(bool on);

	// -- mask all pixels of a column and the coresponding double column
	RPC_EXPORT void roc_Col_Mask(uint8_t col);

	// -- mask all pixels and columns of the chip
	RPC_EXPORT void roc_Chip_Mask();

	// == TBM functions =====================================================
	RPC_EXPORT bool TBM_Present(); 
	RPC_EXPORT void tbm_Enable(bool on);
	RPC_EXPORT void tbm_Addr(uint8_t hub, uint8_t port);
	RPC_EXPORT void mod_Addr(uint8_t hub);
	RPC_EXPORT void tbm_Set(uint8_t reg, uint8_t value);
	RPC_EXPORT bool tbm_Get(uint8_t reg, uint8_t &value);
	RPC_EXPORT bool tbm_GetRaw(uint8_t reg, uint32_t &value);

	// --- Wafer test functions
	RPC_EXPORT bool TestColPixel(uint8_t col, uint8_t trimbit, bool sensor_cal, vectorR<uint8_t> &res);

	// Ethernet test functions
	RPC_EXPORT void Ethernet_Send(string &message);
	RPC_EXPORT uint32_t Ethernet_RecvPackets();

	RPC_EXPORT void VectorTest(vector<uint16_t> &in, vectorR<uint16_t> &out);
	
	RPC_EXPORT int32_t CountReadouts(int32_t nTriggers);
	RPC_EXPORT int32_t CountReadouts(int32_t nTriggers, int32_t chipId);
	RPC_EXPORT int32_t CountReadouts(int32_t nTriggers, int32_t dacReg, int32_t dacValue);
	RPC_EXPORT int32_t PixelThreshold(int32_t col, int32_t row, int32_t start, int32_t step, int32_t thrLevel, int32_t nTrig, int32_t dacReg, bool xtalk, bool cals);
	RPC_EXPORT int32_t PH(int32_t col, int32_t row, int32_t trim, int16_t nTriggers);
	RPC_EXPORT bool test_pixel_address(int32_t col, int32_t row);
	RPC_EXPORT int32_t ChipEfficiency_dtb(int16_t nTriggers, vectorR<uint8_t> &res);

	RPC_EXPORT int8_t CalibratePixel(int16_t nTriggers, int16_t col, int16_t row, int16_t &nReadouts, int32_t &PHsum);
	RPC_EXPORT int8_t CalibrateDacScan(int16_t nTriggers, int16_t col, int16_t row, int16_t dacReg1, int16_t dacLower1, int16_t dacUpper1, vectorR<int16_t>& nReadouts, vectorR<int32_t> &PHsum);
	RPC_EXPORT int8_t CalibrateDacDacScan(int16_t nTriggers, int16_t  col, int16_t  row, int16_t  dacReg1, int16_t  dacLower1, int16_t  dacUpper1, int16_t  dacReg2, int16_t  dacLower2, int16_t  dacUpper2, vectorR<int16_t> &nReadouts, vectorR<int32_t> &PHsum);
	RPC_EXPORT void ParallelCalibrateDacDacScan(vector<uint8_t> &roc_i2c, uint16_t nTriggers, uint8_t col, uint8_t row, uint8_t dacReg1, uint8_t dacLower1, uint8_t dacUpper1, uint8_t dacReg2, uint8_t dacLower2, uint8_t dacUpper2, uint16_t flags);
	RPC_EXPORT int16_t CalibrateMap(int16_t nTriggers, vectorR<int16_t> &nReadouts, vectorR<int32_t> &PHsum, vectorR<uint32_t> &address);
	RPC_EXPORT int16_t CalibrateModule(vector<uint8_t> &roc_i2c, uint16_t nTriggers, uint16_t flags);
	RPC_EXPORT int16_t TrimChip(vector<int16_t> &trim);
	RPC_EXPORT int16_t TriggerRow(int16_t nTriggers, int16_t col, vector<int16_t> &rocs, int16_t delay=4);


	// == Wafer Test functions =====================================================
	RPC_EXPORT bool TestColPixel(uint8_t col, uint8_t trimbit, vectorR<uint8_t> &res);


	// == Trigger Loop functions for Host-side DAQ ROC/Module testing ==============
	// Exported RPC-Calls for the Trimbit storage setup:
	RPC_EXPORT bool SetI2CAddresses(std::vector<uint8_t> &roc_i2c);
	RPC_EXPORT bool SetTrimValues(uint8_t roc_i2c, std::vector<uint8_t> &trimvalues);
	
	RPC_EXPORT void SetLoopTriggerDelay(uint16_t delay);
	RPC_EXPORT void LoopInterruptReset();

	// Exported RPC-Calls for Maps
	RPC_EXPORT bool LoopMultiRocAllPixelsCalibrate(vector<uint8_t> &roc_i2c, uint16_t nTriggers, uint16_t flags);
	RPC_EXPORT bool LoopMultiRocOnePixelCalibrate(vector<uint8_t> &roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags);
	RPC_EXPORT bool LoopSingleRocAllPixelsCalibrate(uint8_t roc_i2c, uint16_t nTriggers, uint16_t flags);
	RPC_EXPORT bool LoopSingleRocOnePixelCalibrate(uint8_t roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags);

	  
	// Exported RPC-Calls for 1D DacScans
	RPC_EXPORT bool LoopMultiRocAllPixelsDacScan(vector<uint8_t> &roc_i2c, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high);
	RPC_EXPORT bool LoopMultiRocAllPixelsDacScan(vector<uint8_t> &roc_i2c, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1step, uint8_t dac1low, uint8_t dac1high);

	RPC_EXPORT bool LoopMultiRocOnePixelDacScan(vector<uint8_t> &roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high);
	RPC_EXPORT bool LoopMultiRocOnePixelDacScan(vector<uint8_t> &roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1step, uint8_t dac1low, uint8_t dac1high);

	RPC_EXPORT bool LoopSingleRocAllPixelsDacScan(uint8_t roc_i2c, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high);
	RPC_EXPORT bool LoopSingleRocAllPixelsDacScan(uint8_t roc_i2c, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1step, uint8_t dac1low, uint8_t dac1high);

	RPC_EXPORT bool LoopSingleRocOnePixelDacScan(uint8_t roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high);
	RPC_EXPORT bool LoopSingleRocOnePixelDacScan(uint8_t roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1step, uint8_t dac1low, uint8_t dac1high);


	// Exported RPC-Calls for 2D DacDacScans
	RPC_EXPORT bool LoopMultiRocAllPixelsDacDacScan(vector<uint8_t> &roc_i2c, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2low, uint8_t dac2high);
	RPC_EXPORT bool LoopMultiRocAllPixelsDacDacScan(vector<uint8_t> &roc_i2c, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1step, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2step, uint8_t dac2low, uint8_t dac2high);

	RPC_EXPORT bool LoopMultiRocOnePixelDacDacScan(vector<uint8_t> &roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2low, uint8_t dac2high);
	RPC_EXPORT bool LoopMultiRocOnePixelDacDacScan(vector<uint8_t> &roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1step, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2step, uint8_t dac2low, uint8_t dac2high);

	RPC_EXPORT bool LoopSingleRocAllPixelsDacDacScan(uint8_t roc_i2c, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2low, uint8_t dac2high);
	RPC_EXPORT bool LoopSingleRocAllPixelsDacDacScan(uint8_t roc_i2c, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1step, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2step, uint8_t dac2low, uint8_t dac2high);

	RPC_EXPORT bool LoopSingleRocOnePixelDacDacScan(uint8_t roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2low, uint8_t dac2high);
	RPC_EXPORT bool LoopSingleRocOnePixelDacDacScan(uint8_t roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1step, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2step, uint8_t dac2low, uint8_t dac2high);


	// Debug-RPC-Calls returnung a Checker Board Pattern
	RPC_EXPORT void LoopCheckerBoard(uint8_t roc_i2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1register, uint8_t dac1low, uint8_t dac1high, uint8_t dac2register, uint8_t dac2low, uint8_t dac2high);

};
