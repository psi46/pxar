#include "datatypes.h"
#include "log.h"
#include "exceptions.h"
#include "constants.h"

namespace pxar {


  void pixel::decodeRaw(uint32_t raw, bool invert) {
    value = (raw & 0x0f) + ((raw >> 1) & 0xf0);
    int c =    (raw >> 21) & 7;
    c = c*6 + ((raw >> 18) & 7);
    
    int r2 =    (raw >> 15) & 7;
    if(invert) { r2 ^= 0x7; }
    int r1 = (raw >> 12) & 7;
    if(invert) { r1 ^= 0x7; }
    int r0 = (raw >>  9) & 7;
    if(invert) { r0 ^= 0x7; }
    int r = r2*36 + r1*6 + r0;
    
    row = 80 - r/2;
    column = 2*c + (r&1);
    
    if (row >= ROC_NUMROWS || column >= ROC_NUMCOLS){
      LOG(logCRITICAL) << "invalid pixel from raw value of "<< std::hex << raw << std::dec << ": " << *this;
      throw DataDecoderError("Error decoding pixel raw value");
    }
  }

} // namespace pxar
