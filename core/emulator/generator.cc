#include "api.h"
#include "datatypes.h"
#include "log.h"
#include "constants.h"
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

  pxar::pixel getTriggeredHit(uint8_t rocid, size_t col, size_t row, uint32_t flags) {

    pixel px;

    // Generate a slightly random pulse height between 90 and 100:
    uint16_t pulseheight = rand() % 2 + 90;

    // Introduce some address encoding issues:
    if((flags&FLAG_CHECK_ORDER) != 0 && col == 0 && row == 1) { px = pixel(rocid,col,row+1,pulseheight); } // PX 0,1 answers as PX 0,2
    else if((flags&FLAG_CHECK_ORDER) != 0 && col == 0 && row == 6) { px = pixel(rocid,col,row+1,pulseheight); } // PX 0,6 answers as PX 0,7
    else { px = pixel(rocid,col,row,pulseheight); }

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

  void fillRawData(uint32_t event, std::vector<uint16_t> &data, uint8_t tbm, uint8_t nroc, bool empty, bool noise, size_t col, size_t row, std::vector<uint16_t> pattern, uint32_t flags) {

    size_t pos = data.size();
    
    // Add a TBM header if necessary:
    if(tbm != TBM_NONE) {
      data.push_back(0xa000 | (event%256 & 0x00ff));
      data.push_back(0x8007);
    }

    // For every ROC configured, add one noise hit:
    for(size_t roc = 0; roc < nroc; roc++) {
      // Add a ROC header:
      if(tbm == TBM_EMU) data.push_back(0x47f8);
      else if(tbm != TBM_NONE) data.push_back(0x4001);
      else data.push_back(0x07f8);

      if(!empty) {
	// Add pixel hit:
	pxar::pixel px;
	if(noise) px = getNoiseHit(roc,col,row);
	else px = getTriggeredHit(roc,col,row,flags);
      
	data.push_back(0x0000 | ((px.encode() >> 12) & 0x0fff));
	data.push_back(0x2000 | (px.encode() & 0x0fff));

	// If the full chip is unmasked, add some noise hits:
	if((flags&FLAG_FORCE_UNMASKED) != 0 && (rand()%4) == 0) {
	  px = getNoiseHit(roc,col,row);
	  data.push_back(0x0000 | ((px.encode() >> 12) & 0x0fff));
	  data.push_back(0x2000 | (px.encode() & 0x0fff));
	}
      }
    }

    // Add a TBM trailer if necessary:
    if(tbm != TBM_NONE) {
      bool has_tbm_reset = false;
      bool has_roc_reset = false;
      for(size_t i = 0; i < pattern.size(); i++) {
	if((pattern.at(i)&PG_REST) != 0) has_tbm_reset = true;
	if((pattern.at(i)&PG_RESR) != 0) has_roc_reset = true;
      }
      data.push_back(0xe000 | ((nroc==0) << 7) | (has_tbm_reset << 6) | (has_roc_reset << 5));
      data.push_back(0xc002);
    }
    // Adjust event start and end marker:
    else {
      data.at(pos) = 0x8000 | (data.at(pos) & 0x0fff);
      data.at(data.size()-1) = 0x4000 | (data.back() & 0x8fff);
    }
  }
}
