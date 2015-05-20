#include "datasource_dtb.h"
#include "helper.h"
#include "log.h"
#include "constants.h"
#include "exceptions.h"

namespace pxar {

  uint16_t dtbSource::FillBuffer() {
    pos = 0;
    do {
      dtbState = 0;
    
      if (buffer.size() == 0) {
	if (stopAtEmptyData) throw dsBufferEmpty();
	if (dtbState) throw dsBufferOverflow();
      }
    } while (buffer.size() == 0);

    LOG(logDEBUGPIPES) << "-------------------------";
    LOG(logDEBUGPIPES) << "Channel " << static_cast<int>(channel)
		       << " (" << static_cast<int>(chainlength) << " ROCs)"
		       << (envelopetype == TBM_NONE ? " DESER160 " : (envelopetype == TBM_EMU ? " SOFTTBM " : " DESER400 "));
    LOG(logDEBUGPIPES) << "Remaining " << static_cast<int>(dtbRemainingSize);
    LOG(logDEBUGPIPES) << "-------------------------";
    LOG(logDEBUGPIPES) << "FULL RAW DATA BLOB:";
    LOG(logDEBUGPIPES) << listVector(buffer,true);
    LOG(logDEBUGPIPES) << "-------------------------";

    return lastSample = buffer[pos++];
  }

}
