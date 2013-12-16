#include <unistd.h>
#include "pxar.h"
#include <iostream>

int main()
{

  // Create new API instance:
  try {
    _api = new pxar::api();
  }
  catch (...) {
    std::cout << "pxar cauhgt an exception from the board. Exiting." << std::endl;
    return -1;
  }

  // Try some test or so:

  // Initialize the DUT (power it up and stuff):
  _api->initDUT();

  // Read current:
  std::cout << "Analog current: " << _api->getTBia() << "mA" << std::endl;

  sleep(10);

  // And end that whole thing correcly:
  std::cout << "Done." << std::endl;
  delete _api;
  return 0;
}
