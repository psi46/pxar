#ifdef WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

/** Cannot use stdint.h when running rootcint on WIN32 */
#if ((defined WIN32) && (defined __CINT__))
typedef int int32_t;
typedef unsigned int uint32_t;
typedef unsigned short int uint16_t;
typedef unsigned char uint8_t;
#else
#include <stdint.h>
#endif

namespace pxar {

  /** Delay helper function
   *  Uses usleep() to wait the given time in milliseconds
   */
  void mDelay(uint32_t ms) {
    // Wait for the given time in milliseconds:
#ifdef WIN32
    Sleep(ms);
#else
    usleep(ms*1000);
#endif
  }

}
