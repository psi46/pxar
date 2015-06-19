#ifndef PIXTESTFPIXTEST_H
#define PIXTESTFPIXTEST_H

#include "PixTest.hh"

class DLLEXPORT PixTestFPIXTest: public PixTest {
public:
  PixTestFPIXTest(PixSetup *, std::string);
  PixTestFPIXTest();
  virtual ~PixTestFPIXTest();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void doTest(); 

private:
  
  
  ClassDef(PixTestFPIXTest, 1)

};
#endif
