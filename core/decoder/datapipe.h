#ifndef PXAR_DATAPIPE_H
#define PXAR_DATAPIPE_H

#include <stdexcept>
#include "datatypes.h"
#include "constants.h"

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
    virtual uint8_t ReadChannel() = 0;
    virtual uint16_t ReadFlags() = 0;
    virtual uint8_t ReadTokenChainLength() = 0;
    virtual uint8_t ReadTokenChainOffset() = 0;
    virtual uint8_t ReadEnvelopeType() = 0;
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
    uint8_t ReadChannel() { throw dpNotConnected(); }
    uint16_t ReadFlags() { throw dpNotConnected(); }
    uint8_t ReadTokenChainLength() { throw dpNotConnected(); }
    uint8_t ReadTokenChainOffset() { throw dpNotConnected(); }
    uint8_t ReadEnvelopeType() { throw dpNotConnected(); }
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
    uint8_t GetChannel() { return src->ReadChannel(); }
    uint16_t GetFlags() { return src->ReadFlags(); }
    uint8_t GetTokenChainLength() { return src->ReadTokenChainLength(); }
    uint8_t GetTokenChainOffset() { return src->ReadTokenChainOffset(); }
    uint8_t GetEnvelopeType() { return src->ReadEnvelopeType(); }
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

  // DTB data Event splitter
  class dtbEventSplitter : public dataPipe<uint16_t, rawEvent*> {
    rawEvent record;
    rawEvent* Read();
    rawEvent* ReadLast() { return &record; }
    uint8_t ReadChannel() { return GetChannel(); }
    uint16_t ReadFlags() { return GetFlags(); }
    uint8_t ReadTokenChainLength() { return GetTokenChainLength(); }
    uint8_t ReadTokenChainOffset() { return GetTokenChainOffset(); }
    uint8_t ReadEnvelopeType() { return GetEnvelopeType(); }
    uint8_t ReadDeviceType() { return GetDeviceType(); }

    // The splitter routines:
    void SplitDeser160();
    void SplitDeser400();
    void SplitSoftTBM();

    bool nextStartDetected;
  public:
  dtbEventSplitter() :
    nextStartDetected(false) {}
  };

  // This "splitter" does nothing than passing through all input data as one raw event
  // this can be used for data which is already split into events
  class passthroughSplitter : public dataPipe<uint16_t, rawEvent*> {
    rawEvent record;
    rawEvent* Read();
    rawEvent* ReadLast() { return &record; }
    uint8_t ReadChannel() { return GetChannel(); }
    uint16_t ReadFlags() { return GetFlags(); }
    uint8_t ReadTokenChainLength() { return GetTokenChainLength(); }
    uint8_t ReadTokenChainOffset() { return GetTokenChainOffset(); }
    uint8_t ReadEnvelopeType() { return GetEnvelopeType(); }
    uint8_t ReadDeviceType() { return GetDeviceType(); }
  public:
    passthroughSplitter() {}
  };

  // DTB data decoding class
  class dtbEventDecoder : public dataPipe<rawEvent*, Event*> {
    Event roc_Event;
    Event* Read();
    Event* ReadLast() { return &roc_Event; }
    uint16_t ReadFlags() { return GetFlags(); }
    uint8_t ReadChannel() { return GetChannel(); }
    uint8_t ReadTokenChainLength() { return GetTokenChainLength(); }
    uint8_t ReadTokenChainOffset() { return GetTokenChainOffset(); }
    uint8_t ReadEnvelopeType() { return GetEnvelopeType(); }
    uint8_t ReadDeviceType() { return GetDeviceType(); }

    void DecodeADC(rawEvent * sample);
    void DecodeDeser160(rawEvent * sample);
    void DecodeDeser400(rawEvent * sample);
    void ProcessTBM(rawEvent * sample);
    statistics decodingStats;

    // Readback decoding:
    void evalReadback(uint8_t roc, uint16_t val);
    std::vector<bool> readback_dirty;
    std::vector<uint16_t> count;
    std::vector<uint16_t> shiftReg;
    std::vector<std::vector<uint16_t> > readback;

    // Error checking:
    void evalDeser400Errors(uint16_t data);
    void CheckEventValidity(int16_t roc_n);
    void CheckEventID();
    int16_t eventID;

    // Collection of XOR patterns
    std::vector<uint8_t> xorsum;
    
    // Analog level averaging:
    void AverageAnalogLevel(int16_t word1, int16_t word2);
    int32_t ultrablack;
    int32_t black;
    int16_t levelS;
    int32_t sumUB, sumB;
    size_t slidingWindow;
    
    // Last DAC storage for analog ROCs:
    void evalLastDAC(uint8_t roc, uint16_t val);

    // Debugging mechanism for problematic events
    uint32_t total_event, flawed_event, error_count, dump_count;
    std::vector<std::string> event_ringbuffer;

  public:
  dtbEventDecoder() : decodingStats(), readback_dirty(), count(), shiftReg(), readback(), eventID(-1), ultrablack(0xfff), black(0xfff), levelS(0), sumUB(0), sumB(0), slidingWindow(0), total_event(5), flawed_event(0), error_count(0), dump_count(0), event_ringbuffer(7) {};
    void Clear() { decodingStats.clear(); readback.clear(); count.clear(); shiftReg.clear(); eventID = -1; };
    statistics getStatistics();
    std::vector<std::vector<uint16_t> > getReadback();
    std::vector<uint8_t> getXORsum();
  };
}
#endif
