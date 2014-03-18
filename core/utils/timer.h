#ifdef WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#include <ctime>
#endif

namespace pxar {

  class timer {
  public:
    /** Default timer constructor, storing the start time
     */
    timer() { start = GetTime(); }

    uint64_t get() { return static_cast<uint64_t>(GetTime() - start); }
  private:
    /** Private member function to store start time of the timer object
     */
    uint64_t start;

    /** Returns the amount of milliseconds elapsed since the UNIX epoch.
	Works on both windows and linux.
      */
    uint64_t GetTime() {
#ifdef WIN32
      // Windows
      FILETIME ft;
      LARGE_INTEGER li;

      // Get the amount of 100 nano seconds intervals elapsed since 
      // January 1, 1601 (UTC) and copy it to a LARGE_INTEGER structure.
      GetSystemTimeAsFileTime(&ft);
      li.LowPart = ft.dwLowDateTime;
      li.HighPart = ft.dwHighDateTime;

      uint64_t ret = li.QuadPart;
      // Convert from file time to UNIX epoch time.
      ret -= 116444736000000000LL; 
      // From 100 nano seconds (10^-7) to 1 millisecond (10^-3) intervals
      ret /= 10000;

      return ret;
#else
      // Linux
      struct timeval tv;

      gettimeofday(&tv, NULL);

      uint64_t ret = tv.tv_usec;
      // Convert from micro seconds (10^-6) to milliseconds (10^-3)
      ret /= 1000;

      // Adds the seconds (10^0) after converting them to milliseconds (10^-3)
      ret += (tv.tv_sec * 1000);

      return ret;
#endif
    }

    /** Overloaded ostream operator to give the current elapsed time.
     */
    friend std::ostream & operator<<(std::ostream &out, timer &t) {
      return out << static_cast<uint64_t>(t.GetTime() - t.start);
    }

    /** Overloaded ostream operator to give the current elapsed time.
     */
    friend std::ostream & operator<<(std::ostream &out, timer * t) {
      return out << static_cast<uint64_t>(t->GetTime() - t->start);
    }
  };

}
