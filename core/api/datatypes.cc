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

  void statistics::dump() {
    // Print out the full statistics:
    LOG(logINFO) << "Decoding statistics:";
    LOG(logINFO) << "  General information:";
    LOG(logINFO) << "\t 16bit words read:         " << this->info_words_read();
    LOG(logINFO) << "\t empty events:             " << this->info_events_empty();
    LOG(logINFO) << "\t valid events with pixels: " << this->info_events_valid();
    LOG(logINFO) << "\t valid pixel hits:         " << this->info_pixels_valid();
    LOG(logINFO) << "  Event errors: \t           " << this->errors_event();
    LOG(logINFO) << "\t start marker:             " << this->errors_event_start();
    LOG(logINFO) << "\t stop marker:              " << this->errors_event_stop();
    LOG(logINFO) << "\t overflow:                 " << this->errors_event_overflow();
    LOG(logINFO) << "\t invalid 5bit words:       " << this->errors_event_invalid_words();
    LOG(logINFO) << "\t invalid XOR eye diagram:  " << this->errors_event_invalid_xor();
    LOG(logINFO) << "  TBM errors: \t\t           " << this->errors_tbm();
    LOG(logINFO) << "\t flawed TBM headers:       " << this->errors_tbm_header();
    LOG(logINFO) << "\t flawed TBM trailers:      " << this->errors_tbm_trailer();
    LOG(logINFO) << "\t event ID mismatches:      " << this->errors_tbm_eventid_mismatch();
    LOG(logINFO) << "  ROC errors: \t\t           " << this->errors_roc();
    LOG(logINFO) << "\t missing ROC header(s):    " << this->errors_roc_missing();
    LOG(logINFO) << "\t misplaced readback start: " << this->errors_roc_readback();
    LOG(logINFO) << "  Pixel decoding errors:\t   " << this->errors_pixel();
    LOG(logINFO) << "\t pixel data incomplete:    " << this->errors_pixel_incomplete();
    LOG(logINFO) << "\t pixel address:            " << this->errors_pixel_address();
    LOG(logINFO) << "\t pulse height fill bit:    " << this->errors_pixel_pulseheight();
    LOG(logINFO) << "\t buffer corruption:        " << this->errors_pixel_buffer_corrupt();
  }

  void statistics::clear() {
    m_info_words_read = 0;
    m_info_events_empty = 0;
    m_info_events_valid = 0;
    m_info_pixels_valid = 0;

    m_errors_event_start = 0;
    m_errors_event_stop = 0;
    m_errors_event_overflow = 0;
    m_errors_event_invalid_words = 0;
    m_errors_event_invalid_xor = 0;

    m_errors_tbm_header = 0;
    m_errors_tbm_trailer = 0;
    m_errors_tbm_eventid_mismatch = 0;

    m_errors_roc_missing = 0;
    m_errors_roc_readback = 0;

    m_errors_pixel_incomplete = 0;
    m_errors_pixel_address = 0;
    m_errors_pixel_pulseheight = 0;
    m_errors_pixel_buffer_corrupt = 0;
  }

  statistics& operator+=(statistics &lhs, const statistics &rhs) {
    // Informational bits:
    lhs.m_info_words_read += rhs.m_info_words_read;
    lhs.m_info_events_empty += rhs.m_info_events_empty;
    lhs.m_info_events_valid += rhs.m_info_events_valid;
    lhs.m_info_pixels_valid += rhs.m_info_pixels_valid;

    // Event errors:
    lhs.m_errors_event_start += rhs.m_errors_event_start;
    lhs.m_errors_event_stop += rhs.m_errors_event_stop;
    lhs.m_errors_event_overflow += rhs.m_errors_event_overflow;
    lhs.m_errors_event_invalid_words += rhs.m_errors_event_invalid_words;
    lhs.m_errors_event_invalid_xor += rhs.m_errors_event_invalid_xor;

    // TBM errors:
    lhs.m_errors_tbm_header += rhs.m_errors_tbm_header;
    lhs.m_errors_tbm_trailer += rhs.m_errors_tbm_trailer;
    lhs.m_errors_tbm_eventid_mismatch += rhs.m_errors_tbm_eventid_mismatch;

    // ROC errors:
    lhs.m_errors_roc_missing += rhs.m_errors_roc_missing;
    lhs.m_errors_roc_readback += rhs.m_errors_roc_readback;

    // Pixel decoding errors:
    lhs.m_errors_pixel_incomplete += rhs.m_errors_pixel_incomplete;
    lhs.m_errors_pixel_address += rhs.m_errors_pixel_address;
    lhs.m_errors_pixel_pulseheight += rhs.m_errors_pixel_pulseheight;
    lhs.m_errors_pixel_buffer_corrupt += rhs.m_errors_pixel_buffer_corrupt;

    return lhs;
  }

} // namespace pxar
