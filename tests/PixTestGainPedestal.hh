#ifndef PIXTESTGAINPEDESTAL_H
#define PIXTESTGAINPEDESTAL_H

#include "PixTest.hh"
#include "shist256.hh"

class DLLEXPORT PixTestGainPedestal: public PixTest {
public:
  PixTestGainPedestal(PixSetup *, std::string);
  PixTestGainPedestal();
  virtual ~PixTestGainPedestal();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void runCommand(std::string command); 
  
  void measure();
  void printHistograms();
  void fit(); 
  void saveGainPedestalParameters(); 

  void doTest(); 
  void fullTest(); 

private:

  int         fParNtrig, fParShowFits, fParExtended, fParDumpHists;

  std::vector<shist256*>  fHists;
  std::vector<int>        fLpoints, fHpoints;

  ClassDef(PixTestGainPedestal, 1)

};
#endif
