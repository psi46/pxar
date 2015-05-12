#include "datapipe.h"
#include "helper.h"
#include "log.h"
#include "constants.h"
#include "exceptions.h"

namespace pxar {

  rawEvent* dtbEventSplitter::Read() {
    record.Clear();

    // Split the data stream according to DESER160 alignment markers:
    if(GetEnvelopeType() == TBM_NONE) { SplitDeser160(); }
    // Split the data stream according to DESER400 / TBMEMU alignment markers:
    else { SplitDeser400(); }

    LOG(logDEBUGPIPES) << "SINGLE SPLIT EVENT:";
    if(GetDeviceType() < ROC_PSI46DIG) { LOG(logDEBUGPIPES) << listVector(record.data,false,true); }
    else { LOG(logDEBUGPIPES) << listVector(record.data,true); }
    LOG(logDEBUGPIPES) << "-------------------------";

    return &record;
  }

  void dtbEventSplitter::SplitDeser400() {
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
	return;
      }
      // If total Event size is too big, break:
      if (record.GetSize() < 40000) record.Add(GetLast());
      else record.SetOverflow();
    }
    record.Add(GetLast());
    nextStartDetected = false;
  }

  void dtbEventSplitter::SplitDeser160() {
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
  }

  rawEvent* passthroughSplitter::Read() {
    record.Clear();
    try {
      do {
	record.Add(Get());
      } while(1);
    }
    catch(dsBufferEmpty) {}
    return &record;
  }

  Event* dtbEventDecoder::Read() {

    roc_Event.Clear();
    rawEvent *sample = Get();

    // Count possibe error states:
    if(sample->IsStartError()) { decodingStats.m_errors_event_start++; }
    if(sample->IsEndError()) { decodingStats.m_errors_event_stop++; }
    if(sample->IsOverflow()) { decodingStats.m_errors_event_overflow++; }
    decodingStats.m_info_words_read += sample->GetSize();

    // If a TBM header and trailer should be available, process them first:
    if(GetEnvelopeType() != TBM_NONE) { ProcessTBM(sample); }

    // Decode ADC Data for analog devices:
    if(GetDeviceType() < ROC_PSI46DIG) { DecodeAnalog(sample); }
    // Decode DESER400 Data for digital devices and TBMs:
    else if(GetEnvelopeType() > TBM_EMU) { DecodeDeser400(sample); }
    // Decode DESER160 Data for digital devices without real TBM
    else { DecodeDeser160(sample); }

    LOG(logDEBUGPIPES) << roc_Event;
    return &roc_Event;
  }

  void dtbEventDecoder::ProcessTBM(rawEvent * sample) {
    LOG(logDEBUGPIPES) << "Processing TBM header and trailer...";

    // Check if the data is long enough to hold header and trailer:
    if(sample->GetSize() < 4) {
      decodingStats.m_errors_tbm_header++;
      decodingStats.m_errors_tbm_trailer++;
      return;
    }
    unsigned int size = sample->GetSize();

    // TBM Header:

    // Check the validity of the data words:
    CheckInvalidWord(sample->data.at(0));
    CheckInvalidWord(sample->data.at(1));
    // Check the alignment markers to be correct:
    if((sample->data.at(0) & 0xe000) != 0xa000
       || (sample->data.at(1) & 0xe000) != 0x8000) { decodingStats.m_errors_tbm_header++; }
    // Store the two header words:
    roc_Event.header = ((sample->data.at(0) & 0x00ff) << 8) + (sample->data.at(1) & 0x00ff);

    LOG(logDEBUGPIPES) << "TBM " << static_cast<int>(GetChannel()) << " Header:";
    IFLOG(logDEBUGPIPES) { roc_Event.printHeader(); }

    // Check for correct TBM event ID:
    CheckEventID();


    // TBM Trailer:

    // Check the validity of the data words:
    CheckInvalidWord(sample->data.at(size-2));
    CheckInvalidWord(sample->data.at(size-1));
    // Check the alignment markers to be correct:
    if((sample->data.at(size-2) & 0xe000) != 0xe000
       || (sample->data.at(size-1) & 0xe000) != 0xc000) { decodingStats.m_errors_tbm_trailer++; }
    // Store the two trailer words:
    roc_Event.trailer = ((sample->data.at(size-2) & 0x00ff) << 8) 
      + (sample->data.at(size-1) & 0x00ff);

    LOG(logDEBUGPIPES) << "TBM " << static_cast<int>(GetChannel()) << " Trailer:";
    IFLOG(logDEBUGPIPES) roc_Event.printTrailer();

    // Remove header and trailer:
    sample->data.erase(sample->data.begin(), sample->data.begin() + 2);
    sample->data.erase(sample->data.end() - 2, sample->data.end());
  }

  void dtbEventDecoder::DecodeDeser400(rawEvent * sample) {
    LOG(logDEBUGPIPES) << "Decoding ROC data from DESER400...";

    unsigned int raw = 0;
    unsigned int pos = 0;
    unsigned int size = sample->GetSize();

    uint16_t v;

    // Count the ROC headers:
    int16_t roc_n = -1;

    // Check if ROC has inverted pixel address (ROC_PSI46DIG):
    bool invertedAddress = ( GetDeviceType() == ROC_PSI46DIG ? true : false );

    // --- decode ROC data -----------------------------------

    // while ROC header
    v = (pos < size) ? (*sample)[pos++] : 0x6000; //MDD_ERROR_MARKER;
    CheckInvalidWord(v);

    while ((v & 0xe000) == 0x4000) { // ROC Header

      // Count ROC Headers up:
      roc_n++;

      // Check for DESER400 failure:
      if((v&0x0ff0) == 0x0ff0) {
	LOG(logCRITICAL) << "TBM " << static_cast<int>(GetChannel())
			 << " ROC " << static_cast<int>(roc_n)
			 << " header reports DESER400 failure!";
	decodingStats.m_errors_event_invalid_xor++;
	throw DataDecodingError("Invalid XOR eye diagram encountered.");
      }

      // Decode the readback bits in the ROC header:
      if(GetDeviceType() >= ROC_PSI46DIGV2) { evalReadback(static_cast<uint8_t>(roc_n),v); }

      v = (pos < size) ? (*sample)[pos++] : 0x6000; //MDD_ERROR_MARKER;
      CheckInvalidWord(v);
      while ((v & 0xe000) <= 0x2000) { // R0 ... R1

	for (int i = 0; i <= 1; i++) {
	  if ((v >> 13) != i) { // R<i>
	    if (v & 0x8000) { // TBM header/trailer
	      // Unexpected arrival of TBM marker - pixel data is incomplete:
	      decodingStats.m_errors_pixel_incomplete++;
	      v = (pos < size) ? (*sample)[pos++] : 0x6000; //MDD_ERROR_MARKER;
	      CheckInvalidWord(v);
	      goto trailer;
	    }
	  }
	  raw = (raw << 12) + (v & 0x0fff);
	  v = (pos < size) ? (*sample)[pos++] : 0x6000; //MDD_ERROR_MARKER;
	  CheckInvalidWord(v);
	}

	try {
	  // Check if this is just fill bits of the TBM09 data stream 
	  // accounting for the other channel:
	  if(GetTokenChainLength() == 4 && (raw&0xffffff) == 0xffffff) {
	    LOG(logDEBUGPIPES) << "Empty hit detected (TBM09 data streams). Skipping.";
	    continue;
	  }

	  // Get the correct ROC id: Channel number x ROC offset (= token chain length)
	  // TBM08x: channel 0: 0-7, channel 1: 8-15
	  // TBM09x: channel 0: 0-3, channel 1: 4-7, channel 2: 8-11, channel 3: 12-15
	  pixel pix(raw,static_cast<uint8_t>(roc_n + GetChannel()*GetTokenChainLength()),invertedAddress);
	  roc_Event.pixels.push_back(pix);
	  decodingStats.m_info_pixels_valid++;
	}
	catch(DataInvalidAddressError /*&e*/){
	  // decoding of raw address lead to invalid address
	  decodingStats.m_errors_pixel_address++;
	}
	catch(DataInvalidPulseheightError /*&e*/){
	  // decoding of pulse height featured non-zero fill bit
	  decodingStats.m_errors_pixel_pulseheight++;
	}
	catch(DataCorruptBufferError /*&e*/){
	  // decoding returned row 80 - corrupt data buffer
	  decodingStats.m_errors_pixel_buffer_corrupt++;
	}
      }
    }

  trailer:
    // Check event validity (empty, missing ROCs...):
    CheckEventValidity(roc_n);
  }

  void dtbEventDecoder::AverageAnalogLevel(int32_t &variable, int16_t dataword) {
    // Check if this is the initial measurement:
    if(variable > 0xff) { variable = expandSign(dataword & 0x0fff); }
    // Average the variable:
    else { variable = (variable + expandSign(dataword & 0x0fff))/2; }
  }

  void dtbEventDecoder::DecodeAnalog(rawEvent * sample) {
    LOG(logDEBUGPIPES) << "Decoding ROC data from ADC...";

    int16_t roc_n = -1;

    unsigned int n = sample->GetSize();

    if (n >= 3) {
      // Reserve expected number of pixels from data length (subtract ROC headers):
      if (n - 3*GetTokenChainLength() > 0) roc_Event.pixels.reserve((n - 3*GetTokenChainLength())/6);

      unsigned int pos = 0;
      while (pos+3 <= n) {
	// Check if we have another ROC header (UB and B levels):
	// Here we have to assume the first two words are a ROC header because we rely on
	// its Ultrablack and Black level as initial values for auto-calibration:
	int16_t levelS = (black - ultrablack)/8;

	if(roc_n < 0 || 
	   // Ultrablack level:
	   ((ultrablack-levelS < expandSign((*sample)[pos] & 0x0fff) && ultrablack+levelS > expandSign((*sample)[pos] & 0x0fff))
	   // Black level:
	    && (black-levelS < expandSign((*sample)[pos+1] & 0x0fff) && black+levelS > expandSign((*sample)[pos+1] & 0x0fff)))) {

	  roc_n++;
	  // Save the lastDAC value:
	  evalLastDAC(roc_n, (*sample)[pos+2] & 0x0fff);
	  roc_Event.header = (*sample)[pos+2] & 0x0fff;

	  // Iterate to improve ultrablack and black measurement:
	  AverageAnalogLevel(ultrablack, (*sample)[pos] & 0x0fff);
	  AverageAnalogLevel(black, (*sample)[pos+1] & 0x0fff);

	  LOG(logDEBUGPIPES) << "ROC Header: "
			     << expandSign((*sample)[pos] & 0x0fff) << " (avg. " << ultrablack << ") (UB) "
			     << expandSign((*sample)[pos+1] & 0x0fff) << " (avg. " << black << ") (B) "
			     << expandSign((*sample)[pos+2] & 0x0fff) << " (lastDAC) ";
	  pos += 3;
	}
	// We have a pixel hit:
	else {
	  // Not enough data for a new pixel hit (six words):
	  if(pos > n-6) break;

	  std::vector<uint16_t> data;
	  data.push_back((*sample)[pos] & 0x0fff);
	  data.push_back((*sample)[pos+1] & 0x0fff);
	  data.push_back((*sample)[pos+2] & 0x0fff);
	  data.push_back((*sample)[pos+3] & 0x0fff);
	  data.push_back((*sample)[pos+4] & 0x0fff);
	  data.push_back((*sample)[pos+5] & 0x0fff);
 
	  try{
	    LOG(logDEBUGPIPES) << "Trying to decode pixel: " << listVector(data,false,true);
	    pixel pix(data,roc_n,ultrablack,black);
	    roc_Event.pixels.push_back(pix);
	    decodingStats.m_info_pixels_valid++;
	  }
	  catch(DataDecodingError /*&e*/){
	    // decoding of raw address lead to invalid address
	    decodingStats.m_errors_pixel_address++;
	  }
	  // Advance read pointer by one pixel:
	  pos += 6;
	}
      }
    }

    // Check event validity (empty, missing ROCs...):
    CheckEventValidity(roc_n);
  }

  void dtbEventDecoder::DecodeDeser160(rawEvent * sample) {
    LOG(logDEBUGPIPES) << "Decoding ROC data from DESER160...";

    // Count the ROC headers:
    int16_t roc_n = -1;

    // Check if ROC has inverted pixel address (ROC_PSI46DIG):
    bool invertedAddress = ( GetDeviceType() == ROC_PSI46DIG ? true : false );

    unsigned int n = sample->GetSize();

    if (n > 0) {
      // Reserve expected number of pixels from data length (subtract ROC headers):
      if (n-GetTokenChainLength() > 0) roc_Event.pixels.reserve((n-GetTokenChainLength())/2);

      unsigned int pos = 0;

      // Loop over the full data:
      while (pos < n) {
	// Check if we have a ROC header:
	if(((*sample)[pos] & 0x0ffc) == 0x07f8) {
	  roc_Event.header = (*sample)[pos] & 0x0fff;
	  roc_n++;
	  pos++;

	  // Decode the readback bits in the ROC header:
	  if(GetDeviceType() >= ROC_PSI46DIGV2) { evalReadback(roc_n,roc_Event.header); }
	}
	// We have a pixel:
	// Require that we found at least one ROC header:
	else if(roc_n >= 0) {
	  // Not enough data for a new pixel hit (two words):
	  if(pos >= n-1) {
	    decodingStats.m_errors_pixel_incomplete++;
	    break;
	  }

	  uint32_t raw = ((*sample)[pos++] & 0x0fff) << 12;
	  raw += (*sample)[pos++] & 0x0fff;
	  try {
	    pixel pix(raw,roc_n,invertedAddress);
	    roc_Event.pixels.push_back(pix);
	    decodingStats.m_info_pixels_valid++;
	  }
	  catch(DataInvalidAddressError /*&e*/) {
	    // decoding of raw address lead to invalid address
	    decodingStats.m_errors_pixel_address++;
	  }
	  catch(DataInvalidPulseheightError /*&e*/) {
	    // decoding of pulse height featured non-zero fill bit
	    decodingStats.m_errors_pixel_pulseheight++;
	  }
	  catch(DataCorruptBufferError /*&e*/) {
	    // decoding returned row 80 - corrupt data buffer
	    decodingStats.m_errors_pixel_buffer_corrupt++;
	  }
	}
	// No ROC header present, try with next word:
	else {
	  pos++;
	}
      }
    }

    // Check event validity (empty, missing ROCs...):
    CheckEventValidity(roc_n);
  }

  void dtbEventDecoder::CheckInvalidWord(uint16_t v) {
    // Check last bit of identifier nibble to be zero:
    if((v & 0x1000) == 0x0000) { return; }
    decodingStats.m_errors_event_invalid_words++;
  }

  void dtbEventDecoder::CheckEventID() {
    // After startup, register the first event ID:
    if(eventID == -1) { eventID = roc_Event.triggerCount(); }

    // Check if TBM event ID matches with expectation:
    if(roc_Event.triggerCount() != (eventID%256)) {
      LOG(logERROR) << "   Event ID mismatch:  local ID (" << static_cast<int>(eventID) 
		    << ") !=  TBM ID (" << static_cast<int>(roc_Event.triggerCount()) << ")";
      decodingStats.m_errors_tbm_eventid_mismatch++;
      // To continue readout, set event ID to the currently decoded one:
      eventID = roc_Event.triggerCount();
    }

    // Increment event counter:
    eventID = (eventID%256) + 1;
  }

  void dtbEventDecoder::CheckEventValidity(int16_t roc_n) {

    // Check that we found all expected ROC headers:
    // If the number of ROCs does not correspond to what we expect
    // clear the event and return:
    if(roc_n+1 != GetTokenChainLength()) {
      LOG(logERROR) << "Number of ROCs (" << static_cast<int>(roc_n+1)
		    << ") != Token Chain Length (" << static_cast<int>(GetTokenChainLength()) << ")";
      decodingStats.m_errors_roc_missing++;
      // Clearing event content:
      roc_Event.Clear();
    }
    // Count empty events
    else if(roc_Event.pixels.empty()) { 
      decodingStats.m_info_events_empty++;
      LOG(logDEBUGPIPES) << "Event is empty.";
    }
    // Count valid events
    else {
      decodingStats.m_info_events_valid++;
      LOG(logDEBUGPIPES) << "Event is valid.";
    }
  }

  void dtbEventDecoder::evalLastDAC(uint8_t roc, uint16_t val) {
    // Check if we have seen this ROC already:
    if(readback.size() <= roc) readback.resize(roc+1);
    readback.at(roc).push_back(val);

    LOG(logDEBUGPIPES) << "Readback ROC "
		       << static_cast<int>(roc+GetChannel()*GetTokenChainLength())
		       << " " << static_cast<int>(expandSign(val & 0x0fff));
  }

  void dtbEventDecoder::evalReadback(uint8_t roc, uint16_t val) {
    // Check if we have seen this ROC already:
    if(shiftReg.size() <= roc) shiftReg.resize(roc+1,0);
    shiftReg.at(roc) <<= 1;
    if(val&1) shiftReg.at(roc)++;

    // Count this bit:
    if(count.size() <= roc) count.resize(roc+1,0);
    count.at(roc)++;

    if(val&2) { // start marker detected
      if (count.at(roc) == 16) {
	// Write out the collected data:
	if(readback.size() <= roc) readback.resize(roc+1);
	readback.at(roc).push_back(shiftReg.at(roc));

	LOG(logDEBUGPIPES) << "Readback ROC "
			   << static_cast<int>(roc+GetChannel()*GetTokenChainLength())
			   << " " << ((readback.at(roc).back()>>8)&0x00ff)
			   << " (0x" << std::hex << ((readback.at(roc).back()>>8)&0x00ff)
			   << std::dec << "): " << (readback.at(roc).back()&0xff)
			   << " (0x" << std::hex << (readback.at(roc).back()&0xff)
			   << std::dec << ")";
      }
      else {
	// If this is the first readback cycle of the ROC, ignore the mismatch:
	if(readback.size() <= roc || readback.at(roc).empty()) {
	  LOG(logDEBUGAPI) << "ROC " << static_cast<int>(roc)
			   << ": first readback marker after "
			   << count.at(roc) << " readouts. Ignoring error condition.";
	}
	else {
	  LOG(logWARNING) << "ROC " << static_cast<int>(roc)
			  << ": Readback start marker after "
			  << count.at(roc) << " readouts!";
	  decodingStats.m_errors_roc_readback++;
	}
      }
      // Reset the counter for this ROC:
      count.at(roc) = 0;
    }
  }

  statistics dtbEventDecoder::getStatistics() { 
    // Automatically clear the statistics after it was read out:
    statistics tmp = decodingStats;
    decodingStats.clear();
    return tmp;
  }

  std::vector<std::vector<uint16_t> > dtbEventDecoder::getReadback() {
    // Automatically clear the readback vector after it was read out:
    std::vector<std::vector<uint16_t> > tmp = readback;
    readback.clear();
    return tmp;
  }
}
