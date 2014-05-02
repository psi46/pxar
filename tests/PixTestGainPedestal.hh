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

private:

  std::string fParDac;
  int         fParNtrig, fParNpointsLo, fParNpointsHi;

  ClassDef(PixTestGainPedestal, 1)

};
#endif
