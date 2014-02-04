#include "datapipe.h"
#include "constants.h"

namespace pxar {

  uint16_t dtbSource::FillBuffer() {
    pos = 0;
    do {
      // FIXME read all channels!
      dtbState = tb->Daq_Read(buffer, DTB_SOURCE_BLOCK_SIZE, dtbRemainingSize, 0);
      /*
	if (dtbRemainingSize < 100000) {
	if      (dtbRemainingSize > 1000) tb->mDelay(  1);
	else if (dtbRemainingSize >    0) tb->mDelay( 10);
	else                              tb->mDelay(100);
	}
	LOG(logDEBUGHAL) << "Buffer size: " << buffer.size();
      */
    
      if (buffer.size() == 0) {
	if (stopAtEmptyData) throw dsBufferEmpty();
	if (dtbState) throw dsBufferOverflow();
      }
    } while (buffer.size() == 0);

    /*
      LOG(logDEBUGHAL) << "----------------";
      std::stringstream os;
      for (unsigned int i = 0; i < buffer.size(); i++) {
      os << " " << std::setw(4) << std::hex << (unsigned int)(buffer.at(i));
      }
      LOG(logDEBUGHAL) << os.str();
      LOG(logDEBUGHAL) << "----------------";
    */

    return lastSample = buffer[pos++];
  }


  rawEvent* dtbEventSplitter::Read() {
    record.Clear();

    if (GetLast() & 0x4000) Get();
    if (!(GetLast() & 0x8000)) {
      record.SetStartError();
      while (!(GetLast() & 0x8000)) Get();
    }

    do {
      if (record.GetSize() >= 40000) {
	record.SetOverflow();
	break;
      }
      record.Add(GetLast() & 0x0fff);
    
    } while ((Get() & 0xc000) == 0);
  
    if (GetLast() & 0x4000) record.Add(GetLast() & 0x0fff);
    else record.SetEndError();

    /*
      LOG(logDEBUGHAL) << "-------------------------";
      std::stringstream os;
      for (unsigned int i=0; i<record.data.size(); i++)
      os << " " << std::setw(4) << std::hex 
      << static_cast<uint16_t>(record.data[i]);
      LOG(logDEBUGHAL) << os.str();
    */

    return &record;
  }

  event* dtbEventDecoder::Read() {
    rawEvent *sample = Get();
    roc_event.header = 0;
    roc_event.pixels.clear();
    unsigned int n = sample->GetSize();
    if (n > 0) {
      if (n > 1) roc_event.pixels.reserve((n-1)/2);
      roc_event.header = (*sample)[0];
      unsigned int pos = 1;
      while (pos < n-1) {
	uint32_t raw = (*sample)[pos++] << 12;
	raw += (*sample)[pos++];
	pixel pix(raw);
	roc_event.pixels.push_back(pix);
      }
    }

    // LOG(logDEBUGHAL) << roc_event;

    return &roc_event;
  }

  pixel* dtbEventToPixels::Read() {

    // Check if data is left in current event:
    while(buffered_event == NULL || it == buffered_event->pixels.end()) {
      // Get a new event:
      buffered_event = Get();
      
      // Empty event:
      if(buffered_event->pixels.size() == 0) {
	buffered_event = NULL;
      }
      else {
	// Event with pixels:
	it = buffered_event->pixels.begin();
      }
    }
    
    // Copy data:
    pix = *it;
    // LOG(logDEBUGHAL) << (*it);

    // Increment iterator:
    ++it;
    return &pix;
  }

}
