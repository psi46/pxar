#ifndef WIN32
#include <unistd.h>
#endif

#include "api.h"
#include <iostream>
#include <string>
#include <cstring>

int main(int argc, char* argv[]) {

  // API pointer:
  pxar::pxarCore * _api;

  // By default use wildcard as DTB name:
  std::string dtbname = "*";
  std::string verbosity = "INFO";
  std::string flashfilename;

  for (int i = 1; i < argc; i++) {
    // Setting name of the DTB:
    if (std::string(argv[i]) == "-n") { 
      dtbname = std::string(argv[++i]);
      std::cout << "Attempting to flash DTB \"" << dtbname << "\"." << std::endl;
      continue;
    }
    // Setting verbosity:
    if (std::string(argv[i]) == "-v") { 
      verbosity = std::string(argv[++i]);
      continue;
    } 
    // Use as flash file path:
    else { 
      flashfilename = std::string(argv[i]);
      std::cout << "Flashing file \"" << flashfilename << "\"." << std::endl;
    }
  }

  // Check if we have a file at all:
  if(flashfilename.compare("") == 0) {
    std::cout << "No flash file provided!\n";
    return -1;
  }


  // Create new API instance:
  try {
    _api = new pxar::pxarCore(dtbname, verbosity);

    // Let's flash the DTB with the provided file:
    _api->flashTB(flashfilename);

    // And end that whole thing correcly:
    delete _api;
    std::cout << "Flashing done. Power cycle the DTB now." << std::endl;
  }
  catch (pxar::UsbConnectionError &e) {
    std::cout << "pxar caught an exception due to a USB communication problem: " << e.what() << std::endl;
    // Do not delete the API - we don't have a live connection to the DTB...
    return -1;
  }
  catch (pxar::pxarException &e){
    std::cout << "pxar caught an internal exception: " << e.what() << std::endl;
    delete _api;
    return -1;    
  }
  catch (...) {
    std::cout << "pxar caught an unknown exception. Exiting." << std::endl;
    delete _api;
    return -1;
  }
  
  return 0;
}
