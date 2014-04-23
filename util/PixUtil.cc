#include "PixUtil.hh"

#include <math.h>       /* sqrt */
#include <cmath>        // std::abs

using namespace std;

// ----------------------------------------------------------------------
bool PixUtil::bothAreSpaces(char lhs, char rhs) { 
  return (lhs == rhs) && (lhs == ' '); 
}

// ----------------------------------------------------------------------
void PixUtil::replaceAll(string& str, const string& from, const string& to) {
  if (from.empty()) return;
  size_t start_pos = 0;
  while((start_pos = str.find(from, start_pos)) != string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
  }
}

// ----------------------------------------------------------------------
double PixUtil::dEff(int in, int iN) {
  double n = (double)in;
  double N = (double)iN;
  return sqrt(((n+1)*(N-n+1))/((N+3)*(N+2)*(N+2)));
}

// ----------------------------------------------------------------------
double PixUtil::dBinomial(int in, int iN) {
  double n = (double)in;
  double N = (double)iN;
  double w = n/N;
  if (n == N) return 0.05;
  if (n == 0) return 0.3/sqrt(N);
  return sqrt(std::abs(w*(1-w)/N));
}
