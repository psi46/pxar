#ifndef PXAR_DATAPIPE_H
#define PXAR_DATAPIPE_H

#include <exception>
#include "datatypes.h"
#include "rpc_calls.h"

namespace pxar {

  class dataPipeException : public std::exception {
  public:
  dataPipeException(const char */*message*/) : std::exception() {};
  };
    
  class dpNotConnected : public dataPipeException {
  public:
  dpNotConnected() : dataPipeException("Not connected") {};
  };

  // Data pipe classes

  template <class T> 
    class dataSource {
    // The inheritor must define ReadLast and Read:
    virtual T ReadLast() = 0;
    virtual T Read() = 0;
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
  dsBufferOverflow() : dataPipeException("Buffer overflow") {};
  };

  class dsBufferEmpty : public dataPipeException
  {
  public:
  dsBufferEmpty() : dataPipeException("Buffer empty") {};
  };

  // DTB data source class
  class dtbSource : public dataSource<uint16_t> {
    volatile bool stopAtEmptyData;

    // --- DTB control/state
    CTestboard * tb;
    uint8_t channel;
    uint32_t dtbRemainingSize;
    uint8_t  dtbState;
    bool connected;

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
  public:
  dtbSource(CTestboard * src, uint8_t daqchannel, bool endlessStream)
    : stopAtEmptyData(endlessStream), tb(src), channel(daqchannel), connected(true), lastSample(0x4000), pos(0) {};
  dtbSource() : connected(false) {};

    // --- control and status
    uint8_t  GetState() { return dtbState; }
    uint32_t GetRemainingSize() { return dtbRemainingSize; }
    void Stop() { stopAtEmptyData = true; }
  };

  // DTB data event splitter
  class dtbEventSplitter : public dataPipe<uint16_t, rawEvent*> {
    rawEvent record;
    rawEvent* Read();
    rawEvent* ReadLast() { return &record; }
  public:
    dtbEventSplitter() {}
  };

  // DTB data decoding class
  class dtbEventDecoder : public dataPipe<rawEvent*, event*> {
    event roc_event;
    event* Read();
    event* ReadLast() { return &roc_event; }
  };

  // Events to pixels only:
  class dtbEventToPixels : public dataPipe<event*, pixel*> {
    pixel pix;
    event* buffered_event;
    std::vector<pixel>::iterator it;
    pixel* Read();
    pixel* ReadLast() { return &pix; }
  public:
  dtbEventToPixels() : buffered_event(NULL) {};
  };

}
#endif
