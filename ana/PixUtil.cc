#include "PixUtil.hh"

#include "TMath.h"
#include "TStyle.h"
#include "TColor.h"

using namespace std;

// ----------------------------------------------------------------------
void PixUtil::setPlotStyle() {
  const Int_t NRGBs = 5;
  const Int_t NCont = 255;

  Double_t stops[NRGBs] = { 0.00, 0.34, 0.61, 0.84, 1.00 };
  Double_t red[NRGBs]   = { 0.00, 0.00, 0.87, 1.00, 0.51 };
  Double_t green[NRGBs] = { 0.00, 0.81, 1.00, 0.20, 0.00 };
  Double_t blue[NRGBs]  = { 0.51, 1.00, 0.12, 0.00, 0.00 };
  TColor::CreateGradientColorTable(NRGBs, stops, red, green, blue, NCont);
  gStyle->SetNumberContours(NCont);
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
  return TMath::Sqrt(((n+1)*(N-n+1))/((N+3)*(N+2)*(N+2)));
}

// ----------------------------------------------------------------------
double PixUtil::dBinomial(int in, int iN) {
  double n = (double)in;
  double N = (double)iN;
  double w = n/N;
  if (n == N) return 0.05;
  if (n == 0) return 0.3/TMath::Sqrt(N);
  return TMath::Sqrt(TMath::Abs(w*(1-w)/N));
}

// ----------------------------------------------------------------------
int PixUtil::rcr2idx(int iroc, int icol, int irow) {
  if (irow < 0 || irow > 79) return -1; 
  if (icol < 0 || icol > 51) return -1; 
  return iroc*80*52 + icol*80 + irow;
}


// ----------------------------------------------------------------------
void PixUtil::idx2rcr(int idx, int &iroc, int &icol, int &irow) {
  iroc = idx/4160;
  int r = idx - iroc*4160;
  icol = r/80;
  irow = r%80;
}

// ----------------------------------------------------------------------
void PixUtil::cleanupString(string &s) {
  replaceAll(s, "\t", " "); 
  string::size_type s1 = s.find("#");
  if (string::npos != s1) s.erase(s1); 
  if (0 == s.length()) return;
  string::iterator new_end = unique(s.begin(), s.end(), bothAreSpaces);
  s.erase(new_end, s.end()); 
  if (s.substr(0, 1) == string(" ")) s.erase(0, 1); 
  if (s.substr(s.length()-1, 1) == string(" ")) s.erase(s.length()-1, 1); 
}

// ----------------------------------------------------------------------
bool PixUtil::bothAreSpaces(char lhs, char rhs) { 
  return (lhs == rhs) && (lhs == ' '); 
}
