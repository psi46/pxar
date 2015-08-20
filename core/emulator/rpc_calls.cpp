// RPC functions for the DTB emulator
#include "rpc_calls.h"
#include "generator.h"
#include "helper.h"
#include "config.h"
#include "constants.h"
#include <vector>

using namespace pxar;

void CTestboard::GetInfo(std::string &message) {
  LOG(pxar::logDEBUGRPC) << "called.";
  message = " pxarCore DTB Emulator \n "
    + std::string(PACKAGE_STRING)
    + "\n";
}

uint16_t CTestboard::GetBoardId() {
  LOG(pxar::logDEBUGRPC) << "called.";
  return 0x0;
}

void CTestboard::GetHWVersion(std::string &rpc_par1) {
  LOG(pxar::logDEBUGRPC) << "called.";
  rpc_par1 = "Hardware Revision 0";
}

uint16_t CTestboard::GetFWVersion() {
  LOG(pxar::logDEBUGRPC) << "called.";
  return 0x0;
}

uint16_t CTestboard::GetSWVersion() {
  LOG(pxar::logDEBUGRPC) << "called.";
  return 0x0;
}

uint16_t CTestboard::UpgradeGetVersion() {
  LOG(pxar::logDEBUGRPC) << "called.";
  return 0x0100;
}

uint8_t CTestboard::UpgradeStart(uint16_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
  return 0;
}

uint8_t CTestboard::UpgradeData(std::string &) {
  LOG(pxar::logDEBUGRPC) << "called.";
  return 0;
}

uint8_t CTestboard::UpgradeError() {
  LOG(pxar::logDEBUGRPC) << "called.";
  return 0;
}

void CTestboard::UpgradeErrorMsg(std::string &rpc_par1) {
  LOG(pxar::logDEBUGRPC) << "called.";
  rpc_par1 = "No error.";
}

void CTestboard::UpgradeExec(uint16_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Init() {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Welcome() {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::SetLed(uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::cDelay(uint16_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::uDelay(uint16_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
  pxar::mDelay(1);
}

// Emulator always has the same clock:
void CTestboard::SetClockSource(uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

bool CTestboard::IsClockPresent() {
  LOG(pxar::logDEBUGRPC) << "called.";
  return true;
}

void CTestboard::SetClock(uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::SetClockStretch(uint8_t, uint16_t, uint16_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Sig_SetMode(uint8_t, uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Sig_SetPRBS(uint8_t, uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Sig_SetDelay(uint8_t, uint16_t, int8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Sig_SetLevel(uint8_t, uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Sig_SetOffset(uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Sig_SetLVDS() {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Sig_SetLCDS() {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Sig_SetRdaToutDelay(uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::SignalProbeD1(uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::SignalProbeD2(uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::SignalProbeA1(uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::SignalProbeA2(uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::SignalProbeADC(uint8_t, uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Pon() {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Poff() {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::_SetVD(uint16_t voltage) {
  LOG(pxar::logDEBUGRPC) << "called.";
  vd = voltage;
}

void CTestboard::_SetVA(uint16_t voltage) {
  LOG(pxar::logDEBUGRPC) << "called.";
  va = voltage;
}

void CTestboard::_SetID(uint16_t current) {
  LOG(pxar::logDEBUGRPC) << "called.";
  id = current;
}

void CTestboard::_SetIA(uint16_t current) {
  LOG(pxar::logDEBUGRPC) << "called.";
  ia = current;
}

uint16_t CTestboard::_GetVD() {
  LOG(pxar::logDEBUGRPC) << "called.";
  return vd;
}

uint16_t CTestboard::_GetVA() {
  LOG(pxar::logDEBUGRPC) << "called.";
  return va;
}

uint16_t CTestboard::_GetID() {
  LOG(pxar::logDEBUGRPC) << "called.";
  return id;
}

uint16_t CTestboard::_GetIA() {
  LOG(pxar::logDEBUGRPC) << "called.";
  return ia;
}

uint16_t CTestboard::_GetVD_Reg() {
  LOG(pxar::logDEBUGRPC) << "called.";
  return vd;
}

uint16_t CTestboard::_GetVDAC_Reg() {
  LOG(pxar::logDEBUGRPC) << "called.";
  return vd;
}

uint16_t CTestboard::_GetVD_Cap() {
  LOG(pxar::logDEBUGRPC) << "called.";
  return vd;
}

void CTestboard::HVon() {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::HVoff() {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::ResetOn() {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::ResetOff() {
  LOG(pxar::logDEBUGRPC) << "called.";
}

uint8_t CTestboard::GetStatus() {
  LOG(pxar::logDEBUGRPC) << "called.";
  return 0;
}

void CTestboard::SetRocAddress(uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Pg_SetCmd(uint16_t, uint16_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

// FIXME PG could be used for real.
void CTestboard::Pg_SetCmdAll(std::vector<uint16_t> &pg) {
  LOG(pxar::logDEBUGRPC) << "called.";
  // Store pattern generator:
  pg_setup = std::vector<uint16_t>(pg);
  LOG(logDEBUGRPC) << "Pattern Generator is:" << listVector(pg_setup);
}

void CTestboard::Pg_SetSum(uint16_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Pg_Stop() {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Pg_Single() {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Pg_Trigger() {
  LOG(pxar::logDEBUGRPC) << "called.";
}

// Receiving triggers for PG:
void CTestboard::Pg_Triggers(uint32_t nTriggers, uint16_t) {
  LOG(pxar::logDEBUGRPC) << "called.";

  // Check how many open DAQ channels we have:
  size_t channels = std::count(daq_status.begin(), daq_status.end(), true);
  // Distribute the ROCs evenly:
  size_t roc_per_ch = roci2c.size()/channels;

  for(size_t i = 0; i < nTriggers; i++) {
    for(size_t ch = 0; ch < channels; ch++) {
      size_t rocs = notokenpass(tbmtype,ch) ? 0 : roc_per_ch;
      fillRawData(i,daq_buffer.at(ch),tbmtype,rocs,false,true,0,0);
    }
  }
}

// FIXME Receiving loop command
void CTestboard::Pg_Loop(uint16_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
  // Set DAQ into state where it always returns events.
}

// Trigger selection
void CTestboard::Trigger_Select(uint16_t src) {
  LOG(pxar::logDEBUGRPC) << "called.";
  // Store trigger:
  trigger = src;
  // Triggers via TBM Emulator:
  if((src & 0x00f0) != 0 || src == 0x0100) tbmtype = TBM_EMU;
}

void CTestboard::Trigger_Delay(uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Trigger_Timeout(uint16_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Trigger_SetGenPeriodic(uint32_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Trigger_SetGenRandom(uint32_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Trigger_Send(uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

// DAQ Open
uint32_t CTestboard::Daq_Open(uint32_t buffersize, uint8_t channel) {
  LOG(pxar::logDEBUGRPC) << "called.";

  if(channel > daq_buffer.size()) return 0;

  // more than 2 channels? TBM09!
  if(channel > 2) tbmtype = TBM_09;
  
  // Reserve some memory (not necessary but nice...)
  // Dividing by 2 since we're talking abour 16bit words here, not bytes:
  daq_buffer.at(channel).reserve(buffersize/2);
  return daq_buffer.at(channel).capacity()*2;
}

void CTestboard::Daq_Close(uint8_t channel) {
  LOG(pxar::logDEBUGRPC) << "called.";
  // Clear DAQ buffer
  daq_buffer.at(channel).clear();
}

void CTestboard::Daq_Start(uint8_t channel) {
  LOG(pxar::logDEBUGRPC) << "called.";
  daq_status.at(channel) = true;
  daq_event.at(channel) = 0;
}

void CTestboard::Daq_Stop(uint8_t channel) {
  LOG(pxar::logDEBUGRPC) << "called.";
  daq_status.at(channel) = false;
}

void CTestboard::Daq_MemReset(uint8_t channel) {
  LOG(pxar::logDEBUGRPC) << "called.";
  daq_buffer.at(channel).clear();
}

uint32_t CTestboard::Daq_GetSize(uint8_t channel) {
  LOG(pxar::logDEBUGRPC) << "called.";
  if(daq_status.at(channel)) return daq_buffer.at(channel).size();
  else return 0;
}

uint8_t CTestboard::Daq_FillLevel(uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
  // We are always on 30%:
  return 30;
}

uint8_t CTestboard::Daq_FillLevel() {
  LOG(pxar::logDEBUGRPC) << "called.";
  // We are always on 30%:
  return 30;
}

uint8_t CTestboard::Daq_Read(std::vector<uint16_t> &data, uint32_t blocksize, uint8_t channel) {
  LOG(pxar::logDEBUGRPC) << "called.";
  uint32_t available = 0;
  return Daq_Read(data, blocksize, available, channel);
}

uint8_t CTestboard::Daq_Read(std::vector<uint16_t> &data, uint32_t blocksize, uint32_t &available, uint8_t channel) {
  LOG(pxar::logDEBUGRPC) << "called.";
  data.clear();

  // Fake buffer empty after 1k events:
  if(eventcounter >= 1000) {
    eventcounter = 0;
    return 0;
  }
  
  // If we are on external triggers, just deliver one event per channel:
  if(trigger == TRG_SEL_ASYNC || trigger == TRG_SEL_ASYNC_DIR) {
    eventcounter++;
    LOG(logDEBUGRPC) << "Event counter: " << eventcounter;
    if(!daq_status.at(channel)) { available = 0; return 0; }
    fillRawData(daq_event.at(channel)++,daq_buffer.at(channel),channel,tbmtype,roci2c.size(),false,true,0,0,pg_setup);
    mDelay(10);
  }

  // Return correct blocksize. Since this is given in Bytes,
  // we deliver blocksize/2 16bit words:

  std::vector<uint16_t>::iterator copy_end =
    (blocksize/2 < daq_buffer.at(channel).size() ? (daq_buffer.at(channel).begin() + blocksize/2) : daq_buffer.at(channel).end());

  data.insert(data.end(),daq_buffer.at(channel).begin(),copy_end);
  daq_buffer.at(channel).erase(daq_buffer.at(channel).begin(),copy_end);
  
  return 0;
}

void CTestboard::Daq_Select_ADC(uint16_t, uint8_t, uint8_t, uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Daq_Select_Deser160(uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Daq_Select_Deser400() {
  LOG(pxar::logDEBUGRPC) << "called.";
  // Produce TBM headers!
}

void CTestboard::Daq_Deser400_Reset(uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Daq_Deser400_OldFormat(bool) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Daq_Select_Datagenerator(uint16_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::Daq_DeselectAll() {
  LOG(pxar::logDEBUGRPC) << "called.";
}

// Collect all ROCs that have ever been programmed:
void CTestboard::roc_I2cAddr(uint8_t i2c) {
  LOG(pxar::logDEBUGRPC) << "called.";
  std::vector<uint8_t>::iterator thisroc = std::find(roci2c.begin(),roci2c.end(),i2c);
  if(thisroc == roci2c.end()) roci2c.push_back(i2c);
}

void CTestboard::roc_ClrCal() {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::roc_SetDAC(uint8_t, uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::roc_Pix(uint8_t, uint8_t, uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::roc_Pix_Trim(uint8_t, uint8_t, uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::roc_Pix_Mask(uint8_t, uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::roc_Pix_Cal(uint8_t, uint8_t, bool) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::roc_Col_Enable(uint8_t, bool) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::roc_AllCol_Enable(bool) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::roc_Col_Mask(uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::roc_Chip_Mask() {
  LOG(pxar::logDEBUGRPC) << "called.";
}

bool CTestboard::TBM_Present() {
  LOG(pxar::logDEBUGRPC) << "called.";
  return (tbmtype != TBM_NONE);
}

void CTestboard::tbm_Enable(bool tbm) {
  LOG(pxar::logDEBUGRPC) << "called.";
  if(tbm) tbmtype = TBM_08;
  else tbmtype = TBM_NONE;
}

void CTestboard::tbm_Addr(uint8_t, uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::mod_Addr(uint8_t hubid) {
  LOG(pxar::logDEBUGRPC) << "called.";
  // Add this TBM to the map:
  std::map<uint8_t, uint8_t> regmap;
  std::map<uint8_t, std::map<uint8_t,uint8_t> > coremap;
  coremap.insert(std::make_pair(0xE0,regmap));
  coremap.insert(std::make_pair(0xF0,regmap));
  tbm_registers.insert(std::make_pair(hubid,coremap));

  // Set it active:
  active_tbm = hubid;
}

void CTestboard::mod_Addr(uint8_t, uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::tbm_Set(uint8_t reg, uint8_t val) {
  LOG(pxar::logDEBUGRPC) << "called.";

  LOG(logDEBUGRPC) << "Set " << (int)active_tbm << " " << std::hex << (int)(reg&0xF0) << " " << (int)(reg&0x0F) << " " << (int)val;
  
  std::pair<std::map<uint8_t,uint8_t>::iterator,bool> ret;
  ret = tbm_registers[active_tbm][reg&0xF0].insert(std::make_pair(reg&0x0F,val));
  if(ret.second == false) {
    LOG(logDEBUGRPC) << "Overwriting existing DAC \"" << (int)(reg&0x0F)
		     << "\" value " << static_cast<int>(ret.first->second)
		     << " with " << static_cast<int>(val);
    tbm_registers[active_tbm][reg&0xF0][reg&0x0F] = val;
  }
}

bool CTestboard::tbm_Get(uint8_t, uint8_t &) {
  LOG(pxar::logDEBUGRPC) << "called.";
  return true;
}

bool CTestboard::tbm_GetRaw(uint8_t, uint32_t &) {
  LOG(pxar::logDEBUGRPC) << "called.";
  return true;
}

int16_t CTestboard::TrimChip(std::vector<int16_t> &) {
  LOG(pxar::logDEBUGRPC) << "called.";
  return 0;
}

// FIXME do we emulate loop interrupts?
void CTestboard::LoopInterruptReset() {
  LOG(pxar::logDEBUGRPC) << "called.";
}

void CTestboard::SetLoopTriggerDelay(uint16_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
}

bool CTestboard::SetI2CAddresses(std::vector<uint8_t> &rpc_par1) {
  LOG(pxar::logDEBUGRPC) << "called.";
  nrocs_loops = rpc_par1.size();
  return true;
}

// FIXME here we could implement masked pixels
bool CTestboard::SetTrimValues(uint8_t, std::vector<uint8_t> &) {
  LOG(pxar::logDEBUGRPC) << "called.";
  return true;
}

bool CTestboard::notokenpass(uint8_t tbmtype, uint8_t channel) {

  // No or emulated TBM - token passes:
  if(tbmtype <= TBM_EMU) return false;
  
  // Check for NTP bit on this TBM core:
  uint8_t channels_per_tbm = (tbmtype >= TBM_09 ? 4 : 2);
  // Maps are sorted:
  uint8_t hubid = (channel/channels_per_tbm ? tbm_registers.rbegin()->first : tbm_registers.begin()->first);
  uint8_t core = ((channel%channels_per_tbm)/(channels_per_tbm/2) > 0 ? 0xF0 : 0xE0);
  return (tbm_registers.at(hubid).at(core)[0x0]&0x40);
}

bool CTestboard::LoopMultiRocAllPixelsCalibrate(std::vector<uint8_t> &roci2cs, uint16_t nTriggers, uint16_t flags) {
  LOG(pxar::logDEBUGRPC) << "called.";

  // Check how many open DAQ channels we have:
  size_t channels = std::count(daq_status.begin(), daq_status.end(), true);
  // Distribute the ROCs evenly:
  size_t roc_per_ch = (roci2cs.size()/channels);

  uint32_t event = 0;
  for(size_t i = 0; i < ROC_NUMCOLS; i++) {
    for(size_t j = 0; j < ROC_NUMROWS; j++) {
      for(size_t k = 0; k < nTriggers; k++) {
	for(size_t ch = 0; ch < channels; ch++) {
	  size_t rocs = notokenpass(tbmtype,ch) ? 0 : roc_per_ch;
	  fillRawData(event,daq_buffer.at(ch),tbmtype,rocs,false,false,i,j,pg_setup,flags);
	}
	event++;
      }
    }
  }
  
  return 1;
}

bool CTestboard::LoopMultiRocOnePixelCalibrate(std::vector<uint8_t> &roci2cs, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags) {
  LOG(pxar::logDEBUGRPC) << "called.";

  // Check how many open DAQ channels we have:
  size_t channels = std::count(daq_status.begin(), daq_status.end(), true);
  // Distribute the ROCs evenly:
  size_t roc_per_ch = roci2cs.size()/channels;

  uint32_t event = 0;
  for(size_t k = 0; k < nTriggers; k++) {
    for(size_t ch = 0; ch < channels; ch++) {
      fillRawData(event,daq_buffer.at(ch),tbmtype,roc_per_ch,false,false,column,row,pg_setup,flags);
    }
    event++;
  }
  
  return 1;
}

bool CTestboard::LoopSingleRocAllPixelsCalibrate(uint8_t, uint16_t nTriggers, uint16_t flags) {
  LOG(pxar::logDEBUGRPC) << "called.";
  
  uint32_t event = 0;
  for(size_t i = 0; i < ROC_NUMCOLS; i++) {
    for(size_t j = 0; j < ROC_NUMROWS; j++) {
      for(size_t k = 0; k < nTriggers; k++) {
	fillRawData(event,daq_buffer.at(0),tbmtype,1,false,false,i,j,pg_setup,flags);
	event++;
      }
    }
  }
  
  return 1;
}

bool CTestboard::LoopSingleRocOnePixelCalibrate(uint8_t, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags) {
  LOG(pxar::logDEBUGRPC) << "called.";

  uint32_t event = 0;
  for(size_t k = 0; k < nTriggers; k++) {
    fillRawData(event,daq_buffer.at(0),tbmtype,1,false,false,column,row,pg_setup,flags);
    event++;
  }
  
  return 1;
}

bool CTestboard::LoopMultiRocAllPixelsDacScan(std::vector<uint8_t> &roci2cs, uint16_t nTriggers, uint16_t flags, uint8_t dacreg, uint8_t dacmin, uint8_t dacmax) {
  return LoopMultiRocAllPixelsDacScan(roci2cs, nTriggers, flags, dacreg, 1, dacmin, dacmax);
}

bool CTestboard::LoopMultiRocAllPixelsDacScan(std::vector<uint8_t> &roci2cs, uint16_t nTriggers, uint16_t flags, uint8_t, uint8_t dacstep, uint8_t dacmin, uint8_t dacmax) {
  LOG(pxar::logDEBUGRPC) << "called.";

  // Check how many open DAQ channels we have:
  size_t channels = std::count(daq_status.begin(), daq_status.end(), true);
  // Distribute the ROCs evenly:
  size_t roc_per_ch = roci2cs.size()/channels;

  uint8_t dachalf = static_cast<uint8_t>(dacmax-dacmin)/2;
  uint32_t event = 0;

  for(size_t i = 0; i < ROC_NUMCOLS; i++) {
    for(size_t j = 0; j < ROC_NUMROWS; j++) {
      for(size_t dac = 0; dac < static_cast<size_t>(dacmax-dacmin+1); dac += dacstep) {
	for(size_t k = 0; k < nTriggers; k++) {
	  for(size_t ch = 0; ch < channels; ch++) {
	    // Mimic some edge at 50% of the DAC range:
	    if(((flags&FLAG_RISING_EDGE) && dac > dachalf) || (!(flags&FLAG_RISING_EDGE) && dac < dachalf)) {
	      fillRawData(event,daq_buffer.at(ch),tbmtype,roc_per_ch,false,false,i,j,pg_setup,flags);
	    }
	    else { fillRawData(event,daq_buffer.at(ch),tbmtype,roc_per_ch,true, false,i,j,pg_setup,flags); }
	  }
	  event++;
	}
      }
    }
  }
  
  return 1;
}

bool CTestboard::LoopMultiRocOnePixelDacScan(std::vector<uint8_t> &roci2cs, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dacreg, uint8_t dacmin, uint8_t dacmax) {
  return LoopMultiRocOnePixelDacScan(roci2cs, column, row, nTriggers, flags, dacreg, 1, dacmin, dacmax);
}

bool CTestboard::LoopMultiRocOnePixelDacScan(std::vector<uint8_t> &roci2cs, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t, uint8_t dacstep, uint8_t dacmin, uint8_t dacmax) {
  LOG(pxar::logDEBUGRPC) << "called.";
  
  // Check how many open DAQ channels we have:
  size_t channels = std::count(daq_status.begin(), daq_status.end(), true);
  // Distribute the ROCs evenly:
  size_t roc_per_ch = roci2cs.size()/channels;

  uint8_t dachalf = static_cast<uint8_t>(dacmax-dacmin)/2;
  uint32_t event = 0;

  for(size_t dac = 0; dac < static_cast<size_t>(dacmax-dacmin+1); dac += dacstep) {
    for(size_t k = 0; k < nTriggers; k++) {
      for(size_t ch = 0; ch < channels; ch++) {
	// Mimic some edge at 50% of the DAC range:
	if(((flags&FLAG_RISING_EDGE) && dac > dachalf) || (!(flags&FLAG_RISING_EDGE) && dac < dachalf)) {
	  fillRawData(event,daq_buffer.at(ch),tbmtype,roc_per_ch,false,false,column,row,pg_setup,flags);
	}
	else { fillRawData(event,daq_buffer.at(ch),tbmtype,roc_per_ch,true, false,column,row,pg_setup,flags); }
      }
      event++;
    }
  }
  
  return 1;
}

bool CTestboard::LoopSingleRocAllPixelsDacScan(uint8_t roci2c, uint16_t nTriggers, uint16_t flags, uint8_t dacreg, uint8_t dacmin, uint8_t dacmax) {
  return LoopSingleRocAllPixelsDacScan(roci2c, nTriggers, flags, dacreg, 1, dacmin, dacmax);
}

bool CTestboard::LoopSingleRocAllPixelsDacScan(uint8_t, uint16_t nTriggers, uint16_t flags, uint8_t, uint8_t dacstep, uint8_t dacmin, uint8_t dacmax) {
  LOG(pxar::logDEBUGRPC) << "called.";

  uint8_t dachalf = static_cast<uint8_t>(dacmax-dacmin)/2;
  uint32_t event = 0;

  for(size_t i = 0; i < ROC_NUMCOLS; i++) {
    for(size_t j = 0; j < ROC_NUMROWS; j++) {
      for(size_t dac = 0; dac < static_cast<size_t>(dacmax-dacmin+1); dac += dacstep) {
	for(size_t k = 0; k < nTriggers; k++) {
	  // Mimic some edge at 50% of the DAC range:
	  if(((flags&FLAG_RISING_EDGE) && dac > dachalf) || (!(flags&FLAG_RISING_EDGE) && dac < dachalf)) {
	    fillRawData(event,daq_buffer.at(0),tbmtype,1,false,false,i,j,pg_setup,flags);
	  }
	  else { fillRawData(event,daq_buffer.at(0),tbmtype,1,true, false,i,j,pg_setup,flags); }
	  event++;
	}
      }
    }
  }
  
  return 1;
}

bool CTestboard::LoopSingleRocOnePixelDacScan(uint8_t roci2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dacreg, uint8_t dacmin, uint8_t dacmax) {
  return LoopSingleRocOnePixelDacScan(roci2c, column, row, nTriggers, flags, dacreg, 1, dacmin, dacmax);
}

bool CTestboard::LoopSingleRocOnePixelDacScan(uint8_t, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t, uint8_t dacstep, uint8_t dacmin, uint8_t dacmax) {
  LOG(pxar::logDEBUGRPC) << "called.";
  
  uint8_t dachalf = static_cast<uint8_t>(dacmax-dacmin)/2;
  uint32_t event = 0;

  for(size_t dac = 0; dac < static_cast<size_t>(dacmax-dacmin+1); dac += dacstep) {
    for(size_t k = 0; k < nTriggers; k++) {
      // Mimic some edge at 50% of the DAC range:
      if(((flags&FLAG_RISING_EDGE) && dac > dachalf) || (!(flags&FLAG_RISING_EDGE) && dac < dachalf)) {
	fillRawData(event,daq_buffer.at(0),tbmtype,1,false,false,column,row,pg_setup,flags);
      }
      else { fillRawData(event,daq_buffer.at(0),tbmtype,1,true, false,column,row,pg_setup,flags); }
    }
    event++;
  }
  
  return 1;
}

bool CTestboard::LoopMultiRocAllPixelsDacDacScan(std::vector<uint8_t> &roci2cs, uint16_t nTriggers, uint16_t flags, uint8_t dac1reg, uint8_t dac1min, uint8_t dac1max, uint8_t dac2reg, uint8_t dac2min, uint8_t dac2max) {
  return LoopMultiRocAllPixelsDacDacScan(roci2cs, nTriggers, flags, dac1reg, 1, dac1min, dac1max, dac2reg, 1, dac2min, dac2max);
}

bool CTestboard::LoopMultiRocAllPixelsDacDacScan(std::vector<uint8_t> &roci2cs, uint16_t nTriggers, uint16_t flags, uint8_t, uint8_t dac1step, uint8_t dac1min, uint8_t dac1max, uint8_t, uint8_t dac2step, uint8_t dac2min, uint8_t dac2max) {
  LOG(pxar::logDEBUGRPC) << "called.";

  // Check how many open DAQ channels we have:
  size_t channels = std::count(daq_status.begin(), daq_status.end(), true);
  // Distribute the ROCs evenly:
  size_t roc_per_ch = roci2cs.size()/channels;

  uint32_t event = 0;

  for(size_t i = 0; i < ROC_NUMCOLS; i++) {
    for(size_t j = 0; j < ROC_NUMROWS; j++) {
      for(size_t dac1 = 0; dac1 < static_cast<size_t>(dac1max-dac1min+1); dac1 += dac1step) {
	for(size_t dac2 = 0; dac2 < static_cast<size_t>(dac2max-dac2min+1); dac2 += dac2step) {
	  for(size_t k = 0; k < nTriggers; k++) {
	    for(size_t ch = 0; ch < channels; ch++) {
	      // Mimic some working band of the two DACs:
	      if(isInTornadoRegion(dac1min, dac1max, dac1, dac2min, dac2max, dac2)) {
		fillRawData(event,daq_buffer.at(ch),tbmtype,roc_per_ch,false,false,i,j,pg_setup,flags);
	      }
	      else { fillRawData(event,daq_buffer.at(ch),tbmtype,roc_per_ch,true, false,i,j,pg_setup,flags); }
	    }
	    event++;
	  }
	}
      }
    }
  }
  
  return 1;
}

bool CTestboard::LoopMultiRocOnePixelDacDacScan(std::vector<uint8_t> &roci2cs, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1reg, uint8_t dac1min, uint8_t dac1max, uint8_t dac2reg, uint8_t dac2min, uint8_t dac2max) {
  return LoopMultiRocOnePixelDacDacScan(roci2cs, column, row, nTriggers, flags, dac1reg, 1, dac1min, dac1max, dac2reg, 1, dac2min, dac2max);
}

bool CTestboard::LoopMultiRocOnePixelDacDacScan(std::vector<uint8_t> &roci2cs, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t, uint8_t dac1step, uint8_t dac1min, uint8_t dac1max, uint8_t, uint8_t dac2step, uint8_t dac2min, uint8_t dac2max) {
  LOG(pxar::logDEBUGRPC) << "called.";

  // Check how many open DAQ channels we have:
  size_t channels = std::count(daq_status.begin(), daq_status.end(), true);
  // Distribute the ROCs evenly:
  size_t roc_per_ch = roci2cs.size()/channels;

  uint32_t event = 0;

  for(size_t dac1 = 0; dac1 < static_cast<size_t>(dac1max-dac1min+1); dac1 += dac1step) {
    for(size_t dac2 = 0; dac2 < static_cast<size_t>(dac2max-dac2min+1); dac2 += dac2step) {
      for(size_t k = 0; k < nTriggers; k++) {
	for(size_t ch = 0; ch < channels; ch++) {
	  // Mimic some working band of the two DACs:
	  if(isInTornadoRegion(dac1min, dac1max, dac1, dac2min, dac2max, dac2)) {
	    fillRawData(event,daq_buffer.at(ch),tbmtype,roc_per_ch,false,false,column,row,pg_setup,flags);
	  }
	  else { fillRawData(event,daq_buffer.at(ch),tbmtype,roc_per_ch,true, false,column,row,pg_setup,flags); }
	}
	event++;
      }
    }
  }
  
  return 1;
}

bool CTestboard::LoopSingleRocAllPixelsDacDacScan(uint8_t roci2c, uint16_t nTriggers, uint16_t flags, uint8_t dac1reg, uint8_t dac1min, uint8_t dac1max, uint8_t dac2reg, uint8_t dac2min, uint8_t dac2max) {
  return LoopSingleRocAllPixelsDacDacScan(roci2c, nTriggers, flags, dac1reg, 1, dac1min, dac1max, dac2reg, 1, dac2min, dac2max);
}

bool CTestboard::LoopSingleRocAllPixelsDacDacScan(uint8_t, uint16_t nTriggers, uint16_t flags, uint8_t, uint8_t dac1step, uint8_t dac1min, uint8_t dac1max, uint8_t, uint8_t dac2step, uint8_t dac2min, uint8_t dac2max) {
  LOG(pxar::logDEBUGRPC) << "called.";

  uint32_t event = 0;

  for(size_t i = 0; i < ROC_NUMCOLS; i++) {
    for(size_t j = 0; j < ROC_NUMROWS; j++) {
      for(size_t dac1 = 0; dac1 < static_cast<size_t>(dac1max-dac1min+1); dac1 += dac1step) {
	for(size_t dac2 = 0; dac2 < static_cast<size_t>(dac2max-dac2min+1); dac2 += dac2step) {
	  for(size_t k = 0; k < nTriggers; k++) {
	    // Mimic some working band of the two DACs:
	    if(isInTornadoRegion(dac1min, dac1max, dac1, dac2min, dac2max, dac2)) {
	      fillRawData(event,daq_buffer.at(0),tbmtype,1,false,false,i,j,pg_setup,flags);
	    }
	    else { fillRawData(event,daq_buffer.at(0),tbmtype,1,true, false,i,j,pg_setup,flags); }
	  }
	  event++;
	}
      }
    }
  }
  
  return 1;
}

bool CTestboard::LoopSingleRocOnePixelDacDacScan(uint8_t roci2c, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t dac1reg, uint8_t dac1min, uint8_t dac1max, uint8_t dac2reg, uint8_t dac2min, uint8_t dac2max) {
  return LoopSingleRocOnePixelDacDacScan(roci2c, column, row, nTriggers, flags, dac1reg, 1, dac1min, dac1max, dac2reg, 1, dac2min, dac2max);
}

bool CTestboard::LoopSingleRocOnePixelDacDacScan(uint8_t, uint8_t column, uint8_t row, uint16_t nTriggers, uint16_t flags, uint8_t, uint8_t dac1step, uint8_t dac1min, uint8_t dac1max, uint8_t, uint8_t dac2step, uint8_t dac2min, uint8_t dac2max) {
  LOG(pxar::logDEBUGRPC) << "called.";

  uint32_t event = 0;

  for(size_t dac1 = 0; dac1 < static_cast<size_t>(dac1max-dac1min+1); dac1 += dac1step) {
    for(size_t dac2 = 0; dac2 < static_cast<size_t>(dac2max-dac2min+1); dac2 += dac2step) {
      for(size_t k = 0; k < nTriggers; k++) {
	// Mimic some working band of the two DACs:
	if(isInTornadoRegion(dac1min, dac1max, dac1, dac2min, dac2max, dac2)) {
	  fillRawData(event,daq_buffer.at(0),tbmtype,1,false,false,column,row,pg_setup,flags);
	}
	else { fillRawData(event,daq_buffer.at(0),tbmtype,1,true, false,column,row,pg_setup,flags); }
      }
      event++;
    }
  }
  
  return 1;
}

uint16_t CTestboard::GetADC(uint8_t) {
  LOG(pxar::logDEBUGRPC) << "called.";
  return 0;
}

