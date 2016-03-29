#ifndef PXAR_TYPES_H
#define PXAR_TYPES_H

/** Declare all classes that need to be included in shared libraries on Windows
 *  as class DLLEXPORT className
 */
#include "pxardllexport.h"

/** Cannot use stdint.h when running rootcint on WIN32 */
#if ((defined WIN32) && (defined __CINT__))
typedef int int32_t;
typedef short int int16_t;
typedef unsigned int uint32_t;
typedef unsigned short int uint16_t;
typedef unsigned char uint8_t;
#else
#include <stdint.h>
#endif

#include <iostream>
#include <vector>
#include <map>
#include <limits>
#include <cmath>

#include "constants.h"

namespace pxar {

  /** Class for storing decoded pixel readout data
   */
  class DLLEXPORT pixel {
  public:

    /** Default constructor for pixel objects, defaulting all member variables to zero
     */
  pixel() : _roc_id(0), _column(0), _row(0), _mean(0), _variance(0) {}

    /** Constructor for pixel objects with address and value initialization.
     */
  pixel(uint8_t roc_id, uint8_t column, uint8_t row, double value) : _roc_id(roc_id), _column(column), _row(row), _variance(0) { setValue(value); }

    /** Constructor for pixel objects with rawdata pixel address & value and ROC id initialization.
     */
  pixel(uint32_t rawdata, uint8_t rocid, bool invertAddress = false, bool linearAddress = false) : _roc_id(rocid) {
      if(linearAddress) { decodeLinear(rawdata); }
      else { decodeRaw(rawdata,invertAddress); }
    }

    /** Constructor for pixel objects with analog levels data, ultrablack & black levels and ROC id initialization.
     */
  pixel(std::vector<uint16_t> analogdata, uint8_t rocid, int16_t ultrablack, int16_t black) : _roc_id(rocid) { decodeAnalog(analogdata,ultrablack,black); }

    /** Getter function to return ROC ID
     */
    uint8_t roc() const { return _roc_id; };

    /** Setter function to set the ROC id
     */
    void setRoc(uint8_t roc) { _roc_id = roc; };

    /** Getter function to return column id
     */
    uint8_t column() const { return _column; };

    /** Setter function to set the column id
     */
    void setColumn(uint8_t column) { _column = column; };

    /** Getter function to return row id
     */
    uint8_t row() const { return _row; };

    /** Setter function to set the row id
     */
    void setRow(uint8_t row) { _row = row; };

    /** Member function to get the signal variance for this pixel hit
     */
    double variance() { return expandFloat(_variance); };

    /** Member function to set the signal variance for this pixel hit
     */
    void setVariance(double var) { _variance = compactFloat(var); };

    /** Member function to get the value stored for this pixel hit
     */
    double value() { 
      return static_cast<double>(_mean);
    };

    /** Member function to get the value stored for this pixel hit
     */
    void setValue(double val) { 
      _mean = static_cast<int16_t>(val);
    };

    /** Member function to re-encode pixel into raw data
     */
    uint32_t encode();

    /** Member function to re-encode pixel into raw data, linear address space
     */
    uint32_t encodeLinear();

    /** Overloaded comparison operator
     */
    bool operator == (const pixel& px) {
      return ((px.roc() == _roc_id )
	      && (px.column() == _column)
	      && (px.row() == _row));
    }

    /** Overloaded < operator
     */
    bool operator < (const pixel& px) const {
      if(_roc_id == px.roc()) {
	if(_column == px.column()) {
	  return _row < px.row();
	}
	return _column < px.column();
      }
      return _roc_id < px.roc();
    }

  private:
    /** ROC ID - continuously numbered according to their appeareance
     *  in the readout chain
     */
    uint8_t _roc_id;

    /** Pixel column address
     */
    uint8_t _column;

    /** Pixel row address
     */
    uint8_t _row;

    /** 16bit unsigned int for storing compressed floating point
     *  mean value (either pulse height or efficiency)
     */
    int16_t _mean;

    /** 16bit unsigned int for storing compressed floating point
     *  variance
     */
    uint16_t _variance;

    /** Decoding function for PSI46 dig raw ROC data. Parameter "invert"
     *  allows decoding of PSI46dig data which has an inverted pixel
     *  address.
     *  This function throws a pxar::DataDecodingError exception in
     *  case of a failed decoding attempts.
     */
    void decodeRaw(uint32_t raw, bool invert);

    /** Decoding function for PSI46digPlus raw ROC data with linear 
     *  address space.
     *  This function throws a pxar::DataDecodingError exception in
     *  case of a failed decoding attempts.
     */
    void decodeLinear(uint32_t raw);

    /** Decoding function for PSI46 analog levels ROC data. Parameters "black"
     *  and "ultrablack" refer to the ROC identifier levels and are used to calculate
     *  all other address levels.
     *  This function throws a pxar::DataDecodingError exception in
     *  case of a failed decoding attempts.
     */
    void decodeAnalog(std::vector<uint16_t> analog, int16_t ultrablack, int16_t black);

    /** Helper function to translate ADC values into address levels
     */
    uint8_t translateLevel(uint16_t x, int16_t level0, int16_t level1, int16_t levelS);

    /** Helper function to compress double input value into
     *  a 16bit fixed-width integer for storage
     */
    uint16_t compactFloat(double input) {
      return static_cast<uint16_t>(round(input*std::numeric_limits<uint16_t>::max()));
    }

    /** Helper function to expand 16bit fixed-width integer value to
     *  floating point value with precision roughly ~10^-4
     */
    double expandFloat(uint16_t input) {
      return static_cast<double>(input)/std::numeric_limits<uint16_t>::max();
    }

    /** Overloaded ostream operator for simple printing of pixel data
     */
    friend std::ostream & operator<<(std::ostream &out, pixel& px) {
      out << "ROC " << static_cast<int>(px.roc())
	  << " [" << static_cast<int>(px.column()) << "," << static_cast<int>(px.row()) 
	  << "," << static_cast<double>(px.value()) << "]";
      return out;
    }
  };

  /** Class to store Events containing a header and a std::vector of pixels
   */
  class DLLEXPORT Event {
  public:
    Event() : header(0), trailer(0), pixels() {}

    /** Helper function to clear the event content
     */
    void Clear() { header = 0; trailer = 0; pixels.clear();}

    /** TBM Header Information: returns the 8 bit event counter of the TBM
     */
    uint8_t triggerCount() { return ((header >> 8) & 0xff); };

    /** TBM Emulator Header: returns the phase of the trigger relative to the clock.
     *  These are the "data" bits stored by the SoftTBM and is equivalent to
     *  Event::dataValue()
     */
    uint8_t triggerPhase() { return dataValue(); };

    /** TBM Header Information: returns the Data ID bits
     */
    uint8_t dataID()       { return ((header & 0x00c0) >> 6); };

    /** TBM Header Information: returns the value for the data bits
     */
    uint8_t dataValue()    { return (header & 0x003f); };

    /** TBM Trailer Information: reports if no token out has been received
     *  returns true if the token was not passed successfully
     */
    bool hasNoTokenPass() { return ((trailer & 0x8000) != 0); };

    /** TBM Trailer Information: reports if no token out has been received
     *  returns true if the token out has been rceived correctly
     */
    bool hasTokenPass() { return !hasNoTokenPass(); };

    /** TBM Trailer Information: reports if a TBM reset has been sent
     */
    bool hasResetTBM()    { return ((trailer & 0x4000) != 0); };

    /** TBM Trailer Information: reports if a ROC reset has been sent
     */
    bool hasResetROC()    { return ((trailer & 0x2000) != 0); };

    /** TBM Trailer Information: reports if a sync error occured
     */
    bool hasSyncError()   { return ((trailer & 0x1000) != 0); };

    /** TBM Trailer Information: reports if event contains a sync trigger
     */
    bool hasSyncTrigger() { return ((trailer & 0x0800) != 0); };

    /** TBM Trailer Information: reports if the trigger count has been reset
     */
    bool hasClearTriggerCount() { return ((trailer & 0x0400) != 0); };

    /** TBM Trailer Information: reports if the event had a calibrate signal
     */
    bool hasCalTrigger()  { return ((trailer & 0x0200) != 0); };

    /** TBM Trailer Information: reports if the TBM stack is full
     */
    bool stackFull()      { return ((trailer & 0x0100) != 0); };

    /** TBM Trailer Information: reports if a auto reset has been sent
     */
    bool hasAutoReset() { return ((trailer & 0x0080) != 0); };

    /** TBM Trailer Information: reports if a PKAM counter reset has been sent
     */
    bool hasPkamReset() { return ((trailer & 0x0040) != 0); };

    /** TBM Trailer Information: reports the current 6 bit trigger stack count
     */
    uint8_t stackCount() { return (trailer & 0x003f); };

    /** TBM Header
     */
    uint16_t header;

    /** Helper function to print the TBM Header
     */
    void printHeader();

    /** TBM Trailer
     */
    uint16_t trailer;

    /** Helper function to print the TBM Trailer
     */
    void printTrailer();

    /** Vector of successfully decoded pxar::pixel objects
     */
    std::vector<pixel> pixels;

  private:

    /** Overloaded sum operator for adding up data from different events
     */
    friend Event& operator+=(Event &lhs, const Event &rhs) {
      // FIXME this currently only transports pixels, no header information:
      lhs.pixels.insert(lhs.pixels.end(), rhs.pixels.begin(), rhs.pixels.end());
      lhs.header = rhs.header;
      lhs.trailer = rhs.trailer;
      return lhs;
    };

    /** Overloaded ostream operator for simple printing of Event data
     */
    friend std::ostream & operator<<(std::ostream &out, Event& evt) {
      out << "====== " << std::hex << static_cast<uint16_t>(evt.header) << std::dec << " ====== ";
      for (std::vector<pixel>::iterator it = evt.pixels.begin(); it != evt.pixels.end(); ++it)
	out << (*it) << " ";
      out << "====== " << std::hex << static_cast<uint16_t>(evt.trailer) << std::dec << " ====== ";
      return out;
    }
  };


  /** Class to store raw evet data records containing a list of flags to indicate the 
   *  Event status as well as a vector of uint16_t data records containing the actual
   *  Event data in undecoded raw format.
   */
  class DLLEXPORT rawEvent {
  public:
  rawEvent() : data(), flags(0) {}
    void SetStartError() { flags |= 1; }
    void SetEndError()   { flags |= 2; }
    void SetOverflow()   { flags |= 4; }
    void ResetStartError() { flags &= static_cast<unsigned int>(~1); }
    void ResetEndError()   { flags &= static_cast<unsigned int>(~2); }
    void ResetOverflow()   { flags &= static_cast<unsigned int>(~4); }
    void Clear() { flags = 0; data.clear(); }
    bool IsStartError() { return (flags & 1) != 0; }
    bool IsEndError()   { return (flags & 2) != 0; }
    bool IsOverflow()   { return (flags & 4) != 0; }
	
    size_t GetSize() { return data.size(); }
    void Add(uint16_t value) { data.push_back(value); }
    uint16_t operator[](size_t index) { return data.at(index); }

    std::vector<uint16_t> data;

  private:
    /*
      bit 0 = misaligned start
      bit 1 = no end detected
      bit 2 = overflow
    */
    unsigned int flags;

    /** Overloaded sum operator for adding up data from different events
     */
    friend rawEvent& operator+=(rawEvent &lhs, const rawEvent &rhs) {
      // Add the raw data:
      lhs.data.insert(lhs.data.end(), rhs.data.begin(), rhs.data.end());
      // Also carry over event flags:
      lhs.flags |= rhs.flags;
      return lhs;
    };
    
    /** Overloaded ostream operator for simple printing of raw data blobs
     */
    friend std::ostream & operator<<(std::ostream &out, rawEvent& record) {
      out << "====== " << std::hex << static_cast<uint16_t>(record.flags) << std::dec << " ====== ";
      for (std::vector<uint16_t>::iterator it = record.data.begin(); it != record.data.end(); ++it)
	out << std::hex << (*it) << std::dec << " ";
      return out;
    }
  };

  /** Class to store the configuration for single pixels (i.e. their mask state,
   *  trim bit settings and whether they belong to the currently run test ("enable").
   *  By default, pixelConfigs have the mask bit set.
   */
  class DLLEXPORT pixelConfig {
  public:
  pixelConfig() : 
    _column(0), _row(0), _roc_id(0),
      _trim(15), _mask(true), _enable(false) {}
  pixelConfig(uint8_t column, uint8_t row, uint8_t trim, bool mask = true, bool enable = false) : 
    _column(column), _row(row), _roc_id(0), _trim(trim),
      _mask(mask), _enable(enable) {}
  pixelConfig(uint8_t roc, uint8_t column, uint8_t row, uint8_t trim, bool mask = true, bool enable = false) : 
    _column(column), _row(row), _roc_id(roc), _trim(trim),
      _mask(mask), _enable(enable) {}
    uint8_t column() const { return _column; }
    void setColumn(uint8_t column) { _column = column; }
    uint8_t row() const { return _row; }
    void setRow(uint8_t row) { _row = row; }
    uint8_t roc() const { return _roc_id; }
    void setRoc(uint8_t roc) { _roc_id = roc; }
    uint8_t trim() const { return _trim; }
    void setTrim(uint8_t trim) { _trim = trim; }
    bool mask() const { return _mask; }
    void setMask(bool mask) { _mask = mask; }
    bool enable() const { return _enable; }
    void setEnable(bool enable) { _enable = enable; }
  private:
    uint8_t _column;
    uint8_t _row;
    uint8_t _roc_id;
    uint8_t _trim;
    bool _mask;
    bool _enable;
  };

  /** Class for ROC states
   *
   *  Contains a DAC map for the ROC programming settings, a type flag, enable switch
   *  and a vector of pixelConfigs.
   */
  class DLLEXPORT rocConfig {
  public:
  rocConfig() : pixels(), dacs(), type(0), _enable(true) {}
    std::vector< pixelConfig > pixels;
    std::map< uint8_t,uint8_t > dacs;
    uint8_t type;
    uint8_t i2c_address;
    bool enable() const { return _enable; }
    void setEnable(bool enable) { _enable = enable; }
  private:
    bool _enable;
  };

  /** Class for TBM states
   *
   *  Contains a register map for the device register settings, a type flag and an enable switch
   */
  class DLLEXPORT tbmConfig {
  public:
    tbmConfig(uint8_t tbmtype);
    std::map< uint8_t,uint8_t > dacs;
    uint8_t type;
    uint8_t hubid;
    uint8_t core;
    std::vector<uint8_t> tokenchains;
    bool enable;

    // Check token pass setting:
    bool NoTokenPass() { return (dacs[0x00]&0x40); };
    // Return readable name of the core:
    std::string corename() { return ((core&0x10) ? "Beta" : "Alpha"); };
  };

  /** Class for statistics on event and pixel decoding
   *
   *  The class collects all decoding statistics gathered during one DAQ 
   *  session (i.e. one test command from pxarCore or one session started 
   *  with daqStart() and ended with daqStop()).
   */
  class DLLEXPORT statistics {
    /** Allow the dtbEventDecoder to directly alter private members of the statistics
     */
    friend class dtbEventDecoder;

  public:
  statistics() :
    m_info_words_read(0),
      m_info_events_empty(0),
      m_info_events_valid(0),
      m_info_pixels_valid(0),
      m_errors_event_start(0),
      m_errors_event_stop(0),
      m_errors_event_overflow(0),
      m_errors_event_invalid_words(0),
      m_errors_event_invalid_xor(0),
      m_errors_event_frame(0),
      m_errors_event_idledata(0),
      m_errors_event_nodata(0),
      m_errors_tbm_header(0),
      m_errors_tbm_trailer(0),
      m_errors_tbm_eventid_mismatch(0),
      m_errors_roc_missing(0),
      m_errors_roc_readback(0),
      m_errors_pixel_incomplete(0),
      m_errors_pixel_address(0),
      m_errors_pixel_pulseheight(0),
      m_errors_pixel_buffer_corrupt(0)
	{};
    // Print all statistics to stdout:
    void dump();
    friend statistics& operator+=(statistics &lhs, const statistics &rhs) {
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
      lhs.m_errors_event_frame += rhs.m_errors_event_frame;
      lhs.m_errors_event_idledata += rhs.m_errors_event_idledata;
      lhs.m_errors_event_nodata += rhs.m_errors_event_nodata;

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
    };

    uint32_t info_words_read() {return m_info_words_read; }
    uint32_t info_events_empty() {return m_info_events_empty; }
    uint32_t info_events_valid() {return m_info_events_valid; }
    uint32_t info_events_total() {return (m_info_events_valid + m_info_events_empty); }
    uint32_t info_pixels_valid() {return m_info_pixels_valid; }

    uint32_t errors() {
      return (errors_pixel() + errors_tbm() + errors_roc() + errors_event());
    };
    uint32_t errors_event() {
      return (errors_event_start()
	      + errors_event_stop()
	      + errors_event_overflow()
	      + errors_event_invalid_words()
	      + errors_event_invalid_xor()
	      + errors_event_frame()
	      + errors_event_idledata()
	      + errors_event_nodata());
    };
    uint32_t errors_tbm() {
      return (errors_tbm_header()
	      + errors_tbm_trailer()
	      + errors_tbm_eventid_mismatch());
    };
    uint32_t errors_roc() {
      return (errors_roc_missing()
	      + errors_roc_readback());
    };
    uint32_t errors_pixel() { 
      return (errors_pixel_incomplete()
	      + errors_pixel_address()
	      + errors_pixel_pulseheight()
	      + errors_pixel_buffer_corrupt());
    };
    uint32_t errors_event_start() { return m_errors_event_start; }
    uint32_t errors_event_stop() { return m_errors_event_stop; }
    uint32_t errors_event_overflow() { return m_errors_event_overflow; }
    uint32_t errors_event_invalid_words() { return m_errors_event_invalid_words; }
    uint32_t errors_event_invalid_xor() { return m_errors_event_invalid_xor; }
    uint32_t errors_event_frame() { return m_errors_event_frame; }
    uint32_t errors_event_idledata() { return m_errors_event_idledata; }
    uint32_t errors_event_nodata() { return m_errors_event_nodata; }

    uint32_t errors_tbm_header() { return m_errors_tbm_header; }
    uint32_t errors_tbm_eventid_mismatch() { return m_errors_tbm_eventid_mismatch; }
    uint32_t errors_tbm_trailer() { return m_errors_tbm_trailer; }
    uint32_t errors_roc_missing() { return m_errors_roc_missing; }
    uint32_t errors_roc_readback() { return m_errors_roc_readback; }
    uint32_t errors_pixel_incomplete() { return m_errors_pixel_incomplete; }
    uint32_t errors_pixel_address() { return m_errors_pixel_address; };
    uint32_t errors_pixel_pulseheight() { return m_errors_pixel_pulseheight; };
    uint32_t errors_pixel_buffer_corrupt() { return m_errors_pixel_buffer_corrupt; };
  private:
    // Clear all statistics:
    void clear();

    // Total number of words read:
    uint32_t m_info_words_read;
    // Total number of empty events (no pixel hit):
    uint32_t m_info_events_empty;
    // Total number of valid events (with pixel hits):
    uint32_t m_info_events_valid;
    // Total number of pixel hits:
    uint32_t m_info_pixels_valid;

    // Total number of events with misplaced start
    uint32_t m_errors_event_start;
    // Total number of events with misplaced stop:
    uint32_t m_errors_event_stop;
    // Total number of events with data overflow:
    uint32_t m_errors_event_overflow;
    // Total number of invalid 5bit words detected by DESER400:
    uint32_t m_errors_event_invalid_words;
    // Total number of events with invalid XOR eye diagram:
    uint32_t m_errors_event_invalid_xor;
    // Total number of DESER400 Frame errors (failed synchronization):
    uint32_t m_errors_event_frame;
    // Total number of DESER400 idle data errors (no TBM trailer received):
    uint32_t m_errors_event_idledata;
    // Total number of DESER400 no-data error (only TBM header received):
    uint32_t m_errors_event_nodata;

    // Total number of events with flawed TBM header:
    uint32_t m_errors_tbm_header;
    // Total number of events with flawed TBM trailer:
    uint32_t m_errors_tbm_trailer;
    // Total number of event ID mismatches in the datastream:
    uint32_t m_errors_tbm_eventid_mismatch;

    // Total number of events with missing ROC header(s):
    uint32_t m_errors_roc_missing;
    // Total number of misplaced ROC readback start markers:
    uint32_t m_errors_roc_readback;

    // Total number of undecodable pixels (data missing)
    uint32_t m_errors_pixel_incomplete;
    // Total number of undecodable pixels (by address)
    uint32_t m_errors_pixel_address;
    // Total number of undecodable pixels (by pulse height fill bit)
    uint32_t m_errors_pixel_pulseheight;
    // Total number of pixels with row 80:
    uint32_t m_errors_pixel_buffer_corrupt;
  };
}
#endif
