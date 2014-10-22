#ifndef SHIST256_H
#define SHIST256_H

#include "pxardllexport.h"

class DLLEXPORT shist256{
public:
  shist256(); 
  ~shist256();

  void  fill(int x, float w = 1.); 
  void  clear();
  float get(int i); 
  float get(float i); 
  float getSumOfWeights(); 
  
private:
  static const int NBINS = 256;
  float fX[NBINS+2];
  // fX[0]   = underflow
  // fX[1]   =   0 ..   1
  // fX[256] = 255 .. 256
  // fX[257] = overflow
};

#endif
