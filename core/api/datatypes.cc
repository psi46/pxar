#include "datatypes.h"
#include "log.h"
#include "exceptions.h"
#include "constants.h"

namespace pxar {


  void pixel::decodeRaw(uint32_t raw, bool invert) {
    // Get the pulse height:
    setValue(static_cast<double>((raw & 0x0f) + ((raw >> 1) & 0xf0)));
    if((raw & 0x10) > 0) {
      LOG(logDEBUGAPI) << "invalid pulse-height fill bit from raw value of "<< std::hex << raw << std::dec << ": " << *this;
      throw DataInvalidPulseheightError("Error decoding pixel raw value");
    }

    // Decode the pixel address
    int r2 =    (raw >> 15) & 7;
    if(invert) { r2 ^= 0x7; }
    int r1 = (raw >> 12) & 7;
    if(invert) { r1 ^= 0x7; }
    int r0 = (raw >>  9) & 7;
    if(invert) { r0 ^= 0x7; }
    int r = r2*36 + r1*6 + r0;
    _row = 80 - r/2;
    _column = 2*(((raw >> 21) & 7)*6 + ((raw >> 18) & 7)) + (r&1);

    // Perform range checks:
    if(_row >= ROC_NUMROWS || _column >= ROC_NUMCOLS) {
      LOG(logDEBUGAPI) << "Invalid pixel from raw value of "<< std::hex << raw << std::dec << ": " << *this;
      if(_row == ROC_NUMROWS) throw DataCorruptBufferError("Error decoding pixel raw value");
      else throw DataInvalidAddressError("Error decoding pixel raw value");
    }
  }

  void statistics::print() {
    // Print out the full statistics:
    LOG(logINFO) << "Decoding statistics:";
    LOG(logINFO) << "  Event errors: \t        " << this->errors_event();
    LOG(logINFO) << "\t missing ROC header(s): " << this->errors_event_roc_missing();
    LOG(logINFO) << "  Decoding errors: \t     " << this->errors_decoding();
    LOG(logINFO) << "\t pixel address:         " << this->errors_decoding_pixel();
    LOG(logINFO) << "\t pulse height fill bit: " << this->errors_decoding_pulseheight();
    LOG(logINFO) << "\t buffer corruption:     " << this->errors_decoding_buffer_corrupt();
  }

  void statistics::clear() {
    //FIXME fill...
  }

  statistics& operator+=(statistics &lhs, const statistics &rhs) {
    // Event errors:
    lhs.m_errors_event_roc_missing += rhs.m_errors_event_roc_missing;
    // Pixel decoding errors:
    lhs.m_errors_decoding_pixel += rhs.m_errors_decoding_pixel;
    lhs.m_errors_decoding_pulseheight += rhs.m_errors_decoding_pulseheight;
    lhs.m_errors_decoding_buffer_corrupt += rhs.m_errors_decoding_buffer_corrupt;
    // FIXME fill...
    return lhs;
  }

} // namespace pxar
