#include "datatypes.h"
#include "helper.h"
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

  void pixel::decodeLinear(uint32_t raw) {
    // Get the pulse height:
    setValue(static_cast<double>((raw & 0x0f) + ((raw >> 1) & 0xf0)));
    if((raw & 0x10) > 0) {
      LOG(logDEBUGAPI) << "invalid pulse-height fill bit from raw value of "<< std::hex << raw << std::dec << ": " << *this;
      throw DataInvalidPulseheightError("Error decoding pixel raw value");
    }

    // Perform checks on the fill bits:
    if((raw & 0x1000) > 0 || (raw & 0x100000) > 0) {
      LOG(logDEBUGAPI) << "invalid address fill bit from raw value of "<< std::hex << raw << std::dec << ": " << *this;
      throw DataInvalidAddressError("Error decoding pixel raw value");
    }

    // Decode the pixel address
    _column = ((raw >> 17) & 0x07) + ((raw >> 18) & 0x38);
    _row = ((raw >> 9) & 0x07) + ((raw >> 10) & 0x78);
    
    // Perform range checks:
    if(_row >= ROC_NUMROWS || _column >= ROC_NUMCOLS) {
      LOG(logDEBUGAPI) << "Invalid pixel from raw value of "<< std::hex << raw << std::dec << ": " << *this;
      if(_row == ROC_NUMROWS) throw DataCorruptBufferError("Error decoding pixel raw value");
      else throw DataInvalidAddressError("Error decoding pixel raw value");
    }
  }

  uint8_t pixel::translateLevel(uint16_t x, int16_t level0, int16_t level1, int16_t levelS) {
    int16_t y = expandSign(x) - level0;
    if (y >= 0) y += levelS; else y -= levelS;
    return level1 ? y/level1 + 1: 0;
  }

  void pixel::decodeAnalog(std::vector<uint16_t> analog, int16_t ultrablack, int16_t black) {
    // Check pixel data length:
    if(analog.size() != 6) {
      LOG(logDEBUGAPI) << "Received wrong number of data words for a pixel: " << analog.size();
      throw DataInvalidAddressError("Received wrong number of data words for a pixel");
    }

    // Calculate the levels:
    int16_t level0 = black;
    int16_t level1 = (black - ultrablack)/4;
    int16_t levelS = level1/2;

    // Get the pulse height:
    setValue(static_cast<double>(expandSign(analog.back() & 0x0fff) - level0));

    // Decode the pixel address
    int c1 = translateLevel(analog.at(0),level0,level1,levelS);
    int c0 = translateLevel(analog.at(1),level0,level1,levelS);
    int c  = c1*6 + c0;

    int r2 = translateLevel(analog.at(2),level0,level1,levelS);
    int r1 = translateLevel(analog.at(3),level0,level1,levelS);
    int r0 = translateLevel(analog.at(4),level0,level1,levelS);
    int r  = (r2*6 + r1)*6 + r0;

    _row = 80 - r/2;
    _column = 2*c + (r&1);

    // Perform range checks:
    if(_row >= ROC_NUMROWS || _column >= ROC_NUMCOLS) {
      LOG(logDEBUGAPI) << "Invalid pixel from levels "<< listVector(analog) << ": " << *this;
      throw DataInvalidAddressError("Error decoding pixel address");
    }
  }

  uint32_t pixel::encode() {
    uint32_t raw = 0;
    // Set the pulse height:
    raw = ((static_cast<int>(value()) & 0xf0) << 1) + (static_cast<int>(value()) & 0xf);

    // Encode the pixel address
    int r = 2*(80 - _row);
    raw |= ((r/36) << 15);
    raw |= (((r%36)/6) << 12);
    raw |= (((r%36)%6 + _column%2) << 9);

    int dcol = _column/2;
    raw |= ((dcol)/6 << 21);
    raw |= (((dcol%6)) << 18);

    LOG(logDEBUGPIPES) << "Pix  " << static_cast<int>(_column) << "|" 
		       << static_cast<int>(_row) << " = "
		       << dcol << "/" << r << " = "
		       << dcol/6 << " " << dcol%6 << " "
		       << r/36 << " " << (r%36)/6 << " " << (r%36)%6;

    // Return the 24 bits belonging to the pixel:
    return (raw & 0x00ffffff);
  }

  uint32_t pixel::encodeLinear() {
    uint32_t raw = 0;
    // Set the pulse height:
    raw = ((static_cast<int>(value()) & 0xf0) << 1) + (static_cast<int>(value()) & 0xf);

    // Encode the pixel address
    raw |= ((_column & 0x07) << 17) | ((_column & 0x38) << 18);
    raw |= ((_row & 0x07) << 9) | ((_row & 0x78) << 10);

    LOG(logDEBUGPIPES) << "Pix  " << static_cast<int>(_column) << "|" 
		       << static_cast<int>(_row) << " = "
		       << (raw & 0x00ffffff);

    // Return the 24 bits belonging to the pixel:
    return (raw & 0x00ffffff);
  }

  /** Overloaded ostream operator for simple printing of Event data
   */
  std::ostream & operator<<(std::ostream &out, Event& evt) {
    // FIXME fix printout of multiple headers/trailers
    out << "====== " << std::hex << listVector(evt.getHeaders(),true) << std::dec << " ====== ";
    for (std::vector<pixel>::iterator it = evt.pixels.begin(); it != evt.pixels.end(); ++it)
      out << (*it) << " ";
    out << "====== " << std::hex << listVector(evt.getTrailers(),true) << std::dec << " ====== ";
    return out;
  }

  std::vector<uint8_t> Event::triggerCounts() {
    std::vector<uint8_t> counts;
    for(size_t i = 0; i < this->header.size(); i++) {
      counts.push_back(this->triggerCount(i));
    }
    return counts;
  }
  
  std::vector<uint8_t> Event::dataIDs() {
    std::vector<uint8_t> counts;
    for(size_t i = 0; i < this->header.size(); i++) {
      counts.push_back(this->dataID(i));
    }
    return counts;
  }

  std::vector<uint8_t> Event::dataValues() {
    std::vector<uint8_t> counts;
    for(size_t i = 0; i < this->header.size(); i++) {
      counts.push_back(this->dataValue(i));
    }
    return counts;
  }

  std::vector<bool> Event::haveTokenPass() {
    std::vector<bool> counts;
    for(size_t i = 0; i < this->trailer.size(); i++) {
      counts.push_back(this->hasTokenPass(i));
    }
    return counts;
  }

  std::vector<bool> Event::haveNoTokenPass() {
    std::vector<bool> counts;
    for(size_t i = 0; i < this->trailer.size(); i++) {
      counts.push_back(this->hasNoTokenPass(i));
    }
    return counts;
  }

  std::vector<bool> Event::haveResetTBM() {
    std::vector<bool> counts;
    for(size_t i = 0; i < this->trailer.size(); i++) {
      counts.push_back(this->hasResetTBM(i));
    }
    return counts;
  }

  std::vector<bool> Event::haveResetROC() {
    std::vector<bool> counts;
    for(size_t i = 0; i < this->trailer.size(); i++) {
      counts.push_back(this->hasResetROC(i));
    }
    return counts;
  }

  std::vector<bool> Event::haveSyncError() {
    std::vector<bool> counts;
    for(size_t i = 0; i < this->trailer.size(); i++) {
      counts.push_back(this->hasSyncError(i));
    }
    return counts;
  }

  std::vector<bool> Event::haveSyncTrigger() {
    std::vector<bool> counts;
    for(size_t i = 0; i < this->trailer.size(); i++) {
      counts.push_back(this->hasSyncTrigger(i));
    }
    return counts;
  }

  std::vector<bool> Event::haveClearTriggerCount() {
    std::vector<bool> counts;
    for(size_t i = 0; i < this->trailer.size(); i++) {
      counts.push_back(this->hasClearTriggerCount(i));
    }
    return counts;
  }

  std::vector<bool> Event::haveCalTrigger() {
    std::vector<bool> counts;
    for(size_t i = 0; i < this->trailer.size(); i++) {
      counts.push_back(this->hasCalTrigger(i));
    }
    return counts;
  }

  std::vector<bool> Event::stacksFull() {
    std::vector<bool> counts;
    for(size_t i = 0; i < this->trailer.size(); i++) {
      counts.push_back(this->stackFull(i));
    }
    return counts;
  }

  std::vector<bool> Event::haveAutoReset() {
    std::vector<bool> counts;
    for(size_t i = 0; i < this->trailer.size(); i++) {
      counts.push_back(this->hasAutoReset(i));
    }
    return counts;
  }

  std::vector<bool> Event::havePkamReset() {
    std::vector<bool> counts;
    for(size_t i = 0; i < this->trailer.size(); i++) {
      counts.push_back(this->hasPkamReset(i));
    }
    return counts;
  }

  std::vector<uint8_t> Event::stackCounts() {
    std::vector<uint8_t> counts;
    for(size_t i = 0; i < this->trailer.size(); i++) {
      counts.push_back(this->stackCount(i));
    }
    return counts;
  }
  
  void Event::printHeader() {
    LOG(logINFO) << "Header contents: \t" << listVector(getHeaders(),true,false,true);
    LOG(logINFO) << "\t Event ID \t" << listVector(this->triggerCounts(),false,false,true);
    LOG(logINFO) << "\t Data ID \t" << listVector(this->dataIDs(),false,false,true);
    LOG(logINFO) << "\t      Values \t" << listVector(this->dataValues(),false,false,true);
  }

  void Event::printTrailer() {
    LOG(logINFO) << "Trailer content: \t" << listVector(getTrailers(),true,false,true);
    LOG(logINFO) << "\t Token Pass \t" << textBools(this->haveTokenPass(),true);
    LOG(logINFO) << "\t Reset TBM \t" << textBools(this->haveResetTBM(),true);
    LOG(logINFO) << "\t Reset ROC \t" << textBools(this->haveResetROC(),true);
    LOG(logINFO) << "\t Sync Err \t" << textBools(this->haveSyncError(),true);
    LOG(logINFO) << "\t Sync Trigger \t" << textBools(this->haveSyncTrigger(),true);
    LOG(logINFO) << "\t ClearTrig Cnt \t" << textBools(this->haveClearTriggerCount(),true);
    LOG(logINFO) << "\t Cal Trigger \t" << textBools(this->haveCalTrigger(),true);
    LOG(logINFO) << "\t Stack Full \t" << textBools(this->stacksFull(),true);

    LOG(logINFO) << "\t Auto Reset \t" << textBools(this->haveAutoReset(),true);
    LOG(logINFO) << "\t PKAM Reset \t" << textBools(this->havePkamReset(),true);
    LOG(logINFO) << "\t Stack Count \t" << listVector(this->stackCounts(),false,false,true);
  }

  void statistics::dump() {
    // Print out the full statistics:
    LOG(logINFO) << "Decoding statistics:";
    LOG(logINFO) << "  General information:";
    LOG(logINFO) << "\t 16bit words read:         " << this->info_words_read();
    LOG(logINFO) << "\t valid events total:       " << this->info_events_total();
    LOG(logINFO) << "\t empty events:             " << this->info_events_empty();
    LOG(logINFO) << "\t valid events with pixels: " << this->info_events_valid();
    LOG(logINFO) << "\t valid pixel hits:         " << this->info_pixels_valid();
    LOG(logINFO) << "  Event errors: \t           " << this->errors_event();
    LOG(logINFO) << "\t start marker:             " << this->errors_event_start();
    LOG(logINFO) << "\t stop marker:              " << this->errors_event_stop();
    LOG(logINFO) << "\t overflow:                 " << this->errors_event_overflow();
    LOG(logINFO) << "\t invalid 5bit words:       " << this->errors_event_invalid_words();
    LOG(logINFO) << "\t invalid XOR eye diagram:  " << this->errors_event_invalid_xor();
    LOG(logINFO) << "\t frame (failed synchr.):   " << this->errors_event_frame();
    LOG(logINFO) << "\t idle data (no TBM trl):   " << this->errors_event_idledata();
    LOG(logINFO) << "\t no data (only TBM hdr):   " << this->errors_event_nodata();
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
    m_errors_event_frame = 0;
    m_errors_event_idledata = 0;
    m_errors_event_nodata = 0;

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

  tbmConfig::tbmConfig(uint8_t tbmtype) : dacs(), type(tbmtype), hubid(31), core(0xE0), tokenchains(), enable(true) {

    if(tbmtype == 0x0) {
      LOG(logCRITICAL) << "Invalid TBM type \"" << tbmtype << "\"";
      throw InvalidConfig("Invalid TBM type.");
    }
    
    // Standard setup for token chain lengths:
    // Four ROCs per stream for dual-400MHz, eight ROCs for single-400MHz readout:
    if(type >= TBM_09) { for(size_t i = 0; i < 2; i++) tokenchains.push_back(4); }
    else if(type >= TBM_08) { tokenchains.push_back(8); }
  }

} // namespace pxar
