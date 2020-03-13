{
#include <string>

  using namespace std;
  using namespace TMVA;
  using namespace RooFit;
  using namespace RooStats;

  cout << "Loading libs for pxar/ana" << endl;
  gSystem->Load("../lib/libpxarutil.so");
  gSystem->Load("../lib/libana.so");
  gStyle->SetPalette(1);
  gStyle->SetOptStat(111111);

  TLatex *tl = new TLatex();
  tl->SetNDC(kTRUE);
  tl->SetTextFont(42);

  TLine *pl = new TLine();

  // -- this is a dummy entry to force loading libAnaUtil.so?!?!?!
  //  double bla = dEff(1,2);

  // --- Cleanup if this is not the first call to rootlogon.C
  TCanvas *c = 0;
  c = (TCanvas*)gROOT->FindObject("c0"); if (c) c->Delete(); c = 0;
  p = (TPad*)gROOT->FindObject("p0"); if (p) p->Delete(); p = 0;
  // --- Create a new canvas.
  TCanvas c0("c0","--c0--",2303,0,656,700);
  c0.ToggleEventStatus();
}
