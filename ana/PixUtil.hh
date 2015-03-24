#ifndef PIXUTIL_H
#define PIXUTIL_H

#include "pxardllexport.h"

#include <string>

class DLLEXPORT PixUtil {

public: 
  static void setPlotStyle();
  static double dEff(int in, int iN); 
  static double dBinomial(int in, int iN);

  /// convert ROC/COL/ROW into idx
  static int rcr2idx(int iroc, int icol, int irow);
  /// and back again
  static void idx2rcr(int idx, int &iroc, int &icol, int &irow);
  /// cleanup a string: remove everything behind #, concatenate multiple spaces into one, translate tabs into spaces
  static void cleanupString(std::string &); 
  /// in str, replace all occurences of from to to
  static void replaceAll(std::string& str, const std::string& from, const std::string& to);
  /// what would you expect?
  static bool bothAreSpaces(char lhs, char rhs);

};


#endif

