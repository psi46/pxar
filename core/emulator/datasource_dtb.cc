#include "datasource_dtb.h"
#include "helper.h"
#include "generator.h"
#include "log.h"
#include "constants.h"
#include "exceptions.h"

namespace pxar {

  uint16_t dtbSource::FillBuffer() {
    pos = 0;
    buffer.clear();

    // Add some fake data:
    
    // Add a TBM header if necessary:
    if(envelopetype != TBM_NONE) {
      buffer.push_back(0xa019);
      buffer.push_back(0x8007);
    }

    // For every ROC configured, add one noise hit:
    for(size_t roc = 0; roc < chainlength; roc++) {
      // Add a ROC header:
      if(envelopetype != TBM_NONE) buffer.push_back(0x47f8);
      else buffer.push_back(0x07f8);

      // Add one pixel hit:
      pxar::pixel px = getNoiseHit(roc,0,0);
      buffer.push_back(0x2000 | ((px.encode() >> 12) & 0x0fff));
      buffer.push_back(0x1000 | (px.encode() & 0x0fff));
    }

    // Add a TBM trailer if necessary:
    if(envelopetype != TBM_NONE) {
      buffer.push_back(0xe000);
      buffer.push_back(0xc002);
    }
    // Adjust event start and end marker:
    else {
      buffer.at(0) = 0x8000 | (buffer.front() & 0x0fff);
      buffer.at(buffer.size()-1) = 0x4000 | (buffer.back() & 0x0fff);
    }

    LOG(logDEBUGPIPES) << "-------------------------";
    LOG(logDEBUGPIPES) << "Channel " << static_cast<int>(channel)
		       << " (" << static_cast<int>(chainlength) << " ROCs)"
		       << (envelopetype == TBM_NONE ? " DESER160 " : (envelopetype == TBM_EMU ? " SOFTTBM " : " DESER400 "));
    LOG(logDEBUGPIPES) << "Remaining: as many as you want. ";
    LOG(logDEBUGPIPES) << "-------------------------";
    LOG(logDEBUGPIPES) << "FULL RAW DATA BLOB:";
    LOG(logDEBUGPIPES) << listVector(buffer,true);
    LOG(logDEBUGPIPES) << "-------------------------";

    return lastSample = buffer[pos++];
  }

}
