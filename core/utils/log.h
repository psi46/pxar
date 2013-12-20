/**
 * pxar API logging class
 */

#ifndef PXAR_LOG_H
#define PXAR_LOG_H

#include <sstream>
#include <sys/time.h>
#include <iomanip>
#include <cstdio>

namespace pxar {

  enum TLogLevel {
    logQUIET,
    logSUMMARY,
    logERROR,
    logWARNING,
    logINFO,
    logDEBUG,
    logDEBUGAPI,
    logDEBUGHAL,
    logDEBUGRPC,
    logDEBUGUSB
  };

  class Log {
  public:
    Log();
    virtual ~Log();
    std::ostringstream& Get(TLogLevel level = logINFO);
  public:
    static TLogLevel& ReportingLevel();
    static std::string ToString(TLogLevel level);
    static TLogLevel FromString(const std::string& level);
  protected:
    std::ostringstream os;
  private:
    Log(const Log&);
    Log& operator =(const Log&);
    std::string NowTime();
  };

  inline Log::Log() : os() {}

  inline std::string Log::NowTime() {
    char buffer[11];
    time_t t;
    time(&t);
    tm r = * localtime(&t);//{0};
    strftime(buffer, sizeof(buffer), "%X", localtime_r(&t, &r));
    struct timeval tv;
    gettimeofday(&tv, 0);
    char result[100] = {0};
    std::sprintf(result, "%s.%03ld", buffer, static_cast<long>(tv.tv_usec) / 1000); 
    return result;
  }

  inline std::ostringstream& Log::Get(TLogLevel level) {
    os << "[" << NowTime();
    os << "] " << std::setw(7) << ToString(level) << ": ";
    os << std::string(level > logDEBUG ? level - logDEBUG : 0, '\t');
    return os;
  }

  inline Log::~Log() {
    os << std::endl;
    fprintf(stderr, "%s", os.str().c_str());
    fflush(stderr);
  }

  inline TLogLevel& Log::ReportingLevel() {
    static TLogLevel reportingLevel = logSUMMARY;
    return reportingLevel;
  }

  inline std::string Log::ToString(TLogLevel level) {
    static const char* const buffer[] = {"QUIET","SUMMARY","ERROR", "WARNING", "INFO", "DEBUG", "DEBUGAPI", "DEBUGHAL", "DEBUGRPC", "DEBUGUSB"};
    return buffer[level];
  }

  inline TLogLevel Log::FromString(const std::string& level) {
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
    if (level == "SUMMARY")
      return logSUMMARY;
    if (level == "QUIET")
      return logQUIET;
    Log().Get(logWARNING) << "Unknown logging level '" << level << "'. Using WARNING level as default.";
    return logWARNING;
  }

#define LOG(level)				\
  if (level > Log::ReportingLevel()) ;		\
  else Log().Get(level)

} //namespace pxar

#endif /* PXAR_LOG_H */
