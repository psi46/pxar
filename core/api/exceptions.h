/**
 * pxar API exception classes
 */

#ifndef PXAR_EXCEPTIONS_H
#define PXAR_EXCEPTIONS_H

#include <exception>

namespace pxar {

  /** Base class for all exceptions thrown by the pxar framework */
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


  class InvalidConfig : public pxarException {
  public:
    InvalidConfig(const std::string& what_arg) : pxarException(what_arg) {}
  };

  class FirmwareVersionMismatch : public pxarException {
  public:
    FirmwareVersionMismatch(const std::string& what_arg) : pxarException(what_arg) {}
  };

  class RPCError : public pxarException {
  public:
    RPCError(const std::string& what_arg) : pxarException(what_arg) {}
  };

  /** What exceptions do we need?

   * RPC exception catched
   * UsbFailure: USB communication failed
   * Range check: value out of range (DACs etc.)
   * Data format/structure size invalid
   * Some object not initialized yet (HAL/DUT)

   */
  
} //namespace pxar

#endif /* PXAR_EXCEPTIONS_H */
