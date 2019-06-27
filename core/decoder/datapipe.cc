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
    // Split the data stream according to TBMEMU alignment markers:
    else if(GetEnvelopeType() == TBM_EMU) { SplitSoftTBM(); }
    // Split the data stream according to DESER400 alignment markers:
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
    // Store the TBM header word and attach the channel ID in unused header bits:
    record.Add(GetLast() | ((GetChannel() & 0x7) << 8));

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

  void dtbEventSplitter::SplitSoftTBM() {
    // If last one had Event end marker, get a new sample:
    if (!nextStartDetected) { Get(); }

    // If new sample does not have start marker keep on reading until we find it:
    if ((GetLast() & 0xe000) != 0xa000) {
      record.SetStartError();
      Get();
    }
    record.Add(GetLast());

    // Else keep reading and adding samples until we find the last trailer marker.
    // Make sure to look for "c0" and not "c" - the latter one is also the DESER160 end marker!
    while ((Get() & 0xef00) != 0xc000) {
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

    if(dump_count < 100 && (GetFlags() & FLAG_DUMP_FLAWED_EVENTS) != 0) {
      // Store the current error count for comparison:
      // Exclude pixel decoding problems, we are looking for more serious things...
      error_count = decodingStats.errors_event()
	+ decodingStats.errors_tbm()
	+ decodingStats.errors_roc();

      std::stringstream thisevent; thisevent << *sample;
      event_ringbuffer.at(total_event%7) = thisevent.str();
    }

    // Count possibe error states:
    if(sample->IsStartError()) { decodingStats.m_errors_event_start++; }
    if(sample->IsEndError()) { decodingStats.m_errors_event_stop++; }
    if(sample->IsOverflow()) { decodingStats.m_errors_event_overflow++; }
    decodingStats.m_info_words_read += sample->GetSize();

    try {
      // If a TBM header and trailer should be available, process them first:
      if(GetEnvelopeType() != TBM_NONE) { ProcessTBM(sample); }

      // Decode ADC Data for analog devices:
      if(GetDeviceType() < ROC_PSI46DIG) { DecodeADC(sample); }
      // Decode DESER400 Data for digital devices and TBMs:
      else if(GetEnvelopeType() > TBM_EMU) { DecodeDeser400(sample); }
      // Decode DESER160 Data for digital devices without real TBM
      else { DecodeDeser160(sample); }
    }
    catch(DataDeserializerError /*e*/) {
      // Clearing event content:
      roc_Event.Clear();
    }

    if(dump_count < 100 && (GetFlags() & FLAG_DUMP_FLAWED_EVENTS) != 0) {
      if(error_count != (decodingStats.errors_event()
			 + decodingStats.errors_tbm()
			 + decodingStats.errors_roc())) { flawed_event = total_event; }

      if(total_event == flawed_event+3) {
	// Dump the ring buffer:
	LOG(logERROR) << "Dumping the flawed event +- 3 events:";
	for(size_t i = total_event; i < total_event+event_ringbuffer.size(); i++) {
	  LOG(logERROR) << event_ringbuffer.at(i%7);
	}
	dump_count++;
	if(dump_count == 100) {
	  LOG(logERROR) << "Channel " << static_cast<int>(GetChannel()) << ": Reached 100 dumped events, stopping now...";
	}
      }
      total_event++;
    }

    LOG(logDEBUGPIPES) << roc_Event;
    return &roc_Event;
  }

  void dtbEventDecoder::ProcessTBMHeader(uint16_t h1, uint16_t h2) {
    // Check the alignment markers to be correct:
    if((h1 & 0xe000) != 0xa000 || (h2 & 0xe000) != 0x8000)
      { decodingStats.m_errors_tbm_header++; }
    // Store the two header words:
    roc_Event.addHeader(((h1 & 0x00ff) << 8) + (h2 & 0x00ff));

    LOG(logDEBUGPIPES) << "TBM " << static_cast<int>(GetChannel()) << " Header:";
    IFLOG(logDEBUGPIPES) { roc_Event.printHeader(); }
  }

  void dtbEventDecoder::ProcessTBMTrailer(uint16_t t1, uint16_t t2) {
    // Check the alignment markers to be correct:
    if((t1 & 0xe000) != 0xe000 || (t2 & 0xe000) != 0xc000)
      { decodingStats.m_errors_tbm_trailer++; }

    // Check possible DESER400 error flags in the TBM trailer:
    if((t1 & 0x1000) == 0x1000 || (t2 & 0x1000) == 0x1000) {
      // Currently the same error bits are stored in both trailer words, so only evaluating one of them.
      evalDeser400Errors(t1);
    }

    // No Error flag is set by the DESER400, just decode the TBM header as usual:
    else {
      // Store the two trailer words:
      roc_Event.addTrailer(((t1 & 0x00ff) << 8) + (t2 & 0x00ff));

      LOG(logDEBUGPIPES) << "TBM " << static_cast<int>(GetChannel()) << " Trailer:";
      IFLOG(logDEBUGPIPES) roc_Event.printTrailer();
    }
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
    ProcessTBMHeader(sample->data.at(0),sample->data.at(1));

    // TBM Trailer:
    ProcessTBMTrailer(sample->data.at(size-2),sample->data.at(size-1));

    // Check for correct TBM event ID:
    CheckEventID();

    // Remove header and trailer:
    sample->data.erase(sample->data.begin(), sample->data.begin() + 2);
    sample->data.erase(sample->data.end() - 2, sample->data.end());
  }

  void dtbEventDecoder::DecodeDeser400(rawEvent * sample) {
    LOG(logDEBUGPIPES) << "Decoding ROC data from DESER400...";

    // Count the ROC headers:
    int16_t roc_n = -1;

    // Check if ROC has inverted pixel address (ROC_PSI46DIG):
    bool invertedAddress = ( GetDeviceType() == ROC_PSI46DIG ? true : false );
    // Check if ROC is a Layer1 chip with different address encoding:
    bool linearAddress = ( GetDeviceType() >= ROC_PROC600 ? true : false );

    // Loop over the full data:
    for(std::vector<uint16_t>::iterator word = sample->data.begin(); word != sample->data.end(); word++) {

      // Check if we have a ROC header:
      if(((*word) & 0xe000) == 0x4000) {

	// Count ROC Headers up:
	roc_n++;

	// Maybe store the XOR sum:
	if((GetFlags() & FLAG_ENABLE_XORSUM_LOGGING) != 0) { xorsum.push_back(((*word) & 0x0ff0) >> 4); }

	// Check for DESER400 failure:
	if(((*word) & 0x0ff0) == 0x0ff0) {
	  LOG(logCRITICAL) << "Channel " << static_cast<int>(GetChannel())
			   << " ROC " << static_cast<int>(roc_n)
			   << " header reports DESER400 failure!";
	  decodingStats.m_errors_event_invalid_xor++;
	  throw DataDecodingError("Invalid XOR eye diagram encountered.");
	}

	// Decode the readback bits in the ROC header:
	if(GetDeviceType() >= ROC_PSI46DIGV2) { evalReadback(static_cast<uint8_t>(roc_n),(*word)); }
      }
      // FIXME for linearized channels read from EUDAQ, check for interleaved TBM headers and trailers:
      else if(((*word) & 0xe000) == 0xa000) {
	uint16_t tmp = *word;
	ProcessTBMHeader(tmp,*(++word));
      }
      else if(((*word) & 0xe000) == 0xe000) {
	uint16_t tmp = *word;
	ProcessTBMTrailer(tmp,*(++word));
      }
      // We have a pixel hit:
      else if(((*word) & 0xe000) <= 0x2000) {

	// Only one word left or unexpected alignment marker:
	if(sample->data.end() - word < 2 || ((*word) & 0x8000)) {
	  decodingStats.m_errors_pixel_incomplete++;
	  break;
	}

	// FIXME optional check:
	// (*word) >> 13 == 0
	// (*(word+1) >> 13 == 1

	uint32_t raw = (((*word) & 0x0fff) << 12) + ((*(++word)) & 0x0fff);
	try {
	  // Check if this is just fill bits of the TBM09 data stream
	  // accounting for the other channel:
	  if(GetEnvelopeType() >= TBM_09 && (raw&0xffffff) == 0xffffff) {
	    LOG(logDEBUGPIPES) << "Empty hit detected (TBM09 data streams). Skipping.";
	    continue;
	  }

	  // Get the correct ROC id: Channel number x ROC offset (= token chain length)
	  // TBM08x: channel 0: 0-7, channel 1: 8-15
	  // TBM09x: channel 0: 0-3, channel 1: 4-7, channel 2: 8-11, channel 3: 12-15
	  pixel pix(raw,static_cast<uint8_t>(roc_n + GetTokenChainOffset()),invertedAddress,linearAddress);
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

    // FIXME: woohoo, this is soooo evil:
    roc_Event.flipTrailers();

    // Check event validity (empty, missing ROCs...):
    CheckEventValidity(roc_n);
  }

  void dtbEventDecoder::DecodeADC(rawEvent * sample) {
    LOG(logDEBUGPIPES) << "Decoding ROC data from ADC...";

    int16_t roc_n = -1;

    // Reserve expected number of pixels from data length (subtract ROC headers):
    if (static_cast<int>(sample->GetSize()) - 3*GetTokenChainLength() > 0) {
      roc_Event.pixels.reserve((sample->GetSize() - 3*GetTokenChainLength())/6);
    }

    // Loop over the full data:
    for(std::vector<uint16_t>::iterator word = sample->data.begin(); word != sample->data.end(); word++) {

      // Not enough data for anything, stop here - and assume it was half a pixel hit:
      if((sample->data.end() - word < 2)) {
	decodingStats.m_errors_pixel_incomplete++;
	break;
      }

      // Check if we have another ROC header (UB and B levels):
      // Here we have to assume the first two words are a ROC header because we rely on
      // its Ultrablack and Black level as initial values for auto-calibration:

      if(roc_n < 0 ||
	 // Ultrablack level:
	 ((ultrablack-levelS < expandSign((*word) & 0x0fff) && ultrablack+levelS > expandSign((*word) & 0x0fff))
	  // Black level:
	  && (black-levelS < expandSign((*(word+1)) & 0x0fff) && black+levelS > expandSign((*(word+1)) & 0x0fff)))) {

	roc_n++;
	// Save the lastDAC value:
	evalLastDAC(roc_n, (*(word+2)) & 0x0fff);

	// Iterate to improve ultrablack and black measurement:
	AverageAnalogLevel((*word) & 0x0fff, (*(word+1)) & 0x0fff);

	LOG(logDEBUGPIPES) << "ROC Header: "
			   << expandSign((*word) & 0x0fff) << " (avg. " << ultrablack << ") (UB) "
			   << expandSign((*(word+1)) & 0x0fff) << " (avg. " << black << ") (B) "
			   << expandSign((*(word+2)) & 0x0fff) << " (lastDAC) ";
	// Advance iterator:
	word +=  2;
      }
      // We have a pixel hit:
      else {
	// Not enough data for a new pixel hit (six words):
	if(sample->data.end() - word < 6) {
	  decodingStats.m_errors_pixel_incomplete++;
	  break;
	}

	std::vector<uint16_t> data;
	data.push_back((*word) & 0x0fff);
	for(size_t i = 0; i < 5; i++) { data.push_back((*(++word)) & 0x0fff); }

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
    // Check if ROC is a Layer1 chip with different address encoding:
    //    bool linearAddress = ( (GetDeviceType() == ROC_PROC600 || GetDeviceType() == ROC_PROC600V2 || GetDeviceType() == ROC_PROC600V3)  ? true : false );
    bool linearAddress = ( GetDeviceType() >= ROC_PROC600 ? true : false );

    // Reserve expected number of pixels from data length (subtract ROC headers):
    if(static_cast<int>(sample->GetSize())-GetTokenChainLength() > 0) {
      roc_Event.pixels.reserve((sample->GetSize()-GetTokenChainLength())/2);
    }

    // Loop over the full data:
    for(std::vector<uint16_t>::iterator word = sample->data.begin(); word != sample->data.end(); word++) {

      // Check if we have a ROC header:
      if(((*word) & 0x0ffc) == 0x07f8) {
	roc_n++;

	// Decode the readback bits in the ROC header:
	if(GetDeviceType() >= ROC_PSI46DIGV2) { evalReadback(roc_n,(*word) & 0x0fff); }
      }
      // We might have a pixel:
      // Require that we found at least one ROC header and have two or more words left:
      else if(roc_n >= 0) {
	// It's not a ROC header but the last word:
	if(sample->data.end() - word < 2) {
	  decodingStats.m_errors_pixel_incomplete++;
	  continue;
	}

	uint32_t raw = (((*word) & 0x0fff) << 12) + ((*(++word)) & 0x0fff);
	try {
	  pixel pix(raw,roc_n,invertedAddress,linearAddress);
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
    }

    // Check event validity (empty, missing ROCs...):
    CheckEventValidity(roc_n);
  }

  void dtbEventDecoder::CheckEventID() {
    // After startup, register the first event ID:
    if(eventID == -1) { eventID = roc_Event.triggerCount(); }

    // Check if event contains TBM reset:
    if(roc_Event.hasResetTBM()) {
      LOG(logDEBUGPIPES) << "Channel " <<  static_cast<int>(GetChannel())
			 << " Event ID reset due to ResetTBM";
      eventID = roc_Event.triggerCount();
    }

    // Check if the event ID cross-check is disabled:
    if((GetFlags() & FLAG_DISABLE_EVENTID_CHECK) == 0) {
      // Check if TBM event ID matches with expectation:
      if(roc_Event.triggerCount() != (eventID%256)) {
	LOG(logERROR) << "Channel " <<  static_cast<int>(GetChannel()) << " Event ID mismatch:  local ID (" << static_cast<int>(eventID)
		      << ") !=  TBM ID (" << static_cast<int>(roc_Event.triggerCount()) << ")";
	decodingStats.m_errors_tbm_eventid_mismatch++;
	// To continue readout, set event ID to the currently decoded one:
	eventID = roc_Event.triggerCount();
      }
    }

    // Increment event counter:
    eventID = (eventID%256) + 1;
  }

  void dtbEventDecoder::CheckEventValidity(int16_t roc_n) {

    // Check that we found all expected ROC headers:
    // If a PKAM has been detected, the NoTokenPass bis is set and the content should be discarded:
    if(roc_Event.hasPkamReset() && roc_Event.hasNoTokenPass()) {
      LOG(logERROR) << "Channel " <<  static_cast<int>(GetChannel())
		    << " detected a PKAM reset, event cleared.";

      // This breaks the readback for the missing roc, let's ignore this readback cycle for all ROCs:
      std::fill(readback_dirty.begin(), readback_dirty.end(), true);
      // Clearing event content:
      roc_Event.Clear();
    }
    // In case of a NoTokenPass flag, no ROC headers are expected
    else if(roc_Event.hasNoTokenPass() && (roc_n+1 > 0)) {
      LOG(logERROR) << "Channel " <<  static_cast<int>(GetChannel())
		    << " has NoTokenPass but " << static_cast<int>(roc_n+1)
		    << " ROCs were found";
      decodingStats.m_errors_roc_missing++;
      // This breaks the readback for the missing roc, let's ignore this readback cycle for all ROCs:
      std::fill(readback_dirty.begin(), readback_dirty.end(), true);
      // Clearing event content:
      roc_Event.Clear();
    }
    // If the number of ROCs does not correspond to what we expect clear the event and return
    else if(roc_Event.hasTokenPass() && (roc_n+1 != GetTokenChainLength())) {
      LOG(logERROR) << "Channel " <<  static_cast<int>(GetChannel()) << " Number of ROCs (" << static_cast<int>(roc_n+1)
		    << ") != Token Chain Length (" << static_cast<int>(GetTokenChainLength()) << ")";
      decodingStats.m_errors_roc_missing++;
      // This breaks the readback for the missing roc, let's ignore this readback cycle for all ROCs:
      std::fill(readback_dirty.begin(), readback_dirty.end(), true);
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

  void dtbEventDecoder::AverageAnalogLevel(int16_t word1, int16_t word2) {

    // Take the mean for a window of 1000 samples, initial measurement included
    if(slidingWindow < 1000) {
      slidingWindow++;
      sumUB += expandSign(word1 & 0x0fff);
      ultrablack = static_cast<float>(sumUB)/slidingWindow;
      sumB += expandSign(word2 & 0x0fff);
      black = static_cast<float>(sumB)/slidingWindow;
    }
    // Sliding window:
    else {
      ultrablack = static_cast<float>(999)/1000*ultrablack + static_cast<float>(1)/1000*expandSign(word1 & 0x0fff);
      black = static_cast<float>(999)/1000*black + static_cast<float>(1)/1000*expandSign(word2 & 0x0fff);
    }
    levelS = (black - ultrablack)/8;
  }

  void dtbEventDecoder::evalDeser400Errors(uint16_t data) {

    // Evaluate the four error bits of the TBM trailer word:
    if((data & 0x0100) != 0x0000) {
      decodingStats.m_errors_event_nodata++;
      LOG(logWARNING) << "Detected DESER400 trailer error bits: \"NO DATA\"";
      throw DataDeserializerError("No data");
    }
    if((data & 0x0200) != 0x0000) {
      LOG(logWARNING) << "Detected DESER400 trailer error bits: \"IDLE DATA\"";
      decodingStats.m_errors_event_idledata++;
      throw DataDeserializerError("Idle data");
    }
    if((data & 0x0400) != 0x0000) {
      LOG(logWARNING) << "Detected DESER400 trailer error bits: \"CODE ERROR\"";
      decodingStats.m_errors_event_invalid_words++;
      throw DataDeserializerError("Code errro");
    }
    if((data & 0x0800) != 0x0000) {
      LOG(logWARNING) << "Detected DESER400 trailer error bits: \"FRAME ERROR\"";
      decodingStats.m_errors_event_frame++;
      throw DataDeserializerError("Frame error");
    }
  }

  void dtbEventDecoder::evalLastDAC(uint8_t roc, uint16_t val) {
    // Obey disable flag:
    if((GetFlags() & FLAG_DISABLE_READBACK_COLLECTION) != 0) { return; }

    // Check if we have seen this ROC already:
    if(readback.size() <= roc) readback.resize(roc+1);
    readback.at(roc).push_back(val);

    LOG(logDEBUGPIPES) << "Readback ROC "
		       << static_cast<int>(roc+GetChannel()*GetTokenChainLength())
		       << " " << static_cast<int>(expandSign(val & 0x0fff));
  }

  void dtbEventDecoder::evalReadback(uint8_t roc, uint16_t val) {
    // Obey disable flag:
    if((GetFlags() & FLAG_DISABLE_READBACK_COLLECTION) != 0) { return; }

    // Check if we have seen this ROC already:
    if(shiftReg.size() <= roc) shiftReg.resize(roc+1,0);
    if(readback_dirty.size() <= roc) readback_dirty.resize(roc+1,false);
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
	if(readback.size() <= roc || readback.at(roc).empty() || readback_dirty.at(roc)) {
	  LOG(logDEBUGAPI) << "Channel " <<  static_cast<int>(GetChannel()) << " ROC " << static_cast<int>(roc)
			   << ": first readback marker after "
			   << count.at(roc) << " readouts. Ignoring error condition.";
	  readback_dirty.at(roc) = false;
	}
	else {
	  LOG(logWARNING) << "Channel " <<  static_cast<int>(GetChannel()) << " ROC " << static_cast<int>(roc)
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

  std::vector<uint8_t> dtbEventDecoder::getXORsum() {
    // Automatically clear the XOR sum vector after it was read out:
    std::vector<uint8_t> tmp = xorsum;
    xorsum.clear();
    return tmp;
  }
}
