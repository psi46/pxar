#ifndef PXAR_DATASOURCE_EVT_H
#define PXAR_DATASOURCE_EVT_H

#include <stdexcept>
#include "datapipe.h"
#include "helper.h"
#include "log.h"

namespace pxar {

  // Single event data source class
  class evtSource : public dataSource<uint16_t> {
    // --- Control/state
    uint8_t channel;
    uint16_t flags;
    uint8_t chainlength;
    uint8_t chainlengthOffset;
    uint8_t envelopetype;
    uint8_t devicetype;

    // --- data buffer
    uint16_t lastSample;
    unsigned int pos;
    bool connected;
    std::vector<uint16_t> buffer;

    // --- virtual data access methods
    uint16_t Read() {
      if(!connected) throw dpNotConnected();
      // LOG(logDEBUGPIPES) << "pos " << pos;
      return (pos < buffer.size()) ? lastSample = buffer[pos++] : throw dsBufferEmpty();
    }
    uint16_t ReadLast() {
      if(!connected) throw dpNotConnected();
      return lastSample;
    }
    uint8_t ReadChannel() {
      if(!connected) throw dpNotConnected();
      return channel;
    }
    uint16_t ReadFlags() {
      if(!connected) throw dpNotConnected();
      return flags;
    }
    uint8_t ReadTokenChainLength() {
      if(!connected) throw dpNotConnected();
      return chainlength;
    }
    uint8_t ReadTokenChainOffset() {
      if(!connected) throw dpNotConnected();
      return chainlengthOffset;
    }
    uint8_t ReadEnvelopeType() {
      if(!connected) throw dpNotConnected();
      return envelopetype;
    }
    uint8_t ReadDeviceType() {
      if(!connected) throw dpNotConnected();
      return devicetype;
    }
  public:
  evtSource(uint8_t daqchannel, uint8_t tokenChainLength, uint8_t offset, uint8_t tbmtype, uint8_t roctype, uint16_t daqflags = 0)
    : channel(daqchannel), flags(daqflags), chainlength(tokenChainLength), chainlengthOffset(offset), envelopetype(tbmtype), devicetype(roctype), lastSample(0x4000), pos(0), connected(true) {
      LOG(logDEBUGPIPES) << "New evtSource instantiated with properties:";
      LOG(logDEBUGPIPES) << "-------------------------";
      LOG(logDEBUGPIPES) << "Channel " << static_cast<int>(channel)
			 << " (" << static_cast<int>(chainlength) << " ROCs, "
			 << static_cast<int>(chainlengthOffset) << "-" << static_cast<int>(chainlengthOffset+chainlength)<< ")"
			 << (envelopetype == TBM_NONE ? " DESER160 " : (envelopetype == TBM_EMU ? " SOFTTBM " : " DESER400 "));
    }
  evtSource() : connected(false) {};
    void AddData(uint16_t data) {
      buffer.push_back(data);
      LOG(logDEBUGPIPES) << buffer.size() << " words buffered.";
    }
    void AddData(std::vector<uint16_t> data) {
      buffer.insert(buffer.end(), data.begin(), data.end());
      LOG(logDEBUGPIPES) << "-------------------------";
      LOG(logDEBUGPIPES) << "FULL RAW DATA BLOB (" << buffer.size() << " words buffered):";
      if(devicetype < ROC_PSI46DIG) { LOG(logDEBUGPIPES) << listVector(buffer,false,true); }
      LOG(logDEBUGPIPES) << "-------------------------";
    }

    // --- control and status
    uint8_t  GetState() { return (!buffer.empty() && connected); }
    uint32_t GetRemainingSize() { return buffer.size(); }
  };
}
#endif // PXAR_DATASOURCE_EVT_H
