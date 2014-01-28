#include "helper.h"

void util::mSleep(unsigned ms){
#ifdef WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}


std::tm util::localtime( std::time_t t ) {
#ifdef _MSC_VER >= 1400 
    // MSVCRT (2005+): std::localtime is threadsafe
    return *std::localtime(&t) ;
#else 
    // POSIX
    std::tm temp ;
    return *::localtime_r( &t, &temp ) ;
#endif 
    // _MSC_VER
}
