#include "api.h"
#include "datatypes.h"
#include "log.h"
#include <stdlib.h>

namespace pxar {
  
  pxar::pixel getNoiseHit(uint8_t rocid, size_t i, size_t j) {

    // Generate a slightly random pulse height between 80 and 100:
    uint16_t pulseheight = rand()%20 + 80;

    // We can't pulse the same pixel twice in one trigger:
    size_t col = rand()%52;
    while(col == i) col = rand()%52;
    size_t row = rand()%80;
    while(row == j) row = rand()%80;

    pixel px = pixel(rocid,col,row,pulseheight);
    LOG(logDEBUGPIPES) << "Adding noise hit: " << px;
    return px;
  }

  bool isInTornadoRegion(size_t dac1min, size_t dac1max, size_t dac1, size_t dac2min, size_t dac2max, size_t dac2) {

    size_t epsilon = 5;
    double tornadowidth = 40;
    double ymax = dac2max / (1 + exp(0.05*((dac1max-dac1min+1)/2 - tornadowidth - dac1))) + dac2min;
    double ymin = dac2max / (1 + exp(0.05*((dac1max-dac1min+1)/2 + tornadowidth - dac1))) + dac2min;
    if(dac2 < ymax && dac2 > ymin) {
      size_t dymax = ymax - dac2;
      size_t dymin = dac2 - ymin;
      if(dymax < epsilon) return (rand()%(epsilon-dymax) == 0);
      else if(dymin < epsilon) return (rand()%(epsilon-dymin) == 0);
      else return true;
    }
    else return false;
  }

  void fillEvent(pxar::Event * evt, uint8_t rocid, size_t col, size_t row, uint32_t flags) {

    // Generate a slightly random pulse height between 90 and 100:
    uint16_t pulseheight = rand() % 2 + 90;

    // Introduce some address encoding issues:
    if((flags&FLAG_CHECK_ORDER) != 0 && col == 0 && row == 1) { evt->pixels.push_back(pixel(rocid,col,row+1,pulseheight));} // PX 0,1 answers as PX 0,2
    else if((flags&FLAG_CHECK_ORDER) != 0 && col == 0 && row == 2) { } // PX 0,2 is dead
    else if((flags&FLAG_CHECK_ORDER) != 0 && col == 0 && row == 6) { evt->pixels.push_back(pixel(rocid,col,row+1,pulseheight));} // PX 0,6 answers as PX 0,7
    else { evt->pixels.push_back(pixel(rocid,col,row,pulseheight)); }

    // If the full chip is unmasked, add some noise hits:
    if((flags&FLAG_FORCE_UNMASKED) != 0 && (rand()%4) == 0) { evt->pixels.push_back(getNoiseHit(rocid,col,row)); }

  }

}
