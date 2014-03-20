#ifndef PIXTESTFULLTEST_H
#define PIXTESTFULLTEST_H

#include "PixTest.hh"

class DLLEXPORT PixTestFullTest: public PixTest {
public:
  PixTestFullTest(PixSetup *, std::string);
  PixTestFullTest();
  virtual ~PixTestFullTest();
  virtual bool setParameter(std::string parName, std::string sval); 
  void init(); 
  void setToolTips();
  void bookHist(std::string); 

  void doTest(); 

private:
  
  
  ClassDef(PixTestFullTest, 1)

};
#endif
