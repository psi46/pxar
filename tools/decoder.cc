#include "datasource_evt.h"
#include "log.h"
#include "constants.h"
#include <stdlib.h>
#include <iomanip>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>

using namespace pxar;

pxar::pixel getNoiseHit(uint8_t rocid, size_t i, size_t j) {

  // Generate a slightly random pulse height between 80 and 100:
  uint16_t pulseheight = rand()%20 + 80;

  // We can't pulse the same pixel twice in one trigger:
  size_t col = rand()%52;
  while(col == i) col = rand()%52;
  size_t row = rand()%80;
  while(row == j) row = rand()%80;

  pixel px = pixel(rocid,col,row,pulseheight);
  LOG(logDEBUGPIPES) << "Adding noise hit: " << px;
  return px;
}

void deser400_event(evtSource & src, int id, bool readback) {

  src.AddData(0xa000 | (id%256));
  src.AddData(0x8007);
  src.AddData(0x47f8 | (readback ? 2 : 0));
  src.AddData(0x47f8 | (readback ? 2 : 0));
  src.AddData(0x47f8 | (readback ? 2 : 0));

  src.AddData(0x0349);
  src.AddData(0x28a3);
  src.AddData(0x0349);
  src.AddData(0x28a3);

  src.AddData(0x47f8 | (readback ? 2 : 0));
  src.AddData(0x47f8 | (readback ? 2 : 0));
  src.AddData(0x47f8 | (readback ? 2 : 0));
  src.AddData(0x47f8 | (readback ? 2 : 0));

  src.AddData(0x0349);
  src.AddData(0x28a3);

  src.AddData(0x47f8 | (readback ? 2 : 0));
  src.AddData(0xe000);
  src.AddData(0xc002);

}

void test_readback_reset() {

  Log::ReportingLevel() = Log::FromString("DEBUGPIPES");
  LOG(logINFO) << std::endl << std::endl << "Start Readback Reset test...";
  
  evtSource src;
  dtbEventSplitter splitter;
  dtbEventDecoder decoder;
  dataSink<Event*> Eventpump;

  pixel sample = getNoiseHit(1,0,0);
  std::cout << "0x" << std::hex << sample.encode() << std::dec << std::endl;
  Event* evt;
    
  src >> splitter >> decoder >> Eventpump;
  src = evtSource(0,8,0,TBM_08B,ROC_PSI46DIGV21RESPIN);

  int id = 0;

  // Send first event without readback marker, triggers warning about initial condition:
  deser400_event(src,++id,false);

  // Event with S bits in ROC headers:
  deser400_event(src,++id,true);
  // 15 events without S bit but correct evt id:
  for(int i = 0; i < 15; i++) { deser400_event(src,++id,false); }
  // This was one correct readback cycle.

  // Event with S bits in ROC headers:
  deser400_event(src,++id,true);
  // 10 events without S bit but correct evt id:
  for(int i = 0; i < 10; i++) { deser400_event(src,++id,false); }
  // Next one already with - this should trigger the error:
  deser400_event(src,++id,true);
  // Warning triggers, looks fine.
  
  // Now sending correct readback cycle but garbling one TBM event ID:
  // Event with S bits in ROC headers:
  deser400_event(src,++id,true);
  // 10 events without S bit but correct evt id:
  for(int i = 0; i < 10; i++) { deser400_event(src,++id,false); }
  // One event with broken event ID:
  deser400_event(src,++id-3,false);
  // Again 4 correct events:
  for(int i = 0; i < 4; i++) { deser400_event(src,++id,false); }
  // Next one with S bit, marking new readback cycle:
  deser400_event(src,++id,true);
  // This was one correct readback cycle, but one broken event ID
  
  // Read until we are done:
  while(1) {
    try {
      evt = Eventpump.Get();
    }
    catch(dsBufferEmpty &) {
      break;
    }
  }
  decoder.getStatistics().dump();

  LOG(logINFO) << "End Readback Reset test." << std::endl << std::endl;
}

int main(int argc, char* argv[]) {
  try {

    Log::ReportingLevel() = Log::FromString("DEBUGPIPES");

    evtSource src;
    passthroughSplitter splitter;
    dtbEventDecoder decoder;
    dataSink<Event*> Eventpump;

    pixel sample = getNoiseHit(1,0,0);
    std::cout << "0x" << std::hex << sample.encode() << std::dec << std::endl;
    Event* evt;
    
    src >> splitter >> decoder >> Eventpump;
    src = evtSource(0,1,0,TBM_NONE,7);

    // Broken pixel hit at the end:
    src.AddData(0x87f8);
    src.AddData(0x0349);
    src.AddData(0x08a3);
    src.AddData(0xc8a3);
    evt = Eventpump.Get();
    std::cout << "Result:\n"  << *evt << std::endl;
    decoder.getStatistics().dump();

    // No ROC header:
    src.AddData(0x83f8);
    src.AddData(0x0349);
    src.AddData(0xc8a3);
    evt = Eventpump.Get();
    std::cout << "Result:\n"  << *evt << std::endl;
    decoder.getStatistics().dump();

    // Too many ROC headers:
    src.AddData(0x87f8);
    src.AddData(0x07f8);
    src.AddData(0x0349);
    src.AddData(0xc8a3);
    evt = Eventpump.Get();
    std::cout << "Result:\n"  << *evt << std::endl;
    decoder.getStatistics().dump();

    // Address error:
    src.AddData(0x87f8);
    src.AddData(0x0fff);
    src.AddData(0xcfff);
    evt = Eventpump.Get();
    std::cout << "Result:\n"  << *evt << std::endl;
    decoder.getStatistics().dump();

    // Empty event:
    src.AddData(0xc7f8);
    evt = Eventpump.Get();
    std::cout << "Result:\n"  << *evt << std::endl;
    decoder.getStatistics().dump();



    // Let's try some analog with TBM EMU:
    src = evtSource(0,1,0,TBM_EMU,ROC_PSI46V2);
    src.AddData(0xa019);
    src.AddData(0x8007);
    src.AddData(0x8f3e);
    src.AddData(0x0006);
    src.AddData(0x0074);
    src.AddData(0x0fd8);
    src.AddData(0x0007);
    src.AddData(0x0fd3);
    src.AddData(0x00d5);
    src.AddData(0x00ad);
    src.AddData(0x0092);
    src.AddData(0xe000);
    src.AddData(0xc002);
    evt = Eventpump.Get();
    std::cout << "Result:\n"  << *evt << std::endl;
    decoder.getStatistics().dump();

    // More analog:
    src = evtSource(0,1,0,TBM_NONE,ROC_PSI46V2);
    src.AddData(0x8f3e);
    src.AddData(0x0006);
    src.AddData(0x0074);
    src.AddData(0x0fd8);
    src.AddData(0x0007);
    src.AddData(0x0fd3);
    src.AddData(0x00d5);
    src.AddData(0x00ad);
    src.AddData(0x0092);
    src.AddData(0x0fd8);
    src.AddData(0x0007);
    src.AddData(0x0fd3);
    src.AddData(0x00d5);
    src.AddData(0x00ad);
    src.AddData(0x0092);
    src.AddData(0x8f3e);
    src.AddData(0x0006);
    src.AddData(0x0074);
    evt = Eventpump.Get();
    std::cout << "Result:\n"  << *evt << std::endl;
    decoder.getStatistics().dump();

    // Some digital module:
    src = evtSource(0,8,0,TBM_08B,ROC_PSI46DIGV21RESPIN);
    src.AddData(0xa019);
    src.AddData(0x8007);
    src.AddData(0x47f8);
    src.AddData(0x47f8);
    src.AddData(0x47f8);

    src.AddData(0x0349);
    src.AddData(0x28a3);
    src.AddData(0x0349);
    src.AddData(0x28a3);

    src.AddData(0x47f8);
    src.AddData(0x47f8);
    src.AddData(0x47f8);
    src.AddData(0x47f8);

    src.AddData(0x0349);
    src.AddData(0x28a3);

    src.AddData(0x47f8);
    src.AddData(0xe000);
    src.AddData(0xc002);
    evt = Eventpump.Get();
    std::cout << "Result:\n"  << *evt << std::endl;
    decoder.getStatistics().dump();

    test_readback_reset();
  }
  catch (std::exception &e){
    std::cout << "exception: " << e.what() << std::endl;
    return -1;
  }
  catch (...) {
    std::cout << "exception. Exiting." << std::endl;
    return -1;
  }
  return 0;
}
