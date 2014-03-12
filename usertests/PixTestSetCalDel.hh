// -- author: Daniel Pitzl
#ifndef PIXTESTSETCALDEL_H
#define PIXTESTSETCALDEL_H

#include "PixTest.hh"

class DLLEXPORT PixTestSetCalDel: public PixTest {
public:
  PixTestSetCalDel(PixSetup *, std::string);
  PixTestSetCalDel();
  virtual ~PixTestSetCalDel();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 
  
  void doTest(); 

private:

  int     fParNtrig; 

  ClassDef(PixTestSetCalDel, 1)

};
#endif
