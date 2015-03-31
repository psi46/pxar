#include "PHCalibration.hh"

#include <iostream>
#include <TMath.h>
#include <TH1.h>

using namespace std;

// ----------------------------------------------------------------------
PHCalibration::PHCalibration(int mode): fMode(mode) {

}


// ----------------------------------------------------------------------
PHCalibration::~PHCalibration() {

}

// ----------------------------------------------------------------------
double PHCalibration::vcal(int iroc, int icol, int irow, double ph) {
  if (0 == fMode) {
    return vcalErr(iroc, icol, irow, ph); 
  } else if (1 == fMode) {
    return vcalTanH(iroc, icol, irow, ph); 
  } else {
    return -99.;
  }
}

// ----------------------------------------------------------------------
double PHCalibration::ph(int iroc, int icol, int irow, double vcal) {
  if (0 == fMode) {
    return phErr(iroc, icol, irow, vcal);
  } else if (1 == fMode) {
    return phTanH(iroc, icol, irow, vcal);
  } else {
    return -99.;
  }

}

// ----------------------------------------------------------------------
double PHCalibration::vcalTanH(int iroc, int icol, int irow, double ph) {
  int idx = icol*80+irow; 
//   cout << "parameters: " << fParameters[iroc][idx].p0 << ", " << fParameters[iroc][idx].p1 
//        << ", " << fParameters[iroc][idx].p2 << ", " << fParameters[iroc][idx].p3 
//        << endl;
  double x = (TMath::ATanH((ph - fParameters[iroc][idx].p3)/fParameters[iroc][idx].p2) + fParameters[iroc][idx].p1)
    / fParameters[iroc][idx].p0;
  return x;
}

// ----------------------------------------------------------------------
double PHCalibration::phTanH(int iroc, int icol, int irow, double vcal) {
  int idx = icol*80+irow; 
//   cout << "parameters: " << fParameters[iroc][idx].p0 << ", " << fParameters[iroc][idx].p1 
//        << ", " << fParameters[iroc][idx].p2 << ", " << fParameters[iroc][idx].p3 
//        << endl;
  double x = fParameters[iroc][idx].p3 + fParameters[iroc][idx].p2 
    * TMath::TanH(fParameters[iroc][idx].p0 * vcal - fParameters[iroc][idx].p1);
  return x;
}

// ----------------------------------------------------------------------
double PHCalibration::vcalErr(int iroc, int icol, int irow, double ph) {
  int idx = icol*80+irow; 
  double arg = ph/fParameters[iroc][idx].p3 - fParameters[iroc][idx].p2;
//   cout << "parameters: " << fParameters[iroc][idx].p0 << ", " << fParameters[iroc][idx].p1 
//        << ", " << fParameters[iroc][idx].p2 << ", " << fParameters[iroc][idx].p3 
//        << endl;
//   cout << "fParameters[iroc][idx].p1 = " << fParameters[iroc][idx].p1 << endl;
//   cout << "arg: " << arg 
//        << " ph =  " << ph 
//        << " fParameters[iroc][idx].p3 = " << fParameters[iroc][idx].p3 
//        << " fParameters[iroc][idx].p2 = " <<  fParameters[iroc][idx].p2
//        << endl;
//   cout << "TMath::ErfInverse(arg) = " << TMath::ErfInverse(arg) << endl;
  double x = fParameters[iroc][idx].p0 + fParameters[iroc][idx].p1 * TMath::ErfInverse(arg);
  return x;
}

// ----------------------------------------------------------------------
double PHCalibration::phErr(int iroc, int icol, int irow, double vcal) {
  int idx = icol*80+irow; 
//   cout << "parameters: " << fParameters[iroc][idx].p0 << ", " << fParameters[iroc][idx].p1 
//        << ", " << fParameters[iroc][idx].p2 << ", " << fParameters[iroc][idx].p3 
//        << endl;
  double x = fParameters[iroc][idx].p3*(TMath::Erf((vcal - fParameters[iroc][idx].p0)/fParameters[iroc][idx].p1)
					+ fParameters[iroc][idx].p2);
  return x;
}

// ----------------------------------------------------------------------
void PHCalibration::setPHParameters(std::vector<std::vector<gainPedestalParameters> >v) {
  fParameters = v; 
} 

// ----------------------------------------------------------------------
string PHCalibration::getParameters(int iroc, int icol, int irow) {
  int idx = icol*80+irow; 
  return Form("%2d/%2d/%2d: %e %e %e %e", iroc, icol, irow, 
	      fParameters[iroc][idx].p0, fParameters[iroc][idx].p1, fParameters[iroc][idx].p2, fParameters[iroc][idx].p3);
}
