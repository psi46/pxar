#include "datapipe.h"
#include "helper.h"
#include "constants.h"
#include "exceptions.h"

namespace pxar {

  uint16_t dtbSource::FillBuffer() {

    LOG(logDEBUGPIPES) << "-------------------------";
    LOG(logDEBUGPIPES) << "Channel " << static_cast<int>(channel)
		       << " (" << static_cast<int>(chainlength) << " ROCs)"
		       << (envelopetype == TBM_NONE ? " DESER160 " : (envelopetype == TBM_EMU ? " SOFTTBM " : " DESER400 "));
    LOG(logDEBUGPIPES) << "Remaining " << static_cast<int>(dtbRemainingSize);
    LOG(logDEBUGPIPES) << "-------------------------";
    LOG(logDEBUGPIPES) << "FULL RAW DATA BLOB:";
    LOG(logDEBUGPIPES) << listVector(buffer,true);
    LOG(logDEBUGPIPES) << "-------------------------";

    throw dsBufferEmpty();
    return 0;
  }

  rawEvent* dtbEventSplitter::SplitDeser400() {
    record.Clear();
    return &record;
  }

  rawEvent* dtbEventSplitter::SplitDeser160() {
    record.Clear();
    return &record;
  }

  rawEvent* dtbEventSplitter::SplitSoftTBM() {
    record.Clear();
    return &record;
  }

  Event* dtbEventDecoder::DecodeDeser400() {

    roc_Event.Clear();
    return &roc_Event;
  }

  Event* dtbEventDecoder::DecodeDeser160() {

    roc_Event.Clear();
    return &roc_Event;
  }

  Event* dtbEventDecoder::DecodeAnalog() {

    roc_Event.Clear();
    return &roc_Event;
  }

}
