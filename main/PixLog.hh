// Logging System Implementation. Defined Logs:
//    pix::LogDebug
//    pix::LogInfo
//    pix::LogError
//    pix::endl
//
// Each of them dumps log messages into output file. It is possible to set
// message head (for example class name where message was posted from).
//
// Differences:
//   LogDebug   Dump all messages into file
//   LogInfo    print messages on screen via std::cout, dump into output
//              Info file, dump into Debug
//   LogError   print messages on screen via std::cerr, dump into output
//              Errorr file, dump into Debug
//
// Use pix::endl with all Loggers. std::endl does not work. Nothing will be
// dummped into file unless it is opened. File will be automatically closed
// at the end of program.
//
// Examples using LogInfo. LogDebug and LogError work in the same way:
//
//   pix::LogInfo.setOutput( "info.log"); // set output filename: works only once
//
//   pix::LogInfo << "This is a message into Info Log" << pix::endl;
//
//   pix::LogInfo() << "Analog of previous line: not head is output"
//                  << pix::endl;
//
//   pix::LogInfo( "Head") << "This is a message with head" << pix::endl;
//
//   pix::LogInfo( __func__) << "Message from some function" << pix::endl;
//
//   pix::LogInfo( "Test1") << "Voltage: " << _voltage << pix::endl;

#ifndef PIXLOG_HH
#define PIXLOG_HH

#include <stdio.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <TGTextView.h>
#include <TTimeStamp.h>

namespace pix
{
  // Colors
  enum Color { Default,
               Black , Red , Green , Yellow , Blue , Pink , Cyan , White , 
               BlackB, RedB, GreenB, YellowB, BlueB, PinkB, CyanB, WhiteB };

  class LEnd {};

  extern LEnd endl;

  // Logging Base Class. It's tasks:
  //   - Open Logging file (only once)
  //   - Perform actual logging. Can not be called directly. Use inheritance to
  //     define custom logging system. See examples below.
  template<class L>
  class Log
  {
  public:
    explicit Log() {} // force explicit use of Logging system
    virtual ~Log() {}

    // WARNING: Log file can be opened only once
    void setOutput( const std::string &file, TGTextView *logger);
    TGTextView *fLogger;
    TTimeStamp fnow;
      
  protected:
    // Actual logging is done here via template method. Nothing will
    // be logged unless file is opened.
    template<class T> void log( const T &_val);

    void log( const LEnd &_endl);

    bool setHead( const std::string &_head);

    inline const std::string getHead() const { return head; }

    void logHead();

  private:
    // Prevent Copying including children
    Log( const Log &);
    Log &operator =( const Log &);
        

    // File will be automatically closed when AutoPointer gets released 
    // resulting in deleting object it refers to and calling
    // std::ofstream::~ofstream this way which in its turn calls
    // std::ofstream::close.
    static std::auto_ptr<std::ofstream> output;

    std::string head;
  };
  // Plugs (empty classes). They are used to Instantiate Log Services
  class Info;
  class Debug;
  class Error;

  // LogDebug should be used for all debugging (intermediate) information:
  // voltages, currents, 'hello world's, 'I am here', etc. Its output is
  // stored in separate file and not displayed on Monitor. Very useful for
  // later review by experts.
  class _LogDebug: public Log<Debug>
  {
  public:
    static _LogDebug &instance();

    template<class T>
    _LogDebug &operator <<( const T &_val);

    _LogDebug &operator <<( const Color &_color);
    _LogDebug &operator <<( const LEnd  &_endl);

    // Set Head for message. It will only be output for new lines.
    _LogDebug &operator()( const std::string &_head = "");
    _LogDebug &operator()(       std::string  _head,
				 const unsigned int &_width);

  private:
    _LogDebug(): loghead( false), newline( true) {}

    bool loghead;
    bool newline;

    static std::auto_ptr<_LogDebug> ginstance;
  };

  class _LogInfo: public Log<Info>
  {
  public:
    static _LogInfo &instance();

    template<class T>
    _LogInfo &operator <<( const T &_val);

    _LogInfo &operator <<( const Color &_color);
    _LogInfo &operator <<( const LEnd  &_endl);

    // Set Head for message. It will only be output for new lines.
    _LogInfo &operator()( const std::string &_head = "");
    _LogInfo &operator()(       std::string  _head,
				const unsigned int &_width);

  private:
    _LogInfo(): loghead( false), newline( true) {}

    bool loghead;
    bool newline;

    static std::auto_ptr<_LogInfo> ginstance;
  };

  class _LogError: public Log<Error>
  {
  public:
    static _LogError &instance();

    template<class T>
    _LogError &operator <<( const T &_val);

    _LogError &operator <<( const Color &_color);
    _LogError &operator <<( const LEnd  &_endl);

    // Set Head for message. It will only be output for new lines.
    _LogError &operator()( const std::string &_head = "");
    _LogError &operator()(       std::string  _head,
				 const unsigned int &_width);

  private:
    _LogError(): loghead( false), newline( true) {}

    bool loghead;
    bool newline;

    static std::auto_ptr<_LogError> ginstance;
  };

  extern _LogDebug &LogDebug;
  extern _LogInfo  &LogInfo;
  extern _LogError &LogError;
}

// ----------------------------------------------------------------------------
// Log
template<class L> std::auto_ptr<std::ofstream> pix::Log<L>::output;

// Definitions
template<class L>
template<class T>
void pix::Log<L>::log( const T &_val)
{
  if( output.get() ) *output << _val;
}

template<class L>
void pix::Log<L>::log( const LEnd &_endl)
{
  if( output.get() ) *output << std::endl;
}

template<class L>
void pix::Log<L>::setOutput( const std::string &_file, TGTextView *logger)
{
  // Set output file only once
  if( output.get() || _file.empty() ) return;
  // Set TGTextView 
  fLogger = logger;

  output.reset( new std::ofstream( _file.c_str() ) );
}

template<class L>
bool pix::Log<L>::setHead( const std::string &_head)
{
  return _head.size() ? ( head = _head, true) : false; 
}

template<class L>
void pix::Log<L>::logHead()
{
  if( head.size() ) log( ( head + "] ").insert( 0, "[") );
}

// _LogDebug templates
template<class T> pix::_LogDebug &pix::_LogDebug::operator <<( const T &_val)
{
  // Dump message into Debug file
  if( newline && loghead) {
    logHead();

    loghead = false;
  }

  newline = false;

  log( _val); return *this;
}

// _LogInfo templates
template<class T> pix::_LogInfo &pix::_LogInfo::operator <<( const T &_val)
{
  // Dump message into Info file
  if( newline) 
    { 
      if( loghead) {
        logHead();

        std::cout << ( getHead() + "\033[0m] ").insert( 0, "[\033[1;30m");
  
	loghead = false;
      }
      newline = false;
    }
  UInt_t h = 0;
  UInt_t m = 0;
  UInt_t s = 0;

  setHead("Pixelgui V1.0");
  fnow.Set();
  fnow.GetTime(false, 0, &h,&m,&s);
  fLogger->AddLine(Form("%d:%d:%d - %s",h,m,s,_val));
  fLogger->ShowBottom();
  log(Form("%s - %s",fnow.AsString("s"), _val)); 
  //fixme    LogDebug << Form("%s - %s",fnow.AsString("c"), _val) ;
    
  //    std::cout << _val;

  return *this;
}

// _LogError templates
template<class T> pix::_LogError &pix::_LogError::operator <<( const T &_val)
{
  // Dump message into Error file
  if( newline) 
    { 
      if( loghead) {
        logHead();

        //std::cerr << ( getHead() + "\033[0m] ").insert( 0, "[\033[1;31mERROR: ");
	fLogger->AddLine(getHead().c_str());
        fLogger->ShowBottom();
        loghead = false;
      }

      newline = false;
    }

  log( _val); //fixme LogDebug << _val;

  //std::cerr << _val;

  return *this;
}

#endif // End PIXLOG_HH
