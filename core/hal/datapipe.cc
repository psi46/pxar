#include "datapipe.h"
#include "helper.h"
#include "log.h"
#include "constants.h"
#include "exceptions.h"

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
    LOG(logDEBUGPIPES) << "Channel " << static_cast<int>(channel)
		       << " (" << static_cast<int>(chainlength) << " ROCs)"
		       << (tbm_present ? " DESER400 " : " DESER160 ");
    LOG(logDEBUGPIPES) << "Remaining " << static_cast<int>(dtbRemainingSize);
    LOG(logDEBUGPIPES) << "----------------";
    LOG(logDEBUGPIPES) << listVector(buffer,true);
    LOG(logDEBUGPIPES) << "----------------";

    return lastSample = buffer[pos++];
  }

  rawEvent* dtbEventSplitter::SplitDeser400() {
    record.Clear();

    // If last one had Event end marker, get a new sample:
    if (!nextStartDetected) { Get(); }

    // If new sample does not have start marker keep on reading until we find it:
    if ((GetLast() & 0xe000) != 0xa000) {
      record.SetStartError();
      Get();
    }
    record.Add(GetLast());

    // Else keep reading and adding samples until we find any marker.
    while ((Get() & 0xe000) != 0xc000) {
      // Check if the last read sample has Event end marker:
      if ((GetLast() & 0xe000) == 0xa000) {
	record.SetEndError();
	nextStartDetected = true;
	return &record;
      }
      // If total Event size is too big, break:
      if (record.GetSize() < 40000) record.Add(GetLast());
      else record.SetOverflow();
    }
    record.Add(GetLast());

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

      record.Add(GetLast());
    } while ((Get() & 0xc000) == 0);

    // Check if the last read sample has Event end marker:
    if (GetLast() & 0x4000) record.Add(GetLast());
    // Else set Event end error:
    else record.SetEndError();

    LOG(logDEBUGPIPES) << "-------------------------";
    LOG(logDEBUGPIPES) << listVector(record.data,true);

    return &record;
  }

  Event* dtbEventDecoder::DecodeDeser400() {

    roc_Event.Clear();
    rawEvent *sample = Get();

    unsigned int raw = 0;
    unsigned int pos = 0;
    unsigned int size = sample->GetSize();
    uint16_t v;

    // Count the ROC headers:
    int16_t roc_n = -1;

    // Check if ROC has inverted pixel address (ROC_PSI46DIG):
    bool invertedAddress = ( GetDeviceType() == ROC_PSI46DIG ? true : false );
    

    // --- decode TBM header ---------------------------------

    // H1
    v = (pos < size) ? (*sample)[pos++] : 0x6000; //MDD_ERROR_MARKER;
    //if ((v & 0xe000) != 0xa000) roc_Event.error |= 0x0800;
    raw = (v & 0x00ff) << 8;

    // H2
    v = (pos < size) ? (*sample)[pos++] : 0x6000; //MDD_ERROR_MARKER;
    //if ((v & 0xe000) != 0x8000) roc_Event.error |= 0x0400;
    raw += v & 0x00ff;

    roc_Event.header = raw;

    // --- decode ROC data -----------------------------------

    // while ROC header
    v = (pos < size) ? (*sample)[pos++] : 0x6000; //MDD_ERROR_MARKER;
    while ((v & 0xe000) == 0x4000) { // ROC Header

      // Count ROC Headers up:
      roc_n++;

      // Decode the readback bits in the ROC header:
      if(GetDeviceType() >= ROC_PSI46DIGV2) { evalReadback(roc_n,v); }

      v = (pos < size) ? (*sample)[pos++] : 0x6000; //MDD_ERROR_MARKER;
      while ((v & 0xe000) <= 0x2000) { // R0 ... R1

	for (int i = 0; i <= 1; i++) {
	  if ((v >> 13) != i) { // R<i>
	    //px_error |= (1<<i);
	    if (v & 0x8000) { // TBM header/trailer
	      //pixel.error = 0x1fff;
	      //roc.error |= 0x0001;
	      //roc.pixel.push_back(pixel);
	      //x.roc.push_back(roc);
	      v = (pos < size) ? (*sample)[pos++] : 0x6000; //MDD_ERROR_MARKER;
	      goto trailer;
	    }
	  }
	  raw = (raw << 12) + (v & 0x0fff);
	  v = (pos < size) ? (*sample)[pos++] : 0x6000; //MDD_ERROR_MARKER;
	}

	try {
	  // Check if this is just fill bits of the TBM09 data stream accounting for the other channel:
	  if(GetTokenChainLength() == 4 && (raw&0xffffff) == 0xffffff) {
	    LOG(logDEBUGPIPES) << "Empty hit detected (TBM09 data streams). Skipping.";
	    continue;
	  }

	  // Get the right ROC id: Channel number x ROC offset (= token chain length)
	  // TBM08x: channel 0: 0-7, channel 1: 8-15
	  // TBM09x: channel 0: 0-3, channel 1: 4-7, channel 2: 8-11, channel 3: 12-15
	  pixel pix(raw,static_cast<uint8_t>(roc_n + GetChannel()*GetTokenChainLength()),invertedAddress);
	  roc_Event.pixels.push_back(pix);
	}
	catch(DataDecoderError /*&e*/){
	  // decoding of raw address lead to invalid address
	  decodingErrors++;
	}
      }
      //if (roc.error) x.error |= 0x0001;
      //x.roc.push_back(roc);
    }

    // --- decode TBM trailer --------------------------------
  trailer:
    raw = 0;

    // T1
    //if ((v & 0xe000) != 0xe000) roc_Event.error |= 0x0080;
    raw = (v & 0x00ff) << 8;

    // T2
    v = (pos < size) ? (*sample)[pos++] : 0x6000; //MDD_ERROR_MARKER;
    //if ((v & 0xe000) != 0xc000) roc_Event.error |= 0x0040;
    raw += v & 0x00ff;

    roc_Event.trailer = raw;

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
      roc_Event.header = (*sample)[0] & 0x0fff;

      // Decode the readback bits in the ROC header:
      if(GetDeviceType() >= ROC_PSI46DIGV2) { evalReadback(0,roc_Event.header); }

      unsigned int pos = 1;
      while (pos < n-1) {
	uint32_t raw = ((*sample)[pos++] & 0x0fff) << 12;
	raw += (*sample)[pos++] & 0x0fff;
	try{
	  pixel pix(raw,0,invertedAddress);
	  roc_Event.pixels.push_back(pix);
	}
	catch(DataDecoderError /*&e*/){
	  // decoding of raw address lead to invalid address
	  decodingErrors++; // keep track of number of such errors
	}
      }
    }

    LOG(logDEBUGPIPES) << roc_Event;
    return &roc_Event;
  }

  void dtbEventDecoder::evalReadback(uint8_t roc, uint16_t val) {
    // Check if we have seen this ROC already:
    if(shiftReg.size() <= roc) shiftReg.resize(roc+1,0);
    shiftReg.at(roc) <<= 1;
    if(val&1) shiftReg.at(roc)++;

    // Count this bit:
    if(count.size() <= roc) count.resize(roc+1,0);
    count.at(roc)++;

    if(val&2) { // start marker
      if (count.at(roc) == 16) {
	// Write out the collected data:
	if(readback.size() <= roc) readback.resize(roc+1);
	readback.at(roc).push_back(shiftReg.at(roc));

	LOG(logDEBUGPIPES) << "Readback ROC " << static_cast<int>(roc+GetChannel()*GetTokenChainLength()) << " "
			   << ((readback.at(roc).back()>>8)&0x00ff) 
			   << " (0x" << std::hex << ((readback.at(roc).back()>>8)&0x00ff) << std::dec << "): " 
			   << (readback.at(roc).back()&0xff) 
			   << " (0x" << std::hex << (readback.at(roc).back()&0xff) << std::dec << ")";
      }
      count.at(roc) = 0;
    }
  }

  uint32_t dtbEventDecoder::getErrorCount() { 
    // Automatically clear the number after it was read out:
    uint32_t tmp = decodingErrors;
    decodingErrors = 0;
    return tmp; 
  }

  std::vector<std::vector<uint16_t> > dtbEventDecoder::getReadback() {
    // Automatically clear the readback vector after it was read out:
    std::vector<std::vector<uint16_t> > tmp = readback;
    readback.clear();
    return tmp;
  }
}
