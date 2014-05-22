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
