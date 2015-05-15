#ifndef PXAR_DATASOURCE_DTB_H
#define PXAR_DATASOURCE_DTB_H

#include <stdexcept>
#include "datapipe.h"
#include "rpc_calls.h"

namespace pxar {

  // DTB data source class
  class dtbSource : public dataSource<uint16_t> {
    volatile bool stopAtEmptyData;

    // --- DTB control/state
    CTestboard * tb;
    uint8_t channel;
    std::vector<uint8_t> chainlength;
    uint8_t precedingChainlength;
    uint32_t dtbRemainingSize;
    uint8_t  dtbState;
    bool connected;
    uint8_t envelopetype;
    uint8_t devicetype;

    // --- data buffer
    uint16_t lastSample;
    unsigned int pos;
    std::vector<uint16_t> buffer;
    uint16_t FillBuffer();

    // --- virtual data access methods
    uint16_t Read() { 
      if(!connected) throw dpNotConnected();
      return (pos < buffer.size()) ? lastSample = buffer[pos++] : FillBuffer();
    }
    uint16_t ReadLast() {
      if(!connected) throw dpNotConnected();
      return lastSample;
    }
    uint8_t ReadChannel() {
      if(!connected) throw dpNotConnected();
      return channel;
    }
    uint8_t ReadTokenChainLength() {
      if(!connected) throw dpNotConnected();
      return chainlength.at(channel);
    }
    uint8_t ReadPrecedingTokenChainLength() {
      if(!connected) throw dpNotConnected();
      return precedingChainlength;
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
  dtbSource(CTestboard * src, uint8_t daqchannel, const std::vector<uint8_t> &tokenChainLength, uint8_t tbmtype, uint8_t roctype, bool endlessStream)
    : stopAtEmptyData(endlessStream), tb(src), channel(daqchannel), chainlength(tokenChainLength), connected(true), envelopetype(tbmtype), devicetype(roctype), lastSample(0x4000), pos(0) {
      precedingChainlength = 0;
      for (unsigned i = 0; i < channel; i++)
        precedingChainlength += chainlength.at(i);
    }
  dtbSource() : connected(false) {}
    bool isConnected() { return connected; }

    // --- control and status
    uint8_t  GetState() { return dtbState; }
    uint32_t GetRemainingSize() { return dtbRemainingSize; }
    void Stop() { stopAtEmptyData = true; }
  };

}
#endif // PXAR_DATASOURCE_DTB_H
