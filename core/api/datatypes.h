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

namespace pxar {

  /** Class for storing decoded pixel readout data
   */
  class DLLEXPORT pixel {
  public:

    /** Default constructor for pixel objects, defaulting all member variables to zero
     */
  pixel() : roc_id(0), column(0), row(0), _mean(0), _variance(0) {}

    /** Constructor for pixel objects with address and value initialization.
     */
  pixel(uint8_t _roc_id, uint8_t _column, uint8_t _row, double _value) : roc_id(_roc_id), column(_column), row(_row), _variance(0) { setValue(_value); }

    /** Constructor for pixel objects with rawdata pixel address & value and ROC id initialization.
     */
  pixel(uint32_t rawdata, uint8_t rocid, bool invertAddress = false) : roc_id(rocid) { decodeRaw(rawdata,invertAddress); }

    /** Getter function to return ROC ID
     */
    uint8_t getRoc() { return roc_id; };

    /** Getter function to return column id
     */
    uint8_t getColumn() { return column; };

    /** Getter function to return row id
     */
    uint8_t getRow() { return row; };

    /** ROC ID - continuously numbered according to their appeareance
     *  in the readout chain
     */
    uint8_t roc_id;

    /** Pixel column address
     */
    uint8_t column;

    /** Pixel row address
     */
    uint8_t row;

    /** Member function to get the signal variance for this pixel hit
     */
    double getVariance() { return expandFloat(_variance); };

    /** Member function to set the signal variance for this pixel hit
     */
    void setVariance(double var) { _variance = compactFloat(var); };

    /** Member function to get the value stored for this pixel hit
     */
    double getValue() { 
      return static_cast<double>(_mean);
    };

    /** Member function to get the value stored for this pixel hit
     */
    void setValue(double val) { 
      _mean = static_cast<int16_t>(val);
    };

    /** Overloaded comparison operator
     */
    bool operator == (const pixel& px) {
      return ((px.roc_id == roc_id )
	      && (px.column == column)
	      && (px.row == row));
    }

    /** Overloaded < operator
     */
    bool operator < (const pixel& px) const {
      if(roc_id == px.roc_id) {
	if(column == px.column) {
	  return row < px.row;
	}
	return column < px.column;
      }
      return roc_id < px.roc_id;
    }

  private:
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

    /** Helper function to compress double input value into
     *  a 16bit fixed-width integer for storage
     */
    uint16_t compactFloat(double input) {
      return round(input*std::numeric_limits<uint16_t>::max());
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
      out << "ROC " << static_cast<int>(px.roc_id)
	  << " [" << static_cast<int>(px.column) << "," << static_cast<int>(px.row) 
	  << "," << static_cast<double>(px.getValue()) << "]";
      return out;
    }
  };

  /** Class to store Events containing a header and a std::vector of pixels
   */
  class DLLEXPORT Event {
  public:
    Event() : header(0), trailer(0), pixels(), numDecoderErrors(0) {}
    void Clear() { header = 0; trailer = 0; pixels.clear(); numDecoderErrors=0;}
    uint16_t header;
    uint16_t trailer;
    std::vector<pixel> pixels;
    uint16_t numDecoderErrors;
  private:
    /** Overloaded ostream operator for simple printing of Event data
     */
    friend std::ostream & operator<<(std::ostream &out, Event& evt) {
      out << "====== " << std::hex << static_cast<uint16_t>(evt.header) << std::dec << " ====== ";
      for (std::vector<pixel>::iterator it = evt.pixels.begin(); it != evt.pixels.end(); ++it)
	out << (*it) << " ";
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
   *  By default, pixelConfigs have the  mask bit set.
   */
  class DLLEXPORT pixelConfig {
  public:
  pixelConfig() : 
    column(0), row(0), 
      trim(15), mask(true), enable(false) {}
  pixelConfig(uint8_t _column, uint8_t _row, uint8_t _trim) : 
    column(_column), row(_row), trim(_trim),
      mask(true), enable(false) {}
    uint8_t column;
    uint8_t row;
    uint8_t roc_id;
    uint8_t trim;
    bool mask;
    bool enable;
  };

  /** Class for ROC states
   *
   *  Contains a DAC map for the ROC programming settings, a type flag, enable switch
   *  and a vector of pixelConfigs.
   */
  class DLLEXPORT rocConfig {
  public:
  rocConfig() : pixels(), dacs(), type(0), enable(true) {}
    std::vector< pixelConfig > pixels;
    std::map< uint8_t,uint8_t > dacs;
    uint8_t type;
    uint8_t i2c_address;
    bool enable;
  };

  /** Class for TBM states
   *
   *  Contains a register map for the device register settings, a type flag and an enable switch
   */
  class DLLEXPORT tbmConfig {
  public:
  tbmConfig() : dacs(), type(0), enable(true) {}
    std::map< uint8_t,uint8_t > dacs;
    uint8_t type;
    bool enable;
  };

}
#endif
