#include "PixUtil.hh"

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

