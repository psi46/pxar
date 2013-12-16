#include <unistd.h>
#include "pxar.h"
#include <iostream>

int main(int argc, char * argv[])
{

  // Create new API instance:
  try {
    api_inst = new pxar::api();
  }
  catch (...) {
    std::cout << "pXar cauhgt an exception from the board. Exiting." << std::endl;
    return -1;
  }

  std::cout << "Done." << std::endl;
  sleep(2);

  delete api_inst;
  return 0;
}
