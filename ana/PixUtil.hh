#ifndef PIXUTIL_H
#define PIXUTIL_H

#include "pxardllexport.h"

#include <string>

class DLLEXPORT PixUtil {

public: 
  static bool bothAreSpaces(char lhs, char rhs);
  static void replaceAll(std::string& str, const std::string& from, const std::string& to);
  static double dEff(int in, int iN); 
  static double dBinomial(int in, int iN);
};

#endif

