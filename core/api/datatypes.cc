#include "datatypes.h"
#include "log.h"
#include "exceptions.h"
#include "constants.h"

namespace pxar {


  void pixel::decodeRaw(uint32_t raw, bool invert) {
    setValue(static_cast<double>((raw & 0x0f) + ((raw >> 1) & 0xf0)));
    if( (raw & 0x10) >0) {
      LOG(logDEBUGAPI) << "invalid pulse-height fill bit from raw value of "<< std::hex << raw << std::dec << ": " << *this;
      throw DataDecoderError("Error decoding pixel raw value");
    }
    int c =    (raw >> 21) & 7;
    c = c*6 + ((raw >> 18) & 7);
    
    int r2 =    (raw >> 15) & 7;
    if(invert) { r2 ^= 0x7; }
    int r1 = (raw >> 12) & 7;
    if(invert) { r1 ^= 0x7; }
    int r0 = (raw >>  9) & 7;
    if(invert) { r0 ^= 0x7; }
    int r = r2*36 + r1*6 + r0;
    
    _row = 80 - r/2;
    _column = 2*c + (r&1);
    
    if (_row >= ROC_NUMROWS || _column >= ROC_NUMCOLS){
      LOG(logDEBUGAPI) << "invalid pixel from raw value of "<< std::hex << raw << std::dec << ": " << *this;
      throw DataDecoderError("Error decoding pixel raw value");
    }
  }

} // namespace pxar
