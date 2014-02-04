#ifndef PIXTESTSETVTHRCOMP_H
#define PIXTESTSETVTHRCOMP_H

#include "PixTest.hh"

class PixTestSetVthrComp: public PixTest
{
public:
  PixTestSetVthrComp(PixSetup *, std::string);
  PixTestSetVthrComp();
  virtual ~PixTestSetVthrComp();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void bookHist(std::string); 
  
  void doTest(); 

private:

  ClassDef(PixTestSetVthrComp, 1);

};
#endif
