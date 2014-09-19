#include "datapipe.h"
#include "helper.h"
#include "constants.h"
#include "exceptions.h"

namespace pxar {

  uint16_t dtbSource::FillBuffer() {
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

  Event* dtbEventDecoder::DecodeDeser400() {

    roc_Event.Clear();
    return &roc_Event;
  }

  Event* dtbEventDecoder::DecodeDeser160() {

    roc_Event.Clear();
    return &roc_Event;
  }
}
