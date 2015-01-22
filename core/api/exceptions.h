/**
 * pxar API exception classes
 */

#ifndef PXAR_EXCEPTIONS_H
#define PXAR_EXCEPTIONS_H

#include <exception>
#include <string>

namespace pxar {

  /** Base class for all exceptions thrown by the pxar framework.
   */
  class pxarException : public std::exception {
  public:
    pxarException(const std::string& what_arg) : std::exception(),ErrorMessage(what_arg) {}
    ~pxarException() throw() {};
    virtual const char* what() const throw(){
      return ErrorMessage.c_str();
    };
  private:
    std::string ErrorMessage;
  };

  /**  This class of exceptions covers issues with the configuration found during runtime:
   *    - out-of-range parameters
   *    - missing (crucial) parameters
   *    - inconsistent or mismatched configuration sets
   */
  class InvalidConfig : public pxarException {
  public:
    InvalidConfig(const std::string& what_arg) : pxarException(what_arg) {}
  };

  /**  This exception class covers issues with a DTB firmware version
   *   mismatch (i.e. between the RPC interfaces of pxar and the NIOS code).
   */
  class FirmwareVersionMismatch : public pxarException {
  public:
    FirmwareVersionMismatch(const std::string& what_arg) : pxarException(what_arg) {}
  };

  /**  This exception class covers read/write issues during the USB communication 
   *   or problems opening the connection to the specified testboard.
   */
  class UsbConnectionError : public pxarException {
  public:
    UsbConnectionError(const std::string& what_arg) : pxarException(what_arg) {}
  };

  /**  This exception class is used for timeouts occuring during USB readout.
   */
  class UsbConnectionTimeout : public pxarException {
  public:
    UsbConnectionTimeout(const std::string& what_arg) : pxarException(what_arg) {}
  };

  /**  This exception class is used when the DAQ readout is incomplete (i.e. missing events).
   */
  class DataMissingEvent : public pxarException {
  public:
    uint32_t numberMissing;
    DataMissingEvent(const std::string& what_arg, uint32_t nmiss) : pxarException(what_arg), numberMissing(nmiss) {}
  };

  /**  This exception class is used when raw pixel values could not be decoded
   */
  class DataDecodingError : public pxarException {
  public:
    DataDecodingError(const std::string& what_arg) : pxarException(what_arg) {}
  };

  /** This exception class is used when out-of-range pixel addresses
   *  are found during the decoding of the raw values.
   */
  class DataInvalidAddressError : public DataDecodingError {
  public:
    DataInvalidAddressError(const std::string& what_arg) : DataDecodingError(what_arg) {}
  };

  /** This exception class is used when the pulse-height fill-bit (dividing the eight bits
   *  into two blocks of four bits) is not zero as it should.
   */
  class DataInvalidPulseheightError : public DataDecodingError {
  public:
    DataInvalidPulseheightError(const std::string& what_arg) : DataDecodingError(what_arg) {}
  };

  /** This exception class is used when the decoded pixel address returns row == 80, which
   *  points to corrupt data buffer rather than invalid address.
   */
  class DataCorruptBufferError : public DataDecodingError {
  public:
    DataCorruptBufferError(const std::string& what_arg) : DataDecodingError(what_arg) {}
  };
  
} //namespace pxar

#endif /* PXAR_EXCEPTIONS_H */
