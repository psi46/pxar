#include <unistd.h>
#include "pxar.h"
#include <iostream>

int main(int argc, char * argv[])
{

  // Create new API instance:
  try {
    pxar = new pXarAPI();
  }
  catch (...) {
    std::cout << "pXar cauhgt an exception from the board. Exiting." << std::endl;
    return -1;
  }

  //if (usbId == "*") cTestboard->FindDTB(usbId);
  /*  

  */
  //FIXME before initializing the attached hardware we should check for a needed firmware update!

  //FIXME would be a good point to transfer configs!
  pxar->Configure();

  sleep(10);

  delete pxar;
  return 0;
}
