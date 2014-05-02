#ifndef PIXTESTGAINPEDESTAL_H
#define PIXTESTGAINPEDESTAL_H

#include "PixTest.hh"

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
  void doTest(); 
  void output4moreweb();
  
  void measure();
  void fit(); 
  void saveGainPedestalParameters(); 

private:

  int         fParShowFits, fParNtrig, fParNpointsLo, fParNpointsHi;

  std::map<std::string, TH1D*> fHists; 

  ClassDef(PixTestGainPedestal, 1)

};
#endif
