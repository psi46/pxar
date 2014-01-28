/**
 * pxar API logging class
 */

#ifndef PXAR_LOG_H
#define PXAR_LOG_H

#include <sstream>
#ifndef WIN32
#include <sys/time.h>
#else
#include <time.h>
#include <windows.h>
#endif

#if defined ( WIN32 )
#define __func__ __FUNCTION__
#endif

#include <iomanip>
#include <cstdio>
#include <stdint.h>
#include <string.h>

#include "helper.h"

namespace pxar {

  enum TLogLevel {
    logQUIET,
    logCRITICAL,
    logERROR,
    logWARNING,
    logINFO,
    logDEBUG,
    logDEBUGAPI,
    logDEBUGHAL,
    logDEBUGRPC,
    logDEBUGUSB
  };

  template <typename T>
    class pxarLog {
  public:
    pxarLog();
    virtual ~pxarLog();
    std::ostringstream& Get(TLogLevel level = logINFO, std::string file = "", std::string function = "", uint32_t line = 0);
  public:
    static TLogLevel& ReportingLevel();
    static std::string ToString(TLogLevel level);
    static TLogLevel FromString(const std::string& level);
  protected:
    std::ostringstream os;
  private:
    pxarLog(const pxarLog&);
    pxarLog& operator =(const pxarLog&);
    std::string NowTime();
  };

  template <typename T>
    pxarLog<T>::pxarLog() {}

  template <typename T>
    std::string pxarLog<T>::NowTime() {
    char buffer[11];
    std::time_t t;
    time(&t);
    std::tm r = util::localtime(t);//{0};
    strftime(buffer, sizeof(buffer), "%X", &r);
    return buffer;
  }

  template <typename T>
    std::ostringstream& pxarLog<T>::Get(TLogLevel level, std::string file, std::string function, uint32_t line) {
    os << "[" << NowTime() << "] ";
    os << std::setw(8) << ToString(level) << ": ";
    
    // For debug levels we want also function name and line number printed:
    if (level == logDEBUG || level == logDEBUGAPI || level == logDEBUGHAL)
      os << "<" << file << "/" << function << ":L" << line << "> ";
    else if(level == logDEBUGRPC)
      os << "\"" << function << "\" ";

    return os;
  }

  template <typename T>
    pxarLog<T>::~pxarLog() {
    os << std::endl;
    T::Output(os.str());
  }

  template <typename T>
    TLogLevel& pxarLog<T>::ReportingLevel() {
    static TLogLevel reportingLevel = logINFO;
    return reportingLevel;
  }

  template <typename T>
    std::string pxarLog<T>::ToString(TLogLevel level) {
    static const char* const buffer[] = {"QUIET","CRITICAL","ERROR", "WARNING", "INFO", "DEBUG", "DEBUGAPI", "DEBUGHAL", "DEBUGRPC", "DEBUGUSB"};
    return buffer[level];
  }

  template <typename T>
    TLogLevel pxarLog<T>::FromString(const std::string& level) {
    if (level == "DEBUGUSB")
      return logDEBUGUSB;
    if (level == "DEBUGRPC")
      return logDEBUGRPC;
    if (level == "DEBUGHAL")
      return logDEBUGHAL;
    if (level == "DEBUGAPI")
      return logDEBUGAPI;
    if (level == "DEBUG")
      return logDEBUG;
    if (level == "INFO")
      return logINFO;
    if (level == "WARNING")
      return logWARNING;
    if (level == "ERROR")
      return logERROR;
    if (level == "CRITICAL")
      return logCRITICAL;
    if (level == "QUIET")
      return logQUIET;
    pxarLog<T>().Get(logWARNING) << "Unknown logging level '" << level << "'. Using WARNING level as default.";
    return logWARNING;
  }


  class SetLogOutput
  {
  public:
    static FILE*& Stream();
    static void Output(const std::string& msg);
  };

  inline FILE*& SetLogOutput::Stream()
  {
    static FILE* pStream = stderr;
    return pStream;
  }

  inline void SetLogOutput::Output(const std::string& msg)
  {   
    FILE* pStream = Stream();
    if (!pStream)
      return;
    fprintf(pStream, "%s", msg.c_str());
    fflush(pStream);
  }

typedef pxarLog<SetLogOutput> Log;

#define __FILE_NAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define LOG(level)				\
  if (level > pxar::Log::ReportingLevel() || !pxar::SetLogOutput::Stream()) ; \
  else pxar::Log().Get(level,__FILE_NAME__,__func__,__LINE__)

} //namespace pxar

#endif /* PXAR_LOG_H */
