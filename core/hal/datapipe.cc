#include "datapipe.h"
#include "helper.h"
#include "constants.h"

namespace pxar {

  uint16_t dtbSource::FillBuffer() {
    pos = 0;
    do {
      dtbState = tb->Daq_Read(buffer, DTB_SOURCE_BLOCK_SIZE, dtbRemainingSize, channel);
      /*
	if (dtbRemainingSize < 100000) {
	if      (dtbRemainingSize > 1000) mDelay(  1);
	else if (dtbRemainingSize >    0) mDelay( 10);
	else                              mDelay(100);
	}
	LOG(logDEBUGPIPES) << "Buffer size: " << buffer.size();
      */
    
      if (buffer.size() == 0) {
	if (stopAtEmptyData) throw dsBufferEmpty();
	if (dtbState) throw dsBufferOverflow();
      }
    } while (buffer.size() == 0);

    LOG(logDEBUGPIPES) << "----------------";
    LOG(logDEBUGPIPES) << "Channel " << static_cast<int>(channel) << " DESER400: " << (int)tbm_present;
    LOG(logDEBUGPIPES) << "Remaining " << static_cast<int>(dtbRemainingSize);
    LOG(logDEBUGPIPES) << "----------------";
    LOG(logDEBUGPIPES) << listVector(buffer,true);
    LOG(logDEBUGPIPES) << "----------------";

    return lastSample = buffer[pos++];
  }


  rawEvent* dtbEventSplitter::SplitDeser400() {
    record.Clear();

    // If last one had Event end marker, get a new sample:
    if (GetLast() & 0x00f0) { Get(); }

    // If new sample does not have start marker keep on reading until we find it:
    if (!(GetLast() & 0x0080)) {
      record.SetStartError();
      while (!(GetLast() & 0x0080)) Get();
    }

    // Else keep reading and adding samples until we find any marker.
    do {
      // If total Event size is too big, break:
      if (record.GetSize() >= 40000) {
	record.SetOverflow();
	break;
      }

      // FIXME Very first Event starts with 0xC - which srews up empty Event detection here!
      // If the Event start sample is also Event end sample, write and quit:
      //if((GetLast() & 0xf000) == 0xf000) { break; }

      record.Add(GetLast() & 0x00ff);
    } while ((Get() & 0x00f0) != 0x00f0);

    // Check if the last read sample has Event end marker:
    if (GetLast() & 0x00f0) record.Add(GetLast() & 0x00ff);
    // Else set Event end error:
    else record.SetEndError();

    LOG(logDEBUGPIPES) << "-------------------------";
    LOG(logDEBUGPIPES) << listVector(record.data,true);

    return &record;
  }


  rawEvent* dtbEventSplitter::SplitDeser160() {
    record.Clear();

    // If last one had Event end marker, get a new sample:
    if (GetLast() & 0x4000) { Get(); }

    // If new sample does not have start marker keep on reading until we find it:
    if (!(GetLast() & 0x8000)) {
      record.SetStartError();
      while (!(GetLast() & 0x8000)) Get();
    }

    // Else keep reading and adding samples until we find any marker.
    do {
      // If total Event size is too big, break:
      if (record.GetSize() >= 40000) {
	record.SetOverflow();
	break;
      }

      // FIXME Very first Event starts with 0xC - which srews up empty Event detection here!
      // If the Event start sample is also Event end sample, write and quit:
      if((GetLast() & 0xc000) == 0xc000) { break; }

      record.Add(GetLast() & 0x0fff);
    } while ((Get() & 0xc000) == 0);

    // Check if the last read sample has Event end marker:
    if (GetLast() & 0x4000) record.Add(GetLast() & 0x0fff);
    // Else set Event end error:
    else record.SetEndError();

    LOG(logDEBUGPIPES) << "-------------------------";
    LOG(logDEBUGPIPES) << listVector(record.data,true);

    return &record;
  }


  Event* dtbEventDecoder::DecodeDeser400() {

    roc_Event.Clear();
    rawEvent *sample = Get();

    uint16_t hdr = 0, trl = 0;
    unsigned int raw = 0;

    // Get the right ROC id, channel 0: 0-7, channel 1: 8-15
    int16_t roc_n = -1 + GetChannel() * 8;

    // Check if ROC has inverted pixel address (ROC_PSI46DIG):
    bool invertedAddress = ( GetDeviceType() == ROC_PSI46DIG ? true : false );
    
    for (unsigned int i = 0; i < sample->GetSize(); i++) {
      int d = (*sample)[i] & 0xf;
      int q = ((*sample)[i]>>4) & 0xf;

      switch (q) {
      case  0: break;

      case  1: raw = d; break;
      case  2: raw = (raw<<4) + d; break;
      case  3: raw = (raw<<4) + d; break;
      case  4: raw = (raw<<4) + d; break;
      case  5: raw = (raw<<4) + d; break;
      case  6: raw = (raw<<4) + d;
	{
	  pixel pix(raw,invertedAddress);
	  pix.roc_id = static_cast<uint8_t>(roc_n);
	  roc_Event.pixels.push_back(pix);
	  break;
	}
      case  7: roc_n++; break;

      case  8: hdr = d; break;
      case  9: hdr = (hdr<<4) + d; break;
      case 10: hdr = (hdr<<4) + d; break;
      case 11: hdr = (hdr<<4) + d;
	roc_Event.header = hdr;
	roc_n = -1 + GetChannel() * 8;
	break;

      case 12: trl = d; break;
      case 13: trl = (trl<<4) + d; break;
      case 14: trl = (trl<<4) + d; break;
      case 15: trl = (trl<<4) + d;
	roc_Event.trailer = trl;
	break;
      }
    }

    LOG(logDEBUGPIPES) << roc_Event;
    return &roc_Event;
  }

  Event* dtbEventDecoder::DecodeDeser160() {

    roc_Event.Clear();

    // Check if ROC has inverted pixel address (ROC_PSI46DIG):
    bool invertedAddress = ( GetDeviceType() == ROC_PSI46DIG ? true : false );

    rawEvent *sample = Get();
    unsigned int n = sample->GetSize();
    if (n > 0) {
      if (n > 1) roc_Event.pixels.reserve((n-1)/2);
      roc_Event.header = (*sample)[0];
      unsigned int pos = 1;
      while (pos < n-1) {
	uint32_t raw = (*sample)[pos++] << 12;
	raw += (*sample)[pos++];
	pixel pix(raw,invertedAddress);
	roc_Event.pixels.push_back(pix);
      }
    }

    LOG(logDEBUGPIPES) << roc_Event;
    return &roc_Event;
  }
}
