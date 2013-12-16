#ifndef PXAR_HAL_H
#define PXAR_HAL_H

#include "rpc_impl.h"

namespace pxar {

  class hal
  {

  public:
    hal(std::string name = "*");

    void Configure();

  private:

    /** Private instance of the testboard RPC interface
     */
    CTestboard * tb;

    void PrintInfo();
    void CheckCompatibility();

  };

}
#endif
