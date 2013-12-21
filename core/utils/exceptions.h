/**
 * pxar API exception classes
 */

#ifndef PXAR_EXCEPTIONS_H
#define PXAR_EXCEPTIONS_H

#include <exception>

namespace pxar {

  class pxarException : public std::exception {
  public:
  pxarException() : std::exception("pxarException") {}
    virtual const char* what() const throw() {
      return "pxar exception";
    }
  };
  

  /** What exceptions do we need?

   * RPC exception catched
   * Range check: value out of range (DACs etc.)
   * Data format/structure size invalid
   * Some object not initialized yet (HAL/DUT)

   */
  
} //namespace pxar

#endif /* PXAR_EXCEPTIONS_H */
