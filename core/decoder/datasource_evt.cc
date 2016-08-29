#include "datasource_evt.h"
#include "helper.h"
#include "log.h"
#include "constants.h"
#include "exceptions.h"

namespace pxar {

  evtSource::evtSource(uint8_t daqchannel, uint8_t tokenChainLength, uint8_t offset, uint8_t tbmtype, uint8_t roctype, uint16_t daqflags) : channel(daqchannel), flags(daqflags), chainlength(tokenChainLength), chainlengthOffset(offset), envelopetype(tbmtype), devicetype(roctype), lastSample(0x4000), pos(0), connected(true) {
    LOG(logDEBUGPIPES) << "New evtSource instantiated with properties:";
    LOG(logDEBUGPIPES) << "-------------------------";
    LOG(logDEBUGPIPES) << "Channel " << static_cast<int>(channel)
		       << " (" << static_cast<int>(chainlength) << " ROCs, "
		       << static_cast<int>(chainlengthOffset) << "-" << static_cast<int>(chainlengthOffset+chainlength)<< ")"
		       << (envelopetype == TBM_NONE ? " DESER160 " : (envelopetype == TBM_EMU ? " SOFTTBM " : " DESER400 "));
  }
  
  uint16_t evtSource::Read() {
    if(!connected) throw dpNotConnected();
    if(pos < buffer.size()) {
      lastSample = buffer[pos++];
      return lastSample;
    }
    else {
      buffer.clear();
      pos = 0;
      throw dsBufferEmpty();
    }
  }

  void evtSource::AddData(uint16_t data) {
    buffer.push_back(data);
    LOG(logDEBUGPIPES) << buffer.size() << " words buffered.";
  }
  void evtSource::AddData(std::vector<uint16_t> data) {
    buffer.insert(buffer.end(), data.begin(), data.end());
    LOG(logDEBUGPIPES) << "-------------------------";
    LOG(logDEBUGPIPES) << "FULL RAW DATA BLOB (" << buffer.size() << " words buffered):";
    if(devicetype < ROC_PSI46DIG) { LOG(logDEBUGPIPES) << listVector(buffer,false,true); }
    else { LOG(logDEBUGPIPES) << listVector(buffer,true); }
    LOG(logDEBUGPIPES) << "-------------------------";
  }

}
