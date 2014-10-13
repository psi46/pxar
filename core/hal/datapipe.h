#ifndef PXAR_DATAPIPE_H
#define PXAR_DATAPIPE_H

#include <stdexcept>
#include "datatypes.h"
#include "rpc_calls.h"

namespace pxar {

  class dataPipeException : public std::runtime_error {
  public:
  dataPipeException(const char *message) : std::runtime_error(message) {}
  };
    
  class dpNotConnected : public dataPipeException {
  public:
  dpNotConnected() : dataPipeException("Not connected") {}
  };

  // Data pipe classes

  template <class T> 
    class dataSource {
    // The inheritor must define ReadLast and Read:
    virtual T ReadLast() = 0;
    virtual T Read() = 0;
    virtual bool ReadState() = 0;
    virtual uint8_t ReadChannel() = 0;
    virtual uint8_t ReadTokenChainLength() = 0;
    virtual uint8_t ReadDeviceType() = 0;
  public:
    virtual ~dataSource() {}
    template <class S> friend class dataSink;
  };
    
  // Null source for not connected sinks
  template <class T>
    class nullSource : public dataSource<T> {
  protected:
    nullSource() {}
    nullSource(const nullSource&) {}
    ~nullSource() {}
    T ReadLast() { throw dpNotConnected(); }
    T Read()     { return ReadLast();     }
    bool ReadState() { return false; }
    uint8_t ReadChannel() { throw dpNotConnected(); }
    uint8_t ReadTokenChainLength() { throw dpNotConnected(); }
    uint8_t ReadDeviceType() { throw dpNotConnected(); }
    template <class TO> friend class dataSink;
  };

  // Formward declaration of dataPipe class:
  template <class TI, class TO> class dataPipe;

  template <class T> 
    class dataSink {
  protected: 
    dataSource<T> *src;
    static nullSource<T> null;
  public: 
  dataSink() : src(&null) {}
    T GetLast() { return src->ReadLast(); }
    T Get() { return src->Read(); }
    bool GetState() { return src->ReadState(); }
    uint8_t GetChannel() { return src->ReadChannel(); }
    uint8_t GetTokenChainLength() { return src->ReadTokenChainLength(); }
    uint8_t GetDeviceType() { return src->ReadDeviceType(); }
    void GetAll() { while (true) Get(); }
    template <class TI, class TO> friend void operator >> (dataSource<TI> &, dataSink<TO> &); 
    template  <class TI, class TO> friend dataSource<TO>& operator >> (dataSource<TI> &in, dataPipe<TI,TO> &out);
  };

  template <class T>
    nullSource<T> dataSink<T>::null;
   
  // The data pipe:
  template <class TI, class TO=TI>
    class dataPipe : public dataSink<TI>, public dataSource<TO> {};

  // Operator to connect source -> sink; source -> datapipe
  template <class TI, class TO>
    void operator >> (dataSource<TI> &in, dataSink<TO> &out) {
    out.src = &in;
  }
    
  // Operator to connect source -> datapipe -> datapipe -> sink
  template <class TI, class TO>
    dataSource<TO>& operator >> (dataSource<TI> &in, dataPipe<TI,TO> &out) {
    out.src = &in;
    return out;
  }


  class dsBufferOverflow : public dataPipeException {
  public:
  dsBufferOverflow() : dataPipeException("Buffer overflow") {}
  };

  class dsBufferEmpty : public dataPipeException
  {
  public:
  dsBufferEmpty() : dataPipeException("Buffer empty") {}
  };

  // DTB data source class
  class dtbSource : public dataSource<uint16_t> {
    volatile bool stopAtEmptyData;

    // --- DTB control/state
    CTestboard * tb;
    uint8_t channel;
    uint8_t chainlength;
    uint32_t dtbRemainingSize;
    uint8_t  dtbState;
    bool connected;
    bool tbm_present;
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
    bool ReadState() {
      if(!connected) throw dpNotConnected();
      return tbm_present;
    }
    uint8_t ReadChannel() {
      if(!connected) throw dpNotConnected();
      return channel;
    }
    uint8_t ReadTokenChainLength() {
      if(!connected) throw dpNotConnected();
      return chainlength;
    }
    uint8_t ReadDeviceType() {
      if(!connected) throw dpNotConnected();
      return devicetype;
    }
  public:
  dtbSource(CTestboard * src, uint8_t daqchannel, uint8_t tokenChainLength, bool module, uint8_t roctype, bool endlessStream)
    : stopAtEmptyData(endlessStream), tb(src), channel(daqchannel), chainlength(tokenChainLength), connected(true), tbm_present(module), devicetype(roctype), lastSample(0x4000), pos(0) {}
  dtbSource() : connected(false) {}
    bool isConnected() { return connected; }

    // --- control and status
    uint8_t  GetState() { return dtbState; }
    uint32_t GetRemainingSize() { return dtbRemainingSize; }
    void Stop() { stopAtEmptyData = true; }
  };

  // DTB data Event splitter
  class dtbEventSplitter : public dataPipe<uint16_t, rawEvent*> {
    rawEvent record;
    rawEvent* Read() {
      if(GetState()) return SplitDeser400();
      else return SplitDeser160();
    }
    rawEvent* ReadLast() { return &record; }
    bool ReadState() { return GetState(); }
    uint8_t ReadChannel() { return GetChannel(); }
    uint8_t ReadTokenChainLength() { return GetTokenChainLength(); }
    uint8_t ReadDeviceType() { return GetDeviceType(); }

    // The splitter routines:
    rawEvent* SplitDeser160();
    rawEvent* SplitDeser400();

    bool nextStartDetected;
  public:
    dtbEventSplitter() {}
  };

  // DTB data decoding class
  class dtbEventDecoder : public dataPipe<rawEvent*, Event*> {
    Event roc_Event;
    Event* Read() {
      if(GetState()) return DecodeDeser400();
      else return DecodeDeser160();
    }
    Event* ReadLast() { return &roc_Event; }
    bool ReadState() { return GetState(); }
    uint8_t ReadChannel() { return GetChannel(); }
    uint8_t ReadTokenChainLength() { return GetTokenChainLength(); }
    uint8_t ReadDeviceType() { return GetDeviceType(); }

    Event* DecodeDeser160();
    Event* DecodeDeser400();
    uint32_t decodingErrors;

    // Readback decoding:
    void evalReadback(uint8_t roc, uint16_t val);
    std::vector<uint16_t> count;
    std::vector<uint16_t> shiftReg;
    std::vector<std::vector<uint16_t> > readback;

  public:
  dtbEventDecoder() : decodingErrors(0), readback() {};
    void Clear() { decodingErrors = 0; readback.clear(); count.clear(); shiftReg.clear(); };
    uint32_t getErrorCount();
    std::vector<std::vector<uint16_t> > getReadback();
  };
}
#endif
